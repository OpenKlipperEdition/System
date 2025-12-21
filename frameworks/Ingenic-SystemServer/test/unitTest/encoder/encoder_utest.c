#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "systemserver_config.h"
#include "encoder.h"


#undef CU_TRUE
#undef CU_FALSE
#define CU_TRUE 0
#define CU_FALSE 1
#include <assert.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>



void stream_callback(DataBuffer_t* buffer,void* priv)
{
	printf("start process stream byteused = %d ##\r\n",buffer->size);
}

void TEST_ENCODER_001(void)
{
	EncChnHandle_t handle = ISS_CreateEncodeChn(NULL);
	CU_ASSERT_EQUAL_FATAL(handle,NULL);
}

void TEST_ENCODER_002(void)
{
	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	strcpy(chn_param.videoNode,"/dev/video4");
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
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;

	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}

void TEST_ENCODER_003(void)
{
	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;
	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;
	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}


void TEST_ENCODER_004(void)
{
	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;
	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;
	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Start(handle),0);
	sleep(1);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Stop(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}


void TEST_ENCODER_005(void)
{
	EncoderChnParam_t  chn_param;
	DataBuffer_t data;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;
	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;
	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Start(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_RequestSrcBuffer(handle,&data),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SendSrcFrame(handle,&data),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Stop(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}

void TEST_ENCODER_006(void)
{
	EncoderChnParam_t  chn_param;
	DataBuffer_t data;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;
	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;
	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_NOT_EQUAL_FATAL(ISS_EncodeChn_SetFinishCallBack(handle,NULL),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}


void TEST_ENCODER_007(void)
{
	EncoderChnParam_t  chn_param;
	DataBuffer_t data;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
	chn_param.srcBufferNum = 2;
	chn_param.dstBufferNum = 2;
	chn_param.param.type = ENC_JPEG;
	ENC_JPEG_Param_t *enc_param = &chn_param.param.EncParam.jpeg_param;
	enc_param->qp = 20;
	enc_param->quality = 70;
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;


	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = stream_callback;
	finish_cb.priv = NULL;

	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SetFinishCallBack(handle,&finish_cb),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Start(handle),0);
	for(int i = 0; i < 6; i++){
		CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_RequestSrcBuffer(handle,&data),0);
		memset(data.vaddr,0x11,data.size);
		CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SendSrcFrame(handle,&data),0);
	}
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Stop(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}


void TEST_ENCODER_008(void)
{

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
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;


	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = stream_callback;
	finish_cb.priv = NULL;

	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SetFinishCallBack(handle,&finish_cb),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Start(handle),0);
	sleep(1);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Stop(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}


void TEST_ENCODER_009(void)
{

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
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;

	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = stream_callback;
	finish_cb.priv = NULL;

	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SetFinishCallBack(handle,&finish_cb),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Start(handle),0);
	sleep(3);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Stop(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}



void TEST_ENCODER_010(void)
{
	EncoderChnParam_t  chn_param;
	DataBuffer_t data;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_USERDATA;
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
	enc_param->enc_width = 640;
	enc_param->enc_height = 480;
	enc_param->srcfmt = PIX_FMT_NV12;


	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = stream_callback;
	finish_cb.priv = NULL;

	EncChnHandle_t handle = ISS_CreateEncodeChn(&chn_param);
	CU_ASSERT_NOT_EQUAL_FATAL(handle,NULL);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SetFinishCallBack(handle,&finish_cb),0);
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Start(handle),0);
	int sfd = open("/1.yuv",O_RDONLY);
	for(int i = 0; i < 10; i++){
		CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_RequestSrcBuffer(handle,&data),0);
		lseek(sfd,0,SEEK_SET);
		read(sfd,data.vaddr,data.size);
		CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_SendSrcFrame(handle,&data),0);
	}
	CU_ASSERT_EQUAL_FATAL(ISS_EncodeChn_Stop(handle),0);
	CU_ASSERT_EQUAL_FATAL(ISS_DestroyEncodeChn(handle),0);
}



CU_TestInfo encoder_test[] = {
	{"test encoder 001",TEST_ENCODER_001},
	{"test encoder 002",TEST_ENCODER_002},
	{"test encoder 003",TEST_ENCODER_003},
	{"test encoder 004",TEST_ENCODER_004},
	{"test encoder 005",TEST_ENCODER_005},
	{"test encoder 006",TEST_ENCODER_006},
	{"test encoder 007",TEST_ENCODER_007},
	{"test encoder 008",TEST_ENCODER_008},
	{"test encoder 009",TEST_ENCODER_009},
	{"test encoder 010",TEST_ENCODER_010},
	CU_TEST_INFO_NULL,
};


CU_SuiteInfo encoder_test_suite[] = {
	{"encoder API unit test",NULL,NULL,NULL,NULL,encoder_test},
	CU_TEST_INFO_NULL,
};


static void add_test(void)
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());
    if(CUE_SUCCESS != CU_register_suites(encoder_test_suite)){
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

