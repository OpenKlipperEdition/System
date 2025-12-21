#ifndef __CODEC_COMMON_H__
#define __CODEC_COMMON_H__

#include <stdbool.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include "impp.h"
#include "codec.h"

#define DATA_MAX_PLANES     3
#define MAX_PACK_NUM        4

typedef struct {
        IHAL_UINT32 seq;
        IHAL_UINT32 packcnt;
        EncoderPack pack[MAX_PACK_NUM];
} encoder_stream_t;

// nv12 / nv21 / yuv420
typedef struct {
	IHAL_UINT32 *vaddr[DATA_MAX_PLANES];    //internal-map
	IHAL_INT32  dmafd[DATA_MAX_PLANES];
	IHAL_UINT32 bufsize[DATA_MAX_PLANES];
	IHAL_UINT32 offset[DATA_MAX_PLANES];
	encoder_stream_t  stream;
	IHAL_UINT32 totalsize;
	IHAL_INT32  index;
} video_data_buffer_t;

typedef struct {
	char name[32];
	unsigned int version;

	int (*codec_init)(void);
	int (*codec_deinit)(void);

	/* chn ops */
	void* (*codec_chn_ctx_create)(void);
	int (*codec_chn_ctx_destroy)(void* chn_ctx);
	int (*codec_param_set)(void *chn_ctx, IHal_CodecParam *param);
	int (*codec_param_get)(void *chn_ctx, IHal_CodecParam *param);
	int (*codec_chn_start)(void* chn_ctx);
	int (*codec_chn_stop)(void* chn_ctx);

	/* buffer ops  */
	int (*src_buffer_create)(void* chn_ctx, IMPP_BUFFER_TYPE buftype, int num);
	int (*src_buffer_free)(void* chn_ctx);
	int (*dst_buffer_create)(void* chn_ctx, IMPP_BUFFER_TYPE buftype, int num);
	int (*dst_buffer_free)(void* chn_ctx);

	/* share mem */
	int (*get_src_buffer)(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index);
	int (*set_src_buffer)(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index);
	int (*get_dst_buffer)(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index);
	int (*set_dst_buffer)(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index);

	/* src or dst state  */
	int (*poll_src_buffer_avaliable)(void* chn_ctx, int wait_type);
	int (*poll_dst_buffer_avaliable)(void* chn_ctx, int wait_type);

	/* data ops */
	int (*queue_src_buffer)(void* chn_ctx, IMPP_BufferInfo_t *bufinfo);
	int (*dequeue_src_buffer)(void* chn_ctx, IMPP_BufferInfo_t *bufinfo);
	int (*queue_dst_buffer)(void* chn_ctx, IHAL_CodecStreamInfo_t *bufinfo);
	int (*dequeue_dst_buffer)(void* chn_ctx, IHAL_CodecStreamInfo_t *outbuf);

	int (*control)(void *chn_ctx, IHal_CodecControlParam_t *param);
} ImppCodec_t;

#endif		/* __CODEC_COMMON_H__ */
