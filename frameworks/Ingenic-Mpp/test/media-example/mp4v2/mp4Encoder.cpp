#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#define TAG "Mp4ENC"
extern "C"{
#include <mp4v2.h>
#include "audio.h"
#include "codec.h"
#include "mp4Encoder.h"
#include "log.h"
}

typedef struct {
	unsigned int mWidth;
	unsigned int mHeight;
	unsigned int mFramerate;
	unsigned int mTimescale;
	unsigned int mSamplerate;
	char         isFirstFrame;
	char         isFirstSPS;
	char         isFirstPPS;
	char         isFirstAudio;
	//unsigned long long       videoTime;
	//unsigned long long       audioTime;
	double       videoTime;
	double       audioTime;
	MP4FileHandle mp4Handle;
	MP4TrackId	videoId;
	MP4TrackId	audioId;
	pthread_mutex_t lock;
}mp4_ctx_t;

static int _sample_rate_to_aac_index(unsigned int sample_rate)
{
    switch(sample_rate){
	        case 96000:
	            return 0;
	        case 48000:
	            return 3;
	        case 44100:
	            return 4;
	        case 32000:
	            return 5;
	        case 16000:
	            return 8;
	        case 8000:
	            return 11;
	        default:
	            return -1;
	    }
}

static double _get_wall_time()
{
    struct timeval time;
    if(gettimeofday(&time,NULL))
        return 0;
    return (double)(time.tv_sec * 1000 + time.tv_usec / 1000);
}

IHal_Mp4EncoderHandle_t* IHal_Mp4FileCreate(IHal_Mp4EncoderInit_t* init)
{
	//int ret = 0;
	mp4_ctx_t *ctx = (mp4_ctx_t*)malloc(sizeof(mp4_ctx_t));
	if(NULL == ctx){
		IMPP_LOGE(TAG,",mp4 ctx init failed");
		return IHAL_RNULL;
	}
	ctx->mp4Handle = MP4Create(init->filename,0);
	if(!ctx->mp4Handle){
		IMPP_LOGE(TAG,"mp4 file create failed");
		goto free_ctx;
	}

	ctx->mWidth		= init->videowidth;
	ctx->mHeight	= init->videoheight;
	ctx->mTimescale	= init->timescale;
	ctx->mSamplerate = init->audioSamplerate;
	ctx->mTimescale	 = init->timescale;
	ctx->mFramerate  = init->framerate;
	MP4SetTimeScale(ctx->mp4Handle,ctx->mTimescale);
	ctx->audioId = MP4AddAudioTrack(ctx->mp4Handle,ctx->mSamplerate,1024,MP4_MPEG4_AUDIO_TYPE);
	//unsigned short confnum = 0;
	//int rate_idx = _sample_rate_to_aac_index(ai_attr.SampleRate);
	//confnum |= (2 <<  11);
	//confnum |= (rate_idx << 7);
	//confnum |= (ai_attr.channels << 3);
	//unsigned char conf[] = {((confnum & 0xff00) >> 8 ),confnum & 0xff,0x00,0x00};
	//int confsize = sizeof(conf);
	//MP4SetTrackESConfiguration(mp4Handle,audioId,conf,confsize);
	ctx->isFirstPPS = 1;
	ctx->isFirstSPS = 1;
	ctx->isFirstFrame = 1;
	ctx->isFirstAudio = 1;
	pthread_mutex_init(&ctx->lock,NULL);
	return (IHal_Mp4EncoderHandle_t*)ctx;
free_ctx:
	free(ctx);
	return IHAL_RNULL;
}

IHAL_INT32 IHal_Mp4WriteH264(IHal_Mp4EncoderHandle_t* handle,IHAL_UINT8* buffer,IHAL_UINT32 framesize)
{
	int ret = 0;
	assert(handle);
	mp4_ctx_t *ctx = (mp4_ctx_t*)handle;
	unsigned int nalu_len = framesize - 4;
	int nalu_type = buffer[4] & 0x1f;
	double timenow = _get_wall_time();
	unsigned char* pNal = (unsigned char*)&buffer[4];
	if(nalu_type == 0x07 && 1 == ctx->isFirstSPS){	// SPS
		printf("write the first frame SPS \r\n");
		ctx->videoId = MP4AddH264VideoTrack(ctx->mp4Handle,
								ctx->mTimescale,
								ctx->mTimescale / ctx->mFramerate,
								ctx->mWidth,
								ctx->mHeight,
								buffer[1],
								buffer[2],
								buffer[3],
								3);
		if(ctx->videoId == MP4_INVALID_TRACK_ID){
			IMPP_LOGE(TAG,"add video track failed");
			return -IHAL_RERR;
		}
		MP4SetVideoProfileLevel(ctx->mp4Handle,0x7F);
		MP4AddH264SequenceParameterSet(ctx->mp4Handle,ctx->videoId,pNal,nalu_len);
		ctx->isFirstSPS = 0;
	} else if(nalu_type == 0x08 && 1 == ctx->isFirstPPS){	//PPS
		printf("write the first frame PPS \r\n");
		MP4AddH264PictureParameterSet(ctx->mp4Handle,ctx->videoId,pNal,nalu_len);
		ctx->isFirstPPS = 0;
	} else if(!ctx->isFirstSPS && !ctx->isFirstPPS) {
		char header[4];
		memcpy(header,buffer,4);
		buffer[0] = (nalu_len >> 24) & 0xff;
		buffer[1] = (nalu_len >> 16) & 0xff;
		buffer[2] = (nalu_len >> 8) & 0xff;
		buffer[3] = nalu_len & 0xff;
		if(ctx->isFirstFrame == 1){
			if(nalu_type = 0x05){	//IDR frame
				printf("write the first frame IDR\r\n");
				MP4WriteSample(ctx->mp4Handle,
							ctx->videoId,(const uint8_t*)buffer,nalu_len + 4,
							ctx->mTimescale / ctx->mFramerate,0,1);
				ctx->videoTime = timenow;
				ctx->audioTime = timenow;
				ctx->isFirstFrame = 0;
			}
		} else {
			char isSyncFrame = 0;
			if(nalu_type == 0x05)	//IDR Frame
				isSyncFrame = 1;
			pthread_mutex_lock(&ctx->lock);
			MP4WriteSample(ctx->mp4Handle,
							ctx->videoId,(const uint8_t*)buffer,nalu_len + 4,
							(timenow - ctx->videoTime) * ctx->mTimescale / 1000,0,isSyncFrame);
			pthread_mutex_unlock(&ctx->lock);
			ctx->videoTime = timenow;
		}
		memcpy(buffer,header,4);
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Mp4WriteAAC(IHal_Mp4EncoderHandle_t *handle,IHAL_UINT8* buffer,IHAL_UINT32 datasize)
{
	int ret = 0;
	assert(handle);
	mp4_ctx_t *ctx = (mp4_ctx_t*)handle;
	if(!ctx->isFirstFrame){
		double timenow = _get_wall_time();
		double timestamp = (timenow - ctx->audioTime) * (ctx->mSamplerate / 1000);
		ctx->audioTime = timenow;
		pthread_mutex_lock(&ctx->lock);
		ret = MP4WriteSample(ctx->mp4Handle, ctx->audioId,(const uint8_t*)buffer,datasize,timestamp,0, 1);
		if(!ret){
			IMPP_LOGE(TAG,"write audio failed");
			pthread_mutex_unlock(&ctx->lock);
			return -IHAL_RERR;
		}
		pthread_mutex_unlock(&ctx->lock);
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Mp4FileClose(IHal_Mp4EncoderHandle_t *handle)
{
	int ret = 0;
	assert(handle);
	mp4_ctx_t *ctx = (mp4_ctx_t*)handle;
	pthread_mutex_destroy(&ctx->lock);
	MP4Close(ctx->mp4Handle,0);
	free(ctx);
	return IHAL_ROK;
}

