#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include "impp.h"
#include "log.h"
#include "codec_common.h"

#define TAG "h264enc_v1"
#define H264E_DEV_PATH "/dev/video1"

struct format_description {
	IHAL_INT8   *description;
	IHAL_INT32  v4l2_format;
	IHAL_INT32  v4l2_buffers_count;
	bool        v4l2_mplane;
	IHAL_INT32  planes_count;
};

typedef struct {
    IHAL_INT32 fd;

	pthread_mutex_t mutex;
	IHAL_INT32              chn_idr_request;

    IMPP_BUFFER_TYPE src_buf_type;
    IHAL_INT16 src_buf_num;
    video_data_buffer_t *inputbuffer;

    IMPP_BUFFER_TYPE dst_buf_type;
    IHAL_INT16 dst_buf_num;
    video_data_buffer_t *outputbuffer;

    IHAL_UINT32 seq;

    enum v4l2_buf_type capbuf_type;
    enum v4l2_buf_type outbuf_type;
    struct format_description *srcfmt_des;
    struct format_description *dstfmt_des;

    IHAL_INT16 image_width;
    IHAL_INT16 image_height;

    // buffer state
    sem_t src_available;
    sem_t dst_available;

	IHal_H264E_Param h264e_param;
} h264e_chn_ctx_t;

static struct format_description formats[] = {
        {
                .v4l2_format        = V4L2_PIX_FMT_NV12,
                .v4l2_buffers_count     = 2,
                .v4l2_mplane        = true,
                .planes_count       = 2,
        },
        {
                .v4l2_format        = V4L2_PIX_FMT_NV21,
                .v4l2_buffers_count = 2,
                .v4l2_mplane        = true,
                .planes_count       = 2,
        },
        {
                .v4l2_format        = V4L2_PIX_FMT_YUV420,
                .v4l2_buffers_count = 3,
                .v4l2_mplane        = true,
                .planes_count       = 3,
        },
        {
                .v4l2_format        = V4L2_PIX_FMT_H264,
                .v4l2_buffers_count = 1,
                .v4l2_mplane        = true,
                .planes_count       = 1,
        },
};

static int _init_mmap(h264e_chn_ctx_t *ctx, int type, void *bufferinfo, int num)
{
	int ret = 0;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_plane planes[DATA_MAX_PLANES];
	struct v4l2_buffer buffer;

	bzero(&reqbuf, sizeof(reqbuf));
	reqbuf.count = num;
	reqbuf.type = type;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(ctx->fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret) {
		IMPP_LOGE(TAG, "codec request mmap buffer failed.");
		return -IHAL_RERR;
	}

	video_data_buffer_t *pBufinfo = (video_data_buffer_t *)bufferinfo;
	for (int i = 0; i < num; i++) {
		bzero(&buffer, sizeof(buffer));
		buffer.type = type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;
		if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == type || V4L2_BUF_TYPE_VIDEO_OUTPUT == type) {
			buffer.length = ctx->srcfmt_des->v4l2_buffers_count;
		} else {
			buffer.length = ctx->dstfmt_des->v4l2_buffers_count;
		}
		buffer.m.planes = planes;
		if (-1 == ioctl(ctx->fd, VIDIOC_QUERYBUF, &buffer)) {
			IMPP_LOGE(TAG, "Fail to ioctl : VIDIOC_QUERYBUF.");
			return -IHAL_RERR;
		}
		if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE || type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
			for (int j = 0; j < buffer.length; j++) {
				pBufinfo[i].bufsize[j] = buffer.m.planes[j].length;
				pBufinfo[i].offset[j] = buffer.m.planes[j].m.mem_offset;
				pBufinfo[i].vaddr[j] = mmap(NULL, pBufinfo[i].bufsize[j],
						PROT_READ| PROT_WRITE, MAP_SHARED, ctx->fd,
						pBufinfo[i].offset[j]);
				if (!pBufinfo[i].vaddr[j]) {
					IMPP_LOGE(TAG, "mmap buffer failed (mplane).");
					return -IHAL_RERR;
				}
				IMPP_LOGI(TAG, "mmap vaddr = %#x bufsize = %d", pBufinfo[i].vaddr[j], pBufinfo[i].bufsize[j]);
				/* TODO export dmabuf */
			}
		} else {
			/* TODO single plane */
			pBufinfo[i].bufsize[0] = buffer.length;
			pBufinfo[i].offset[0]  = buffer.m.offset;
			pBufinfo[i].vaddr[0] = mmap(NULL, buffer.length,
					PROT_READ | PROT_WRITE, MAP_SHARED, ctx->fd,
					buffer.m.offset);
			if (!pBufinfo[i].vaddr[0]) {
				IMPP_LOGE(TAG, "mmap buffer failed.");
				return -IHAL_RERR;
			}
		}
		pBufinfo[i].index = i;
	}

	return IHAL_ROK;
}

static int _init_dmabuf(h264e_chn_ctx_t *ctx, int type, int num)
{
	struct v4l2_requestbuffers reqbuf;
	int ret = 0;

	assert(ctx);
	bzero(&reqbuf, sizeof(reqbuf));
	reqbuf.count = num;
	reqbuf.type = type;
	reqbuf.memory = V4L2_MEMORY_DMABUF;

	ret = ioctl(ctx->fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		perror("init dma-buf:");
		IMPP_LOGE(TAG, "vcodec request dma-buffer failed.");
		return -IHAL_RERR;
	}

	IMPP_LOGI(TAG, "vcodec request dma-buffer ok!!");

	return IHAL_ROK;
}

static int _init_userptr(h264e_chn_ctx_t *ctx, int type, int num)
{
	struct v4l2_requestbuffers reqbuf;
	int ret = 0;
	assert(ctx);

	bzero(&reqbuf, sizeof(reqbuf));
	reqbuf.count = num;
	reqbuf.type = type;
	reqbuf.memory = V4L2_MEMORY_USERPTR;

	ret = ioctl(ctx->fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		IMPP_LOGE(TAG, "vcodec request userptr-buffer failed");
		return -IHAL_RERR;
	}

	IMPP_LOGI(TAG, "vcodec request userptr-buffer ok!!");

	return IHAL_ROK;
}

static int _h264_enc_init_exit_buffers(video_data_buffer_t *pbuf, int width, int height, int planes, unsigned int *startaddr, unsigned int totalsize)
{
	if (planes > 3) {
		return -IHAL_RERR;
	}

	if (planes == 1) {
		pbuf->vaddr[0]   = (IHAL_UINT32 *)startaddr;
		pbuf->bufsize[0] = totalsize;
		pbuf->offset[0]  = 0;
	} else if (planes == 2) {
		// y planes
		pbuf->vaddr[0]   = (IHAL_UINT32 *)startaddr;
		pbuf->bufsize[0] = width * height;
		pbuf->offset[0]  = 0;
		// uv planes
		pbuf->vaddr[1]   = (IHAL_UINT32 *)((IHAL_UINT32)startaddr + width * height);
		pbuf->bufsize[1] = width * height / 2;
		pbuf->offset[1]  = 0;
	} else {
		// y planes
		pbuf->vaddr[0]   = (IHAL_UINT32 *)startaddr;
		pbuf->bufsize[0] = width * height;
		pbuf->offset[0]  = 0;

		// u planes
		pbuf->vaddr[1]   = (IHAL_UINT32 *)((IHAL_UINT32)startaddr + width * height / 4);
		pbuf->bufsize[1] = width * height / 4;
		pbuf->offset[1]  = 0;

		// v planes
		pbuf->vaddr[2]   = (IHAL_UINT32 *)((IHAL_UINT32)startaddr + width * height / 2);
		pbuf->bufsize[2] = width * height / 4;
		pbuf->offset[2]  = 0;
	}

	return IHAL_ROK;
}

static int _h264_enc_srcfmt_set(void *chn_ctx)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;

	int ret = 0;
	int width = 0;
	int height = 0;
	struct v4l2_format format;
	unsigned int sizeimage = 0;

	memset(&format, 0, sizeof(format));
	width = ctx->h264e_param.src_width;
	height = ctx->h264e_param.src_height;
	ctx->image_width = width;
	ctx->image_height = height;

	if (ctx->srcfmt_des->v4l2_mplane) {
		ctx->outbuf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
		format.type = ctx->outbuf_type;
		format.fmt.pix_mp.width = width;
		format.fmt.pix_mp.height = height;
		format.fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
		format.fmt.pix_mp.pixelformat = ctx->srcfmt_des->v4l2_format;
		IMPP_LOGI(TAG, "outbuf_type is output mp.");
	} else {
		ctx->outbuf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		format.type = ctx->outbuf_type;
		format.fmt.pix.width = width;
		format.fmt.pix.height = height;
		format.fmt.pix.sizeimage = sizeimage;
		format.fmt.pix.pixelformat = ctx->srcfmt_des->v4l2_format;
	}

	ret = ioctl(ctx->fd, VIDIOC_S_FMT, &format);
	if (ret) {
		IMPP_LOGE(TAG, "encoder srcfmt set failed.");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int _h264_enc_dstfmt_set(void *chn_ctx)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;

	int ret = 0;
	int width = 0;
	int height = 0;
	struct v4l2_format format;
	unsigned int sizeimage = 0;

	memset(&format, 0, sizeof(format));
	width = ctx->h264e_param.enc_width;
	height = ctx->h264e_param.enc_height;
	sizeimage = width * height * 2;

	if (ctx->dstfmt_des->v4l2_mplane) {
		ctx->capbuf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		format.type = ctx->capbuf_type;
		format.fmt.pix_mp.width = width;
		format.fmt.pix_mp.height = height;
		format.fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
		format.fmt.pix_mp.pixelformat = ctx->dstfmt_des->v4l2_format;
		IMPP_LOGI(TAG, "capbuf_type is mp");
	} else {
		ctx->capbuf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.type = ctx->capbuf_type;
		format.fmt.pix.width = width;
		format.fmt.pix.height = height;
		format.fmt.pix.sizeimage = sizeimage;
		format.fmt.pix.pixelformat = ctx->dstfmt_des->v4l2_format;
	}

	ret = ioctl(ctx->fd, VIDIOC_S_FMT, &format);
	if (ret) {
		IMPP_LOGE(TAG, "encoder dstfmt set failed.");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int _h264_enc_qDst(void* chn_ctx, int idx)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;

	int ret = 0;
	assert(index >= 0);
	int fd = ctx->fd;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[DATA_MAX_PLANES];
	bzero(&buf, sizeof(buf));

	switch (ctx->dst_buf_type) {
		case IMPP_INTERNAL_BUFFER:
			buf.memory = V4L2_MEMORY_MMAP;
			break;
		case IMPP_EXT_USERBUFFER:
			buf.memory = V4L2_MEMORY_USERPTR;
			break;
		case IMPP_EXT_DMABUFFER:
			buf.memory = V4L2_MEMORY_DMABUF;
			break;
		default:
			IMPP_LOGW(TAG, "not support buffer type.");
			return -IHAL_RERR;
	}

	buf.index = idx;
	buf.m.planes = planes;
	buf.length = ctx->dstfmt_des->v4l2_buffers_count;
	if (ctx->dst_buf_type == IMPP_INTERNAL_BUFFER) {
		if (ctx->capbuf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			for (int i = 0; i < ctx->dstfmt_des->v4l2_buffers_count; i++) {
				buf.m.planes[i].bytesused = 0;
				buf.m.planes[i].length = ctx->outputbuffer[idx].bufsize[i];
			}
		} else {
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.bytesused = 0;
		}
	} else if (ctx->dst_buf_type == IMPP_EXT_USERBUFFER) {
		if (ctx->capbuf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
			for (int i = 0; i < ctx->dstfmt_des->v4l2_buffers_count; i++) {
				buf.m.planes[i].bytesused = 0;
				buf.m.planes[i].m.userptr = (void *)ctx->outputbuffer[idx].vaddr[i];
				buf.m.planes[i].length = ctx->outputbuffer[idx].bufsize[i];
			}
		} else {
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.bytesused = 0;
			buf.m.userptr = ctx->outputbuffer[idx].vaddr[0];
			buf.length = ctx->outputbuffer[idx].bufsize[0];
		}
	} else {
		/* exit dma buf */
	}

	ret = ioctl(fd, VIDIOC_QBUF, &buf);
	if (ret) {
		IMPP_LOGE(TAG, "Fail to ioctl dstbuf 'VIDIOC_QBUF'.");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int _h264_pack_praser(unsigned char *data, unsigned int size, encoder_stream_t *stream)
{
        unsigned char *p = data;
        int nalu_cnt = 0;
        for (unsigned int i = 0; i < size; i++) {
                if (p[i] == 0 && p[i + 1] == 0 && p[i + 2] == 1) {
                        //printf("start code len 3 ##########\r\n");
                        stream->pack[nalu_cnt].offset = i;
                        stream->pack[nalu_cnt].nalType.h264e_nalu_type = p[i + 3];
                        nalu_cnt++;
                        i += 4;
                        if (nalu_cnt > 3) {
                                break;
                        }
                } else if (p[i] == 0 && p[i + 1] == 0 && p[i + 2] == 0 && p[i + 3] == 1) {
                        //printf("start code len 4 ##########\r\n");
                        stream->pack[nalu_cnt].offset = i;
                        stream->pack[nalu_cnt].nalType.h264e_nalu_type = p[i + 4];
                        nalu_cnt++;
                        i += 5;
                        if (nalu_cnt > 3) {
                                break;
                        }
                }
        }
        stream->packcnt = nalu_cnt;
        if (nalu_cnt == 1) {
                stream->pack[0].length = size;
        } else if (nalu_cnt == 2) {
                stream->pack[0].length = stream->pack[1].offset;
                stream->pack[1].length = size - stream->pack[0].length;
        } else if (nalu_cnt == 3) {
                stream->pack[0].length = stream->pack[1].offset;
                stream->pack[1].length = stream->pack[2].offset - stream->pack[1].offset;
                stream->pack[2].length = size - stream->pack[0].length - stream->pack[1].length;
        } else if (nalu_cnt == 4) {
                stream->pack[0].length = stream->pack[1].offset;
                stream->pack[1].length = stream->pack[2].offset - stream->pack[1].offset;
                stream->pack[2].length = stream->pack[3].offset - stream->pack[2].offset;
                stream->pack[3].length = size - stream->pack[0].length - stream->pack[1].length - stream->pack[2].length;
        } else {
                ;
        }
        return 0;
}

static int _h264_enc_request_idr(h264e_chn_ctx_t *ctx)
{
	assert(ctx);

	pthread_mutex_lock(&ctx->mutex);
	ctx->chn_idr_request = 0x01;
	pthread_mutex_unlock(&ctx->mutex);

	return IHAL_ROK;
}


static int h264_enc_init(void)
{
	return IHAL_ROK;
}

static int h264_enc_deinit(void)
{
	return IHAL_ROK;
}

static void* h264_enc_chn_ctx_create(void)
{
	h264e_chn_ctx_t *chn_ctx = NULL;

	chn_ctx = (h264e_chn_ctx_t *)calloc(1, sizeof(h264e_chn_ctx_t));
	if (!chn_ctx)
		return IHAL_RNULL;

	chn_ctx->fd = open(H264E_DEV_PATH, O_RDWR);
	if (!chn_ctx->fd) {
		free(chn_ctx);
		chn_ctx = NULL;
		IMPP_LOGE(TAG, "open h264 encoder video node %s failed.", H264E_DEV_PATH);
		return IHAL_RNULL;
	}

	pthread_mutex_init(&chn_ctx->mutex, NULL);
	sem_init(&chn_ctx->src_available, 0, 0);


	return chn_ctx;
}

static int h264_enc_chn_ctx_destroy(void* chn_ctx)
{
	assert(chn_ctx);

	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	close(ctx->fd);
	sem_destroy(&ctx->src_available);
	pthread_mutex_destroy(&ctx->mutex);

	free(ctx);
	ctx = NULL;

	return IHAL_ROK;
}

static int h264_enc_param_set(void *chn_ctx, IHal_CodecParam *param)
{
	assert(chn_ctx);
	assert(param);

	int ret = 0;
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	struct v4l2_ext_controls ctrls = { { 0 } };
	struct v4l2_ext_control ctrl[12] = { 0 };

	memcpy(&ctx->h264e_param, &param->codecparam.h264e_param, sizeof(IHal_H264E_Param));

	switch (param->codecparam.h264e_param.src_fmt) {
		case IMPP_PIX_FMT_NV12:
			ctx->srcfmt_des = &formats[0];
			break;
		case IMPP_PIX_FMT_NV21:
			ctx->srcfmt_des = &formats[1];
			break;
		case IMPP_PIX_FMT_YUV420p:
			ctx->srcfmt_des = &formats[2];
			break;
		default:
			IMPP_LOGE(TAG, "src_fmt type(%d) error!", param->codecparam.h264e_param.src_fmt);
			return -IHAL_RERR;
	}

	ctx->dstfmt_des = &formats[3];

	ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
	ctrls.controls = ctrl;
	ctrls.count = 12;

	ctrl[0].value = param->codecparam.h264e_param.max_bitrate * 1000;
	ctrl[0].id = V4L2_CID_MPEG_VIDEO_BITRATE;

	ctrl[1].value = param->codecparam.h264e_param.gop_len;
	ctrl[1].id = V4L2_CID_MPEG_VIDEO_GOP_SIZE;

	ctrl[2].value = param->codecparam.h264e_param.IFrameQp;
	ctrl[2].id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;

	ctrl[3].value = param->codecparam.h264e_param.PFrameQp;
	ctrl[3].id = V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;

	ctrl[4].value = param->codecparam.h264e_param.max_Qp;
	ctrl[4].id = V4L2_CID_MPEG_VIDEO_H264_MAX_QP;

	ctrl[5].value = param->codecparam.h264e_param.mini_Qp;
	ctrl[5].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;

	ctrl[6].value = 0;      // default set as 0
	ctrl[6].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;

	ctrl[7].value = V4L2_MPEG_VIDEO_H264_PROFILE_MAIN;      //keep this
	ctrl[7].id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;

	ctrl[8].value = V4L2_MPEG_VIDEO_H264_LEVEL_3_0;         // keep this
	ctrl[8].id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;

	ctrl[9].value = 0;
	ctrl[9].id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE;

	ctrl[10].value = 0;
	ctrl[10].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;

	switch (param->codecparam.h264e_param.rc_mode) {
	case IMPP_ENC_RC_MODE_VBR:
        	ctrl[11].value = 0;
        	break;
	case IMPP_ENC_RC_MODE_CBR:
        	ctrl[11].value = 1;
        	break;
	case IMPP_ENC_RC_MODE_CAPPED_QUALITY:
        	ctrl[11].value = 2;
        	break;
	default:
        	IMPP_LOGE(TAG, "not support rc mode ,x2000 only support cbr && vbr");
        	return -1;
	}
	ctrl[11].id = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;

	ret = ioctl(ctx->fd, VIDIOC_S_EXT_CTRLS, &ctrls);
	if (ret) {
        	IMPP_LOGE(TAG, " enc param set error");
        	return -1;
	}

	ret = _h264_enc_srcfmt_set(ctx);
	if (ret) {
		IMPP_LOGE(TAG, "src fmt set error.");
		return -IHAL_RERR;
	}

	ret = _h264_enc_dstfmt_set(ctx);
	if (ret) {
		IMPP_LOGE(TAG, "dst fmt set error.");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int h264_enc_param_get(void *chn_ctx, IHal_CodecParam *param)
{
	return IHAL_ROK;
}

static int h264_enc_chn_start(void* chn_ctx)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int ret = 0;
	enum v4l2_buf_type type;

	type = ctx->outbuf_type;
	ret = ioctl(ctx->fd, VIDIOC_STREAMON, &type);
	if (ret) {
		IMPP_LOGE(TAG, "out put buf streamon failed.");
		return -IHAL_RERR;
	}

	type = ctx->capbuf_type;
	ret = ioctl(ctx->fd, VIDIOC_STREAMON, &type);
	if (ret) {
		IMPP_LOGE(TAG, "out put buf streamon failed.");
		return -IHAL_RERR;
	}

	/* queue dst buffer to driver */
	for (int i = 0; i < ctx->dst_buf_num; i++) {
		ret = _h264_enc_qDst(chn_ctx, i);
		if (ret) {
			IMPP_LOGE(TAG, "h264enc start dst buffer prepare failed.");
			return -IHAL_RERR;
		}
	}

	return IHAL_ROK;
}

static int h264_enc_chn_stop(void* chn_ctx)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	enum v4l2_buf_type type;
	int ret = 0;

	type = ctx->capbuf_type;
	ret = ioctl(ctx->fd, VIDIOC_STREAMOFF, &type);
	if (ret) {
		IMPP_LOGE(TAG, "out put buf streamoff failed.");
		return -IHAL_RERR;
	}

	type = ctx->outbuf_type;
	ret = ioctl(ctx->fd, VIDIOC_STREAMOFF, &type);
	if (ret) {
		IMPP_LOGE(TAG, "out put buf streamoff failed.");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int h264_enc_srcbuffer_create(void* chn_ctx, IMPP_BUFFER_TYPE buftype, int num)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int ret = 0;
	int type = 0;

	ctx->src_buf_num = num;
	ctx->inputbuffer = (video_data_buffer_t *)calloc(num, sizeof(video_data_buffer_t));
	if (!ctx->inputbuffer) {
		IMPP_LOGE(TAG, "calloc ctx->inputbuffer failed.");
		return -IHAL_RERR;
	}

	type = ctx->outbuf_type;
	if (IMPP_INTERNAL_BUFFER == buftype) {
		ret = _init_mmap(ctx, type, (void *)ctx->inputbuffer, num);
		ctx->src_buf_type = IMPP_INTERNAL_BUFFER;
	} else if (IMPP_EXT_DMABUFFER == buftype) {
		/**
		 * 如果是DMA-BUF,vpu-driver先初始化为userpr，待设置buffers时，
		 * 通过mmap函数将dmabuf映射到用户空间，
		 * 然后以userptr方式给到vpu驱动
		 */
		ret = _init_userptr(ctx, type, num);
		ctx->src_buf_type = IMPP_EXT_DMABUFFER;
	} else if (IMPP_EXT_USERBUFFER == buftype) {
		ret = _init_userptr(ctx, type, num);
		ctx->src_buf_type = IMPP_EXT_USERBUFFER;
	} else {
		IMPP_LOGE(TAG, "not support buffer type.");
		return -IHAL_RERR;
	}

	if (ret) {
		IMPP_LOGE(TAG, "create src buffer failed.");
		return -IHAL_RERR;
	}

	return num;
}

static int h264_enc_srcbuffer_free(void* chn_ctx)
{
	assert(chn_ctx);

	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	unsigned int totalsize = 0;

	/* TODO munmap buffer */
	if (IMPP_INTERNAL_BUFFER == ctx->src_buf_type) {
		for (int i = 0; i < ctx->src_buf_num; i++) {
			for (int j = 0; j < ctx->srcfmt_des->v4l2_buffers_count; j++) {
				munmap(ctx->inputbuffer[i].vaddr[j], ctx->inputbuffer[i].bufsize[j]);
			}
		}
	} else if (IMPP_EXT_DMABUFFER == ctx->src_buf_type) {
		for (int i = 0; i < ctx->src_buf_num; i++) {
			for (int j = 0; j < ctx->srcfmt_des->v4l2_buffers_count; j++) {
				totalsize +=  ctx->inputbuffer[i].bufsize[j];
			}
			munmap(ctx->inputbuffer[i].vaddr[0], totalsize);
		}
	}

	free(ctx->inputbuffer);
	ctx->inputbuffer = NULL;

	return IHAL_ROK;
}

static int h264_enc_dstbuffer_create(void* chn_ctx, IMPP_BUFFER_TYPE buftype, int num)
{
	assert(chn_ctx);

	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int ret = 0;
	int type = 0;

	ctx->dst_buf_num = num;
	ctx->outputbuffer = (video_data_buffer_t *)calloc(num, sizeof(video_data_buffer_t));
	if (!ctx->outputbuffer) {
		IMPP_LOGE(TAG, "calloc ctx->outputbuffer failed.");
		return -IHAL_RERR;
	}

	type = ctx->capbuf_type;
	if (IMPP_INTERNAL_BUFFER == buftype) {
		ret = _init_mmap(ctx, type, (void *)ctx->outputbuffer, num);
		ctx->dst_buf_type = IMPP_INTERNAL_BUFFER;
	} else if (IMPP_EXT_DMABUFFER == buftype) {
		ret = _init_dmabuf(ctx, type, num);
		ctx->dst_buf_type = IMPP_EXT_DMABUFFER;
	} else if (IMPP_EXT_USERBUFFER == buftype) {
		ret = _init_userptr(ctx, type, num);
		ctx->dst_buf_type = IMPP_EXT_USERBUFFER;
	} else {
		IMPP_LOGE(TAG, "not support buffer type.");
		return -IHAL_RERR;
	}

	if (ret) {
		IMPP_LOGE(TAG, "create dst buffer failed.");
		return -IHAL_RERR;
	}

	return num;
}

static int h264_enc_dstbuffer_free(void* chn_ctx)
{
	assert(chn_ctx);

	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	if (ctx->dst_buf_type == IMPP_INTERNAL_BUFFER) {
		for (int i = 0; i < ctx->dst_buf_num; i++) {
			for (int j = 0; j < ctx->dstfmt_des->v4l2_buffers_count; j++) {
				munmap(ctx->outputbuffer[i].vaddr[j],ctx->outputbuffer[i].bufsize[j]);
			}
		}
	}

	free(ctx->outputbuffer);
	ctx->outputbuffer = NULL;

	return IHAL_ROK;
}

static int h264_enc_srcbuffer_get(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	/* only support userptr */
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int mp_num = ctx->srcfmt_des->v4l2_buffers_count;
	video_data_buffer_t *pSrc = (video_data_buffer_t *)ctx->inputbuffer;
	if (!pSrc) {
		IMPP_LOGE(TAG, "src buffer has not been created.");
		return -IHAL_RERR;
	}

	buffer->mplane.numPlanes = mp_num;
	for (int i = 0; i < mp_num; i++) {
		buffer->mplane.vaddr[i] = pSrc[index].vaddr[i];
		buffer->mplane.len[i] = pSrc[index].bufsize[i];
	}
	buffer->index = index;

	return IHAL_ROK;
}

static int h264_enc_srcbuffer_set(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	/* only support userptr */
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	video_data_buffer_t *pSrc = (video_data_buffer_t *)ctx->inputbuffer;
	int buffer_planes = ctx->srcfmt_des->v4l2_buffers_count;
	int width = ctx->image_width;
	int height = ctx->image_height;

	if (IMPP_EXT_USERBUFFER == ctx->src_buf_type) {
		_h264_enc_init_exit_buffers(&pSrc[index], width, height,
				buffer_planes, buffer->vaddr, buffer->size);
		pSrc[index].index = index;
	} else if (IMPP_EXT_DMABUFFER == ctx->src_buf_type) {
		unsigned int *mmapaddr = NULL;
		mmapaddr = mmap(NULL, buffer->size,
						PROT_READ,
						MAP_SHARED, buffer->fd, 0);
		if (NULL == mmapaddr) {
			IMPP_LOGE(TAG, "mmap dma-buf-fd failed.");
			return -IHAL_RERR;
		} else {
			IMPP_LOGI(TAG, "mmap dma-buf mmapaddr = %#x", mmapaddr);
		}

		_h264_enc_init_exit_buffers(&pSrc[index], width, height,
				buffer_planes, mmapaddr, buffer->size);
		pSrc[index].index = index;
		pSrc[index].dmafd[0] = buffer->fd; // save fd;
	} else {
		IMPP_LOGE(TAG, "srcbuffer set error only support userptr/userptr_mp/dma-buf yet!!");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int h264_enc_dstbuffer_get(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	/* only support userptr */
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	// int dstfmt = ctx->srcfmt_des->v4l2_format;
	// int mp_num = ctx->srcfmt_des->v4l2_buffers_count;
	int dstfmt = ctx->dstfmt_des->v4l2_format;
	int mp_num = ctx->dstfmt_des->v4l2_buffers_count;

	video_data_buffer_t *pDst = (video_data_buffer_t *)ctx->outputbuffer;
	if (!pDst) {
		IMPP_LOGE(TAG, "dst buffer has not been created.");
		return -IHAL_RERR;
	}

	buffer->mplane.numPlanes = mp_num;
	for (int i = 0; i < mp_num; i++) {
		buffer->mplane.vaddr[i] = pDst[index].vaddr[i];
		buffer->mplane.len[i] = pDst[index].bufsize[i];
	}

	return IHAL_ROK;
}

static int h264_enc_dstbuffer_set(void* chn_ctx,IMPP_BufferInfo_t *buffer,int index)
{
	/* only support userptr */
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int dstfmt = ctx->dstfmt_des->v4l2_format;
	int mp_num = ctx->dstfmt_des->v4l2_buffers_count;

	video_data_buffer_t *pDst = (video_data_buffer_t *)ctx->outputbuffer;
	int width = ctx->image_width;
	int height = ctx->image_height;

	if (IMPP_EXT_USERBUFFER == ctx->dst_buf_type) {
		_h264_enc_init_exit_buffers(&pDst[index], width, height,
				mp_num, buffer->vaddr, buffer->size);
		pDst[index].index = index;
	} else {
		IMPP_LOGE(TAG, "dstbuffer set error only support userptr yet!!");
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int h264_enc_poll_srcbuffer_avaliable(void* chn_ctx, int wait_type)
{
	assert(chn_ctx);
	int ret = 0;
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;

	if (wait_type == IMPP_WAIT_FOREVER) {
		ret = sem_wait(&ctx->src_available);
	} else {
		ret = sem_trywait(&ctx->src_available);
	}

	return ret;
}

int h264_enc_poll_dstbuffer_avaliable(void* chn_ctx, int wait_type)
{
	int ret = 0;
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int fd = ctx->fd;
	struct timeval tv;
	assert(fd > 0);

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = 10;
	tv.tv_usec = 50000;

	ret = select(fd + 1, &fds, NULL, NULL, NULL);
	if (ret == -1) {
		perror("select : ");
		IMPP_LOGE(TAG, "vcodec wait dstbuffer available failed.");
		return -IHAL_RERR;
	} else if (ret == 0) {
		IMPP_LOGE(TAG, "camera wait dstbuffer available timeout");
		return -IHAL_RERR;
	}

	sem_post(&ctx->src_available);

	return IHAL_ROK;
}

static int h264_enc_qSrc(void* chn_ctx, IMPP_BufferInfo_t *bufinfo)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;

	int index = bufinfo->index;
	assert(index >= 0);

	int ret = 0;
	int fd = ctx->fd;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[DATA_MAX_PLANES];

	bzero(&buf, sizeof(buf));
	switch (ctx->src_buf_type) {
		case IMPP_INTERNAL_BUFFER:
			buf.memory = V4L2_MEMORY_MMAP;
			break;
		case IMPP_EXT_DMABUFFER:
		case IMPP_EXT_USERBUFFER:
			buf.memory = V4L2_MEMORY_USERPTR;
			break;
		default:
			IMPP_LOGW(TAG, "not support buffer type.");
			return -IHAL_RERR;
	}

	pthread_mutex_lock(&ctx->mutex);
	if (ctx->chn_idr_request == 0x01) {
		buf.flags |= V4L2_BUF_FLAG_KEYFRAME;
		ctx->chn_idr_request = 0x00;
	}
	pthread_mutex_unlock(&ctx->mutex);

	buf.index = index;
	buf.m.planes = planes;
	buf.length = ctx->srcfmt_des->v4l2_buffers_count;
	if (IMPP_INTERNAL_BUFFER == ctx->src_buf_type) {
		if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == ctx->outbuf_type) {
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			for (int i = 0; i < ctx->srcfmt_des->v4l2_buffers_count; i++) {
				buf.m.planes[i].bytesused = ctx->inputbuffer[index].bufsize[i];
				buf.m.planes[i].length = ctx->inputbuffer[index].bufsize[i];
				buf.m.planes[i].data_offset = 0;        // must set 0
			}
		} else {
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
		}
	} else if (IMPP_EXT_USERBUFFER == ctx->src_buf_type || IMPP_EXT_DMABUFFER == ctx->src_buf_type) {
		if (V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE == ctx->outbuf_type) {
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
			for (int i = 0; i < ctx->srcfmt_des->v4l2_buffers_count; i++) {
				buf.m.planes[i].m.userptr = (unsigned long)ctx->inputbuffer[index].vaddr[i];
				buf.m.planes[i].bytesused = (unsigned long)ctx->inputbuffer[index].bufsize[i];
				buf.m.planes[i].length = ctx->inputbuffer[index].bufsize[i];
				buf.m.planes[i].data_offset = 0;
			}
		} else {
			buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			buf.bytesused = 0;
			buf.m.userptr = (void *)ctx->inputbuffer[index].vaddr[0];
			buf.length = ctx->inputbuffer[index].bufsize[0];
		}
	} else {
		/* exit dma buf */
	}

	ret = ioctl(fd, VIDIOC_QBUF, &buf);
	if (ret) {
		perror("qsc buf ioctl: ");
		IMPP_LOGE(TAG, "Fail to ioctl srcbuf 'VIDIOC_QBUF' ret = %d", ret);
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

static int h264_enc_dqSrc(void* chn_ctx, IMPP_BufferInfo_t *bufinfo)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int fd = ctx->fd;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[DATA_MAX_PLANES];
	int ret = 0;
	bzero(&buf, sizeof(buf));

	switch (ctx->src_buf_type) {
		case IMPP_INTERNAL_BUFFER:
			buf.memory = V4L2_MEMORY_MMAP;
			break;
		case IMPP_EXT_USERBUFFER:
		case IMPP_EXT_DMABUFFER:
			buf.memory = V4L2_MEMORY_USERPTR;
			break;
		default:
			IMPP_LOGE(TAG, "not support buffer type.");
			return -IHAL_RERR;
	}

	buf.m.planes = planes;
	buf.length = ctx->srcfmt_des->v4l2_buffers_count;   // n_planes
	if (ctx->outbuf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	} else {
		buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	}

	ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	if (ret) {
		IMPP_LOGE(TAG, "Dq src failed.");
		return -IHAL_RERR;
	}

	if (ctx->outbuf_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		bufinfo->mplane.numPlanes = buf.length;
		for (int i = 0; i < buf.length; i++) {
			bufinfo->mplane.vaddr[i] = ctx->inputbuffer[buf.index].vaddr[i];
			bufinfo->mplane.len[i] = ctx->inputbuffer[buf.index].bufsize[i];
		}
	} else {
		bufinfo->vaddr = ctx->inputbuffer[buf.index].vaddr[0];
		bufinfo->size = ctx->inputbuffer[buf.index].bufsize[0];
	}

	bufinfo->index = buf.index;

	return IHAL_ROK;
}

static int h264_enc_qDst(void* chn_ctx, IHAL_CodecStreamInfo_t *bufinfo)
{
	int ret = 0;
	int index = bufinfo->index;
	assert(chn_ctx);
	assert(index >= 0);

	ret = _h264_enc_qDst(chn_ctx, index);
	if (ret)
		return -IHAL_RERR;

	return ret;
}

static int h264_enc_dqDst(void* chn_ctx, IHAL_CodecStreamInfo_t *outbuf)
{
	assert(chn_ctx);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;
	int fd = ctx->fd;
	struct v4l2_buffer buf;
	struct v4l2_plane planes[DATA_MAX_PLANES];
	int ret = 0;

	bzero(&buf, sizeof(buf));
	switch (ctx->dst_buf_type) {
		case IMPP_INTERNAL_BUFFER:
			buf.memory = V4L2_MEMORY_MMAP;
			break;
		case IMPP_EXT_USERBUFFER:
			buf.memory = V4L2_MEMORY_USERPTR;
			break;
		case IMPP_EXT_DMABUFFER:
			buf.memory = V4L2_MEMORY_DMABUF;
			break;
		default:
			IMPP_LOGE(TAG, "not support buffer type.");
			return -IHAL_RERR;
	}

	buf.m.planes = planes;
	buf.length = ctx->dstfmt_des->v4l2_buffers_count;   // n_planes
	if (ctx->capbuf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	} else {
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	}

	ret = ioctl(fd, VIDIOC_DQBUF, &buf);
	if (ret) {
		IMPP_LOGE(TAG, "ioctl dqdst failed.");
		return -IHAL_RERR;
	}

	ctx->seq++;

	int nalu_offset[3];
	int nalu_type[3];

	if (ctx->dstfmt_des->v4l2_buffers_count == 1 &&
		ctx->capbuf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		outbuf->vaddr = ctx->outputbuffer[buf.index].vaddr[0];
		outbuf->size = buf.m.planes[0].bytesused;

		_h264_pack_praser(outbuf->vaddr, outbuf->size, &ctx->outputbuffer[buf.index].stream);
		outbuf->pack.seq = ctx->seq;
		outbuf->pack.packcnt = ctx->outputbuffer[buf.index].stream.packcnt;
		outbuf->pack.pack = &ctx->outputbuffer[buf.index].stream.pack;
	}

	outbuf->index = buf.index;

	return IHAL_ROK;
}

static int h264_enc_control(void* chn_ctx, IHal_CodecControlParam_t *param)
{
	assert(chn_ctx);
	assert(param);
	h264e_chn_ctx_t *ctx = (h264e_chn_ctx_t *)chn_ctx;

	int ret = 0;

	switch (param->cmd) {
		case ENCODER_IDR_REQUEST:
			ret = _h264_enc_request_idr(ctx);
			break;
		default:
			IMPP_LOGE(TAG, "not support cmd(%d) type", param->cmd);
			return -IHAL_RERR;
	}

	if (ret) {
		return -IHAL_RERR;
	}

	return IHAL_ROK;
}

ImppCodec_t h264enc_v1 = {
	.name = "h264enc_v1",
	.version = 0x0001,

	.codec_init = NULL,
	.codec_deinit = NULL,

	.codec_chn_ctx_create = h264_enc_chn_ctx_create,
	.codec_chn_ctx_destroy = h264_enc_chn_ctx_destroy,
	.codec_param_set = h264_enc_param_set,
	.codec_param_get = h264_enc_param_get,
	.codec_chn_start = h264_enc_chn_start,
	.codec_chn_stop = h264_enc_chn_stop,

	.src_buffer_create = h264_enc_srcbuffer_create,
	.src_buffer_free = h264_enc_srcbuffer_free,
	.dst_buffer_create = h264_enc_dstbuffer_create,
	.dst_buffer_free = h264_enc_dstbuffer_free,

	.get_src_buffer = h264_enc_srcbuffer_get,
	.set_src_buffer = h264_enc_srcbuffer_set,
	.get_dst_buffer = h264_enc_dstbuffer_get,
	.set_dst_buffer = h264_enc_dstbuffer_set,

	.poll_src_buffer_avaliable = h264_enc_poll_srcbuffer_avaliable,
	.poll_dst_buffer_avaliable = h264_enc_poll_dstbuffer_avaliable,

	.queue_src_buffer = h264_enc_qSrc,
	.dequeue_src_buffer = h264_enc_dqSrc,
	.queue_dst_buffer = h264_enc_qDst,
	.dequeue_dst_buffer = h264_enc_dqDst,

	.control = h264_enc_control,
};
