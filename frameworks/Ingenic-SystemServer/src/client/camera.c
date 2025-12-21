#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>
#include <sys/mman.h>
#include "sys_common.h"
#include <assert.h>
#include <stdbool.h>
#include "camera_service.h"
#include "fifo.h"
#include "iss_camera.h"
#include "list.h"
#include "systemserver_config.h"

#define LOG_TAG "ISS_CAMC"
#include "dlog.h"


#define MAX_CLIENT_NUM 3

typedef struct {
	fifo_t data_ready_fifo;
	DataBuffer_t* ready_data;
	uint32_t buf_num;
	uint32_t ipc_handle;
	uint32_t magic;
	tIpcThreadInfo* ti;
	int32_t index;
	struct list_head list;
	tBinderService cb;
}camera_priv_ctx_t;

static camera_priv_ctx_t camera_ctx[MAX_CLIENT_NUM];
static int32_t g_cnt = 0;
static uint32_t g_ipcHandle = 0;
static bool g_binder_thread_run = false;
static struct list_head camdev_list_head;

static int received_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int msg_size = 0;
	sys_msg_t* sys_msg;
	int32_t ret  =  binder_io_get_data(msg,&data,&msg_size);
	if(!ret){
		iss_camera_msg_t* cam_msg = (iss_camera_msg_t*)data;
		int32_t devindex = cam_msg->target_dev_index;
		DataBuffer_t* buf = &cam_msg->data[0];

		camera_priv_ctx_t* ctx = NULL;
		list_for_each_entry(ctx,&camdev_list_head,list){
			if(ctx->index == devindex){
				break;
			}
		}
		uint32_t datafd = binder_io_get_fd(msg,0);
		uint32_t* imgVaddr = mmap(NULL,buf->size,PROT_READ,MAP_SHARED,datafd,0);
		if(imgVaddr != MAP_FAILED){
			ctx->ready_data[buf->index].fd = datafd;
			ctx->ready_data[buf->index].size = buf->size;
			ctx->ready_data[buf->index].paddr = buf->paddr;
			ctx->ready_data[buf->index].vaddr = imgVaddr;
			ctx->ready_data[buf->index].index = buf->index;
			queue_fifo(&ctx->data_ready_fifo,&ctx->ready_data[buf->index],WAIT_FOREVER);
		}
		/* printf("imgVaddr = 0x%x bufidx = %d paddr = 0x%x fd=%d serfd = %d ####\r\n",imgVaddr,buf->index,buf->paddr,datafd,buf->fd); */
	}

	return 0;
}


static int32_t client_register(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CLIENT_REGISTER;
	cam_msg->ipc_magic = 0;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char*)&sys_msg, sizeof(sys_msg));
    binder_io_append_obj(&data, &ctx->cb);

	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        uint32_t ipc_magic = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
		if(ipc_magic != 0xFFFF){
			ctx->magic = ipc_magic;
			if(g_binder_thread_run == false){
				binder_thread_enter_loop(0,0);
				g_binder_thread_run = true;
			}
			return 0;
		}
	}
	return -1;
}

static int32_t client_unregister(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CLIENT_UNREGISTER;
	cam_msg->ipc_magic = ctx->magic;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char*)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t client_camera_init(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti,CameraInitParam_t *param,int devindex)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  INIT_REQUEST;
	cam_msg->ipc_magic = ctx->magic;
	cam_msg->data_len = sizeof(CameraInitParam_t);
	cam_msg->target_dev_index = devindex;
	ctx->index = devindex;
	CameraInitParam_t* sparam = &cam_msg->data[0];
	memcpy(sparam,param,sizeof(CameraInitParam_t));

	/* printf("%s %d magic = 0x%x ##\r\n",__func__,__LINE__,ctx->magic); */
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char*)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
		printf("%s ret = 0x%x ###\r\n",__func__,retval);
        binder_cmd_freebuf(ti, reply.data0);
	}

	if(retval == 0xFFFF || retval != devindex)
		return -1;
	else
		return 0;
}


static int32_t client_camera_deinit(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  DEINIT_REQUEST;
	cam_msg->ipc_magic = ctx->magic;
	cam_msg->target_dev_index = ctx->index;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char*)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t client_camera_start(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CAMERA_START;
	cam_msg->ipc_magic = ctx->magic;
	cam_msg->target_dev_index = ctx->index;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char*)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
		printf("%s ret = 0x%x ###\r\n",__func__,retval);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t client_camera_stop(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CAMERA_STOP;
	cam_msg->ipc_magic = ctx->magic;
	cam_msg->target_dev_index = ctx->index;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char*)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t release_data(camera_priv_ctx_t* ctx,tIpcThreadInfo* ti,DataBuffer_t* databuf)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  DATA_RELEASE;
	cam_msg->ipc_magic = ctx->magic;
	cam_msg->data_len = sizeof(DataBuffer_t);
	cam_msg->target_dev_index = ctx->index;
	memcpy(cam_msg->data,databuf,cam_msg->data_len);

	// client 使用完，需关闭，解除映射
	close(databuf->fd);
	munmap(databuf->vaddr,databuf->size);

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        uint32_t retv = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
		if(retv == 0xFFFF)
			return -1;
		else
			return 0;
	} else {
		printf("### %s %d failed ###\r\n",__func__,__LINE__);
	}
	return -1;
}

ISS_CamHandle_t ISS_CameraInit(CameraInitParam_t *param)
{
	if(g_ipcHandle == 0 && g_cnt == 0){
		INIT_LIST_HEAD(&camdev_list_head);
		g_ipcHandle = binder_get_service(CAMERA_SERVICE_NAME);
	}
	tIpcThreadInfo* ti = binder_get_thread_info();
	camera_priv_ctx_t* ctx = malloc(sizeof(camera_priv_ctx_t));
	if(!ctx)
		return NULL;
	memset(ctx,0,sizeof(camera_priv_ctx_t));

	ctx->cb.transact_cb = received_cb;
    ctx->cb.link_to_death_cb = NULL;
    ctx->cb.unlink_to_death_cb = NULL;
    ctx->cb.death_notify_cb = NULL;
	ctx->ipc_handle = g_ipcHandle;

	ctx->ready_data = malloc(param->buffer_num * sizeof(DataBuffer_t));
	if(!ctx->ready_data){
		free(ctx);
		return NULL;
	}
	init_fifo(&ctx->data_ready_fifo,param->buffer_num);

	int ret = client_register(ctx,ti);
	if(ret){
		LOGE("client registe failed !!");
		goto err;
	}
	ret = client_camera_init(ctx,ti,param,g_cnt);
	if(ret){
		LOGE("camera init failed !!");
		goto unreg;
	}
	ret = client_camera_start(ctx,ti);
	if(ret){
		LOGE("camera start failed !!");
		goto deinit;
	}

	list_add_head(&ctx->list,&camdev_list_head);
	g_cnt++;
	return (ISS_CamHandle_t)ctx;
deinit:
	client_camera_deinit(ctx,ti);
unreg:
	client_unregister(ctx,ti);
err:
	free(ctx->ready_data);
	free(ctx);
	return NULL;
}

int32_t ISS_CameraDeInit(ISS_CamHandle_t handle)
{

	assert(handle);
	camera_priv_ctx_t* ctx = (camera_priv_ctx_t*)handle;
	tIpcThreadInfo* ti = binder_get_thread_info();
	client_camera_stop(ctx,ti);
	int retry = 10;
	int num = getFifoElemNum(&ctx->data_ready_fifo);
	while(num > 0) {
		usleep(10*1000);
		num = getFifoElemNum(&ctx->data_ready_fifo);
		if(retry == 0)
			break;
		retry--;
	};
	DataBuffer_t* buf = NULL;
	if(num > 0){
		for(int i = 0; i < num; i++){
			buf = dequeue_fifo(&ctx->data_ready_fifo, NO_WAIT);
			if(buf)
				release_data(ctx,ti,buf);
		}
	}
	client_camera_deinit(ctx,ti);
	client_unregister(ctx,ti);
	list_del(&ctx->list);
	free(ctx);
	g_cnt -= 1;
	return 0;
}

int32_t ISS_GetCameraData(ISS_CamHandle_t handle,DataBuffer_t* data)
{
	assert(handle);
	assert(data);
	camera_priv_ctx_t* ctx = (camera_priv_ctx_t*)handle;
	DataBuffer_t* buf = dequeue_fifo(&ctx->data_ready_fifo, NO_WAIT);
	if(buf){
		memcpy(data,buf,sizeof(DataBuffer_t));
		return 0;
	} else {
		return -1;
	}
}

int32_t ISS_ReleaseCameraData(ISS_CamHandle_t handle,DataBuffer_t* data)
{
	assert(handle);
	assert(data);
	camera_priv_ctx_t* ctx = (camera_priv_ctx_t*)handle;
	tIpcThreadInfo* ti = binder_get_thread_info();
	return release_data(ctx,ti,data);
}

