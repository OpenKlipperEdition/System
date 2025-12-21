#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "iss_common.h"
#include "list.h"
#include "fifo.h"
#include "encoder.h"
#include "encoder_service.h"
#include <sys/time.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include "systemserver_config.h"
#include "sys_common.h"
#include <stdio.h>

#undef CU_TRUE
#undef CU_FALSE
#define CU_TRUE 0
#define CU_FALSE 1
#include <assert.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>


typedef struct {
	int32_t chn;
	tBinderService* cb;
	DataBuffer_t ready_buf[3];
	fifo_t ready_buf_fifo;
}testClientInfo_t;


testClientInfo_t info;

static int received_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int msg_size = 0;
	sys_msg_t* sys_msg;
	int32_t ret  =  binder_io_get_data(msg,&data,&msg_size);
	if(!ret){
		/* printf("binder_io_get_data data = 0x%x size = %d ###\r\n",data,msg_size); */
		iss_encoder_msg_t* enc_msg = (iss_encoder_msg_t*)data;
		int32_t devindex = enc_msg->chn;
		if(devindex > 3){
			printf("target dev index error @@@@\r\n");
			return -1;
		}
		DataBuffer_t* buf = &enc_msg->data[0];
		uint32_t datafd = binder_io_get_fd(msg,0);
		uint32_t* imgVaddr = mmap(NULL,buf->size,PROT_READ,MAP_SHARED,datafd,0);		// camera buffer fd only PROT_READ
		if(imgVaddr != MAP_FAILED){
			info.ready_buf[buf->index].fd = datafd;
			info.ready_buf[buf->index].size = buf->size;
			info.ready_buf[buf->index].paddr = buf->paddr;
			info.ready_buf[buf->index].vaddr = imgVaddr;
			info.ready_buf[buf->index].index = buf->index;
			queue_fifo(&info.ready_buf_fifo,&info.ready_buf[buf->index],WAIT_FOREVER);
		}
		/* printf("imgVaddr = 0x%x dev_index %d buf->index = %d ####\r\n",imgVaddr,devindex,buf->index); */
	}

	return 0;
}

static tBinderService func_cb = {
    .transact_cb = received_cb,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int32_t encoder_cli_register(int32_t ipcHandle,testClientInfo_t* info)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(info,0,sizeof(testClientInfo_t));
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd =  ENCODER_CLI_REGISTER;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	binder_io_append_obj(&data, &func_cb);

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d chn = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result,ser_rep->retval); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			info->chn = ser_rep->retval;
			binder_cmd_freebuf(ti, reply.data0);
			binder_thread_enter_loop(0,0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

int32_t encoder_cli_unregister(int32_t ipcHandle,testClientInfo_t* info)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd =  ENCODER_CLI_UNREGISTER;
	enc_msg->chn = info->chn;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		char* repdata;
		int32_t sz = 0;
        binder_io_get_data(&reply,&repdata,&sz);
		ser_rep = (iss_encoder_reply_t*)repdata;
		/* printf("%s [%d] exec_result = %d ##\r\n",__func__,__LINE__,ser_rep->exec_result); */
		if(ser_rep->exec_result == ISS_SUCCESS){
			info->chn = -1;
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}


int32_t encoder_chn_init(int32_t ipcHandle,testClientInfo_t* info,EncoderChnParam_t* param)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_INIT;
	enc_msg->chn = info->chn;
	enc_msg->data_len = sizeof(EncoderChnParam_t);
	memcpy(enc_msg->data,param,sizeof(EncoderChnParam_t));

	init_fifo(&info->ready_buf_fifo,param->dstBufferNum+3);

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
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

int32_t encoder_chn_deinit(int32_t ipcHandle,testClientInfo_t* info)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_DEINIT;
	enc_msg->chn = info->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
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

int32_t encoder_chn_start(int32_t ipcHandle,testClientInfo_t* info)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_START;
	enc_msg->chn = info->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
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

int32_t encoder_chn_stop(int32_t ipcHandle,testClientInfo_t* info)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_CHN_STOP;
	enc_msg->chn = info->chn;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
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


int32_t encoder_chn_send_src(int32_t ipcHandle,testClientInfo_t* info,DataBuffer_t* buf)
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
	enc_msg->chn = info->chn;
	memcpy(enc_msg->data,buf,sizeof(DataBuffer_t));
	enc_msg->data_len = sizeof(DataBuffer_t);
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	iss_encoder_reply_t* ser_rep;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        ret = binder_io_get_uint32(&reply);
		if(ret == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

int32_t encoder_chn_release_stream(int32_t ipcHandle,testClientInfo_t* info,DataBuffer_t* stream)
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
	enc_msg->chn = info->chn;
	memcpy(enc_msg->data,stream,sizeof(DataBuffer_t));
	enc_msg->data_len = sizeof(DataBuffer_t);

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        ret = binder_io_get_uint32(&reply);
		if(ret == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}

}


int32_t encoder_chn_get_srcbuf(int32_t ipcHandle,testClientInfo_t* info,DataBuffer_t* buf)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_encoder_msg_t* enc_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_encoder_msg_t);
	enc_msg->cmd = ENCODER_GET_SRC_BUF;
	enc_msg->chn = info->chn;
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	DataBuffer_t* rbuf = NULL;
	int ret = binder_cmd_sync_call(ti, &data, &reply, ipcHandle, 0);
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

void TEST_ENC_SERVICE_GET_000(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
}

void TEST_ENC_SERVER_REGISTER_001(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

// test init param invalid_001
void TEST_ENC_SERVER_CHN_INIT_002(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 0;
	chn_param.dstBufferNum = 0;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),-1);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),-1);

	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

// test init param invalid_002
void TEST_ENC_SERVER_CHN_INIT_003(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	/* sprintf(chn_param.videoNode,"/dev/video4"); */
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),-1);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),-1);

	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

// test init param invalid_002
void TEST_ENC_SERVER_CHN_INIT_004(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = 5;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),-1);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),-1);

	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

// test init param invalid_003
void TEST_ENC_SERVER_CHN_INIT_005(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = 5;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_HSV;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),-1);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),-1);

	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

// test init normal
void TEST_ENC_SERVER_CHN_INIT_006(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),0);

	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

// test init normal
void TEST_ENC_SERVER_CHN_INIT_007(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),0);

	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

void TEST_ENC_SERVER_CHN_START_008(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_start(ipchdl,&info),0);
	sleep(1);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_stop(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}


void TEST_ENC_SERVER_CHN_START_009(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_start(ipchdl,&info),0);
	sleep(1);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_stop(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}


void TEST_ENC_SERVER_CHN_SAVE_IMG_010(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;

	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_start(ipchdl,&info),0);
	int sfd = open("TEST_ENC_SERVER_CHN_SAVE_IMG_010.jpg",O_CREAT | O_RDWR | O_TRUNC);
	int rfd = open("/1.yuv",O_RDONLY);
	if(!rfd){
		printf("open 1.yuv failed ##\n");
		return;
	}
	DataBuffer_t *data = NULL;
	DataBuffer_t srcbuf;
	for(int i = 0; i < 3; i++){
		CU_ASSERT_EQUAL_FATAL(encoder_chn_get_srcbuf(ipchdl,&info,&srcbuf),0);
		if(srcbuf.vaddr != (void*)-1){
			lseek(rfd,0,SEEK_SET);
			read(rfd,srcbuf.vaddr,srcbuf.size);
		}
		CU_ASSERT_EQUAL_FATAL(encoder_chn_send_src(ipchdl,&info,&srcbuf),0);
		data = dequeue_fifo(&info.ready_buf_fifo,WAIT_FOREVER);
		lseek(sfd,0,SEEK_SET);
		write(sfd,data->vaddr,data->size);
		CU_ASSERT_EQUAL_FATAL(encoder_chn_release_stream(ipchdl,&info,data),0);
	}
	CU_ASSERT_EQUAL_FATAL(encoder_chn_stop(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}


void TEST_ENC_SERVER_CHN_SAVE_IMG_011(void)
{
	int32_t ipchdl = binder_get_service(ENCODER_SERVICE_NAME);
	CU_ASSERT_NOT_EQUAL_FATAL(ipchdl,0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_register(ipchdl,&info),0);

	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	sprintf(chn_param.videoNode,"/dev/video4");
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_H264;

	ENC_H26x_Param_t *enc_param = &chn_param.param.EncParam.h26x_param;
	enc_param->rc_mode = RC_MODE_VBR;
	enc_param->bitrate = 1500;
	enc_param->frameRate = 30;
	enc_param->gop = 90;
	enc_param->min_qp = 30;
	enc_param->max_qp = 45;
	enc_param->idrFreq = 30;
	enc_param->enc_width = 1280;
	enc_param->enc_height = 720;
	enc_param->srcfmt = PIX_FMT_NV12;
	char file[64] = {0};
	int sfd = open("TEST_ENC_SERVER_CHN_SAVE_IMG_011.h264",O_CREAT | O_RDWR | O_TRUNC);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_init(ipchdl,&info,&chn_param),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_start(ipchdl,&info),0);
	DataBuffer_t *data = NULL;
	for(int i = 0; i < 90; i++){
		data = dequeue_fifo(&info.ready_buf_fifo,WAIT_FOREVER);
		write(sfd,data->vaddr,data->size);
		CU_ASSERT_EQUAL_FATAL(encoder_chn_release_stream(ipchdl,&info,data),0);
	}
	CU_ASSERT_EQUAL_FATAL(encoder_chn_stop(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_chn_deinit(ipchdl,&info),0);
	CU_ASSERT_EQUAL_FATAL(encoder_cli_unregister(ipchdl,&info),0);
}

CU_TestInfo encoder_ser_test[] = {
	{"test encoder service get",TEST_ENC_SERVICE_GET_000},
	{"test encoder register ",TEST_ENC_SERVER_REGISTER_001},
	{"test encoder init param invalid_01 ",TEST_ENC_SERVER_CHN_INIT_002},
	{"test encoder init param invalid_02 ",TEST_ENC_SERVER_CHN_INIT_003},
	{"test encoder init param invalid_03 ",TEST_ENC_SERVER_CHN_INIT_004},
	{"test encoder init fmt error ",TEST_ENC_SERVER_CHN_INIT_005},
	{"test encoder init normal_01 ",TEST_ENC_SERVER_CHN_INIT_006},
	{"test encoder init normal_02 ",TEST_ENC_SERVER_CHN_INIT_007},
	/* {"test encoder start_01 ",TEST_ENC_SERVER_CHN_START_008}, */
	/* {"test encoder start_02 ",TEST_ENC_SERVER_CHN_START_009}, */
	{"test encoder save image_01 ",TEST_ENC_SERVER_CHN_SAVE_IMG_010},
	{"test encoder save image_02 ",TEST_ENC_SERVER_CHN_SAVE_IMG_011},
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo encoder_ser_test_suite[] = {
	{"encoder service API unit test",NULL,NULL,NULL,NULL,encoder_ser_test},
	CU_TEST_INFO_NULL,
};



static void add_test(void)
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());
    if(CUE_SUCCESS != CU_register_suites(encoder_ser_test_suite)){
		printf("error add test ##\r\n");
        exit(-1);
    }
}

static int run_unit_test(void)
{
    if(CU_initialize_registry()){
        printf("initialize CU failed\r\n");
        exit(-1);
    } else {
        add_test();
        // console mode
        CU_console_run_tests();

        CU_cleanup_registry();
        return CU_get_error();
    }
}

int main(int argc,char** argv)
{
    run_unit_test();
    return 0;
}

