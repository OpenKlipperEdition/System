#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "impp.h"
#include "icamera.h"
#include "codec.h"
#include "iss_common.h"
#include "list.h"
#include "fifo.h"
#include "dmabuf_allocator.h"
#include "camera_service.h"
#include "encoder.h"
#include "encoder_service.h"
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "systemserver_config.h"
#include "sys_common.h"

#define LOG_TAG "ISS_ENCS"
#include "dlog.h"
typedef struct {
	int32_t camera_service_handle;
	uint32_t cam_ser_ipc_magic;
	char camera_node[16];
	IHAL_CameraHandle_t* mpp_camera_hdl;
	IHal_CodecHandle_t* mpp_encoder_handle;
	struct IHal_MemHandle* memHandle;
	pthread_t srcdata_tid;
	pthread_t stream_tid;
	pthread_mutex_t lock;
	bool src_work_sta;
	bool stream_work_sta;
	int32_t chn;
	uint32_t cli_ipc_handle;
	struct list_head list;
	DataBuffer_t srcbuf[3];
	fifo_t src_empty_fifo;
	fifo_t src_full_fifo;
	DataBuffer_t dstbuf[3];
	fifo_t dst_empty_fifo;
	struct binder_death* client_death;
	uint8_t* binder_buf;
}enc_client_ctx_t;


typedef struct {
	uint32_t client_count;
	/* uint32_t client_sta_mask; */
	struct list_head client_manager_list;
	pthread_mutex_t lock;
}encoder_service_info_t;

typedef struct {
	int32_t chn;
	enc_client_ctx_t* ctx;
}client_death_priv_t;

encoder_service_info_t enc_service;

static uint32_t register_cam_client(int32_t handle,tBinderService* cb)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = (iss_camera_msg_t*)&sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CLIENT_REGISTER;
	cam_msg->ipc_magic = 0;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	binder_io_append_obj(&data, cb);

	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        uint32_t ipc_magic = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
		return ipc_magic;
	}
	return 0;
}

static int32_t unregister_cam_client(uint32_t magic,int32_t handle)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = (iss_camera_msg_t*)&sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CLIENT_UNREGISTER;
	cam_msg->ipc_magic = magic;

	tIpcThreadInfo* ti = binder_get_thread_info();

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t exec_result = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply,handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        exec_result = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(exec_result == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t camera_dev_bind(int32_t handle,uint32_t magic,char* node)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = (iss_camera_msg_t*)&sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  BIND_REQUEST;
	cam_msg->ipc_magic = magic;
	strncpy(cam_msg->data,node,strlen(node));

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t exec_result = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        exec_result = binder_io_get_uint32(&reply);
		/* printf("%s ret = 0x%x ###\r\n",__func__,exec_result); */
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(exec_result == 0xFFFF)
		return -1;
	else
		return 0;
}


static int32_t camera_dev_unbind(int32_t handle,uint32_t magic,char* node)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = (iss_camera_msg_t*)&sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  UNBIND_REQUEST;
	cam_msg->ipc_magic = magic;
	strncpy(cam_msg->data,node,strlen(node));

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t exec_result = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        exec_result = binder_io_get_uint32(&reply);
		/* printf("%s ret = 0x%x ###\r\n",__func__,exec_result); */
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(exec_result == 0xFFFF)
		return -1;
	else
		return 0;
}

static enc_client_ctx_t* find_client_ctx(iss_encoder_msg_t *msg)
{

	enc_client_ctx_t* ctx = NULL;
	pthread_mutex_lock(&enc_service.lock);
	list_for_each_entry(ctx,&enc_service.client_manager_list,list){
		if(ctx->chn == msg->chn){
			pthread_mutex_unlock(&enc_service.lock);
			return ctx;
		}
	}
	pthread_mutex_unlock(&enc_service.lock);
	LOGE("can not find client_ctx");
	return NULL;
}

static int cam_client_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	return 0;
}

static tBinderService camera_cli = {
    .transact_cb = cam_client_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};


static int32_t camera_init(enc_client_ctx_t* ctx,int32_t width,int32_t height,int32_t buf_num,char* cam_node,int32_t fmt)
{
	ctx->camera_service_handle =  binder_get_service(CAMERA_SERVICE_NAME);
	if(ctx->camera_service_handle <= 0){
		LOGE("can not find camera service !!");
		return -1;
	}
	ctx->cam_ser_ipc_magic = register_cam_client(ctx->camera_service_handle,&camera_cli);
	if(ctx->cam_ser_ipc_magic == 0){
		LOGE("register_cam_client failed ###");
		return -1;
	}
	camera_dev_bind(ctx->camera_service_handle,ctx->cam_ser_ipc_magic,cam_node);
	IHAL_CameraHandle_t* camera_hdl = IHal_CameraOpen(cam_node);
	if(!camera_hdl){
		LOGE("camera node [%s] open failed !!!",cam_node);
		return -1;
	}

	IHAL_CAMERA_PARAMS params = {
		.imageWidth = width,
   		.imageHeight = height,
   		.imageFmt = fmt,
   		.imageFmtStr = NULL,
	};

	int32_t ret = IHal_CameraSetParams(camera_hdl,&params);
	if(ret){
		LOGE("set  camera param failed ");
		goto close_camera;
	}

	ret = IHal_CameraCreateBuffers(camera_hdl,IMPP_INTERNAL_BUFFER, buf_num);
	if(ret < 0){
		LOGE("set camera buffer failed");
		goto close_camera;
	}
	memcpy(ctx->camera_node,cam_node,strlen(cam_node));
	ctx->mpp_camera_hdl =  camera_hdl;
	return 0;
close_camera:
	IHal_CameraClose(camera_hdl);
	return -1;
}

static int32_t camera_deinit(enc_client_ctx_t* ctx)
{
	camera_dev_unbind(ctx->camera_service_handle,ctx->cam_ser_ipc_magic,ctx->camera_node);
	unregister_cam_client(ctx->cam_ser_ipc_magic,ctx->camera_service_handle);
	return IHal_CameraClose(ctx->mpp_camera_hdl);
}


static void fill_h264_init_param(IHal_CodecParam* param,ENC_H26x_Param_t* target_param)
{
	param->codec_type = H264_ENC;
	param->codecparam.h264e_param.rc_mode = target_param->rc_mode;
	param->codecparam.h264e_param.target_bitrate = target_param->bitrate;
	param->codecparam.h264e_param.max_bitrate    = target_param->bitrate + 1000;
	param->codecparam.h264e_param.gop_len        = target_param->gop;
	param->codecparam.h264e_param.initial_Qp     = 25;
	param->codecparam.h264e_param.mini_Qp        = 10;
	param->codecparam.h264e_param.max_Qp         = 48;
	param->codecparam.h264e_param.level          = 20;
	param->codecparam.h264e_param.maxPSNR        = 40;
	param->codecparam.h264e_param.freqIDR        = 30;
	param->codecparam.h264e_param.frameRateNum   = target_param->frameRate;
	param->codecparam.h264e_param.frameRateDen   = 1;
	param->codecparam.h264e_param.enc_width      = target_param->enc_width;
	param->codecparam.h264e_param.enc_height     = target_param->enc_height;
	param->codecparam.h264e_param.src_width      = target_param->enc_width;
	param->codecparam.h264e_param.src_height     = target_param->enc_height;
	param->codecparam.h264e_param.src_fmt        = target_param->srcfmt;
}

static void fill_h265_init_param(IHal_CodecParam* param,ENC_H26x_Param_t* target_param)
{
	param->codec_type = H265_ENC;
	param->codecparam.h265e_param.rc_mode = target_param->rc_mode;
	param->codecparam.h265e_param.target_bitrate = target_param->bitrate;
	param->codecparam.h265e_param.max_bitrate    = target_param->bitrate + 1000;
	param->codecparam.h265e_param.gop_len        = target_param->gop;
	param->codecparam.h265e_param.initial_Qp     = 25;
	param->codecparam.h265e_param.mini_Qp        = 10;
	param->codecparam.h265e_param.max_Qp         = 48;
	param->codecparam.h265e_param.level          = 20;
	param->codecparam.h265e_param.maxPSNR        = 40;
	param->codecparam.h265e_param.freqIDR        = 30;
	param->codecparam.h265e_param.frameRateNum   = target_param->frameRate;
	param->codecparam.h265e_param.frameRateDen   = 1;
	param->codecparam.h265e_param.enc_width      = target_param->enc_width;
	param->codecparam.h265e_param.enc_height     = target_param->enc_height;
	param->codecparam.h265e_param.src_width      = target_param->enc_width;
	param->codecparam.h265e_param.src_height     = target_param->enc_height;
	param->codecparam.h265e_param.src_fmt        = target_param->srcfmt;
}

static void fill_jpeg_init_param(IHal_CodecParam* param,ENC_JPEG_Param_t* target_param)
{
	param->codec_type = JPEG_ENC;
	param->codecparam.jpegenc_param.initialQp = target_param->qp;
	param->codecparam.jpegenc_param.quality = target_param->quality;
	param->codecparam.jpegenc_param.enc_width      = target_param->enc_width;
	param->codecparam.jpegenc_param.enc_height     = target_param->enc_height;
	param->codecparam.jpegenc_param.src_width      = target_param->enc_width;
	param->codecparam.jpegenc_param.src_height     = target_param->enc_height;
	param->codecparam.jpegenc_param.src_fmt        = target_param->srcfmt;
}


// 根据数据源的类型，决定是否需要绑定camera
// 如果数据源为userdata,则srcbuf和dstbuf都使用dma allocater申请
// 如果数据源为camera，则进行摄像头的服务绑定，初始化，并和encoder进行关联绑定;dst使用
// dma allocater申请buffer
static int32_t encoder_init(enc_client_ctx_t *ctx,EncoderChnParam_t* chnParam)
{

	IHal_CodecHandle_t* handle = NULL;
	IHal_CodecParam encode_param;
	memset(&encode_param, 0, sizeof(IHal_CodecParam));
	int32_t srcbuf_num = chnParam->srcBufferNum;
	int32_t dstbuf_num = chnParam->dstBufferNum;
	int32_t srcfmt = 0;
	int32_t width = 0;
	int32_t height = 0;
	switch(chnParam->param.type){
		case ENC_JPEG:
			width = chnParam->param.EncParam.jpeg_param.enc_width;
			height = chnParam->param.EncParam.jpeg_param.enc_height;
			srcfmt = chnParam->param.EncParam.jpeg_param.srcfmt;
			fill_jpeg_init_param(&encode_param,&chnParam->param.EncParam.jpeg_param);
			handle = IHal_CodecCreate(JPEG_ENC);break;
		case ENC_H264:
			width = chnParam->param.EncParam.h26x_param.enc_width;
			height = chnParam->param.EncParam.h26x_param.enc_height;
			srcfmt = chnParam->param.EncParam.h26x_param.srcfmt;
			fill_h264_init_param(&encode_param,&chnParam->param.EncParam.h26x_param);
			handle = IHal_CodecCreate(H264_ENC);break;
		case ENC_H265:
			width = chnParam->param.EncParam.h26x_param.enc_width;
			height = chnParam->param.EncParam.h26x_param.enc_height;
			srcfmt = chnParam->param.EncParam.h26x_param.srcfmt;
			fill_h265_init_param(&encode_param,&chnParam->param.EncParam.h26x_param);
			handle = IHal_CodecCreate(H265_ENC);break;
		default:
			LOGE("invalid encoder type");
			printf("%s %d ####\r\n",__func__,__LINE__);
			return -1;
	}
	if (!handle) {
		LOGE("codec create failed");
			printf("%s %d ####\r\n",__func__,__LINE__);

		return -1;
	}
	ctx->mpp_encoder_handle = handle;
	int ret =  IHal_Codec_SetParams(handle, &encode_param);
	if (ret) {
	        LOGE("set codec param failed");
			IHal_CodecDestroy(ctx->mpp_encoder_handle);
			return -1;
	}

	IHal_Codec_CreateSrcBuffers(handle, IMPP_EXT_USERBUFFER, srcbuf_num);
	char mem_name [16];
	memset(mem_name,0,16);
	sprintf(mem_name,"encoder_ch%2d",ctx->chn);
	ctx->memHandle = IHal_MemInit("encoder_chn");
	if(!ctx->memHandle){
	    LOGE("mem moudle init failed");
		IHal_CodecDestroy(ctx->mpp_encoder_handle);
		return -1;
	}
	if(chnParam->srcType == SRC_USERDATA){
		// 创建源数据buffer，待客户端发起使用请求
		// 使用dma allocater申请内存
		// codec 创建为 IMPP_EXT_DMABUFFER
		ctx->mpp_camera_hdl = NULL;
		init_fifo(&ctx->src_empty_fifo,srcbuf_num);
		init_fifo(&ctx->src_full_fifo,srcbuf_num);
		IMPP_BufferInfo_t buf;
		int32_t size = width * height * 3 / 2;
		for(int i = 0; i < srcbuf_num; i++){
			IHal_MemAlloc(ctx->memHandle,size,&buf);
			IHal_Codec_SetSrcBuffer(ctx->mpp_encoder_handle,i,&buf);
			ctx->srcbuf[i].fd = buf.fd;
			ctx->srcbuf[i].paddr = buf.paddr;
			ctx->srcbuf[i].vaddr = buf.vaddr;
			ctx->srcbuf[i].size = buf.size;
			ctx->srcbuf[i].index = i;
			queue_fifo(&ctx->src_empty_fifo,&ctx->srcbuf[i],NO_WAIT);
		}
	} else if(chnParam->srcType == SRC_CAMERA){
		// 源数据为camera
		// 初始化camera节点，
		ret = camera_init(ctx,width,height,srcbuf_num,chnParam->videoNode,srcfmt);
		if(ret){
			IHal_MemDeinit(ctx->memHandle);
			IHal_CodecDestroy(ctx->mpp_encoder_handle);
			return -1;
		}
        IMPP_BufferInfo_t share;
		for (int i = 0; i < srcbuf_num; i++) {
			ret = IHal_GetCameraBuffers(ctx->mpp_camera_hdl, i, &share);
        	if (ret) {
				LOGE("get camera buffer failed");
				camera_deinit(ctx);
				IHal_CodecDestroy(ctx->mpp_encoder_handle);
				return -1;
        	}
        	//printf("v4l2 expt-dma-fd = %d\n", share.fd);
        	ret = IHal_Codec_SetSrcBuffer(handle, i, &share);
        	if (ret) {
				LOGE("set h264 encoder src buffer failed");
				camera_deinit(ctx);
				IHal_CodecDestroy(ctx->mpp_encoder_handle);
				return -1;
        	}
        }
	} else {
		LOGE("invalid srcType !!!!\n");
		IHal_CodecDestroy(ctx->mpp_encoder_handle);
		IHal_MemDeinit(ctx->memHandle);		// will release all dmabuf
		return -1;
	}
	// dst buffer使用 dma allocater申请
	init_fifo(&ctx->dst_empty_fifo,dstbuf_num);
	IHal_Codec_CreateDstBuffer(handle,IMPP_EXT_USERBUFFER, dstbuf_num);
	IMPP_BufferInfo_t buf;
	int32_t allocsize = 1024 * 1024;	//TODO
	int32_t* vaddr = NULL;
	for(int i = 0; i < dstbuf_num; i++){
		vaddr = IHal_MemAlloc(ctx->memHandle,allocsize,&buf);
		if(vaddr != NULL){
			IHal_Codec_SetDstBuffer(ctx->mpp_encoder_handle, i,&buf);
			ctx->dstbuf[i].fd = buf.fd;
			ctx->dstbuf[i].paddr = buf.paddr;
			ctx->dstbuf[i].vaddr = buf.vaddr;
			ctx->dstbuf[i].size = buf.size;
			ctx->dstbuf[i].index = i;
		} else {
			if(chnParam->srcType == SRC_CAMERA){
				camera_deinit(ctx);
			}
			IHal_CodecDestroy(ctx->mpp_encoder_handle);
			return -1;
		}
		/* queue_fifo(&ctx->dst_empty_fifo,&ctx->dstbuf[i],NO_WAIT); */
	}

	return 0;
}

static int32_t encoder_deinit(enc_client_ctx_t* ctx)
{
	if(ctx->cam_ser_ipc_magic > 0 && ctx->mpp_camera_hdl != NULL){
		camera_deinit(ctx);
	} else {
		deinit_fifo(&ctx->src_empty_fifo);
		deinit_fifo(&ctx->src_full_fifo);
	}
	deinit_fifo(&ctx->dst_empty_fifo);
    IHal_CodecDestroy(ctx->mpp_encoder_handle);
	ctx->mpp_encoder_handle = NULL;

	IHal_MemDeinit(ctx->memHandle);		// will release all dmabuf
	return 0;
}


void* userdata_thread_func(void* arg)
{
	enc_client_ctx_t* ctx = (enc_client_ctx_t*)arg;
	int ret = 0;
    IMPP_BufferInfo_t encoder_buf;
    IHAL_CodecStreamInfo_t stream;
	DataBuffer_t* dstbuf;
	DataBuffer_t* srcdata;
	for(;;){
		if(ctx->src_work_sta){
			dstbuf = dequeue_fifo(&ctx->dst_empty_fifo,NO_WAIT);
			if(dstbuf != NULL){
				memset(&stream,0,sizeof(stream));
				stream.fd = dstbuf->fd;
				stream.vaddr = dstbuf->vaddr;
				stream.index = dstbuf->index;
				IHal_Codec_QueueDstBuffer(ctx->mpp_encoder_handle, &stream);
			} else {
				/* printf("get buffer element failed ,elenum: %d ##\r\n",getFifoElemNum(&ctx->dst_empty_fifo)); */
			}

			srcdata = (DataBuffer_t*)dequeue_fifo(&ctx->src_full_fifo,NO_WAIT);
			if(srcdata){
				encoder_buf.fd = srcdata->fd;
				encoder_buf.index = srcdata->index;
				encoder_buf.paddr = srcdata->paddr;
				encoder_buf.vaddr = srcdata->vaddr;
				encoder_buf.size = srcdata->size;
				IHal_MemFlush(ctx->memHandle,encoder_buf, MEM_FLUSH_START);
				IHal_MemFlush(ctx->memHandle,encoder_buf, MEM_FLUSH_END);
        		IHal_Codec_QueueSrcBuffer(ctx->mpp_encoder_handle, &encoder_buf);
			}
		} else {
			usleep(32* 1000);
		}
	}
}

void* camera_thread_func(void* arg)
{
	enc_client_ctx_t* ctx = (enc_client_ctx_t*)arg;
    IMPP_BufferInfo_t camera_buf;
    IHAL_CodecStreamInfo_t stream;
	DataBuffer_t* dstbuf;
	int ret = 0;
	for(;;){
		if(ctx->src_work_sta){
			memset(&camera_buf,0,sizeof(camera_buf));
			ret = IHal_Camera_WaitBufferAvailable(ctx->mpp_camera_hdl, IMPP_NO_WAIT);
        	if (!ret) {
        	 	IHal_CameraDeQueueBuffer(ctx->mpp_camera_hdl, &camera_buf);
        	 	IHal_Codec_QueueSrcBuffer(ctx->mpp_encoder_handle, &camera_buf);
        	} else {
				usleep(5*1000);
			}
			//usleep(5*1000);	// 5ms sch
			dstbuf = dequeue_fifo(&ctx->dst_empty_fifo,NO_WAIT);
			if(dstbuf != NULL){
				memset(&stream,0,sizeof(stream));
				stream.fd = dstbuf->fd;
				stream.vaddr = dstbuf->vaddr;
				stream.index = dstbuf->index;
				IHal_Codec_QueueDstBuffer(ctx->mpp_encoder_handle, &stream);
			} else {
				/* printf("get buffer element failed ,elenum: %d ##\r\n",getFifoElemNum(&ctx->dst_empty_fifo)); */
			}

		} else {
			usleep(30* 1000);
		}
	}
}

static int32_t send_stream_to_client(enc_client_ctx_t* ctx,DataBuffer_t* buf,int32_t streamsize)
{
	tBinderIo data;
	tBinderIo reply;
	/* char binder_buf[384]; */
	iss_encoder_msg_t enc_msg;
	DataBuffer_t* send_data = (DataBuffer_t*)enc_msg.data;
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_io_init(&data, ctx->binder_buf,1024,DEFAULT_OFFSET_LIST_SIZE);
	enc_msg.cmd = ENCODER_SYNC_STREAM;
	enc_msg.chn = ctx->chn;
	memcpy(enc_msg.data,buf,sizeof(DataBuffer_t));
	send_data->size = streamsize;
	enc_msg.data_len = sizeof(DataBuffer_t);
	binder_io_append_data(&data, (char *)&enc_msg, sizeof(iss_encoder_msg_t));
	binder_io_append_fd(&data,buf->fd);
	int32_t ret = binder_cmd_async_call(ti, &data,NULL,ctx->cli_ipc_handle, 0);
	if(ret){
		return -1;
	}
	return 0;
}

void* get_stream_thread_func(void* arg)
{
	enc_client_ctx_t* ctx = (enc_client_ctx_t*)arg;
    IHAL_CodecStreamInfo_t stream;
    IMPP_BufferInfo_t encoder_buf;
	DataBuffer_t* dstbuf;
	int ret = 0;
	for(;;){
		if(ctx->stream_work_sta){
			ret = IHal_Codec_WaitSrcAvailable(ctx->mpp_encoder_handle, IMPP_NO_WAIT);
			if(!ret){
				ret = IHal_Codec_DequeueSrcBuffer(ctx->mpp_encoder_handle, &encoder_buf);
				if(!ret){
					if(ctx->mpp_camera_hdl == NULL)
						queue_fifo(&ctx->src_empty_fifo,&ctx->srcbuf[encoder_buf.index],NO_WAIT);
					else
						IHal_CameraQueuebuffer(ctx->mpp_camera_hdl, &encoder_buf);
				}
			} else {
				usleep(5*1000);
			}
			ret = IHal_Codec_WaitDstAvailable(ctx->mpp_encoder_handle, IMPP_NO_WAIT);
        	if (!ret) {
				memset(&stream,0,sizeof(stream));
				ret = IHal_Codec_DequeueDstBuffer(ctx->mpp_encoder_handle, &stream);
				if(!ret){
					ret = send_stream_to_client(ctx,&ctx->dstbuf[stream.index],stream.size);
				}
			} else {
				usleep(5*1000);
			}
		} else {
			usleep(32* 1000);
		}
	}
}

static int param_check(EncoderChnParam_t* chnParam)
{
	if(chnParam->srcType == SRC_CAMERA){
		if(strlen(chnParam->videoNode) <= 0)
			return -1;
	}
	if(chnParam->srcType != SRC_CAMERA && chnParam->srcType != SRC_USERDATA)
		return -1;
	if(chnParam->srcBufferNum <=0 || chnParam->dstBufferNum <= 0){
		return -1;
	}
	return 0;
}

int32_t iss_encoder_chn_init(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_CHN_INIT;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		LOGE("find client ctx failed !!");
		printf("find client ctx failed !! \r\n");
		rep.exec_result = ISS_EXEC_FAILED;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	EncoderChnParam_t* ch_param = (EncoderChnParam_t*)&msg->data;
	int32_t ret = param_check(ch_param);
	if(ret){
		rep.exec_result = ISS_EXEC_FAILED;
		printf(" init param failed !! \r\n");
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	ret = encoder_init(ctx,ch_param);
	if(ret){
		LOGE("encoder init failed !!");
		printf("encoder init failed !! \r\n");
		rep.exec_result = ISS_EXEC_FAILED;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	if(ctx->mpp_camera_hdl != NULL){
		pthread_create(&ctx->srcdata_tid,NULL,camera_thread_func,ctx);
	} else {
		pthread_create(&ctx->srcdata_tid,NULL,userdata_thread_func,ctx);
	}
	pthread_create(&ctx->stream_tid,NULL,get_stream_thread_func,ctx);
	rep.exec_result = ISS_SUCCESS;
	binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
	return 0;
}

int32_t iss_encoder_chn_deinit(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_CHN_DEINIT;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		LOGE("find client ctx failed !!");
		rep.exec_result = ISS_EXEC_FAILED;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	if(ctx->mpp_encoder_handle == NULL){
		LOGE("find client ctx failed !!");
		rep.exec_result = ISS_EXEC_FAILED;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	// TODO lock
	if(ctx->stream_tid && ctx->srcdata_tid){
		if(ctx->stream_work_sta)
			ctx->stream_work_sta = false;
		if(ctx->src_work_sta)
			ctx->src_work_sta = false;
	}
	pthread_cancel(ctx->stream_tid);
	pthread_join(ctx->stream_tid,NULL);
	pthread_cancel(ctx->srcdata_tid);
	pthread_join(ctx->srcdata_tid,NULL);
	encoder_deinit(ctx);
	rep.exec_result = ISS_SUCCESS;
	binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
	return 0;
}

static void client_death_cb(tIpcThreadInfo *info, void* priv)
{
	client_death_priv_t* clipriv = (client_death_priv_t*)priv;
	enc_client_ctx_t* ctx = NULL;
	enc_client_ctx_t* tmp = NULL;
	pthread_mutex_lock(&enc_service.lock);
	list_for_each_entry(tmp,&enc_service.client_manager_list,list){
		if(tmp->chn == clipriv->chn){
			ctx = tmp;
			break;
		}
	}
	pthread_mutex_unlock(&enc_service.lock);
	if(!ctx){
		free(priv);
		return;
	}
	if(ctx->mpp_encoder_handle != NULL && ctx->stream_tid && ctx->srcdata_tid){
		ctx->stream_work_sta = false;
		ctx->src_work_sta = false;

        IHal_Codec_Stop(ctx->mpp_encoder_handle);
		if(ctx->mpp_camera_hdl != NULL)
			IHal_CameraStop(ctx->mpp_camera_hdl);

		pthread_cancel(ctx->stream_tid);
		pthread_join(ctx->stream_tid,NULL);
		pthread_cancel(ctx->srcdata_tid);
		pthread_join(ctx->srcdata_tid,NULL);
		encoder_deinit(ctx);
	}
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_release(ti,ctx->cli_ipc_handle);
	pthread_mutex_lock(&enc_service.lock);
	enc_service.client_count -= 1;
	list_del(&ctx->list);
	pthread_mutex_unlock(&enc_service.lock);
	free(ctx->client_death);
	free(ctx->binder_buf);
	free(ctx);
	free(priv);
	//printf("%s %d ####\r\n",__func__,__LINE__);
}



static int32_t iss_client_obj_register(tBinderIo* msg,tBinderIo* reply)
{
	enc_client_ctx_t* ctx = (enc_client_ctx_t*)malloc(sizeof(enc_client_ctx_t));
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_CLI_REGISTER;
	if(!ctx){
		printf("%s %d  failed !!\r\n",__func__,__LINE__);
		rep.exec_result = ISS_FAILED;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	memset(ctx,0,sizeof(enc_client_ctx_t));
	uint32_t hdl = binder_io_get_ref(msg, 0);
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_acquire(ti, hdl);
	flush_commands(ti);
	ctx->binder_buf = (uint8_t*)malloc(1024);
	ctx->cli_ipc_handle = hdl;
	ctx->stream_tid = 0;
	ctx->srcdata_tid = 0;
	ctx->mpp_camera_hdl = NULL;
	ctx->mpp_encoder_handle = NULL;
	ctx->client_death = (struct binder_death*)malloc(sizeof(struct binder_death));
	if(!ctx->client_death){
		printf("%s %d  failed !!\r\n",__func__,__LINE__);
		free(ctx);
		rep.exec_result = ISS_FAILED;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	client_death_priv_t *death_priv = (client_death_priv_t*)malloc(sizeof(client_death_priv_t));
	death_priv->ctx = ctx;
	death_priv->chn = ctx->chn;
	ctx->client_death->death_cb = client_death_cb;
	ctx->client_death->ptr = death_priv;
	binder_cmd_link_to_death(ti,ctx->cli_ipc_handle,ctx->client_death);

	pthread_mutex_lock(&enc_service.lock);
	ctx->chn = enc_service.client_count;
	enc_service.client_count += 1;
	list_add_head(&ctx->list,&enc_service.client_manager_list);
	pthread_mutex_unlock(&enc_service.lock);
	rep.exec_result = ISS_SUCCESS;
	rep.retval = ctx->chn;
	binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
	return 0;
}

static int32_t iss_client_obj_unregister(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_CLI_UNREGISTER;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_release(ti,ctx->cli_ipc_handle);
	pthread_mutex_lock(&enc_service.lock);
	if(enc_service.client_count > 0)
		enc_service.client_count -= 1;
	list_del(&ctx->list);
	pthread_mutex_unlock(&enc_service.lock);
	free(ctx->client_death);
	free(ctx->binder_buf);
	free(ctx);
	rep.exec_result = ISS_SUCCESS;
	binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
	return 0;
}

int32_t iss_encoder_chn_start(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_CHN_START;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	if(ctx->mpp_camera_hdl != NULL){
		if(IHal_CameraStart(ctx->mpp_camera_hdl)){
			rep.exec_result = ISS_FAILED;
			binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
			return -1;
		}
	}
	ctx->src_work_sta = true;
	if(ctx->mpp_encoder_handle != NULL){
		if(IHal_Codec_Start(ctx->mpp_encoder_handle)){
			rep.exec_result = ISS_FAILED;
			binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		}
		ctx->stream_work_sta = true;
	}
	rep.exec_result = ISS_SUCCESS;
	binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
	return 0;
}

int32_t iss_encoder_chn_stop(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_CHN_STOP;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
	pthread_mutex_lock(&ctx->lock);
	ctx->stream_work_sta = false;
	ctx->src_work_sta = false;
	pthread_mutex_unlock(&ctx->lock);
	if(ctx->mpp_camera_hdl != NULL){
		IHal_CameraStop(ctx->mpp_camera_hdl);
	}
	if(ctx->mpp_encoder_handle != NULL){
		IHal_Codec_Stop(ctx->mpp_encoder_handle);
	}
	rep.exec_result = ISS_SUCCESS;
	binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
	return 0;
}

int32_t iss_encoder_chn_release_stream(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	rep.cmd = ENCODER_RELEASE_STREAM;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}
	DataBuffer_t* data = (DataBuffer_t*)&msg->data[0];
	if(data->index > 3){
		rep.exec_result = ISS_FAILED;
		binder_io_append_uint32(reply,ISS_FAILED);
		return -1;
	}
	rep.exec_result = ISS_SUCCESS;
	int ret = queue_fifo(&ctx->dst_empty_fifo,&ctx->dstbuf[data->index],NO_WAIT);
	if(ret){
		binder_io_append_uint32(reply,ISS_FAILED);
		return -1;
	}
	/* printf("%s %d ###\r\n",__func__,__LINE__); */
	/* binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t)); */
	binder_io_append_uint32(reply,ISS_SUCCESS);
	return 0;
}

int32_t iss_recv_srcbuf(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}
	DataBuffer_t* data = (DataBuffer_t*)&msg->data[0];
	if(data->index > 3){
		rep.exec_result = ISS_FAILED;
		binder_io_append_uint32(reply,ISS_FAILED);
		return -1;
	}
	rep.exec_result = ISS_SUCCESS;
	int ret = queue_fifo(&ctx->src_full_fifo,&ctx->srcbuf[data->index],NO_WAIT);
	if(ret){
		binder_io_append_uint32(reply,ISS_FAILED);
		return -1;
	}
	binder_io_append_uint32(reply,ISS_SUCCESS);
	return 0;
}

int32_t iss_process_buf_req(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		/* rep.exec_result = ISS_CLIENT_NOT_EXIST; */
		binder_io_append_uint32(reply,0);
		return -1;
	}
	DataBuffer_t* data = dequeue_fifo(&ctx->src_empty_fifo,WAIT_FOREVER);
	binder_io_append_data(reply,(char*)data,sizeof(DataBuffer_t));
	binder_io_append_fd(reply,data->fd);
	/* printf("%s append fd = %d ##\r\n",__func__,data->fd); */
	return 0;
}

int32_t iss_encoder_idr_req(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
}

int32_t iss_encoder_set_qp(iss_encoder_msg_t* msg,tBinderIo* reply)
{
	iss_encoder_reply_t rep;
	enc_client_ctx_t* ctx = find_client_ctx(msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
		return -1;
	}
}

static int client_msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int32_t msg_size = 0;
	sys_msg_t* sys_msg;
	iss_encoder_msg_t* enc_msg;
	int32_t ret = binder_io_get_data(msg,&data, &msg_size);
	sys_msg = (sys_msg_t*)data;
	enc_msg = (iss_encoder_msg_t*)sys_msg->msg;
	switch(enc_msg->cmd){
		case ENCODER_CLI_REGISTER:
			iss_client_obj_register(msg,reply);break;
		case ENCODER_CLI_UNREGISTER:
			iss_client_obj_unregister(enc_msg,reply);break;
		case ENCODER_CHN_INIT:
			iss_encoder_chn_init(enc_msg,reply);break;
		case ENCODER_CHN_DEINIT:
			iss_encoder_chn_deinit(enc_msg,reply);break;
		case ENCODER_CHN_START:
			iss_encoder_chn_start(enc_msg,reply);break;
		case ENCODER_CHN_STOP:
			iss_encoder_chn_stop(enc_msg,reply);break;
		case ENCODER_RELEASE_STREAM:
			iss_encoder_chn_release_stream(enc_msg,reply);break;
		case ENCODER_SEND_SRC_BUF:
			iss_recv_srcbuf(enc_msg,reply);break;
		case ENCODER_GET_SRC_BUF:
			iss_process_buf_req(enc_msg,reply);break;
		/* case ENCODER_SET_QP_BOUNDS: */
		/* 	iss_encoder_idr_req(enc_msg,reply);break; */
		/* case ENCODER_IDR_REQ: */
		/* 	iss_encoder_set_qp(enc_msg,reply);break; */
		default:
			LOGE("invalid cmd !!!");
			iss_encoder_reply_t rep;
			rep.cmd = enc_msg->cmd;
			rep.exec_result = ISS_INVALID_CMD;
			binder_io_append_data(reply,(char*)&rep,sizeof(iss_encoder_reply_t));
			return -1;
	}

	return 0;
}

static tBinderService encoder_service = {
    .transact_cb = client_msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int main(int argc,char** argv)
{
    IHal_CodecInit();
	memset(&enc_service,0,sizeof(encoder_service_info_t));
	INIT_LIST_HEAD(&enc_service.client_manager_list);
	pthread_mutex_init(&enc_service.lock,NULL);
	int ret = binder_add_service(ENCODER_SERVICE_NAME,&encoder_service);
	binder_thread_enter_loop(0,0);
	while(1){
		sleep(2);
	}

    IHal_CodecDeInit();
    binder_threads_shutdown();
	return 0;

}

