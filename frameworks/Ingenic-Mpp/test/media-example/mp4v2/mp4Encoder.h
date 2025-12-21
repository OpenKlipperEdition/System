#ifndef __MP4ENCODER_H__
#define __MP4ENCODER_H__

#ifdef __cplusplus
extern "C"{

typedef struct {
	IHAL_INT32 videowidth;
	IHAL_INT32 videoheight;
	IHAL_INT32 timescale;
	IHAL_INT32 framerate;
	IHAL_INT32 audioSamplerate;
	char       filename[64];
}IHal_Mp4EncoderInit_t;

typedef void IHal_Mp4EncoderHandle_t;

IHal_Mp4EncoderHandle_t* IHal_Mp4FileCreate(IHal_Mp4EncoderInit_t* init);

IHAL_INT32 IHal_Mp4WriteH264(IHal_Mp4EncoderHandle_t *handle,IHAL_UINT8* buffer,IHAL_UINT32 framesize);

IHAL_INT32 IHal_Mp4WriteAAC(IHal_Mp4EncoderHandle_t *handle,IHAL_UINT8* buffer,IHAL_UINT32 datasize);

IHAL_INT32 IHal_Mp4FileClose(IHal_Mp4EncoderHandle_t *handle);

}
#endif	//__cplusplus

#endif	// __MP4ENCODER_H__



