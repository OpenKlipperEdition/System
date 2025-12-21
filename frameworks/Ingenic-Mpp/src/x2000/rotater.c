#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>

#include "rotater.h"
#include "log.h"
#include "impp.h"
#include "impp_fifo.h"

#define TAG             "ROTATER"
#define ROT_PATH_DEV    "/dev/video0"
#define MAX_BUF_NUM     3
#define MAX_ROT_CHAN    4

typedef struct {
        IHAL_UINT32 vaddr;
        IHAL_UINT32 paddr;
        IHAL_INT32  fd;
        IHAL_UINT32 size;
        IHAL_INT32 index;
} rot_buffer_t;


typedef struct {
        IHAL_INT32          rot_fd;
        rot_buffer_t        rot_src_buffer[MAX_BUF_NUM];
        rot_buffer_t        rot_dst_buffer[MAX_BUF_NUM];
        impp_fifo_t         dstEmptyFifo;
        impp_fifo_t         dstFullFifo;
        IHAL_INT32          cur_index;
        IHal_Rot_ChanAttr_t  rot_attr;
        pthread_mutex_t     lock;
} rot_ctx_t;


static int _buffer_export(int v4lfd, enum v4l2_buf_type bt, int index, int *dmafd)
{
        struct v4l2_exportbuffer expbuf;
        bzero(&expbuf, sizeof(expbuf));
        /* memset(&expbuf, 0, sizeof(expbuf)); */
        expbuf.type = bt;
        expbuf.index = index;
        if (ioctl(v4lfd, VIDIOC_EXPBUF, &expbuf) == -1) {
                perror("\r\ndma-buf export VIDIOC_EXPBUF\r\n");
                return -1;
        }
        *dmafd = expbuf.fd;

        return 0;
}

static int _rot_query_buffer(rot_ctx_t *ctx, int type, int idx, rot_buffer_t *rotbuf)
{
        assert(ctx);
        assert(rotbuf);
        int ret = 0;
        int fd = ctx->rot_fd;
        struct v4l2_buffer buf;

        bzero(&buf, sizeof(buf));
        buf.type = type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = idx;
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf)) {
                IMPP_LOGE(TAG, "query buffer failed");
                return -1;
        }

        rotbuf->size = buf.length;
        /* if(type == V4L2_BUF_TYPE_VIDEO_OUTPUT){ */
        rotbuf->vaddr = (unsigned int)mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                                           MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == rotbuf->vaddr) {
                perror("Fail to mmap");
                return -1;
        }
        printf("mmap addr = 0x%x ######\r\n", rotbuf->vaddr);
        /* } */


        ret = _buffer_export(fd, type, idx, &rotbuf->fd);
        if (ret) {
                IMPP_LOGE(TAG, "export dma-buf failed");
                return -1;
        }

        return 0;
}

// type : OUTPUT / CAPTURE
static int _rot_init_mmap(rot_ctx_t *ctx, int type, int num)
{
        assert(ctx);
        int ret = 0;
        int fd = ctx->rot_fd;
        struct v4l2_requestbuffers reqbuf;

        bzero(&reqbuf, sizeof(reqbuf));
        reqbuf.count = num;
        reqbuf.type = type;
        reqbuf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
        if (ret < 0) {
                IMPP_LOGE(TAG, "rot reqbuf-mmap failed");
                return -1;
        }

        if (num > reqbuf.count) {
                IMPP_LOGW(TAG, "%d buffers were requested, but %d buffers were actually successfully requested!", num, reqbuf.count);
                return reqbuf.count;
        }

        return num;
}

// type : OUTPUT / CAPTURE
static int _rot_init_dmabuf(rot_ctx_t *ctx, int type, int num)
{
        assert(ctx);
        int ret = 0;
        int fd = ctx->rot_fd;
        struct v4l2_requestbuffers reqbuf;

        bzero(&reqbuf, sizeof(reqbuf));
        reqbuf.count = num;
        reqbuf.type = type;
        reqbuf.memory = V4L2_MEMORY_DMABUF;

        ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
        if (ret < 0) {
                IMPP_LOGE(TAG, "rot reqbuf-dmabuf failed");
                return -1;
        }

        if (num > reqbuf.count) {
                IMPP_LOGW(TAG, "%d buffers were requested, but %d buffers were actually successfully requested!", num, reqbuf.count);
                return reqbuf.count;
        }

        return num;
}

static int _rot_init_userptr(rot_ctx_t *ctx, int type, int num)
{
        assert(ctx);
        int ret = 0;
        int fd = ctx->rot_fd;
        struct v4l2_requestbuffers reqbuf;

        bzero(&reqbuf, sizeof(reqbuf));
        reqbuf.count = num;
        reqbuf.type = type;
        reqbuf.memory = V4L2_MEMORY_USERPTR;

        ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
        if (ret < 0) {
                IMPP_LOGE(TAG, "rot reqbuf-userptr failed");
                return -1;
        }

        if (num > reqbuf.count) {
                IMPP_LOGW(TAG, "%d buffers were requested, but %d buffers were actually successfully requested!", num, reqbuf.count);
                return reqbuf.count;
        }

        return num;
}

static int _rot_init(rot_ctx_t *ctx)
{
        assert(ctx);
        int ret = 0;
        ret = open(ROT_PATH_DEV, O_RDWR);
        if (ret < 0) {
                return -1;
        }
        ctx->rot_fd = ret;

        return 0;
}

static int _Check_dstFmt(IHal_Rot_ChanAttr_t *attr)
{
        assert(attr);
        switch (attr->dstFmt) {
        case IMPP_PIX_FMT_BGRA_8888:
                return 0;
        case IMPP_PIX_FMT_RGB_565:// rgb565le
                return 0;
        case IMPP_PIX_FMT_RGB_555:// rgb555le
                return 0;
        case IMPP_PIX_FMT_YUYV:
                return 0;
        default :
                break;
        }
        return -1;
}

static int _Check_srcFmt(IHal_Rot_ChanAttr_t *attr)
{
        assert(attr);
        switch (attr->srcFmt) {
        case IMPP_PIX_FMT_BGRA_8888:
                return 0;
        case IMPP_PIX_FMT_RGB_565:// rgb565
                return 0;
        case IMPP_PIX_FMT_RGB_555:// rgb555
                return 0;
        case IMPP_PIX_FMT_ARGB_1555:
                return 0;
        case IMPP_PIX_FMT_BGR_888:
                return 0;
        case IMPP_PIX_FMT_YUYV:
                return 0;
        default :
                break;
        }
        return -1;
}

static int _impp_fmt_to_v4l2(IMPP_PIX_FMT fmt)
{
        switch (fmt) {
        case IMPP_PIX_FMT_YUYV:
                return V4L2_PIX_FMT_YUYV;
        case IMPP_PIX_FMT_BGRA_8888:
                return V4L2_PIX_FMT_ARGB32;
        case IMPP_PIX_FMT_BGR_888:
                return V4L2_PIX_FMT_XRGB32;
        case IMPP_PIX_FMT_RGB_565:
                return V4L2_PIX_FMT_RGB565;
        case IMPP_PIX_FMT_RGB_555:
                return V4L2_PIX_FMT_RGB555;
        case IMPP_PIX_FMT_ARGB_1555:
                return V4L2_PIX_FMT_ARGB555;
        default :
                return -1;
        }
}

static int _rot_set_fmt(rot_ctx_t *ctx, int type)
{
        assert(ctx);
        struct v4l2_format fmt;
        int fd = ctx->rot_fd;
        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = type;
        fmt.fmt.pix.width = ctx->rot_attr.sWidth;
        fmt.fmt.pix.height = ctx->rot_attr.sHeight;
        if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
                fmt.fmt.pix.pixelformat = _impp_fmt_to_v4l2(ctx->rot_attr.srcFmt);
        } else {
                fmt.fmt.pix.pixelformat = _impp_fmt_to_v4l2(ctx->rot_attr.dstFmt);
        }
        fmt.fmt.pix.field = V4L2_FIELD_NONE;

        if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt)) {
                perror("VIDIOC_S_FMT Fail to ioctl");
                return -1;
        }
        return 0;

}

static int _rot_stream_on(int fd)
{
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(fd, VIDIOC_STREAMON, &type)) {
                perror("rot'VIDIOC_STREAMON'");
                return -1;
        }
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if (-1 == ioctl(fd, VIDIOC_STREAMON, &type)) {
                perror(" rot 'VIDIOC_STREAMON'");
                return -1;
        }
        return 0;
}

static int _rot_stream_off(int fd)
{
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
                perror("rot 'VIDIOC_STREAMOFF'");
                return -1;
        }
        type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
                perror("rot 'VIDIOC_STREAMOFF'");
                return -1;
        }
        return 0;
}

static int _rot_qbuf(int fd, int type, int memtype, int idx, rot_buffer_t *rot_buf)
{
        assert(idx >= 0);
        assert(rot_buf);
        struct v4l2_buffer buf;
        bzero(&buf, sizeof(buf));
        buf.type = type;
        buf.memory = memtype;
        buf.index = idx;
        buf.bytesused = rot_buf->size;
        if (memtype == V4L2_MEMORY_DMABUF) {
                buf.m.fd = rot_buf->fd;
        } else if (memtype == V4L2_MEMORY_USERPTR) {
                buf.m.userptr = rot_buf->vaddr;
                buf.length = rot_buf->size;
        }
        if (-1 == ioctl(fd, VIDIOC_QBUF, &buf)) {
                perror("rot 'VIDIOC_QBUF'");
                return -1;
        }
        return 0;
}

static int _rot_dqbuf(int fd, int type, int memtype)
{
        struct v4l2_buffer buf;
        bzero(&buf, sizeof(buf));
        buf.type = type;
        buf.memory = memtype;
        if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) {
                perror("rot 'VIDIOC_DEQBUF'");
                return -1;
        }
        return buf.index;
}

static int _rot_process(rot_ctx_t *ctx, int process_type, int value, rot_buffer_t *src, rot_buffer_t *dst)
{
        assert(ctx);
        assert(src);
        assert(dst);
        fd_set fds;
        struct timeval tv;
        int r;
        struct v4l2_control v4l2_ctl;
        int src_mem_type = 0;
        int dst_mem_type = 0;

        switch (process_type) {
        case ROT_PROCESS_ROTATE :
                v4l2_ctl.id = V4L2_CID_ROTATE;
                break;
        case ROT_PROCESS_FLIP:
                v4l2_ctl.id = V4L2_CID_VFLIP;
                break;
        case ROT_PROCESS_MIRROR:
                v4l2_ctl.id = V4L2_CID_HFLIP;
                break;
        default:
                return -1;
        }

        v4l2_ctl.value = value;
        r = ioctl(ctx->rot_fd, VIDIOC_S_CTRL, &v4l2_ctl);
        if (r < 0) {
                IMPP_LOGE(TAG, "Set angle failed!\n");
                return -1;
        }

        if (ctx->rot_attr.srcBufType == IMPP_EXT_DMABUFFER) {
                src_mem_type = V4L2_MEMORY_DMABUF;
        } else if (ctx->rot_attr.srcBufType == IMPP_EXT_USERBUFFER) {
                src_mem_type = V4L2_MEMORY_USERPTR;
        } else {
                src_mem_type = V4L2_MEMORY_MMAP;
        }

        if (ctx->rot_attr.dstBufType == IMPP_EXT_DMABUFFER) {
                dst_mem_type = V4L2_MEMORY_DMABUF;
        } else if (ctx->rot_attr.dstBufType == IMPP_EXT_USERBUFFER) {
                dst_mem_type = V4L2_MEMORY_USERPTR;
        } else {
                dst_mem_type = V4L2_MEMORY_MMAP;
        }

        pthread_mutex_lock(&ctx->lock);
        r = _rot_qbuf(ctx->rot_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT, src_mem_type, src->index, src);
        if (r) {
                pthread_mutex_unlock(&ctx->lock);
                IMPP_LOGD(TAG, "rot q src buf failed");
                return -1;
        }
        r = _rot_qbuf(ctx->rot_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, dst_mem_type, dst->index, dst);
        if (r) {
                pthread_mutex_unlock(&ctx->lock);
                IMPP_LOGD(TAG, "rot q dst buf failed");
                return -1;
        }
        pthread_mutex_unlock(&ctx->lock);

        FD_ZERO(&fds);
        FD_SET(ctx->rot_fd, &fds);

        /*Timeout*/
        tv.tv_sec = 0;
        tv.tv_usec = 1500 * 1000;   // 1500ms
        r = select(ctx->rot_fd + 1, &fds, NULL, NULL, &tv);
        if (-1 == r) {
                IMPP_LOGE(TAG, "failed to select");
                return -1;
        }

        if (0 == r) {
                IMPP_LOGE(TAG, "time to select");
                return -2;
        }

        pthread_mutex_lock(&ctx->lock);
        r = _rot_dqbuf(ctx->rot_fd, V4L2_BUF_TYPE_VIDEO_OUTPUT, src_mem_type);
        if (r < 0) {
                pthread_mutex_unlock(&ctx->lock);
                IMPP_LOGD(TAG, "rot dq src buf failed");
                return -1;
        }

        r = _rot_dqbuf(ctx->rot_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, dst_mem_type);
        if (r < 0) {
                pthread_mutex_unlock(&ctx->lock);
                IMPP_LOGD(TAG, "rot dq dst buf failed");
                return -1;
        }
        pthread_mutex_unlock(&ctx->lock);

        v4l2_ctl.value = 0;
        r = ioctl(ctx->rot_fd, VIDIOC_S_CTRL, &v4l2_ctl);
        if (r < 0) {
                IMPP_LOGE(TAG, "Set angle failed!\n");
                return -1;
        }

        return 0;
}

IHal_Rot_Handle_t *IHal_Rot_CreateChan(IHal_Rot_ChanAttr_t *attr)
{
        int ret = 0;

        assert(attr);

        if (attr->sWidth < 4 || attr->sWidth > 2047 || attr->sHeight < 4 || attr->sHeight > 2047) {
                IMPP_LOGE(TAG, "X2000 supports frame size 4x4 ~ 2047x2047.");
                return IHAL_RNULL;
        }

        assert(attr->numSrcBuf > 0);
        assert(attr->numDstBuf > 0);
        if (MAX_BUF_NUM < attr->numSrcBuf) {
                IMPP_LOGW(TAG, "The maximum number of srcbuf is %d and will be forcibly changed to %d", MAX_BUF_NUM, MAX_BUF_NUM);
                attr->numSrcBuf = MAX_BUF_NUM;
        }
        if (MAX_BUF_NUM < attr->numDstBuf) {
                IMPP_LOGW(TAG, "The maximum number of dstbuf is %d and will be forcibly changed to %d", MAX_BUF_NUM, MAX_BUF_NUM);
                attr->numDstBuf = MAX_BUF_NUM;
        }

        ret = _Check_srcFmt(attr);
        if (ret) {
                IMPP_LOGE(TAG, "src fmt is not support");
                return IHAL_RNULL;
        }
        ret = _Check_dstFmt(attr);
        if (ret) {
                IMPP_LOGE(TAG, "dst fmt is not support");
                return IHAL_RNULL;
        }

        if ((IMPP_PIX_FMT_YUYV == attr->srcFmt && IMPP_PIX_FMT_YUYV != attr->dstFmt) || \
            (IMPP_PIX_FMT_YUYV != attr->srcFmt && IMPP_PIX_FMT_YUYV == attr->dstFmt)) {
                IMPP_LOGE(TAG, "X2000 does not support conversion between RGB and YUV formats.");
                return IHAL_RNULL;
        }

        rot_ctx_t *ctx = (rot_ctx_t *)malloc(sizeof(rot_ctx_t));
        if (!ctx) {
                IMPP_LOGE(TAG, "init rot ctx failed");
                return IHAL_RNULL;
        }
        memset(ctx, 0, sizeof(rot_ctx_t));
        memcpy(&ctx->rot_attr, attr, sizeof(IHal_Rot_ChanAttr_t));
        pthread_mutex_init(&ctx->lock, NULL);
        ret = _rot_init(ctx);
        if (ret) {
                IMPP_LOGE(TAG, "rot init failed");
                goto free_ctx;
        }
        ret = _rot_set_fmt(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT);
        if (ret) {
                IMPP_LOGE(TAG, "set rot fmt failed");
                goto close_rot;
        }
        ret = _rot_set_fmt(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
        if (ret) {
                IMPP_LOGE(TAG, "set rot fmt failed");
                goto close_rot;
        }

        if (attr->srcBufType == IMPP_INTERNAL_BUFFER) {
                ret = _rot_init_mmap(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT, attr->numSrcBuf);
                if (ret < 0) {
                        IMPP_LOGE(TAG, "init src internal buf failed");
                        goto close_rot;
                }
                attr->numSrcBuf = ret;
                ctx->rot_attr.numSrcBuf = ret;

                for (int i = 0; i < ctx->rot_attr.numSrcBuf; i++) {
                        ret = _rot_query_buffer(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT, i, &ctx->rot_src_buffer[i]);
                        if (ret) {
                                IMPP_LOGE(TAG, "query internal buf failed");
                                goto close_rot;
                        }
                        ctx->rot_src_buffer[i].index = i;
                }
        } else if (attr->srcBufType == IMPP_EXT_DMABUFFER) {
                ret = _rot_init_dmabuf(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT, attr->numSrcBuf);
                if (ret < 0) {
                        IMPP_LOGE(TAG, "init exit dma-buf failed");
                        goto close_rot;
                }
                attr->numSrcBuf = ret;
                ctx->rot_attr.numSrcBuf = ret;
        } else if (attr->srcBufType == IMPP_EXT_USERBUFFER) {
                // TODO USERPTR
                ret = _rot_init_userptr(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT, attr->numSrcBuf);
                if (ret < 0) {
                        IMPP_LOGE(TAG, "init exit userptr failed");
                        goto close_rot;
                }
                attr->numSrcBuf = ret;
                ctx->rot_attr.numSrcBuf = ret;
        } else {
                IMPP_LOGE(TAG, "not support buffer type");
                goto close_rot;
        }

        impp_fifo_init(&ctx->dstEmptyFifo, MAX_BUF_NUM);
        impp_fifo_init(&ctx->dstFullFifo, MAX_BUF_NUM);
        if (attr->dstBufType == IMPP_INTERNAL_BUFFER) {
                ret = _rot_init_mmap(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE, attr->numDstBuf);
                if (ret < 0) {
                        IMPP_LOGE(TAG, "init dst internal buf failed");
                        goto close_rot;
                }
                attr->numDstBuf = ret;
                ctx->rot_attr.numDstBuf = ret;

                for (int i = 0; i < ctx->rot_attr.numDstBuf; i++) {
                        ret = _rot_query_buffer(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE, i, &ctx->rot_dst_buffer[i]);
                        if (ret) {
                                IMPP_LOGE(TAG, "query internal dst buf failed");
                                goto close_rot;
                        }
                        ctx->rot_dst_buffer[i].index = i;
                        impp_fifo_queue(&ctx->dstEmptyFifo, &ctx->rot_dst_buffer[i], IMPP_NO_WAIT);
                }
        } else if (attr->dstBufType == IMPP_EXT_DMABUFFER) {
                ret = _rot_init_dmabuf(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE, attr->numDstBuf);
                if (ret < 0) {
                        IMPP_LOGE(TAG, "init exit dma-buf failed");
                        goto close_rot;
                }
                attr->numDstBuf = ret;
                ctx->rot_attr.numDstBuf = ret;
        } else if (attr->dstBufType == IMPP_EXT_USERBUFFER) {
                ret = _rot_init_userptr(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE, attr->numDstBuf);
                if (ret < 0) {
                        IMPP_LOGE(TAG, "init exit userptr failed");
                        goto close_rot;
                }
                attr->numDstBuf = ret;
                ctx->rot_attr.numDstBuf = ret;
        } else {
                IMPP_LOGE(TAG, "not support buffer type");
                goto close_rot;
        }

        _rot_stream_on(ctx->rot_fd);
        return (IHal_Rot_Handle_t *)ctx;
close_rot:
        close(ctx->rot_fd);
free_ctx:
        free(ctx);
        return IHAL_RNULL;
}

IHAL_INT32 IHal_Rot_DestroyChan(IHal_Rot_Handle_t *handle)
{
        int i = 0, ret = 0;
        assert(handle);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;
        ret = _rot_stream_off(ctx->rot_fd);
        if (ret) {
                IMPP_LOGE(TAG, "rot stream of failed");
                return -IHAL_RFAILED;
        }
        pthread_mutex_lock(&ctx->lock);
        if (ctx->rot_attr.srcBufType == IMPP_INTERNAL_BUFFER) {
                for (i = 0; i < ctx->rot_attr.numSrcBuf; i++) {
                        ret = munmap(ctx->rot_src_buffer[i].vaddr, ctx->rot_src_buffer[i].size);
                        if (-1 == ret) {
                                IMPP_LOGE(TAG, "Rotater src buffer munmap failed");
                                return -IHAL_RFAILED;
                        }

                        ret = close(ctx->rot_src_buffer[i].fd);
                        if (-1 == ret) {
                                IMPP_LOGE(TAG, "close dmafd failed");
                                return -IHAL_RFAILED;
                        }

                        ctx->rot_src_buffer[i].vaddr = 0;
                        ctx->rot_src_buffer[i].size = 0;
                        ctx->rot_src_buffer[i].fd = -1;
                }
        }

        if (ctx->rot_attr.dstBufType == IMPP_INTERNAL_BUFFER) {
                for (i = 0; i < ctx->rot_attr.numDstBuf; i++) {
                        ret = munmap(ctx->rot_dst_buffer[i].vaddr, ctx->rot_dst_buffer[i].size);
                        if (-1 == ret) {
                                IMPP_LOGE(TAG, "Rotater dst buffer munmap failed");
                                return -IHAL_RFAILED;
                        }

                        ret = close(ctx->rot_dst_buffer[i].fd);
                        if (-1 == ret) {
                                IMPP_LOGE(TAG, "close dmafd failed");
                                return -IHAL_RFAILED;
                        }

                        ctx->rot_dst_buffer[i].vaddr = 0;
                        ctx->rot_dst_buffer[i].size = 0;
                        ctx->rot_dst_buffer[i].fd = -1;
                }
        }
        pthread_mutex_unlock(&ctx->lock);

        close(ctx->rot_fd);

        pthread_mutex_destroy(&ctx->lock);

        free(ctx);

        return IHAL_ROK;
}

IHAL_INT32 IHal_Rot_SetExtSrcBuffer(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index)
{
        assert(handle);
        assert(share);
        assert(index >= 0);
        assert(share->size > 0);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;

        if (IMPP_EXT_DMABUFFER != ctx->rot_attr.srcBufType && IMPP_EXT_USERBUFFER != ctx->rot_attr.srcBufType) {
                IMPP_LOGE(TAG, "When Rotater channel is created, the srcbuf must use external dma-buf or userptr.");
                return -IHAL_RERR;
        }

        if (index >= ctx->rot_attr.numSrcBuf) {
                IMPP_LOGE(TAG, "Max num of src external buf is %d", ctx->rot_attr.numSrcBuf);
                return -IHAL_RERR;
        }

        if (ctx->rot_attr.srcBufType == IMPP_EXT_DMABUFFER) {
                if (share->fd <= 0 || share->size <= 0) {
                        IMPP_LOGE(TAG, "fd or size is error");
                        return -IHAL_RERR;
                }
        }

        if (ctx->rot_attr.srcBufType == IMPP_EXT_USERBUFFER) {
                if (share->vaddr <= 0 || share->size <= 0) {
                        IMPP_LOGE(TAG, "vaddr or size is error");
                        return -IHAL_RERR;
                }
        }

        rot_buffer_t *buf = &ctx->rot_src_buffer[index];
        pthread_mutex_lock(&ctx->lock);
        buf->fd = share->fd;
        buf->vaddr = share->vaddr;
        buf->size = share->size;
        buf->index = index;
        pthread_mutex_unlock(&ctx->lock);

        return 0;
}

IHAL_INT32 IHal_Rot_SetExtDstBuffer(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index)
{
        assert(handle);
        assert(share);
        assert(index >= 0);
        assert(share->size > 0);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;

        if (IMPP_EXT_DMABUFFER != ctx->rot_attr.dstBufType && IMPP_EXT_USERBUFFER != ctx->rot_attr.dstBufType) {
                IMPP_LOGE(TAG, "When Rotater channel is created, the dstbuf must use external dma-buf or userptr.");
                return -IHAL_RERR;
        }

        if (index >= ctx->rot_attr.numDstBuf) {
                IMPP_LOGE(TAG, "Max num of dst external buf is %d", ctx->rot_attr.numDstBuf);
                return -IHAL_RERR;
        }

        if (ctx->rot_attr.dstBufType == IMPP_EXT_DMABUFFER) {
                if (share->fd <= 0 || share->size <= 0) {
                        IMPP_LOGE(TAG, "fd or size is error");
                        return -IHAL_RERR;
                }
        }

        if (ctx->rot_attr.dstBufType == IMPP_EXT_USERBUFFER) {
                if (share->vaddr <= 0 || share->size <= 0) {
                        IMPP_LOGE(TAG, "vaddr or size is error");
                        return -IHAL_RERR;
                }
        }

        rot_buffer_t *buf = &ctx->rot_dst_buffer[index];
        pthread_mutex_lock(&ctx->lock);
        buf->fd = share->fd;
        buf->vaddr = share->vaddr;
        buf->size = share->size;
        buf->index = index;
        pthread_mutex_unlock(&ctx->lock);
        impp_fifo_queue(&ctx->dstEmptyFifo, buf, IMPP_NO_WAIT);
        return 0;

}

IHAL_INT32 IHal_Rot_SrcDmaBufGet(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index)
{
        assert(handle);
        assert(share);
        assert(index >= 0);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;

        if (IMPP_INTERNAL_BUFFER != ctx->rot_attr.srcBufType) {
                IMPP_LOGE(TAG, "When Rotater channel is created, the srcbuf must use internal Buffer.");
                return -IHAL_RERR;
        }

        if (index >= ctx->rot_attr.numSrcBuf) {
                IMPP_LOGE(TAG, "Max num of src internal buf is %d", ctx->rot_attr.numSrcBuf);
                return -IHAL_RERR;
        }

        rot_buffer_t *buf = &ctx->rot_src_buffer[index];
        pthread_mutex_lock(&ctx->lock);
        share->fd            = buf->fd;
        share->vaddr         = buf->vaddr;
        share->size          = buf->size;
        share->index         = buf->index;
        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_Rot_DstDmaBufGet(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index)
{
        assert(handle);
        assert(share);
        assert(index >= 0);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;

        if (IMPP_INTERNAL_BUFFER != ctx->rot_attr.dstBufType) {
                IMPP_LOGE(TAG, "When Rotater channel is created, the dstbuf must use internal Buffer.");
                return -IHAL_RERR;
        }

        if (index >= ctx->rot_attr.numDstBuf) {
                IMPP_LOGE(TAG, "Max num of dst internal buf is %d", ctx->rot_attr.numDstBuf);
                return -IHAL_RERR;
        }

        rot_buffer_t *buf = &ctx->rot_dst_buffer[index];
        pthread_mutex_lock(&ctx->lock);
        share->fd            = buf->fd;
        share->vaddr         = buf->vaddr;
        share->size          = buf->size;
        share->index         = buf->index;
        pthread_mutex_unlock(&ctx->lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_Rot_ProcessFrame(IHal_Rot_Handle_t *handle, IMPP_FrameInfo_t *frame, IHal_Rot_ProcessType_t process_type, IHAL_INT32 angle)
{
        int ret = 0;
        assert(handle);
        assert(frame);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;
        int index = frame->index;

        if (index < 0 || index >= ctx->rot_attr.numSrcBuf) {
                IMPP_LOGE(TAG, "frame index is error");
                return -IHAL_RERR;
        }

        if (ctx->rot_attr.srcBufType == IMPP_EXT_DMABUFFER) {
                if (frame->fd != ctx->rot_src_buffer[index].fd) {
                        IMPP_LOGE(TAG, "the dma buf : %d should set as src buf\r\n", frame->fd);
                        return -IHAL_RERR;
                }
        } else if (ctx->rot_attr.srcBufType == IMPP_EXT_USERBUFFER) {
                if (frame->vaddr != ctx->rot_src_buffer[index].vaddr) {
                        IMPP_LOGE(TAG, "the user buf : %#x should set as src buf\r\n", frame->vaddr);
                        return -IHAL_RERR;
                }
        }

        rot_buffer_t *dst = impp_fifo_dequeue(&ctx->dstEmptyFifo, IMPP_WAIT_FOREVER);
        switch (process_type) {
        case ROT_PROCESS_ROTATE:
                if (angle == ROT_ANGLE_0 || angle == ROT_ANGLE_90
                    || angle == ROT_ANGLE_180 || angle == ROT_ANGLE_270) {
                        ret = _rot_process(ctx, ROT_PROCESS_ROTATE, angle,
                                           &ctx->rot_src_buffer[index], dst);
                        if (ret) {
                                IMPP_LOGE(TAG, "rotate process error\r\n");
                                return -IHAL_RERR;
                        }
                } else {
                        IMPP_LOGE(TAG, "rotate angle error\r\n");
                        return -IHAL_RERR;
                }
                break;
        case ROT_PROCESS_FLIP:
                ret = _rot_process(ctx, ROT_PROCESS_FLIP, 1,
                                   &ctx->rot_src_buffer[index], dst);
                if (ret) {
                        IMPP_LOGE(TAG, "rotate process error\r\n");
                        return -IHAL_RERR;
                }
                break;
        case ROT_PROCESS_MIRROR:
                ret = _rot_process(ctx, ROT_PROCESS_MIRROR, 1,
                                   &ctx->rot_src_buffer[index], dst);
                if (ret) {
                        IMPP_LOGE(TAG, "rotate process error\r\n");
                        return -IHAL_RERR;
                }
                break;
        default :
                IMPP_LOGE(TAG, "rotate process type error\r\n");
                return -IHAL_RERR;
        }
        impp_fifo_queue(&ctx->dstFullFifo, dst, IMPP_WAIT_FOREVER);
        return IHAL_ROK;
}

IHAL_INT32 IHal_Rot_GetFrame(IHal_Rot_Handle_t *handle, IMPP_FrameInfo_t *dst)
{
        assert(handle);
        assert(dst);
        rot_ctx_t *ctx = (rot_ctx_t *)handle;

        rot_buffer_t *buf = impp_fifo_dequeue(&ctx->dstFullFifo, IMPP_WAIT_FOREVER);
        if (buf) {
                dst->vaddr = buf->vaddr;
                dst->size = buf->size;
                dst->fd = buf->fd;
                dst->index = buf->index;
        } else {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_Rot_ReleaseFrame(IHal_Rot_Handle_t *handle, IMPP_FrameInfo_t *dst)
{
        assert(handle);
        assert(dst);

        int ret = 0;
        rot_ctx_t *ctx = (rot_ctx_t *)handle;

        if (ctx->rot_attr.numDstBuf <= dst->index || dst->index < 0) {
                IMPP_LOGE(TAG, "dst frame index is wrong.");
                return -IHAL_RERR;
        }

        ret = impp_fifo_queue(&ctx->dstEmptyFifo, &ctx->rot_dst_buffer[dst->index], IMPP_NO_WAIT);
        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}


