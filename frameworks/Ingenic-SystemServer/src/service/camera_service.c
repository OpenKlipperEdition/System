#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "icamera.h"
#include "iss_common.h"
#include "iss_camera.h"
#include "list.h"
#include "camera_service.h"
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "systemserver_config.h"

#define LOG_TAG "ISS_CAMS"
#include "dlog.h"
#include "fifo.h" 


#define MAX_CLIENT_NUM 6
#define MAX_DEVICE_NODE_LEN 16
#define BASE_MAGIC_NUM 0xABD51501


typedef struct {
	int32_t ipc_handle;
	uint32_t ipc_magic;
	cam_use_type_t use_type;
	IHAL_CameraHandle_t* mpp_camera_hdl;
	pthread_t cam_work_tid;
	pthread_mutex_t cli_lock;
	bool work_sta;
	struct list_head list;
	struct binder_death* client_death;
	char cam_node[MAX_DEVICE_NODE_LEN];
	int32_t cam_idx;
	DataBuffer_t buf[4];
	fifo_t qbuf_fifo;
}client_ctx_t;


typedef struct {
	char name[12];
	uint16_t node_mask;
}camera_node_info_t;

camera_node_info_t video_node_array[] = {
	{"/dev/video4",(1 << 4)},
	{"/dev/video5",(1 << 5)},
	{"/dev/video6",(1 << 6)},
	{"/dev/video8",(1 << 8)},
	{"/dev/video9",(1 << 9)},
	{"/dev/video10",(1 << 10)},
};

typedef struct {
	uint32_t sta_mask;
	pthread_mutex_t mg_lock;	// camera manager lock
	struct list_head client_reg_list;
	int32_t client_count;
}service_ctx_t;



static service_ctx_t cam_service;

void client_death_cb(tIpcThreadInfo *info, void* priv);

static int32_t set_camera_using_sta(char* node,bool sta)
{
	int32_t node_num = sizeof(video_node_array) / sizeof(camera_node_info_t);
	/* printf("%s node = %s ###\r\n",__func__,node); */
	for(int32_t i = 0; i < node_num; i++){
		if(!strncmp(node,video_node_array[i].name,strlen(node))){
			/* printf("%s %d mask = 0x%x###\r\n",__func__,__LINE__,cam_service.sta_mask); */
			cam_service.sta_mask &= ~(video_node_array[i].node_mask);
			if(sta == true)
				cam_service.sta_mask |= video_node_array[i].node_mask;
			return 0;
		}
	}

	return -1;
}

static int32_t get_camera_using_sta(char* node)
{
	int32_t node_num = sizeof(video_node_array) / sizeof(camera_node_info_t);
	for(int32_t i = 0; i < node_num; i++){
		if(!strncmp(node,video_node_array[i].name,strlen(node))){
			/* printf("%s %d mask = 0x%x###\r\n",__func__,__LINE__,cam_service.sta_mask); */
			if(cam_service.sta_mask & video_node_array[i].node_mask){
				return 1;	// 已使用
			} else {
				return 0;   // 未使用
			}
		}
	}

	return -1;
}


static client_ctx_t* find_client_ctx(iss_camera_msg_t *msg)
{

	client_ctx_t* cli_ctx = NULL;
	pthread_mutex_lock(&cam_service.mg_lock);
	list_for_each_entry(cli_ctx,&cam_service.client_reg_list,list){
		if(cli_ctx->ipc_magic == msg->ipc_magic){
			pthread_mutex_unlock(&cam_service.mg_lock);
			return cli_ctx;
		}
	}
	pthread_mutex_unlock(&cam_service.mg_lock);
	LOGE("can not find client_ctx");
	return NULL;
}


static int32_t send_buf_to_client(client_ctx_t* cli,DataBuffer_t* buf,bool sync)
{
	tBinderIo data;
	tBinderIo reply;
	char binder_buf[384];
	iss_camera_msg_t cam_msg;
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
	cam_msg.cmd = DATA_BUF_SYNC;
	cam_msg.ipc_magic = cli->ipc_magic;
	cam_msg.target_dev_index = cli->cam_idx;
	memcpy(cam_msg.data,buf,sizeof(DataBuffer_t));
	cam_msg.data_len = sizeof(DataBuffer_t);
	binder_io_append_data(&data, (char *)&cam_msg, sizeof(iss_camera_msg_t));
	binder_io_append_fd(&data,buf->fd);
	int32_t ret = 0;
	if(sync){
		ret = binder_cmd_sync_call(ti, &data,&reply,cli->ipc_handle, 0);
	} else {
		ret = binder_cmd_async_call(ti, &data,NULL,cli->ipc_handle, 0);
	}
	if(ret){
		return -1;
	}
	return 0;
}

static int32_t client_data_release_process(iss_camera_msg_t* msg,tBinderIo* reply)
{
	IMPP_BufferInfo_t camerabuf;
	DataBuffer_t* buf = (DataBuffer_t*)&msg->data[0];
	camerabuf.index = buf->index;
	client_ctx_t* cli = find_client_ctx(msg);
	if(cli){
		memcpy(&cli->buf[buf->index],buf,sizeof(DataBuffer_t));
		queue_fifo(&cli->qbuf_fifo,&cli->buf[buf->index],WAIT_FOREVER);
	} else {
		binder_io_append_uint32(reply,0xFFFF);
		return -1;
	}
	binder_io_append_uint32(reply,0);
	return 0;
}

void* camera_work_thread(void* arg)
{
	client_ctx_t* cli_ctx = (client_ctx_t*)arg;
	IMPP_BufferInfo_t camerabuf;
	IMPP_BufferInfo_t release_buf;
	DataBuffer_t databuf;
	DataBuffer_t *release;
	int32_t ret = 0;
	for(;;){
		if(cli_ctx->work_sta){

			release = dequeue_fifo(&cli_ctx->qbuf_fifo, NO_WAIT);
			if(release){
				release_buf.index = release->index;
				ret = IHal_CameraQueuebuffer(cli_ctx->mpp_camera_hdl, &release_buf);
			}

			ret = IHal_Camera_WaitBufferAvailable(cli_ctx->mpp_camera_hdl, IMPP_NO_WAIT);
			if(!ret){
				ret = IHal_CameraDeQueueBuffer(cli_ctx->mpp_camera_hdl, &camerabuf);
				if(ret == 0){
					databuf.fd = camerabuf.fd;
					databuf.size = camerabuf.size;
					databuf.vaddr = camerabuf.vaddr;
					databuf.paddr = camerabuf.paddr;
					databuf.index = camerabuf.index;
					send_buf_to_client(cli_ctx,&databuf,false);
				}
			}
			/* usleep(5*1000); */
		} else {
			usleep(90*1000);
		}
	}
}


void client_death_cb(tIpcThreadInfo *info, void* priv)
{
	client_ctx_t* clipriv = (client_ctx_t*)priv;
	client_ctx_t* tmp = NULL;
	client_ctx_t* ctx = NULL;
	pthread_mutex_lock(&cam_service.mg_lock);
	list_for_each_entry(tmp,&cam_service.client_reg_list,list){
		if(tmp->ipc_magic == clipriv->ipc_magic){
			ctx = tmp;
			break;
		}
	}
	pthread_mutex_unlock(&cam_service.mg_lock);
	if(!ctx){
		return;
	}
	if(ctx->use_type == CAM_DATA_REQUEST && ctx->ipc_magic > 0){
		pthread_mutex_lock(&ctx->cli_lock);
		ctx->work_sta = false;
		pthread_mutex_unlock(&ctx->cli_lock);
		IHal_CameraStop(ctx->mpp_camera_hdl);
		pthread_cancel(ctx->cam_work_tid);
		pthread_join(ctx->cam_work_tid,NULL);
		IHal_CameraClose(ctx->mpp_camera_hdl);
		pthread_mutex_lock(&cam_service.mg_lock);
		list_del(&ctx->list);
		cam_service.client_count -= 1;
		set_camera_using_sta(ctx->cam_node,false);
		pthread_mutex_unlock(&cam_service.mg_lock);
	} else {
		pthread_mutex_lock(&cam_service.mg_lock);
		cam_service.client_count -= 1;
		set_camera_using_sta(ctx->cam_node,false);
		pthread_mutex_unlock(&cam_service.mg_lock);
		list_del(&ctx->list);
		/* free(cli_ctx->client_death); */
		/* free(cli_ctx); */
	}
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_release(ti, ctx->ipc_handle);
	free(ctx->client_death);
	free(ctx);
	printf("%s [%d] client is dead ###\r\n",__func__,__LINE__);
}

static uint32_t client_obj_register(tBinderIo* msg)
{
	client_ctx_t* cli_ctx = (client_ctx_t*)malloc(sizeof(client_ctx_t));
	if(!cli_ctx){
		return 0;
	}
	memset(cli_ctx,0,sizeof(client_ctx_t));

	uint32_t hdl = binder_io_get_ref(msg, 0);
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_acquire(ti, hdl);
	flush_commands(ti);
	cli_ctx->ipc_handle = hdl;
	pthread_mutex_lock(&cam_service.mg_lock);
	cli_ctx->ipc_magic = BASE_MAGIC_NUM + cam_service.client_count;
	list_add_head(&cli_ctx->list,&cam_service.client_reg_list);
	pthread_mutex_unlock(&cam_service.mg_lock);
	cli_ctx->client_death = (struct binder_death*)malloc(sizeof(struct binder_death));
	if(!cli_ctx->client_death){
		free(cli_ctx);
		return 0;
	}
	cli_ctx->client_death->death_cb = client_death_cb;
	cli_ctx->client_death->ptr = cli_ctx;
	pthread_mutex_lock(&cam_service.mg_lock);
	cam_service.client_count++;
	pthread_mutex_unlock(&cam_service.mg_lock);

	binder_cmd_link_to_death(ti,cli_ctx->ipc_handle,cli_ctx->client_death);
	return cli_ctx->ipc_magic;
}

static uint32_t client_obj_unregister(iss_camera_msg_t* msg,tBinderIo *reply)
{
	client_ctx_t* cli_ctx = find_client_ctx(msg);
	if(!cli_ctx){
		binder_io_append_uint32(reply,0xFFFF);
		return -1;
	}
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_release(ti, cli_ctx->ipc_handle);
	pthread_mutex_lock(&cam_service.mg_lock);
	list_del(&cli_ctx->list);
	cam_service.client_count -= 1;
	pthread_mutex_unlock(&cam_service.mg_lock);
	binder_io_append_uint32(reply,0);
	cli_ctx->ipc_magic = 0;
	free(cli_ctx->client_death);
	free(cli_ctx);
	return 0;
}

int32_t iss_service_camera_bind(char* node)
{
	pthread_mutex_lock(&cam_service.mg_lock);
	int32_t ret = get_camera_using_sta(node);
	pthread_mutex_unlock(&cam_service.mg_lock);
	if(ret == 1){
		LOGE("bind camera failed ,%s has been used",node);
		return -1;
	} else if (ret == -1){
		LOGE("invalid node ,please check ");
		return -1;
	} else {
		pthread_mutex_lock(&cam_service.mg_lock);
		set_camera_using_sta(node,true);
		pthread_mutex_unlock(&cam_service.mg_lock);
	}
	return 0;
}

int32_t iss_service_camera_unbind(char* node)
{
	pthread_mutex_lock(&cam_service.mg_lock);
	int32_t ret = get_camera_using_sta(node);
	pthread_mutex_unlock(&cam_service.mg_lock);
	if(ret == 1){
		pthread_mutex_lock(&cam_service.mg_lock);
		set_camera_using_sta(node,false);
		pthread_mutex_unlock(&cam_service.mg_lock);
		return 0;
	} else {
		LOGE("node invalid ");
		return -1;
	}
}

static int32_t process_bind_request(iss_camera_msg_t* cam_msg,tBinderIo* reply,bool bind)
{
	/* char node[MAX_DEVICE_NODE_LEN]; */
	/* memcpy(node,cam_msg->data,MAX_DEVICE_NODE_LEN); */
	int32_t ret = 0;
	client_ctx_t* cli_ctx = find_client_ctx(cam_msg);
	if(!cli_ctx){
		goto err;
	}
	if(bind){
		memcpy(cli_ctx->cam_node,cam_msg->data,MAX_DEVICE_NODE_LEN);
		ret = iss_service_camera_bind(cli_ctx->cam_node);
	} else {
		ret = iss_service_camera_unbind(cli_ctx->cam_node);
	}
	if(ret){
		goto err;
	}
	cli_ctx->use_type = BIND_WITH_DEVICE;
	binder_io_append_uint32(reply,0);
	return 0;
err:
	binder_io_append_uint32(reply,0xFFFF);
	return -1;
}

int32_t iss_service_camera_start(iss_camera_msg_t* cam_msg,tBinderIo* reply)
{
	client_ctx_t* cli_ctx = find_client_ctx(cam_msg);
	if(!cli_ctx || cli_ctx->cam_idx != cam_msg->target_dev_index){
		goto err;
	}
	pthread_mutex_lock(&cli_ctx->cli_lock);
	if(cli_ctx->work_sta == false){
		IHal_CameraStart(cli_ctx->mpp_camera_hdl);
		cli_ctx->work_sta = true;
		binder_io_append_uint32(reply,0);
		pthread_mutex_unlock(&cli_ctx->cli_lock);
		return 0;
	}
	pthread_mutex_unlock(&cli_ctx->cli_lock);
err:
	binder_io_append_uint32(reply,0xFFFF);
	return -1;
}

int32_t iss_service_camera_stop(iss_camera_msg_t* cam_msg,tBinderIo* reply)
{
	client_ctx_t* cli_ctx = find_client_ctx(cam_msg);
	if(!cli_ctx || cli_ctx->cam_idx != cam_msg->target_dev_index){
		goto err;
	}

	pthread_mutex_lock(&cli_ctx->cli_lock);
	if(cli_ctx->work_sta == true){
		cli_ctx->work_sta = false;
		pthread_mutex_unlock(&cli_ctx->cli_lock);
		IHal_CameraStop(cli_ctx->mpp_camera_hdl);
		binder_io_append_uint32(reply,0);
		return 0;
	}
	pthread_mutex_unlock(&cli_ctx->cli_lock);
err:
	binder_io_append_uint32(reply,0xFFFF);
	return -1;
}

int32_t iss_service_camera_init(iss_camera_msg_t *cam_msg,tBinderIo* reply)
{

	client_ctx_t* cli_ctx = find_client_ctx(cam_msg);
	if(!cli_ctx){
		printf("can not find client ctx ##\r\n");
		binder_io_append_uint32(reply,0xFFFF);
		return -1;
	}
	CameraInitParam_t* param = (CameraInitParam_t*)&cam_msg->data[0];
		pthread_mutex_lock(&cam_service.mg_lock);
	int32_t ret = get_camera_using_sta(param->node);
		pthread_mutex_unlock(&cam_service.mg_lock);
	if(ret){
		printf("get camera use sta is using ##\r\n");
		binder_io_append_uint32(reply,0xFFFF);
		return -1;
	}
	IHAL_CameraHandle_t* camera_hdl = IHal_CameraOpen(param->node);
	if(!camera_hdl){
		LOGE("camera node [%s] open failed !!!",param->node);
		binder_io_append_uint32(reply,0xFFFF);
		return -1;
	}

	IHAL_CAMERA_PARAMS params = {
		.imageWidth = param->width,
   		.imageHeight = param->height,
   		.imageFmt = param->fmt,
   		.imageFmtStr = NULL,
	};
	memcpy(cli_ctx->cam_node,param->node,strlen(param->node));
	ret = IHal_CameraSetParams(camera_hdl,&params);
	if(ret){
		LOGE("set  camera param failed ");
		goto close_camera;
	}
	/* printf("%s %d bufnum = %d  ###\r\n",__func__,__LINE__,param->buffer_num); */
	ret = IHal_CameraCreateBuffers(camera_hdl,IMPP_INTERNAL_BUFFER, param->buffer_num);
	if(ret < 0){
		LOGE("set camera buffer failed");
		goto close_camera;
	}
	cli_ctx->mpp_camera_hdl = camera_hdl;
	cli_ctx->cam_idx = cam_msg->target_dev_index;
	cli_ctx->use_type = CAM_DATA_REQUEST;
	init_fifo(&cli_ctx->qbuf_fifo,5);
	pthread_create(&cli_ctx->cam_work_tid,NULL,camera_work_thread,cli_ctx);
	pthread_mutex_init(&cli_ctx->cli_lock,NULL);
	pthread_mutex_lock(&cam_service.mg_lock);
	set_camera_using_sta(param->node,true);
	pthread_mutex_unlock(&cam_service.mg_lock);
	ret = 0;
	binder_io_append_uint32(reply,cli_ctx->cam_idx);
	return 0;
close_camera:
	IHal_CameraClose(camera_hdl);
	binder_io_append_uint32(reply,0xFFFF);
	return -1;
}


int32_t iss_service_camera_deinit(iss_camera_msg_t *cam_msg,tBinderIo* reply)
{
	client_ctx_t* cli = find_client_ctx(cam_msg);
	if(!cli)
		return -1;
	pthread_cancel(cli->cam_work_tid);
	pthread_join(cli->cam_work_tid,NULL);
	//TODO 检查Client 还在使用Camera的Buffer,然后再关闭Camera
	pthread_mutex_lock(&cam_service.mg_lock);
	set_camera_using_sta(cli->cam_node,false);
	pthread_mutex_unlock(&cam_service.mg_lock);
	IHal_CameraClose(cli->mpp_camera_hdl);
	return 0;
}


static int client_msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int32_t msg_size = 0;
	sys_msg_t* sys_msg;
	iss_camera_msg_t* cam_msg;
	int32_t ret = binder_io_get_data(msg,&data, &msg_size);
	sys_msg = (sys_msg_t*)data;
	cam_msg = (iss_camera_msg_t*)sys_msg->msg;
	switch(cam_msg->cmd){
		case BIND_REQUEST:
			process_bind_request(cam_msg,reply,true);
			break;
		case UNBIND_REQUEST:
			process_bind_request(cam_msg,reply,false);
			break;
		case INIT_REQUEST:
			iss_service_camera_init(cam_msg,reply);
			break;
		case DEINIT_REQUEST:
			iss_service_camera_deinit(cam_msg,reply);
			break;
		case DATA_RELEASE:
			client_data_release_process(cam_msg,reply);
			break;
		case CLIENT_REGISTER:
		{
			uint32_t magic = client_obj_register(msg);
			// death link regist
			// 注册成功，将magic返回给client
			if(magic >= BASE_MAGIC_NUM)
				binder_io_append_uint32(reply,magic);
			else
				binder_io_append_uint32(reply,0xFFFF);
		}
		break;
		case CLIENT_UNREGISTER:
		{
			client_obj_unregister(cam_msg,reply);
		}
		break;
		case CAMERA_START:
			iss_service_camera_start(cam_msg,reply);
		break;
		case CAMERA_STOP:
			iss_service_camera_stop(cam_msg,reply);
		break;
		default:
			LOGE("invalid cmd !!!");
			return -1;
	}

	return 0;
}

static tBinderService camera_service = {
    .transact_cb = client_msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};


int main(int argc,char** argv)
{
	int ret = 0;
	cam_service.sta_mask = 0;
	INIT_LIST_HEAD(&cam_service.client_reg_list);
	cam_service.client_count = 0;
	pthread_mutex_init(&cam_service.mg_lock,NULL);
	ret = binder_add_service(CAMERA_SERVICE_NAME,&camera_service);
	binder_thread_enter_loop(0,0);
	while(1){
		sleep(2);
	}

    binder_threads_shutdown();
	return 0;
}



