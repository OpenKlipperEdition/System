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

struct test_priv {
	FILE* file;
}finish_cb_priv;


void stream_callback(DataBuffer_t* buffer,void* priv)
{
	struct test_priv* cb_priv = (struct test_priv*)priv;
	printf("start process stream byteused = %d  file = 0x%x ##\r\n",buffer->size,cb_priv->file);
	size_t ret = fwrite(buffer->vaddr,buffer->size,1,cb_priv->file);
}

#if 1

int main(int argc,char** argv)
{
	if(argc < 3){
		printf("param error ./demo filepath  /dev/videoN!!\r\n");
		return -1;
	}
	EncoderChnParam_t  chn_param;
	memset(&chn_param,0,sizeof(EncoderChnParam_t));
	chn_param.srcType = SRC_CAMERA;
	/* sprintf(chn_param.videoNode,"/dev/video4"); */
	strcpy(chn_param.videoNode,argv[2]);
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

	EncChnHandle_t enc_handle = ISS_CreateEncodeChn(&chn_param);
	if(!enc_handle){
		printf("%s create encode chn failed ##\r\n",__func__);
		return -1;
	}

	/* file_fd = open("/opt/main_test.h264",O_CREAT | O_RDWR | O_TRUNC); */
	finish_cb_priv.file = fopen(argv[1],"w+");
	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = stream_callback;
	finish_cb.priv = &finish_cb_priv;

	ISS_EncodeChn_SetFinishCallBack(enc_handle,&finish_cb);

	ISS_EncodeChn_Start(enc_handle);

	uint32_t times = 360000;
	while(times--){
		sleep(1);
	}
	ISS_EncodeChn_Stop(enc_handle);
	ISS_DestroyEncodeChn(enc_handle);
	return 0;
}
#else
int main(int argc,char** argv)
{
	if(argc < 3){
		printf("param error ./demo srcfilepath  outpath !!\r\n");
		return -1;
	}
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

	EncChnHandle_t enc_handle = ISS_CreateEncodeChn(&chn_param);
	if(!enc_handle){
		printf("%s create encode chn failed ##\r\n",__func__);
		return -1;
	}

	/* file_fd = open("/opt/main_test.h264",O_CREAT | O_RDWR | O_TRUNC); */
	finish_cb_priv.file = fopen(argv[2],"w+");
	EncoderFinish_Cb_t finish_cb;
	finish_cb.enc_finish_cb = stream_callback;
	finish_cb.priv = &finish_cb_priv;

	ISS_EncodeChn_SetFinishCallBack(enc_handle,&finish_cb);

	ISS_EncodeChn_Start(enc_handle);

	uint32_t times = 360000;
	int sfd = open(argv[1],O_RDONLY);
	DataBuffer_t data;
	while(times--){
		ISS_EncodeChn_RequestSrcBuffer(enc_handle,&data);
		lseek(sfd,0,SEEK_SET);
		read(sfd,data.vaddr,data.size);
		ISS_EncodeChn_SendSrcFrame(enc_handle,&data);
	}
	ISS_EncodeChn_Stop(enc_handle);
	ISS_DestroyEncodeChn(enc_handle);
	return 0;
}
#endif
