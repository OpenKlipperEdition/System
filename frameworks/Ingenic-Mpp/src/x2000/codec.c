/**
 * @file codec.c
 * @author
 * @brief
 * @version 0.2
 * @date 2023-08-30
 *
 * @copyright Copyright ingenic 2021
 *
 */
#include <stdio.h>
#include <stdbool.h>
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
#include <pthread.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <semaphore.h>
#include "codec.h"
#include "log.h"
#include "codec_common.h"

#define VIDEO_BUF_MAX_NR    16

#define TAG "x2000-codec"

typedef struct {
	ImppCodec_t *codecOps;
	void *chn_ctx;
} video_codec_ctx_t;

static video_codec_ctx_t *ctx;
extern ImppCodec_t h264enc_v1;
extern ImppCodec_t h264dec_v1;
extern ImppCodec_t jpegenc_v1;
extern ImppCodec_t jpegdec_v1;


IHAL_INT32 IHal_CodecInit(void)
{
	return IHAL_ROK;
}

IHAL_INT32 IHal_CodecDeInit(void)
{
	return IHAL_ROK;
}

IHal_CodecHandle_t *IHal_CodecCreate(CODEC_TYPE type)
{
	IHal_CodecHandle_t *handle = NULL;
	video_codec_ctx_t *ctx = NULL;

	handle = (IHal_CodecHandle_t *)calloc(1, sizeof(IHal_CodecHandle_t));
	if (!handle) {
		IMPP_LOGE(TAG, "calloc codec handle error");
		return IHAL_RNULL;
	}

	ctx = (video_codec_ctx_t *)calloc(1, sizeof(video_codec_ctx_t));
	if (!ctx) {
		free(handle);
		IMPP_LOGE(TAG, "calloc codec ctx failed");
		return IHAL_RNULL;
	}

	switch (type) {
		case H264_ENC:
			ctx->codecOps = &h264enc_v1;
			break;
		case H264_DEC:
			ctx->codecOps = &h264dec_v1;
			break;
		case JPEG_ENC:
			ctx->codecOps = &jpegenc_v1;
			break;
		case JPEG_DEC:
			ctx->codecOps = &jpegdec_v1;
			break;
		default:
			IMPP_LOGW(TAG, "not support codec type");
			return IHAL_RNULL;
	}
	ctx->chn_ctx = ctx->codecOps->codec_chn_ctx_create();

	if (!ctx->chn_ctx) {
		free(ctx);
		free(handle);
		IMPP_LOGE(TAG, "%s create chn_ctx failed.", ctx->codecOps->name);
		return IHAL_RNULL;
	}

	handle->ctx = ctx;
	handle->codectype = type;

	return handle;
}

IHAL_INT32 IHal_CodecDestroy(IHal_CodecHandle_t *handle)
{
	assert(handle);
	assert(handle->ctx);

	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;

	ctx->codecOps->src_buffer_free(ctx->chn_ctx);
	ctx->codecOps->dst_buffer_free(ctx->chn_ctx);
	ctx->codecOps->codec_chn_ctx_destroy(ctx->chn_ctx);

	free(handle->ctx);
	free(handle);

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_SetParams(IHal_CodecHandle_t *handle, IHal_CodecParam *param)
{
	IHAL_INT32 ret = 0;

	assert(handle);
	assert(handle->ctx);

	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->codec_param_set(ctx->chn_ctx, param);
	if (ret) {
		IMPP_LOGE(TAG, "%s codec params set failed", ctx->codecOps->name);
		return -IHAL_RFAILED;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_GetParams(IHal_CodecHandle_t *handlt, IHal_CodecParam *param)
{
	/* TODO */

	return IHAL_ROK;
}


IHAL_INT32 IHal_Codec_Start(IHal_CodecHandle_t *handle)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;

	ret = ctx->codecOps->codec_chn_start(ctx->chn_ctx);
	if (ret) {
		IMPP_LOGE(TAG, "%s codec start failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_Stop(IHal_CodecHandle_t *handle)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->codec_chn_stop(ctx->chn_ctx);
	if (ret) {
		IMPP_LOGE(TAG, "%s codec stop failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_CreateSrcBuffers(IHal_CodecHandle_t *handle, IMPP_BUFFER_TYPE buftype, IHAL_INT32 create_num)
{
	assert(handle);
	assert(handle->ctx);

	if (VIDEO_BUF_MAX_NR < create_num)
		create_num = VIDEO_BUF_MAX_NR;

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->src_buffer_create(ctx->chn_ctx, buftype, create_num);
	if (0 >= ret) {
		IMPP_LOGE(TAG, "%s create src buffers failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return ret;
}

IHAL_INT32 IHal_Codec_SetSrcBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *sharebuf)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->set_src_buffer(ctx->chn_ctx, sharebuf, index);
	if (ret) {
		IMPP_LOGE(TAG, "%s set src buffer failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_GetSrcBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *sharebuf)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->get_src_buffer(ctx->chn_ctx, sharebuf, index);
	if (ret) {
		IMPP_LOGE(TAG, "%s get src buffer failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_CreateDstBuffer(IHal_CodecHandle_t *handle, IMPP_BUFFER_TYPE buftype, IHAL_INT32 create_num)
{
	assert(handle);
	assert(handle->ctx);

	if (VIDEO_BUF_MAX_NR < create_num)
		create_num = VIDEO_BUF_MAX_NR;

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->dst_buffer_create(ctx->chn_ctx, buftype, create_num);
	if (0 >= ret) {
		IMPP_LOGE(TAG, "%s create dst buffer failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return ret;
}

IHAL_INT32 IHal_Codec_SetDstBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *share)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->set_dst_buffer(ctx->chn_ctx, share, index);
	if (ret) {
		IMPP_LOGE(TAG, "%s set dst buffer failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_GetDstBuffer(IHal_CodecHandle_t *handle, int index, IMPP_BufferInfo_t *share)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->get_dst_buffer(ctx->chn_ctx, share, index);
	if (ret) {
		IMPP_LOGE(TAG, "%s get dst buffer failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_WaitSrcAvailable(IHal_CodecHandle_t *handle, IHAL_INT32 wait_type)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->poll_src_buffer_avaliable(ctx->chn_ctx, wait_type);
	if (ret) {
		//IMPP_LOGE(TAG, "%s wait src available failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_QueueSrcBuffer(IHal_CodecHandle_t *handle, IMPP_BufferInfo_t *buf)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->queue_src_buffer(ctx->chn_ctx, buf);
	if (ret) {
		IMPP_LOGE(TAG, "%s video codec qsrc failed", ctx->codecOps->name);
		return -IHAL_RFAILED;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_DequeueSrcBuffer(IHal_CodecHandle_t *handle, IMPP_BufferInfo_t *buf)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->dequeue_src_buffer(ctx->chn_ctx, buf);
	if (ret) {
		IMPP_LOGE(TAG, "%s video codec dqsrc failed", ctx->codecOps->name);
		return -IHAL_RFAILED;
	}

	return IHAL_ROK;
}


IHAL_INT32 IHal_Codec_WaitDstAvailable(IHal_CodecHandle_t *handle, IHAL_INT32 wait_type)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->poll_dst_buffer_avaliable(ctx->chn_ctx, wait_type);
	if (ret) {
		IMPP_LOGE(TAG, "%s wait vcodec dst available failed", ctx->codecOps->name);
		return -IHAL_RFAILED;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_QueueDstBuffer(IHal_CodecHandle_t *handle, IHAL_CodecStreamInfo_t *buf)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->queue_dst_buffer(ctx->chn_ctx, buf);
	if (ret) {
		IMPP_LOGE(TAG, "%s queue dst buffer failed.", ctx->codecOps->name);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_DequeueDstBuffer(IHal_CodecHandle_t *handle, IHAL_CodecStreamInfo_t *buf)
{
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;
	ret = ctx->codecOps->dequeue_dst_buffer(ctx->chn_ctx, buf);
	if (ret) {
		IMPP_LOGE(TAG, "%s dequeue dst buffer failed.", ctx->codecOps->name);
		return -IHAL_RFAILED;
	}

	return IHAL_ROK;
}

IHAL_INT32 IHal_Codec_Control(IHal_CodecHandle_t *handle, IHal_CodecControlParam_t *param)
{
	/* TODO */
	assert(handle);
	assert(handle->ctx);

	int ret = 0;
	video_codec_ctx_t *ctx = NULL;
	ctx = (video_codec_ctx_t *)handle->ctx;

	ret = ctx->codecOps->control(ctx->chn_ctx, param);
	if (ret) {
		IMPP_LOGE(TAG, "%s codec control failed.", ctx->codecOps->name);
		return -IHAL_RFAILED;
	}

	return IHAL_ROK;
}
