/**
 * @file icamera.c
 * @author
 * @brief
 * @version 0.1
 * @date 2021-12-03
 *
 * @copyright Copyright (c) ingenic 2021
 *
 */

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
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

#include "icamera.h"
#include "log.h"

#define LOGTAG  "ICAMERA"


typedef enum {
        ISP_ANTIFLICKER_DISABLE_MODE,
        ISP_ANTIFLICKER_NORMAL_MODE,
        ISP_ANTIFLICKER_AUTO_MODE,// can reach min it
        ISP_ANTIFLICKER_BUTT,
} tisp_antiflicker_mode_t;

typedef struct {
        tisp_antiflicker_mode_t mode;
        unsigned char freq;
} tisp_anfiflicker_attr_t;


enum isp_image_tuning_private_cmd_id {
        IMAGE_TUNING_CID_MODULE_CONTROL = V4L2_CID_PRIVATE_BASE,
        IMAGE_TUNING_CID_DAY_OR_NIGHT,
        IMAGE_TUNING_CID_AE_LUMA,
        IMAGE_TUNING_CID_FACE_AE_CONTROL,
        IMAGE_TUNING_CID_FACE_AWB_CONTROL,
        IMAGE_TUNING_CID_SWITCH_BIN,
        IMAGE_TUNING_CID_SET_DEFAULT_BIN_PATH,
};


typedef struct {
        int vinum;
        char path[64];
} tx_isp_bin_path;

typedef struct {
        IHAL_UINT32 *bufferaddr;
        IHAL_UINT32 *buffer_paddr;
        IHAL_INT32  fd;
        IHAL_INT32  index;
        IHAL_INT32  buffersize;
} camera_prv_bufferinfo;

typedef struct {
        IHAL_INT32 imageWidth;
        IHAL_INT32 imageHeight;
        IHAL_INT32 imageFmt;
        IMPP_BUFFER_TYPE buffertype;
        IHAL_INT32 buffer_num;
        camera_prv_bufferinfo *bufferinfo;
        impp_fifo_t buffer_fifo;
        IHAL_INT32 streamon_flag;                                       // 开流的标志位
} camera_prv_ctx;

static int buffer_export(int v4lfd, enum v4l2_buf_type bt, int index, int *dmafd)
{
        struct v4l2_exportbuffer expbuf;
        /* bzero(&expbuf,sizeof(expbuf)); */
        memset(&expbuf, 0, sizeof(expbuf));
        expbuf.type = bt;
        expbuf.index = index;
        if (ioctl(v4lfd, VIDIOC_EXPBUF, &expbuf) == -1) {
                perror("VIDIOC_EXPBUF");
                return -1;
        }

        *dmafd = expbuf.fd;

        return 0;
}

static IHAL_INT32 camera_init_userptr(IHAL_CameraHandle_t *handle, IHAL_INT32 *nr_buf)
{
        struct v4l2_requestbuffers reqbuf;
        int ret = 0;
        camera_prv_ctx *ctx = handle->ctx;

        assert(ctx);
        bzero(&reqbuf, sizeof(reqbuf));
        reqbuf.count = *nr_buf;
        reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqbuf.memory = V4L2_MEMORY_USERPTR;

        ret = ioctl(handle->vfd, VIDIOC_REQBUFS, &reqbuf);
        if (ret < 0) {
                IMPP_LOGE(LOGTAG, "camera request userptr-buffer failed");
                return -IHAL_RERR;
        }

        if (*nr_buf > reqbuf.count) {
                IMPP_LOGW(LOGTAG, "%d userptr-buffers were requested, but %d userptr-buffers were actually successfully requested!", *nr_buf, reqbuf.count);
                *nr_buf = reqbuf.count;
        }

        ctx->bufferinfo = (camera_prv_bufferinfo *)malloc(*nr_buf * sizeof(camera_prv_bufferinfo));
        if (ctx->bufferinfo == NULL) {
                IMPP_LOGE(LOGTAG, "buffer info malloc failed");
                return -IHAL_RERR;
        }

        IMPP_LOGD(LOGTAG, "camera request userptr-buffer ok!!");
        return IHAL_ROK;
}

static IHAL_INT32 camera_deinit_userptr(IHAL_CameraHandle_t *handle)
{
        camera_prv_ctx *ctx = handle->ctx;
        assert(ctx);

        free(ctx->bufferinfo);
        ctx->bufferinfo = NULL;

        IMPP_LOGD(LOGTAG, "camera release userptr-buffer ok!!");
        return IHAL_ROK;
}

static IHAL_INT32 camera_init_mmap(IHAL_CameraHandle_t *handle, IHAL_INT32 *nr_buf)
{
        struct v4l2_requestbuffers reqbuf;
        camera_prv_ctx *ctx = handle->ctx;
        int ret = 0;
        assert(ctx);
        bzero(&reqbuf, sizeof(reqbuf));
        reqbuf.count = *nr_buf;
        reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqbuf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(handle->vfd, VIDIOC_REQBUFS, &reqbuf);
        if (ret < 0) {
                IMPP_LOGE(LOGTAG, "camera request buffer failed\n");
                return -IHAL_RERR;
        }

        if (*nr_buf > reqbuf.count) {
                IMPP_LOGW(LOGTAG, "%d buffers were requested, but %d buffers were actually successfully requested!", *nr_buf, reqbuf.count);
                *nr_buf = reqbuf.count;
        }

        ctx->bufferinfo = (camera_prv_bufferinfo *)malloc(*nr_buf * sizeof(camera_prv_bufferinfo));
        if (ctx->bufferinfo == NULL) {
                IMPP_LOGE(LOGTAG, "buffer info malloc failed");
                return -IHAL_RERR;
        }

        struct v4l2_buffer buf;
        for (int i = 0; i < *nr_buf; i++) {
                bzero(&buf, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if (-1 == ioctl(handle->vfd, VIDIOC_QUERYBUF, &buf)) {
                        IMPP_LOGE(LOGTAG, "Fail to ioctl : VIDIOC_QUERYBUF");
                        free(ctx->bufferinfo);
                        return -IHAL_RERR;
                }

                //printf("-----buf.length: %d\n", buf.length);
		ctx->bufferinfo[i].buffer_paddr = buf.reserved;
                ctx->bufferinfo[i].bufferaddr = mmap(NULL, buf.length,
                                                     PROT_READ | PROT_WRITE, MAP_SHARED, handle->vfd, buf.m.offset);
                ctx->bufferinfo[i].buffersize = buf.length;
                ctx->bufferinfo[i].index = i;
                if (ctx->bufferinfo[i].bufferaddr == MAP_FAILED) {
                        IMPP_LOGE(LOGTAG, "camera buffer mmap failed");
                        free(ctx->bufferinfo);
                        return -IHAL_RERR;
                }

                // export dma buffer
                ctx->bufferinfo[i].fd = -1;
                ret = buffer_export(handle->vfd, V4L2_BUF_TYPE_VIDEO_CAPTURE, i, &ctx->bufferinfo[i].fd);
                if (ret) {
                        IMPP_LOGW(LOGTAG, "export dma buffer failed");
                }
        }

        IMPP_LOGD(LOGTAG, "camera request mmap-buffer ok!!");
        return IHAL_ROK;
}

static IHAL_INT32 camera_deinit_mmap(IHAL_CameraHandle_t *handle)
{
        camera_prv_ctx *ctx = handle->ctx;
        int ret = 0;
        assert(ctx);

        for (int i = 0; i < ctx->buffer_num; i++) {
                if (-1 == munmap(ctx->bufferinfo[i].bufferaddr, ctx->bufferinfo[i].buffersize)) {
                        IMPP_LOGE(LOGTAG, "camera buffer munmap failed");
                        return -IHAL_RERR;
                }

                ret = close(ctx->bufferinfo[i].fd);
                if (-1 == ret) {
                        IMPP_LOGE(LOGTAG, "close dmafd failed");
                        return -IHAL_RERR;
                }

                ctx->bufferinfo[i].bufferaddr = NULL;
                ctx->bufferinfo[i].buffersize = 0;
        }

        free(ctx->bufferinfo);
        ctx->bufferinfo = NULL;

        IMPP_LOGD(LOGTAG, "camera release mmap-buffer ok!!");
        return IHAL_ROK;
}

static IHAL_INT32 camera_init_dmabuf(IHAL_CameraHandle_t *handle, IHAL_INT32 *nr_buf)
{
        struct v4l2_requestbuffers reqbuf;
        int ret = 0;
        camera_prv_ctx *ctx = handle->ctx;

        assert(ctx);
        bzero(&reqbuf, sizeof(reqbuf));
        reqbuf.count = *nr_buf;
        reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        reqbuf.memory = V4L2_MEMORY_DMABUF;

        ret = ioctl(handle->vfd, VIDIOC_REQBUFS, &reqbuf);
        if (ret < 0) {
                perror("init dma-buf:");
                IMPP_LOGE(LOGTAG, "camera request dma-buffer failed");
                return -IHAL_RERR;
        }

        if (*nr_buf > reqbuf.count) {
                IMPP_LOGW(LOGTAG, "%d dma-buffers were requested, but %d dma-buffers were actually successfully requested!", *nr_buf, reqbuf.count);
                *nr_buf = reqbuf.count;
        }
        ctx->bufferinfo = (camera_prv_bufferinfo *)malloc(*nr_buf * sizeof(camera_prv_bufferinfo));
        if (ctx->bufferinfo == NULL) {
                IMPP_LOGE(LOGTAG, "buffer info malloc failed");
                return -IHAL_RERR;
        }

        IMPP_LOGD(LOGTAG, "camera request dma-buffer ok!!");
        return IHAL_ROK;

}

static IHAL_INT32 camera_deinit_dmabuf(IHAL_CameraHandle_t *handle)
{
        camera_prv_ctx *ctx = handle->ctx;
        assert(ctx);

        free(ctx->bufferinfo);
        ctx->bufferinfo = NULL;

        IMPP_LOGD(LOGTAG, "camera release dma-buffer ok!!");
        return IHAL_ROK;
}

IHAL_CameraHandle_t *IHal_CameraOpen(IHAL_INT8 *videoname)
{
        IHAL_CameraHandle_t *handle;

        assert(videoname);

        handle = (IHAL_CameraHandle_t *)malloc(sizeof(IHAL_CAMERA_HANDLE));
        if (!handle) {
                IMPP_LOGE(LOGTAG, "malloc camera handle failed");
                return IHAL_RNULL;
        }
        handle->ctx = (camera_prv_ctx *)calloc(1, sizeof(camera_prv_ctx));
        if (!handle->ctx) {
                IMPP_LOGE(LOGTAG, "malloc camera prv ctx failed");
                free(handle);
                return IHAL_RNULL;
        }
        handle->vfd = open(videoname, O_RDWR);
        if (handle->vfd < 0) {
                IMPP_LOGE(LOGTAG, "open %s failed\n", videoname);
                free(handle->ctx);
                free(handle);
                return IHAL_RNULL;
        }
        IMPP_LOGD(LOGTAG, "camera open vfd = %d \n", handle->vfd);
        return handle;
}


IHAL_INT32 IHal_CameraClose(IHAL_CameraHandle_t *handle)
{
        camera_prv_ctx *ctx = NULL;
        int ret = 0;

        assert(handle);
        ctx = handle->ctx;
        assert(ctx);
        assert(handle->vfd > 0);

        impp_fifo_deinit(&ctx->buffer_fifo);

        if (ctx->buffertype == IMPP_INTERNAL_BUFFER) {
                ret = camera_deinit_mmap(handle);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "camera release intrenal buffer failed");
                        return -IHAL_RERR;
                }
        } else if (ctx->buffertype == IMPP_EXT_USERBUFFER) {
                ret = camera_deinit_userptr(handle);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "camera release external-userptr buffer failed");
                        return -IHAL_RERR;
                }
        } else if (ctx->buffertype == IMPP_EXT_DMABUFFER) {
                ret = camera_deinit_dmabuf(handle);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "camera release extrenal-dmabuf buffer failed");
                        return -IHAL_RERR;
                }
        }

        close(handle->vfd);
        free(ctx);
        handle->ctx = NULL;
        free(handle);
        handle = NULL;

        return IHAL_ROK;
}




struct fmtStrMap {
        IMPP_PIX_FMT fmt;
        IHAL_INT8 *str;
};

static struct fmtStrMap fmtStrMaps[] = {
        {IMPP_PIX_FMT_NV12, "NV12"},
        {IMPP_PIX_FMT_NV21, "NV21"},
        {IMPP_PIX_FMT_SBGGR8, "SBGGR8"},
        {IMPP_PIX_FMT_SGBRG8, "SGBRG8"},
        {IMPP_PIX_FMT_SGRBG8, "SGRBG8"},
        {IMPP_PIX_FMT_SRGGB8, "SRGGB8"},
        {IMPP_PIX_FMT_SBGGR10, "SBGGR10"},
        {IMPP_PIX_FMT_SGBRG10, "SGBRG10"},
        {IMPP_PIX_FMT_SGRBG10, "SGRBG10"},
        {IMPP_PIX_FMT_SRGGB10, "SRGGB10"},
        {IMPP_PIX_FMT_SBGGR12, "SBGGR12"},
        {IMPP_PIX_FMT_SGBRG12, "SGBRG12"},
        {IMPP_PIX_FMT_SGRBG12, "SGRBG12"},
        {IMPP_PIX_FMT_SRGGB12, "SRGGB12"},
        {IMPP_PIX_FMT_YUYV, "YUYV"},
        {IMPP_PIX_FMT_YYUV, "YYUV"},
        {IMPP_PIX_FMT_YVYU, "YVYU"},
        {IMPP_PIX_FMT_VYUY, "VYUY"},
        {IMPP_PIX_FMT_GREY, "GREY"},
        //      {IMPP_PIX_FMT_YUV422,"YUV422"},
		{IMPP_PIX_FMT_MJPEG, "MJPEG"},
};

static IMPP_PIX_FMT _getFmtFromFmtStr(IHAL_INT8 *fmtStr)
{
        struct fmtStrMap *f = NULL;
        int i = 0;

        if (fmtStr == NULL) {
                return -IHAL_RERR;
        }

        for (i = 0; i < sizeof(fmtStrMaps) / sizeof(struct fmtStrMap) ; i++) {
                f = &fmtStrMaps[i];

                if (!strncmp(f->str, fmtStr, strlen(f->str))) {
                        return f->fmt;
                }

        }
        return -IHAL_RERR;
}


IHAL_INT32 IHal_CameraSetParams(IHAL_CameraHandle_t *handle, IHAL_CAMERA_PARAMS *params)
{
        /* struct v4l2_fmtdesc fmt; */
        struct v4l2_capability cap;
        struct v4l2_format stream_fmt;
        int ret = 0;
        int fmt = 0;
        int fd = -1;
        struct v4l2_control ctrl;
        struct v4l2_crop crop;
        struct v4l2_selection selection;
        IMPP_PIX_FMT impp_fmt;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        assert(params);

        tx_isp_bin_path binpath;
        if (params->bin_select == 1) {
                memset(&binpath, 0, sizeof(binpath));
                memcpy(binpath.path, params->binpath, strlen(params->binpath));
                /* binpath.vinum = 0; */
                ctrl.id = IMAGE_TUNING_CID_SET_DEFAULT_BIN_PATH;
                ctrl.value = (int)&binpath;
                ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "set bin path failed");
                        return -IHAL_RERR;
                }
        }
        if (params->wdr_enable == 1) {
                ctrl.id = V4L2_CID_WIDE_DYNAMIC_RANGE;
                ctrl.value = 1;
                ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "wdr enable failed");
                        return -IHAL_RERR;
                }
        }
        camera_prv_ctx *ctx = handle->ctx;
        assert(ctx);
        ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
        if (ret < 0) {
                IMPP_LOGE(LOGTAG, "FAIL to ioctl VIDIOC_QUERYCAP");
                return -IHAL_RERR;
        }
        if (!(cap.capabilities & V4L2_BUF_TYPE_VIDEO_CAPTURE)) {
                IMPP_LOGE(LOGTAG, "The Current device is not a video capture device\n");
                return -IHAL_RERR;
        }
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                IMPP_LOGE(LOGTAG, "The Current device does not support streaming i/o\n");
                return -IHAL_RERR;
        }


        if (params->imageFmtStr != NULL) {
                impp_fmt = _getFmtFromFmtStr(params->imageFmtStr);
        } else {
                impp_fmt = params->imageFmt;
        }
#define _CASE_FMT(f)    \
    case IMPP_PIX_FMT_##f: fmt = V4L2_PIX_FMT_##f;break;


        switch (impp_fmt) {
                _CASE_FMT(NV12);
                _CASE_FMT(NV21);
                _CASE_FMT(SBGGR8);
                _CASE_FMT(SGBRG8);
                _CASE_FMT(SGRBG8);
                _CASE_FMT(SBGGR10);
                _CASE_FMT(SGBRG10);
                _CASE_FMT(SGRBG10);
                _CASE_FMT(SRGGB10);
                _CASE_FMT(SBGGR12);
                _CASE_FMT(SGBRG12);
                _CASE_FMT(SGRBG12);
                _CASE_FMT(SRGGB12);

                _CASE_FMT(GREY);
                //  _CASE_FMT(YUV422);
                _CASE_FMT(YUYV);
                _CASE_FMT(YYUV);
                _CASE_FMT(YVYU);
                _CASE_FMT(VYUY);

				_CASE_FMT(MJPEG);

        default:
                IMPP_LOGE(LOGTAG, "impp imageFmt (%d) not support\n", params->imageFmt);
                return -IHAL_RERR;
        }

        stream_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        stream_fmt.fmt.pix.width = params->imageWidth;
        stream_fmt.fmt.pix.height = params->imageHeight;
        stream_fmt.fmt.pix.pixelformat = fmt;
        stream_fmt.fmt.pix.field = V4L2_FIELD_ANY;

        // private ctx
        ctx->imageWidth = params->imageWidth;
        ctx->imageHeight = params->imageHeight;
        ctx->imageFmt = fmt;
        if (-1 == ioctl(fd, VIDIOC_S_FMT, &stream_fmt)) {
                IMPP_LOGE(LOGTAG, "VIDIOC_S_FMT Fail to ioctl");
                return -IHAL_RERR;
        }
#ifdef X2500
        /* before mscaler crop. */
        if ((params->crop_width > 0) && (params->crop_height > 0) &&
            (params->crop_posx >= 0) && (params->crop_posy >= 0)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c.left = params->crop_posx;
                crop.c.top = params->crop_posy;
                crop.c.width = params->crop_width;
                crop.c.height = params->crop_height;
                ret = ioctl(fd, VIDIOC_S_CROP, &crop);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "crop enable failed");
                        return -IHAL_RERR;
                }
        }
#endif
        /* after mscaler crop. */
        if ((params->scale_out_crop_width > 0) && (params->scale_out_crop_height > 0) &&
            (params->scale_out_crop_posx >= 0) && (params->scale_out_crop_posy >= 0)) {
                selection.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                selection.target = V4L2_SEL_TGT_CROP;
                selection.r.left = params->scale_out_crop_posx;
                selection.r.top = params->scale_out_crop_posy;
                selection.r.width = params->scale_out_crop_width;
                selection.r.height = params->scale_out_crop_height;
                ret = ioctl(fd, VIDIOC_S_SELECTION, &selection);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "set scale out crop failed,the function only support x2500");
                        return -IHAL_RERR;
                }
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraSetControlParams(IHAL_CameraHandle_t *handle, IHAL_CAMERA_CONTROL_CMD cmd, IHAL_UINT32 value)
{
        int ret = 0;
        struct v4l2_control ctrl;
        int fd = -1;
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);

        if (1 != ctx->streamon_flag) {
                IMPP_LOGE(LOGTAG, "Control parameters must be set after the camera stream starts");
                return -IHAL_RERR;
        }

        bzero(&ctrl, sizeof(ctrl));
        ctrl.id = cmd;
        ctrl.value = value;
        ret = ioctl(fd, VIDIOC_S_CTRL, &ctrl);
        if (ret) {
                IMPP_LOGE(LOGTAG, "set control param failed cmd : %d", cmd);
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraGetControlParams(IHAL_CameraHandle_t *handle, IHAL_CAMERA_CONTROL_CMD cmd, IHAL_UINT32 *value)
{
        int ret = 0;
        struct v4l2_control ctrl;
        int fd = -1;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        assert(value);

        bzero(&ctrl, sizeof(ctrl));
        ctrl.id = cmd;
        ret = ioctl(fd, VIDIOC_G_CTRL, &ctrl);
        if (ret) {
                IMPP_LOGE(LOGTAG, "get control param failed cmd : %d", cmd);
                return -IHAL_RERR;
        }
        *value = ctrl.value;
        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraCreateBuffers(IHAL_CameraHandle_t *handle, IMPP_BUFFER_TYPE type, IHAL_INT32 num_buffers)
{
        IHAL_INT32 ret = 0;
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        ctx = handle->ctx;
        assert(ctx);

        assert(handle->vfd);
        /* IMPP_LOGD(LOGTAG,"%s entry ~~~~",__func__); */
        if (type > IMPP_EXT_USERBUFFER) {
                IMPP_LOGE(LOGTAG, "buffer type not support");
                return -IHAL_RERR;
        }
#if 0
        /*库里面不要限制buffer数量.*/
        if (num_buffers > 0) {
                if (num_buffers > 3) {
                        IMPP_LOGW(LOGTAG, "The requested buffer exceeds 3, it will be automatically modified to 3.");
                        num_buffers = 3;
                }
        } else {
                IMPP_LOGE(LOGTAG, "The number of buffers should be greater than 0");
                return -IHAL_RERR;
        }
#endif
        if (type == IMPP_INTERNAL_BUFFER) {
                ret = camera_init_mmap(handle, &num_buffers);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "camera create intrenal buffer failed");
                        return -IHAL_RERR;
                }
        } else if (type == IMPP_EXT_USERBUFFER) {
                ret = camera_init_userptr(handle, &num_buffers);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "camera create external-userptr buffer failed");
                        return -IHAL_RERR;
                }
        } else if (type == IMPP_EXT_DMABUFFER) {
                ret = camera_init_dmabuf(handle, &num_buffers);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "camera create extrenal-dmabuf buffer failed");
                        return -IHAL_RERR;
                }
        } else {
                IMPP_LOGE(LOGTAG, "not support buffer type");
                return -IHAL_RERR;
        }
        ctx->buffer_num = num_buffers;
        ctx->buffertype = type;
        ret = impp_fifo_init(&ctx->buffer_fifo, num_buffers);
        if (ret) {
                IMPP_LOGE(LOGTAG, "buffer fifo init failed");
                free(ctx->bufferinfo);
                return -IHAL_RERR;
        }
        return num_buffers;
}

IHAL_INT32 IHal_CameraSetBuffers(IHAL_CameraHandle_t *handle, IHAL_INT32 index, IMPP_BufferInfo_t *sharebuf)
{
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        ctx = handle->ctx;
        assert(ctx);
        assert(index >= 0);
        assert(sharebuf);
        assert(sharebuf->size > 0);

        if ((ctx->buffertype == IMPP_EXT_DMABUFFER) && (sharebuf->fd > 0)) {
                ctx->bufferinfo[index].fd = sharebuf->fd;
                ctx->bufferinfo[index].buffersize = sharebuf->size;
        } else if ((ctx->buffertype == IMPP_EXT_USERBUFFER) && (sharebuf->vaddr > 0)) {
                ctx->bufferinfo[index].bufferaddr = sharebuf->vaddr;
                ctx->bufferinfo[index].buffersize = sharebuf->size;
        } else {
                IMPP_LOGE(LOGTAG, "share buffer type not support");
                return -IHAL_RERR;
        }
        ctx->bufferinfo[index].index = index;
        return IHAL_ROK;
}

IHAL_INT32 IHal_GetCameraBuffers(IHAL_CameraHandle_t *handle, IHAL_INT32 index, IMPP_BufferInfo_t *sharebuf)
{
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        ctx = handle->ctx;
        assert(ctx);
        assert(index >= 0);
        assert(sharebuf);

        if (ctx->bufferinfo[index].fd <  0) {
                IMPP_LOGE(LOGTAG, "the dma-buf not export,get camera buffers failed");
                return -IHAL_RERR;
        }
        sharebuf->fd = ctx->bufferinfo[index].fd;
        sharebuf->size = ctx->bufferinfo[index].buffersize;
        sharebuf->vaddr = ctx->bufferinfo[index].bufferaddr;
        sharebuf->paddr = 0;        // not support paddr
        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraStart(IHAL_CameraHandle_t *handle)
{
        int fd = -1;
        int ret = 0;
        enum v4l2_buf_type type;
        struct v4l2_buffer buf;
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);

        // qbuf to video
        for (int i = 0; i < ctx->buffer_num; i++) {
                bzero(&buf, sizeof(buf));
                switch (ctx->buffertype) {
                case IMPP_INTERNAL_BUFFER:
                        buf.memory = V4L2_MEMORY_MMAP;
                        break;
                case IMPP_EXT_USERBUFFER:
                        buf.memory = V4L2_MEMORY_USERPTR;
                        break;
                case IMPP_EXT_DMABUFFER:
                        IMPP_LOGD(LOGTAG, "q ext dma buf");
                        buf.memory = V4L2_MEMORY_DMABUF;
                        break;
                default:
                        IMPP_LOGE(LOGTAG, "not support buffer type");
                        return -IHAL_RERR;
                }
                buf.index = i;
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (ctx->buffertype == IMPP_EXT_DMABUFFER) {
                        IMPP_LOGD(LOGTAG, "q ext dma buf 1");
                        buf.m.fd = ctx->bufferinfo[i].fd;
                } else if (ctx->buffertype == IMPP_EXT_USERBUFFER) {
                        buf.m.userptr = (unsigned long)ctx->bufferinfo[i].bufferaddr;
                        buf.length = ctx->bufferinfo[i].buffersize;
                }
                IMPP_LOGD(LOGTAG, "vfd = %d fd = %d idx = %d ", fd, buf.m.fd, buf.index);
                ret = ioctl(fd, VIDIOC_QBUF, &buf);
                if (ret) {
                        IMPP_LOGE(LOGTAG, "Fail to ioctl 'VIDIOC_QBUF'");
                        return -IHAL_RERR;
                }
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(fd, VIDIOC_STREAMON, &type);
        if (ret) {
                IMPP_LOGE(LOGTAG, "Fail to start camera stream");
                return -IHAL_RERR;
        }

        ctx->streamon_flag = 1;                             // 开流成功

        IMPP_LOGD(LOGTAG, "impp camera start ok");
        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraStop(IHAL_CameraHandle_t *handle)
{
        int fd = -1;
        enum v4l2_buf_type type;
        int ret = 0;
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
        if (ret) {
                IMPP_LOGE(LOGTAG, "Fail to stop camera");
                return -IHAL_RERR;
        }

        ctx->streamon_flag = 0;                         // 关流

        return IHAL_ROK;
}


IHAL_INT32 IHal_Camera_WaitBufferAvailable(IHAL_CameraHandle_t *handle, IHAL_INT32 impp_wait)
{
        int fd = -1;
        camera_prv_ctx *ctx = NULL;
        struct v4l2_buffer buf;
        fd_set fds;
        struct timeval tv;
        int ret = 0;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;    //500ms max
        if (impp_wait == IMPP_WAIT_FOREVER) {
                ret = select(fd + 1, &fds, NULL, NULL, NULL);
        } else {
                ret = select(fd + 1, &fds, NULL, NULL, &tv);
        }

        if (ret == -1) {
                IMPP_LOGE(LOGTAG, "camera wait buffer available failed");
                return -IHAL_RERR;
        }

        if (ret == 0) {
                IMPP_LOGE(LOGTAG, "camera wait buffer available timeout");
                return -IHAL_RERR;
        }

        bzero(&buf, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        switch (ctx->buffertype) {
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
                IMPP_LOGE(LOGTAG, "not support buffer type");
                return -IHAL_RERR;
        }

        ret = ioctl(fd, VIDIOC_DQBUF, &buf);
        if (ret) {
                IMPP_LOGE(LOGTAG, "camera dqbuf failed");
                return -IHAL_RERR;
        }

        ctx->bufferinfo[buf.index].buffersize = buf.bytesused;

        ret = impp_fifo_queue(&ctx->buffer_fifo, (void *)&ctx->bufferinfo[buf.index], IMPP_NO_WAIT);

        if (ret) {
                IMPP_LOGE(LOGTAG, "camera buffer fifo queue failed \n");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraDeQueueBuffer(IHAL_CameraHandle_t *handle, IMPP_BufferInfo_t *buf)
{
        int fd = -1;
        camera_prv_ctx *ctx = NULL;
        camera_prv_bufferinfo *pElem = NULL;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);
        assert(buf);

        pElem = impp_fifo_dequeue(&ctx->buffer_fifo, IMPP_NO_WAIT);  // nowait
        if (NULL == pElem) {
                IMPP_LOGE(LOGTAG, "camera buffer fifo dequeue failed");
                return -IHAL_RERR;
        }

        buf->fd = pElem->fd;
        buf->vaddr = pElem->bufferaddr;
        buf->paddr = pElem->buffer_paddr;
        buf->index = pElem->index;
        buf->size = ctx->bufferinfo[buf->index].buffersize;
#if 0
        switch (ctx->imageFmt) {
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
                buf->size = ctx->bufferinfo[buf->index].buffersize;
                break;
        default:
                buf->size = -1;
                break;
        }
#endif
        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraQueuebuffer(IHAL_CameraHandle_t *handle, IMPP_BufferInfo_t *buffer)
{
        int fd = -1;
        int index = -1;
        struct v4l2_buffer buf;
        int ret = 0;
        camera_prv_ctx *ctx = NULL;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);
        assert(buffer);
        index = buffer->index;
        assert(index >= 0);

        bzero(&buf, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        switch (ctx->buffertype) {
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
                IMPP_LOGE(LOGTAG, "not support buffer type");
                return -IHAL_RERR;
        }
        buf.index = index;
        if (ctx->buffertype == IMPP_EXT_DMABUFFER) {
                buf.m.fd = ctx->bufferinfo[index].fd;
        } else if (ctx->buffertype == IMPP_EXT_USERBUFFER) {
                buf.m.userptr = (unsigned long)ctx->bufferinfo[index].bufferaddr;
                buf.length = ctx->bufferinfo[index].buffersize;
        }

        ret = ioctl(fd, VIDIOC_QBUF, &buf);
        if (ret) {
                IMPP_LOGE(LOGTAG, "Fail to ioctl 'VIDIOC_QBUF'");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_CameraAntiflickerSet(IHAL_CameraHandle_t *handle,IHAL_INT32 freq,IHAL_CameraAntiflickerMode_t mode,IHAL_INT32 enable)
{
	int fd = -1;
        int ret = 0;
        camera_prv_ctx *ctx = NULL;
	struct v4l2_control ctrl;

        assert(handle);
        fd = handle->vfd;
        assert(fd > 0);
        ctx = handle->ctx;
        assert(ctx);
	ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
	ctrl.value = freq;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
	    IMPP_LOGE(LOGTAG,"Failed to set flicker\n");
	    return -IHAL_RERR;
	}
	return IHAL_ROK;
}


