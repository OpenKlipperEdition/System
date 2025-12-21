#ifndef __ENCODER_H__
#define __ENCODER_H__
#include "iss_common.h"
/* 编码通道源数据类型 */
typedef enum  {
	SRC_CAMERA,		// 源数据为Camera	
 	SRC_USERDATA,	// 源数据为非Camera的使用此类型
}EncSourceType_t;
/* H26x 编码码率控制类型 */
typedef enum {
	RC_MODE_FIXQP,         
	RC_MODE_CBR,
	RC_MODE_VBR,
	RC_MODE_CAPPED_VBR,
	RC_MODE_CAPPED_QUALITY,
}EncodeRcMode_t;

/* 编码通道类型 */
typedef enum {
	ENC_JPEG,
	ENC_H264,
	ENC_H265,
}EncType_t;

typedef struct {
	EncodeRcMode_t rc_mode;
	uint32_t bitrate;
	int32_t frameRate;
	int32_t gop;
	int32_t min_qp;
	int32_t max_qp;
	int32_t idrFreq;
	int32_t enc_width;
	int32_t enc_height;
	PixFmt_t srcfmt;
	int32_t max_stream_size;  // not set, only for debug
}ENC_H26x_Param_t;

typedef struct {
	int32_t qp;
	int32_t quality; // [0-100]
	uint32_t enc_width;
	uint32_t enc_height;
	PixFmt_t srcfmt;
	uint32_t max_stream_size;  // not set ,only for debug
}ENC_JPEG_Param_t;


typedef struct {
	EncType_t type;
	union {
		ENC_H26x_Param_t h26x_param;
		ENC_JPEG_Param_t jpeg_param;
	}EncParam;
}EncodeParam_t;

typedef struct {
	EncSourceType_t srcType;	// 源数据类型
	EncodeParam_t param;
	char videoNode[32];			// 源数据为Camera需配置此参数 /dev/videoN
	int dstBufferNum;
	int srcBufferNum;
//TODO 
//bool rotate;
//int angle;
}EncoderChnParam_t;


typedef void* EncChnHandle_t;

typedef struct {
	int (*enc_finish_cb)(DataBuffer_t*,void*);
	void* priv;
}EncoderFinish_Cb_t;

EncChnHandle_t ISS_CreateEncodeChn(EncoderChnParam_t* param);

int ISS_DestroyEncodeChn(EncChnHandle_t handle);

int ISS_EncodeChn_SetFinishCallBack(EncChnHandle_t handle,EncoderFinish_Cb_t* cb);

int ISS_EncodeChn_RequestSrcBuffer(EncChnHandle_t handle,DataBuffer_t* buffer);

int ISS_EncodeChn_SendSrcFrame(EncChnHandle_t handle,DataBuffer_t* frame);

int ISS_EncodeChn_Start(EncChnHandle_t handle);

int ISS_EncodeChn_Stop(EncChnHandle_t handle);

int ISS_EncodeChn_IDR_Request(EncChnHandle_t handle);

int ISS_EncodeChn_SetQPBounds(EncChnHandle_t handle,int32_t maxQp,int32_t minQp);

int ISS_EncodeChn_SetBitrate(EncChnHandle_t handle,uint32_t max_bitrate,uint32_t min_bitrate);

#endif // __ENCODER_H__

