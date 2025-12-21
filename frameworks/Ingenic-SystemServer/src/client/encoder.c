#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "sys_common.h"
#include <assert.h>
#include "encoder.h"
#include "encoder_service.h"
#include "fifo.h"
#include "iss_camera.h"
#include "list.h"
#include "systemserver_config.h"

#define LOG_TAG "ISS_ENCS"
#include "dlog.h"


typedef struct {
	int32_t chn;
	uint32_t ipcHandle;
	tBinderService cb;
	DataBuffer_t* ready_buf;
	fifo_t ready_buf_fifo;
	struct list_head list;
	EncoderFinish_Cb_t* endCb;
	EncoderChnParam_t ch_param;
	bool work_sta;
	pthread_t stream_tid;
}EncClientInfo_Ctx_t;


static struct list_head encoder_manager_list;
static int32_t g_cnt = 0;
static uint32_t g_ipcHandle = 0;
static bool g_binder_thread_run = false;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;


static int received_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int msg_size = 0;
	sys_msg_t* sys_msg;
	int32_t ret  =  binder_io_get_data(msg,&data,&msg_size);
	if(!ret){
		iss_encoder_msg_t* enc_msg = (iss_encoder_msg_t*)data;
		int32_t target_chn = enc_msg->chn;
		DataBuffer_t* buf = &enc_msg->data[0];
		uint32_t datafd = binder_io_get_fd(msg,0);

		EncClientInfo_Ctx_t* ctx = NULL;
		EncClientInfo_Ctx_t* tmp = NULL;
		list_for_each_entry(tmp,&encoder_manager_list,list){
			if(tmp->chn == target_chn){
				ctx = tmp;
				break;
			}
		}
		if(!ctx){
			return -1;
		}
		uint32_t* imgVaddr = mmap(NULL,buf->size,PROT_READ | PROT_WRITE,MAP_SHARED,datafd,0);
		if(imgVaddr != MAP_FAILED){
			ctx->ready_buf[buf->index].fd = datafd;
			ctx->ready_buf[buf->index].size = buf->size;
			ctx->ready_buf[buf->index].paddr = buf->paddr;
			ctx->ready_buf[buf->index].vaddr = imgVaddr;
			ctx->ready_buf[buf->index].index = buf->index;
			/* ctx->ready_buf[buf->index].byteused = buf->byteused; */
			queue_fifo(&ctx->ready_buf_fifo,&ctx->ready_buf[buf->index],NO_WAIT);
		}
		/* printf("imgVaddr = 0x%x target_chn %d buf->index = %d mapsize = %d ####\r\n",imgVaddr,target_chn,buf->index,buf->size); */
	}

	return 0;
}

static int32_t encoder_cli_register(EncClientInfo_Ctx_t* ctx)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd =  ENCODER_CLI_REGISTER;

	ctx->cb.transact_cb = received_cb;
    ctx->cb.link_to_death_cb = NULL;
    ctx->cb.unlink_to_death_cb = NULL;
    ctx->cb.death_notify_cb = NULL;

    tBinderIo data, reply;
    binder_io_init(&data,binder_buf,sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	binder_io_append_obj(&data, &ctx->cb);

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		if(ser_rep->exec_result == ISS_SUCCESS){
			ctx->chn = ser_rep->retval;
			binder_cmd_freebuf(ti, reply.data0);
			if(g_binder_thread_run == false){
				binder_thread_enter_loop(0,0);
				g_binder_thread_run = true;
			}
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_cli_unregister(EncClientInfo_Ctx_t* ctx)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd =  ENCODER_CLI_UNREGISTER;
	enc_msg->chn = ctx->chn;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			ctx->chn = -1;
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}


static int32_t encoder_chn_init(EncClientInfo_Ctx_t* ctx,EncoderChnParam_t* param)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_INIT;
	enc_msg->chn = ctx->chn;
	enc_msg->data_len = sizeof(EncoderChnParam_t);
	memcpy(enc_msg->data,param,sizeof(EncoderChnParam_t));


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_chn_deinit(EncClientInfo_Ctx_t* ctx)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_DEINIT;
	enc_msg->chn = ctx->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_chn_start(EncClientInfo_Ctx_t* ctx)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_START;
	enc_msg->chn = ctx->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result);
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			/* ctx->work_sta = true; */
			pthread_mutex_lock(&g_mutex);
			ctx->work_sta = true;
			pthread_mutex_unlock(&g_mutex);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_chn_stop(EncClientInfo_Ctx_t* ctx)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_STOP;
	enc_msg->chn = ctx->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			pthread_mutex_lock(&g_mutex);
			ctx->work_sta = false;
			pthread_mutex_unlock(&g_mutex);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_chn_set_bitrate(EncClientInfo_Ctx_t* ctx,uint32_t max_bitrate,uint32_t min_bitrate)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	struct bitrate{
		int32_t max;
		int32_t min;
	}target_bitrate;

	target_bitrate.max = max_bitrate;
	target_bitrate.min = min_bitrate;

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_IDR_REQ;
	enc_msg->chn = ctx->chn;

	memcpy(&enc_msg->data,&target_bitrate,sizeof(target_bitrate));

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_chn_set_qp(EncClientInfo_Ctx_t* ctx,int16_t maxQp,int16_t minQp)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	struct qp_bounds {
		int32_t maxQp;
		int32_t minQp;
	}qp_param;

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_IDR_REQ;
	enc_msg->chn = ctx->chn;

	qp_param.maxQp = maxQp;
	qp_param.minQp = minQp;
	memcpy(&enc_msg->data,&qp_param,sizeof(qp_param));

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}


static int32_t encoder_chn_req_idr(EncClientInfo_Ctx_t* ctx)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_IDR_REQ;
	enc_msg->chn = ctx->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}


static int32_t encoder_chn_send_src(EncClientInfo_Ctx_t* ctx,DataBuffer_t* buf)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	if(buf->vaddr != (void*)-1){
		munmap(buf->vaddr,buf->size);
		close(buf->fd);
	}
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_SEND_SRC_BUF;
	enc_msg->chn = ctx->chn;
	memcpy(enc_msg->data,buf,sizeof(DataBuffer_t));
	enc_msg->data_len = sizeof(DataBuffer_t);
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        ret = binder_io_get_uint32(&reply);
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ret); */
		if(ret == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t encoder_chn_release_stream(EncClientInfo_Ctx_t* ctx,DataBuffer_t* stream)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();
	munmap(stream->vaddr,stream->size);
	close(stream->fd);

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_RELEASE_STREAM;
	enc_msg->chn = ctx->chn;
	memcpy(enc_msg->data,stream,sizeof(DataBuffer_t));
	enc_msg->data_len = sizeof(DataBuffer_t);

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        ret = binder_io_get_uint32(&reply);
		if(ret == ISS_SUCCESS){
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ret); */
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
	return -1;

}


static int32_t encoder_chn_get_srcbuf(EncClientInfo_Ctx_t* ctx,DataBuffer_t* buf)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_GET_SRC_BUF;
	enc_msg->chn = ctx->chn;
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	DataBuffer_t* rbuf = NULL;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ctx->ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
		int rfd = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		rbuf = (DataBuffer_t*)repdata;
		rfd = binder_io_get_fd(&reply,0);
		uint32_t* rbufvaddr = mmap(NULL,rbuf->size,PROT_READ | PROT_WRITE,MAP_SHARED,rfd,0);
		/* printf("%s %d rbufvaddr = 0x%x rfd = %d remotefd = %d size = %d ##\r\n",__func__,__LINE__,rbufvaddr,rfd,rbuf->fd,rbuf->size); */
		buf->fd = rfd;
		buf->vaddr = rbufvaddr;
		buf->paddr = rbuf->paddr;
		buf->index = rbuf->index;
		buf->size = rbuf->size;
        binder_cmd_freebuf(ti, reply.data0);
		return 0;
	}
	return -1;
}


void* stream_process_thread(void* arg)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)arg;
	DataBuffer_t* stream = NULL;

	for(;;){
		if(ctx->work_sta == true){
			stream = (DataBuffer_t*)dequeue_fifo(&ctx->ready_buf_fifo,WAIT_FOREVER);
			if(stream){
				if(ctx->endCb->enc_finish_cb){
					ctx->endCb->enc_finish_cb(stream,ctx->endCb->priv);
				}
				encoder_chn_release_stream(ctx,stream);
			}
		} else {
			usleep(5 * 1000);
		}
	}
}

// 创建编码通道
EncChnHandle_t ISS_CreateEncodeChn(EncoderChnParam_t* param)
{
	if(!param)
		return NULL;
	if(g_ipcHandle == 0 && g_cnt == 0){
		INIT_LIST_HEAD(&encoder_manager_list);
		g_ipcHandle = binder_get_service(ENCODER_SERVICE_NAME);
		g_binder_thread_run = false;
	}
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)malloc(sizeof(EncClientInfo_Ctx_t));
	if(!ctx){
		printf("%s %d encoder_chn_init failed !!\r\n",__func__,__LINE__);
		return NULL;
	}
	memset(ctx,0,sizeof(EncClientInfo_Ctx_t));
	if(param->dstBufferNum <= 0){
		free(ctx);
		return NULL;
	}
	ctx->ready_buf = (DataBuffer_t*)malloc(param->dstBufferNum * sizeof(DataBuffer_t));
	if(!ctx->ready_buf){
		printf("%s %d encoder_chn_init failed !!\r\n",__func__,__LINE__);
		free(ctx);
		return NULL;
	}
	ctx->ipcHandle = g_ipcHandle;
	init_fifo(&ctx->ready_buf_fifo,param->dstBufferNum );
	int32_t ret = encoder_cli_register(ctx);
	if(ret){
		printf("%s %d encoder_reg failed !!\r\n",__func__,__LINE__);
		free(ctx);
		return NULL;
	}
	ret = encoder_chn_init(ctx,param);
	if(ret){
		printf("%s %d encoder_chn_init failed !!\r\n",__func__,__LINE__);
		encoder_cli_unregister(ctx);
		free(ctx);
		return NULL;
	}
	memcpy(&ctx->ch_param,param,sizeof(EncoderChnParam_t));
	ret = pthread_create(&ctx->stream_tid,NULL,stream_process_thread,ctx);
	if(ret){
		printf("create stream thread failed ####\r\n");
		encoder_chn_deinit(ctx);
		encoder_cli_unregister(ctx);
		free(ctx->ready_buf);
		deinit_fifo(&ctx->ready_buf_fifo);
		free(ctx);
		return NULL;
	}
	list_add_head(&ctx->list,&encoder_manager_list);
	pthread_mutex_lock(&g_mutex);
	ctx->work_sta = false;
	g_cnt += 1;
	pthread_mutex_unlock(&g_mutex);

	return (EncChnHandle_t)ctx;
}

// 销毁编码通道
int ISS_DestroyEncodeChn(EncChnHandle_t handle)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;

	pthread_mutex_lock(&g_mutex);
	if(ctx->work_sta)
		ctx->work_sta = false;
	pthread_mutex_unlock(&g_mutex);
	if(ctx->stream_tid){
		pthread_cancel(ctx->stream_tid);
		pthread_join(ctx->stream_tid,NULL);
	}

	int32_t ret = encoder_chn_deinit(ctx);
	if(ret){
		return -1;
	}
	ret = encoder_cli_unregister(ctx);
	if(ret){
		return -1;
	}
	list_del(&ctx->list);
	deinit_fifo(&ctx->ready_buf_fifo);
	free(ctx->ready_buf);
	free(ctx);
	pthread_mutex_lock(&g_mutex);
	g_cnt -= 1;
	pthread_mutex_unlock(&g_mutex);
	return 0;
}

// 设置编码完成回调函数
int ISS_EncodeChn_SetFinishCallBack(EncChnHandle_t handle,EncoderFinish_Cb_t* cb)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	if(!cb)
		return -1;
	ctx->endCb = (EncoderFinish_Cb_t*)malloc(sizeof(EncoderFinish_Cb_t));
	if(!ctx->endCb)
		return -1;
	ctx->endCb->enc_finish_cb = cb->enc_finish_cb;
	ctx->endCb->priv = cb->priv;
	return 0;
}

// 请求一个源数据Buffer (用于userdata模式,非camera)
int ISS_EncodeChn_RequestSrcBuffer(EncChnHandle_t handle,DataBuffer_t* buffer)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	if(ctx->ch_param.srcType == SRC_USERDATA){
		return encoder_chn_get_srcbuf(ctx,buffer);
	} else {
		printf("%s only support USERDATA \n",__func__);
		return -1;
	}
}

// 发送一帧原始数据到编码器
int ISS_EncodeChn_SendSrcFrame(EncChnHandle_t handle,DataBuffer_t* frame)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	if(ctx->ch_param.srcType == SRC_USERDATA){
		return encoder_chn_send_src(ctx,frame);
	} else {
		printf("%s only support USERDATA \n",__func__);
		return -1;
	}
}

// 开始编码
int ISS_EncodeChn_Start(EncChnHandle_t handle)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	return encoder_chn_start(ctx);
}

// 停止编码
int ISS_EncodeChn_Stop(EncChnHandle_t handle)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	return encoder_chn_stop(ctx);
}

// IDR 帧请求
int ISS_EncodeChn_IDR_Request(EncChnHandle_t handle)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	return encoder_chn_req_idr(ctx);
}

// QB bounds设置
int ISS_EncodeChn_SetQPBounds(EncChnHandle_t handle,int32_t maxQp,int32_t minQp)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	return encoder_chn_set_qp(ctx,maxQp,minQp);
}

// Bitrate修改
int ISS_EncodeChn_SetBitrate(EncChnHandle_t handle,uint32_t max_bitrate,uint32_t min_bitrate)
{
	EncClientInfo_Ctx_t* ctx = (EncClientInfo_Ctx_t*)handle;
	return encoder_chn_set_bitrate(ctx,max_bitrate,min_bitrate);
}

// 获取当前编码通道编码参数
int ISS_EncodeChn_GetEncParam(EncChnHandle_t handle,EncodeParam_t *param)
{
	return 0;
}

// 获取编码通道的状态
int ISS_EncodeChn_GetStat(EncChnHandle_t handle)
{

	return 0;
}



