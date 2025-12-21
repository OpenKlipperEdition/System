/**
 * @file ipu.c
 * @author
 * @brief
 * @version 0.1
 * @date 2022-03-8
 *
 * @copyright Copyright ingenic 2022
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

#include "ipu.h"
#include "jz_ipu_hal.h"
#include "log.h"
#include "impp_fifo.h"

#define TAG "IPU"

#define JZIPU_IOC_MAGIC  'I'
#define IOCTL_IPU_START             _IO(JZIPU_IOC_MAGIC, 106)
#define IOCTL_IPU_RES_PBUFF         _IO(JZIPU_IOC_MAGIC, 114)
#define IOCTL_IPU_GET_PBUFF         _IO(JZIPU_IOC_MAGIC, 115)
#define IOCTL_IPU_BUF_LOCK          _IO(JZIPU_IOC_MAGIC, 116)
#define IOCTL_IPU_BUF_UNLOCK        _IO(JZIPU_IOC_MAGIC, 117)
#define IOCTL_IPU_BUF_FLUSH_CACHE   _IO(JZIPU_IOC_MAGIC, 118)
#define IOCTL_IPU_GET_DMA_FD        _IO(JZIPU_IOC_MAGIC, 119)
#define IOCTL_IPU_GET_DMA_PHY       _IO(JZIPU_IOC_MAGIC, 120)
#define IOCTL_IPU_FLUSH_DMA_FD       _IO(JZIPU_IOC_MAGIC, 121)

#define PAGE_SIZE 4096
#define ALIGN(addr, size) (addr + size) & ~(PAGE_SIZE-1)
#define PAGE_ALIGN(addr)    ALIGN(addr, PAGE_SIZE)

// #define CSC_INT_MAX_BUF  3
#define CSC_MAX_BUF 3
#define OSD_MAX_BUF 3

#define IPU_PATH    "/dev/ipu"

struct ipu_dma_info {
        unsigned int fd;
        unsigned int size;
        unsigned int phy_addr;
};

typedef struct {
        IHAL_INT32 fd;
        IHAL_UINT32 paddr;
        IHAL_UINT32 vaddr;
        IHAL_UINT32 size;
        IHAL_INT32  index;
        //IHAL_UINT32 cpu_addr;
} ipu_buffer_t;


typedef struct {
        int fd;
        IHal_CSC_ChanAttr_t attr;
        ipu_buffer_t SrcBuf[CSC_MAX_BUF];
        ipu_buffer_t DstBuf[CSC_MAX_BUF];
        impp_fifo_t  dstEmptyFifo;
        impp_fifo_t  dstFullFifo;
        pthread_mutex_t lock;
} csc_ctx_t;

typedef struct {
        int fd;
        int numWaitFifo;
        int allocCnt;
        ipu_buffer_t DstBuf[OSD_MAX_BUF];
        impp_fifo_t outEmptyFifo;
        int run_sta;
        pthread_mutex_t lock;
        pthread_t       osd_process_tid;
} osd_ctx_t;

static int _check_srcfmt_isValid(IMPP_PIX_FMT fmt)
{
        switch (fmt) {
        case IMPP_PIX_FMT_RGBA_8888:
                break;
        case IMPP_PIX_FMT_RGBX_8888:
                break;
        case IMPP_PIX_FMT_BGRA_8888:
                break;
        case IMPP_PIX_FMT_BGRX_8888:
                break;
        case IMPP_PIX_FMT_ABGR_8888:
                break;
        case IMPP_PIX_FMT_ARGB_8888:
                break;
        case IMPP_PIX_FMT_NV12:
                break;
        case IMPP_PIX_FMT_NV21:
                break;
        case IMPP_PIX_FMT_RGBA_5551:
                break;
        case IMPP_PIX_FMT_BGRA_5551:
                break;
        default:
                return -1;
        }
        return 0;
}

static int _check_dstfmt_isValid(IMPP_PIX_FMT fmt)
{
        switch (fmt) {
        case IMPP_PIX_FMT_RGBA_8888:
                break;
        case IMPP_PIX_FMT_RGBX_8888:
                break;
        case IMPP_PIX_FMT_BGRA_8888:
                break;
        case IMPP_PIX_FMT_BGRX_8888:
                break;
        case IMPP_PIX_FMT_ABGR_8888:
                break;
        case IMPP_PIX_FMT_ARGB_8888:
                break;
#if 0
        case IMPP_PIX_FMT_RGBA_5551:
                break;          // not support
        case IMPP_PIX_FMT_BGRA_5551:
                break;          // not support
#endif
        case IMPP_PIX_FMT_NV12:
                break;
        case IMPP_PIX_FMT_NV21:
                break;
        case IMPP_PIX_FMT_HSV:
                break;
        default:
                return -1;
        }
        return 0;
}

static int _impp_fmt_to_ipu(IMPP_PIX_FMT fmt)
{
        switch (fmt) {
        case IMPP_PIX_FMT_RGBA_8888:
                return HAL_PIXEL_FORMAT_RGBA_8888;
        case IMPP_PIX_FMT_RGBX_8888:
                return HAL_PIXEL_FORMAT_RGBX_8888;
        case IMPP_PIX_FMT_BGRA_8888:
                return HAL_PIXEL_FORMAT_BGRA_8888;
        case IMPP_PIX_FMT_BGRX_8888:
                return HAL_PIXEL_FORMAT_BGRX_8888;
        case IMPP_PIX_FMT_ABGR_8888:
                return HAL_PIXEL_FORMAT_ABGR_8888;
        case IMPP_PIX_FMT_ARGB_8888:
                return HAL_PIXEL_FORMAT_ARGB_8888;
        case IMPP_PIX_FMT_RGB_888:
                return HAL_PIXEL_FORMAT_RGB_888;
        case IMPP_PIX_FMT_NV12:
                return HAL_PIXEL_FORMAT_NV12;
        case IMPP_PIX_FMT_NV21:
                return HAL_PIXEL_FORMAT_NV21;
        case IMPP_PIX_FMT_HSV:
                return HAL_PIXEL_FORMAT_HSV;
        case IMPP_PIX_FMT_RGBA_5551:
                return HAL_PIXEL_FORMAT_RGBA_5551;
        case IMPP_PIX_FMT_BGRA_5551:
                return HAL_PIXEL_FORMAT_BGRA_5551;

        default :
                return -1;
        }

}

static int _create_ipu_dmabuf(int devfd, ipu_buffer_t *buf, unsigned int size)
{
        int ret = 0;
        unsigned int vaddr;
        struct ipu_dma_info dma;

        assert(devfd >= 0);
        assert(buf);
        assert(size > 0);

        dma.size = size;
        ret = ioctl(devfd, IOCTL_IPU_GET_DMA_FD, &dma);
        if (ret < 0) {
                IMPP_LOGE(TAG, "ipu get dma fd failed");
                return -1;
        }

        ret = ioctl(devfd, IOCTL_IPU_GET_DMA_PHY, &dma);
        if (ret < 0) {
                IMPP_LOGE(TAG, "ipu get dma phy failed");
                close(dma.fd);
                return -1;
        }

        vaddr = (unsigned int)mmap(0, dma.size, PROT_READ | PROT_WRITE,
                                   MAP_SHARED, dma.fd, 0);

        if (vaddr == MAP_FAILED) {
                IMPP_LOGE(TAG, "get dma user space addr failed");
                close(dma.fd);
                return -2;
        }

        buf->fd = dma.fd;
        buf->size = dma.size;
        buf->paddr = dma.phy_addr;
        buf->vaddr = vaddr;

        return 0;
}

static unsigned int _ipu_import_dmabuf(int devfd, int dmafd, unsigned int size)
{
        struct ipu_dma_info dma;
        dma.size = size;
        dma.fd = dmafd;
        int ret = ioctl(devfd, IOCTL_IPU_GET_DMA_PHY, &dma);
        if (ret < 0) {
                IMPP_LOGE(TAG, "ipu import get dma phy failed");
                return 0;
        }

        return dma.phy_addr;
}

static int _free_ipu_dmabuf(ipu_buffer_t *buf)
{
        assert(buf);

        int ret = 0;

        ret = munmap((void *)buf->vaddr, buf->size);
        if (ret) {
                IMPP_LOGE(TAG, "free dma user space addr failed");
                return -IHAL_RERR;;
        }

        ret = close(buf->fd);
        if (ret) {
                IMPP_LOGE(TAG, "ipu close dmafd failed");
                return -IHAL_RERR;
        }

        buf->vaddr = 0;
        buf->size = 0;
        buf->fd = -1;

        return IHAL_ROK;
}

IHal_CSC_Handle_t *IHal_CSC_ChanCreate(IHal_CSC_ChanAttr_t *attr)
{
        int ret = 0;
        unsigned int bufsize = 0;

        assert(attr);

        if (IMPP_PIX_FMT_NV12 != attr->srcFmt) {
                IMPP_LOGE(TAG, "X2500 src fmt only support NV12.");
                return IHAL_RNULL;
        }

#if 0
        ret = _check_srcfmt_isValid(attr->srcFmt);
        if (ret) {
                IMPP_LOGE(TAG, "src fmt not support");
                return IHAL_RNULL;
        }
#endif

        ret = _check_dstfmt_isValid(attr->dstFmt);
        if (ret) {
                IMPP_LOGE(TAG, "dst fmt not support");
                return IHAL_RNULL;
        }

        csc_ctx_t *ctx = (csc_ctx_t *)malloc(sizeof(csc_ctx_t));
        if (NULL == ctx) {
                IMPP_LOGE(TAG, "init csc ctx failed");
                return IHAL_RNULL;
        }

        ret = open(IPU_PATH, O_RDWR);
        if (ret < 0) {
                IMPP_LOGE(TAG, "init ipu driver failed");
                goto free_ctx;
        }

        assert(attr->sWidth > 0 && attr->sHeight > 0);
        assert(attr->numSrcBuf > 0 && attr->numDstBuf > 0);

        if (CSC_MAX_BUF < attr->numSrcBuf) {
                IMPP_LOGW(TAG, "The maximum number of srcbuf is %d and will be forcibly changed to %d", CSC_MAX_BUF, CSC_MAX_BUF);
                attr->numSrcBuf = CSC_MAX_BUF;
        }

        if (CSC_MAX_BUF < attr->numDstBuf) {
                IMPP_LOGW(TAG, "The maximum number of dstbuf is %d and will be forcibly changed to %d", CSC_MAX_BUF, CSC_MAX_BUF);
                attr->numDstBuf = CSC_MAX_BUF;
        }

        ctx->fd = ret;
        unsigned int size = 0;
        if (!attr->isSrcUseExtDmaBuf) {
                if (attr->srcFmt == IMPP_PIX_FMT_NV21 || attr->srcFmt == IMPP_PIX_FMT_NV12) {
                        size = PAGE_ALIGN(attr->sWidth * attr->sHeight * 3 / 2);
                } else if (attr->srcFmt == IMPP_PIX_FMT_BGRA_5551 || attr->srcFmt == IMPP_PIX_FMT_RGBA_5551) {
                        size = PAGE_ALIGN(attr->sWidth * attr->sHeight * 2);
                } else {
                        size = PAGE_ALIGN(attr->sWidth * attr->sHeight * 4);
                }
                for (int i = 0; i < attr->numSrcBuf; i++) {
                        ret = _create_ipu_dmabuf(ctx->fd, &ctx->SrcBuf[i], size);
                        if (ret) {
                                break;
                        }
                        ctx->SrcBuf[i].index = i;
                }
                if (ret) {
                        goto close_ipu;
                }
        }

        impp_fifo_init(&ctx->dstEmptyFifo, CSC_MAX_BUF);
        impp_fifo_init(&ctx->dstFullFifo, CSC_MAX_BUF);
        if (!attr->isDstUseExtDmaBuf) {
                if (attr->dstFmt == IMPP_PIX_FMT_NV21 || attr->dstFmt == IMPP_PIX_FMT_NV12) {
                        size = PAGE_ALIGN(attr->sWidth * attr->sHeight * 3 / 2);
                } else if (attr->dstFmt == IMPP_PIX_FMT_BGRA_5551 || attr->dstFmt == IMPP_PIX_FMT_RGBA_5551) {
                        size = PAGE_ALIGN(attr->sWidth * attr->sHeight * 2);
                } else {
                        size = PAGE_ALIGN(attr->sWidth * attr->sHeight * 4);
                }
                for (int i = 0; i < attr->numDstBuf; i++) {
                        ret = _create_ipu_dmabuf(ctx->fd, &ctx->DstBuf[i], size);
                        if (ret) {
                                break;
                        }
                        ctx->DstBuf[i].index = i;
                        impp_fifo_queue(&ctx->dstEmptyFifo, &ctx->DstBuf[i], IMPP_NO_WAIT);
                }
                if (ret) {
                        goto close_ipu;
                }
        }
        memcpy(&ctx->attr, attr, sizeof(IHal_CSC_ChanAttr_t));
        pthread_mutex_init(&ctx->lock, NULL);
        return (IHal_CSC_Handle_t *)ctx;
close_ipu:
        impp_fifo_deinit(&ctx->dstEmptyFifo);
        impp_fifo_deinit(&ctx->dstFullFifo);
        close(ctx->fd);
free_ctx:
        free(ctx);
        return IHAL_RNULL;
}

IHAL_INT32 IHal_CSC_ImportDmaBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buffer)
{
        assert(handle);
        assert(buffer);

        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        int ret = 0;
        unsigned int paddr;
        if (buffer->fd <= 0 || buffer->size <= 0) {
                IMPP_LOGE(TAG, "buffer info error");
                return -IHAL_RERR;
        }
        pthread_mutex_lock(&ctx->lock);
        paddr = _ipu_import_dmabuf(ctx->fd, buffer->fd, buffer->size);
        if (paddr == 0) {
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }
        buffer->paddr = paddr;
        pthread_mutex_unlock(&ctx->lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_DestroyChan(IHal_CSC_Handle_t *handle)
{
        assert(handle);
        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        int i;
        pthread_mutex_lock(&ctx->lock);
        if (!ctx->attr.isSrcUseExtDmaBuf) {
                for (i = 0; i < ctx->attr.numSrcBuf; i++) {
                        _free_ipu_dmabuf(&ctx->SrcBuf[i]);
                }
        }
        if (!ctx->attr.isDstUseExtDmaBuf) {
                for (i = 0; i < ctx->attr.numDstBuf; i++) {
                        _free_ipu_dmabuf(&ctx->DstBuf[i]);
                }
        }
        impp_fifo_deinit(&ctx->dstEmptyFifo);
        impp_fifo_deinit(&ctx->dstFullFifo);
        pthread_mutex_unlock(&ctx->lock);

        pthread_mutex_destroy(&ctx->lock);

        close(ctx->fd);

        free(ctx);
        ctx = NULL;

        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_GetSrcBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index)
{
        assert(handle);
        assert(buf);
        assert(index >= 0);

        csc_ctx_t *ctx = (csc_ctx_t *)handle;

        if (ctx->attr.isSrcUseExtDmaBuf) {
                IMPP_LOGE(TAG, "When CSC channel is created, the srcbuf does not use external dma-buf.");
                return -IHAL_RERR;
        }

        if (index >= ctx->attr.numSrcBuf) {
                IMPP_LOGE(TAG, "Max num of src internal buf is %d", ctx->attr.numSrcBuf);
                return -IHAL_RERR;
        }

        ipu_buffer_t *ibuf = &ctx->SrcBuf[index];
        buf->fd = ibuf->fd;
        buf->paddr = ibuf->paddr;
        buf->vaddr = ibuf->vaddr;
        buf->size = ibuf->size;
        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_GetDstBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index)
{
        assert(handle);
        assert(buf);
        assert(index >= 0);

        csc_ctx_t *ctx = (csc_ctx_t *)handle;

        if (ctx->attr.isDstUseExtDmaBuf) {
                IMPP_LOGE(TAG, "When CSC channel is created, the dstbuf does not use external dma-buf.");
                return -IHAL_RERR;
        }

        if (index >= ctx->attr.numDstBuf) {
                IMPP_LOGE(TAG, "Max num of dst internal buf is %d", ctx->attr.numDstBuf);
                return -IHAL_RERR;
        }

        ipu_buffer_t *ibuf = &ctx->DstBuf[index];
        buf->fd    = ibuf->fd;
        buf->paddr = ibuf->paddr;
        buf->vaddr = ibuf->vaddr;
        buf->size  = ibuf->size;
        return IHAL_ROK;
}



IHAL_INT32 IHal_CSC_SetSrcExtBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index)
{
        assert(handle);
        assert(buf);
        assert(index >= 0);

        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        unsigned int paddr = 0;

        if (!ctx->attr.isSrcUseExtDmaBuf) {
                IMPP_LOGE(TAG, "When CSC channel is created, the srcbuf must use external dma-buf");
                return -IHAL_RERR;
        }

        if (index >= ctx->attr.numSrcBuf) {
                IMPP_LOGE(TAG, "Max num of src external buf is %d", ctx->attr.numSrcBuf);
                return -IHAL_RERR;
        }

        if (buf->size <= 0) {
                IMPP_LOGE(TAG, "buf size is 0");
                return -IHAL_RERR;
        }
        ipu_buffer_t *ibuf = &ctx->SrcBuf[index];
        ibuf->size = buf->size;
        ibuf->vaddr = buf->vaddr;
        ibuf->index = index;
        if (buf->fd > 0) {
                ibuf->fd = buf->fd;
                if (buf->paddr > 0) {
                        ibuf->paddr = buf->paddr;
                } else {
                        paddr = _ipu_import_dmabuf(ctx->fd, buf->fd, buf->size);
                        if (paddr > 0) {
                                ibuf->paddr = paddr;
                        }
                }
        } else if (buf->paddr > 0) {
                ibuf->paddr = buf->paddr;
        } else {
                IMPP_LOGE(TAG, "the buf only support dma-buf or valid paddr");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_SetDstExtBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index)
{
        assert(handle);
        assert(buf);
        assert(index >= 0);

        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        unsigned int paddr = -1;

        if (!ctx->attr.isDstUseExtDmaBuf) {
                IMPP_LOGE(TAG, "When CSC channel is created, the dstbuf must use external dma-buf");
                return -IHAL_RERR;
        }

        if (index >= ctx->attr.numDstBuf) {
                IMPP_LOGE(TAG, "Max num of dst external buf is %d", ctx->attr.numDstBuf);
                return -IHAL_RERR;
        }

        if (buf->size <= 0) {
                IMPP_LOGE(TAG, "buf size is error");
                return -IHAL_RERR;
        }

        ipu_buffer_t *ibuf = &ctx->DstBuf[index];
        ibuf->size = buf->size;
        ibuf->vaddr = buf->vaddr;
        ibuf->index = index;
        if (buf->fd > 0) {
                ibuf->fd = buf->fd;
                if (buf->paddr > 0) {
                        ibuf->paddr = buf->paddr;
                } else {
                        paddr = _ipu_import_dmabuf(ctx->fd, buf->fd, buf->size);
                        if (paddr > 0) {
                                ibuf->paddr = paddr;
                        }
                }
        } else if (buf->paddr > 0) {
                ibuf->paddr = buf->paddr;
        } else {
                IMPP_LOGE(TAG, "the buf only support dma-buf or valid paddr");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_FrameProcess(IHal_CSC_Handle_t *handle, IMPP_FrameInfo_t *frame)
{
        assert(handle);
        assert(frame);

        unsigned int srcpaddr = 0;
        ipu_buffer_t *dbuf;

        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        assert(frame->fmt == ctx->attr.srcFmt);

        if ((frame->paddr <= 0) && (frame->index < 0)) {
                IMPP_LOGE(TAG, "both paddr and index not valid");
                return -IHAL_RERR;
        }

        if (frame->paddr > 0) {
                srcpaddr = frame->paddr;
        } else {
                srcpaddr = ctx->SrcBuf[frame->index].paddr;
        }

        dbuf = impp_fifo_dequeue(&ctx->dstEmptyFifo, IMPP_WAIT_FOREVER);
        if (dbuf == NULL) {
                IMPP_LOGE(TAG, "get dst buf failed");
                return -IHAL_RERR;
        }
        pthread_mutex_lock(&ctx->lock);
        struct ipu_param cscp;
        cscp.cmd         = IPU_OSD_FLAGS_BG_BUF_V;
        cscp.bg_w        = ctx->attr.sWidth;
        cscp.bg_h        = ctx->attr.sHeight;
        cscp.bg_fmt      = _impp_fmt_to_ipu(frame->fmt);
        cscp.bg_buf_p    = (unsigned int)srcpaddr;
        cscp.out_fmt     = _impp_fmt_to_ipu(ctx->attr.dstFmt);
        cscp.osd_ch0_buf_p    = (unsigned int)dbuf->paddr;

        int ret = ioctl(ctx->fd, IOCTL_IPU_START, &cscp);
        if (ret < 0) {
                IMPP_LOGE(TAG, "csc process failed ####\r\n");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }
        pthread_mutex_unlock(&ctx->lock);
        impp_fifo_queue(&ctx->dstFullFifo, dbuf, IMPP_NO_WAIT);
        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_GetDstFrame(IHal_CSC_Handle_t *handle, IMPP_FrameInfo_t *frame, IHAL_INT32 wait)
{
        assert(handle);
        assert(frame);
        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        ipu_buffer_t *buf = impp_fifo_dequeue(&ctx->dstFullFifo, wait);
        unsigned int framesize = 0;

        if (ctx->attr.dstFmt == IMPP_PIX_FMT_NV21 || ctx->attr.dstFmt == IMPP_PIX_FMT_NV12) {
                framesize = ctx->attr.sWidth * ctx->attr.sHeight * 3 / 2;
        } else if (ctx->attr.dstFmt == IMPP_PIX_FMT_BGRA_5551 || ctx->attr.dstFmt == IMPP_PIX_FMT_RGBA_5551) {
                framesize = ctx->attr.sWidth * ctx->attr.sHeight * 2;
        } else {
                framesize = ctx->attr.sWidth * ctx->attr.sHeight * 4;
        }
        if (buf) {
                pthread_mutex_lock(&ctx->lock);
                frame->fd    = buf->fd;
                frame->paddr = buf->paddr;
                frame->vaddr = buf->vaddr;
                frame->size  = (buf->size > framesize) ? framesize : buf->size;
                frame->index = buf->index;
                frame->fmt    = ctx->attr.dstFmt;
                pthread_mutex_unlock(&ctx->lock);
        } else {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_ReleaseDstFrame(IHal_CSC_Handle_t *handle, IMPP_FrameInfo_t *frame)
{
        assert(handle);
        assert(frame);
        csc_ctx_t *ctx = (csc_ctx_t *)handle;

        if (ctx->attr.numDstBuf <= frame->index || frame->index < 0) {
                IMPP_LOGE(TAG, "frame index is wrong.");
                return -IHAL_RERR;
        }

        int ret = impp_fifo_queue(&ctx->dstEmptyFifo, &ctx->DstBuf[frame->index], IMPP_NO_WAIT);
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_CSC_FlushCache(IHal_CSC_Handle_t *handle, IPU_DmaBuf_SyncInfo_t *info)
{
        assert(handle);
        assert(info);

        if (DMA_SYNC_FOR_READ != info->sync_type && DMA_SYNC_FOR_WRITE != info->sync_type) {
                IMPP_LOGE(TAG, "IPU DMA sync type is error");
                return -IHAL_RERR;
        }

        csc_ctx_t *ctx = (csc_ctx_t *)handle;
        int ret = ioctl(ctx->fd, IOCTL_IPU_FLUSH_DMA_FD, info);
        if (ret) {
                IMPP_LOGE(TAG, "CSC cache sync failed");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHal_OSD_Handle_t *IHal_OSD_ChanCreate(IHal_OSD_ChanAttr_t *attr)
{
        assert(attr);

        osd_ctx_t *ctx = (osd_ctx_t *)malloc(sizeof(osd_ctx_t));
        if (NULL == ctx) {
                return IHAL_RNULL;
        }
        memset(ctx, 0, sizeof(osd_ctx_t));
        if (attr->maxFifoNum <= 0) {
                IMPP_LOGE(TAG, "the maxFifoNum is error");
                goto free_ctx;
        }

        ctx->numWaitFifo = attr->maxFifoNum;

        ctx->fd = open(IPU_PATH, O_RDWR);
        if (ctx->fd < 0) {
                IMPP_LOGE(TAG, "init osd module failed");
                goto free_ctx;
        }

        pthread_mutex_init(&ctx->lock, NULL);

        return (IHal_OSD_Handle_t *)ctx;
free_ctx:
        free(ctx);
        return IHAL_RNULL;
}

IHAL_INT32 IHal_OSD_DestroyChan(IHal_OSD_Handle_t *handle)
{
        assert(handle);
        osd_ctx_t *ctx = (osd_ctx_t *)handle;
        if (ctx->allocCnt > 0) {
                IMPP_LOGE(TAG, "some buffer not been free ,please check!!!!!!");
                return -IHAL_RERR;
        }
        pthread_mutex_destroy(&ctx->lock);
        close(ctx->fd);
        free(ctx);
        ctx = NULL;
        return IHAL_ROK;
}

IHAL_INT32 IHal_OSD_CHx_BufCreate(IHal_OSD_Handle_t *handle, IMPP_BufferInfo_t *buffer, IHAL_UINT32 size)
{
        assert(handle);
        assert(buffer);
        assert(size > 0);

        osd_ctx_t *ctx = (osd_ctx_t *)handle;
        int ret = 0;
        unsigned int align_size = PAGE_ALIGN(size);
        ipu_buffer_t buf;
        pthread_mutex_lock(&ctx->lock);
        ret = _create_ipu_dmabuf(ctx->fd, &buf, align_size);
        if (ret) {
                IMPP_LOGE(TAG, "create osd ch buffer failed");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }

        buffer->fd = buf.fd;
        buffer->paddr = buf.paddr;
        buffer->vaddr = buf.vaddr;
        buffer->size = align_size;
        ctx->allocCnt += 1;
        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_OSD_CHx_BufFree(IHal_OSD_Handle_t *handle, IMPP_BufferInfo_t *buffer)
{
        assert(handle);
        assert(buffer);
        osd_ctx_t *ctx = (osd_ctx_t *)handle;
        int ret = 0;
        ipu_buffer_t buf;
        buf.fd = buffer->fd;
        buf.vaddr = buffer->vaddr;
        buf.size = buffer->size;

        ret = _free_ipu_dmabuf(&buf);
        if (ret) {
                IMPP_LOGE(TAG, "free ipu dmabuf failed");
                return -IHAL_RERR;
        }

        pthread_mutex_lock(&ctx->lock);
        ctx->allocCnt -= 1;
        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

// if  buffer'paddr not valid, call this api ,then get buffer's paddr
IHAL_INT32 IHal_OSD_ImportDmaBuf(IHal_OSD_Handle_t *handle, IMPP_BufferInfo_t *buffer)
{
        assert(handle);
        assert(buffer);

        osd_ctx_t *ctx = (osd_ctx_t *)handle;
        int ret = 0;
        unsigned int paddr;
        if (buffer->fd <= 0 || buffer->size <= 0) {
                IMPP_LOGE(TAG, "buffer info error");
                return -IHAL_RERR;
        }
        pthread_mutex_lock(&ctx->lock);
        paddr = _ipu_import_dmabuf(ctx->fd, buffer->fd, buffer->size);
        if (paddr == 0) {
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }
        buffer->paddr = paddr;
        pthread_mutex_unlock(&ctx->lock);

        return IHAL_ROK;
}

#define IPU_OSD_CHX_PARA_OSDM_SET(p, x) ((p)=(((p)&(~(0xf<<19)))|((x&0xf)<<19)))
#define IPU_OSD_CHX_PARA_ALPHA_SET(p, x) ((p)=(((p)&(~(0xff<<3)))|((x&0xff)<<3)))
#define IPU_OSD_CHX_PARA_CSCCTL_SET(p, x) ((p)=(((p)&(~(0x3<<24)))|((x&0x3)<<24)))
#define IPU_OSD_CHX_PARA_PICTYPE_SET(p, x) ((p)=(((p)&(~(0x7<<11)))|((x&0x7)<<11)))
#define IPU_OSD_CHX_PARA_MASK_SET(p, x) ((p)=(((p)&(~(0x1<<23)))|((x&0x1)<<23)))
#define IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, x) ((p)=(((p)&(~(0xf<<14)))|((x&0xf)<<14)))

static int _set_osd_chx_para(unsigned int *para, int fmt, int bgfmt, int alpha)
{
        assert(para);

        int ret = 0;
        int bg_fmt_yuv = 0;

        if ((bgfmt == IMPP_PIX_FMT_NV12) || (bgfmt == IMPP_PIX_FMT_NV21)) {
                bg_fmt_yuv = 1;
        } else {
                bg_fmt_yuv = 0;
        }
        unsigned int p = 0x02347f9;
        if ((alpha & (0x1 << 8)) > 0) {  //disable gAlphaEn use pixel alpha
                p = 0x020347f9;
                alpha = alpha & 0xff;
        } else {                         //enable gAlphaEn use pixsel alpha * (global alpha/256)
                if ((fmt == IMPP_PIX_FMT_NV12) || (fmt == IMPP_PIX_FMT_NV21)) {
                        p = 0x020347fb;
                } else {
                        p = 0x020347fd;
                }
        }
        ret = 0;
        switch (fmt) {
        case IMPP_PIX_FMT_RGBA_8888:
        case IMPP_PIX_FMT_RGBX_8888:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
                IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 8);
                break;
        case IMPP_PIX_FMT_BGRA_8888:
        case IMPP_PIX_FMT_BGRX_8888:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
                IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0x0d);
                break;
        case IMPP_PIX_FMT_ABGR_8888:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
                IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 5);
                break;
        case IMPP_PIX_FMT_ARGB_8888:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
                IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0);
                break;
        case IMPP_PIX_FMT_BGRA_5551:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 4);
                IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0xd);
                break;
        case IMPP_PIX_FMT_RGBA_5551:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 4);
                IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0x8);
                break;
        case IMPP_PIX_FMT_NV12:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 1);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 2);
                break;
        case IMPP_PIX_FMT_NV21:
                if (bg_fmt_yuv) {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 0);
                } else {
                        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 1);
                }
                IPU_OSD_CHX_PARA_PICTYPE_SET(p, 3);
                break;
        default:
                IMPP_LOGE(TAG, "fmt error");
                ret = -1;
                break;
        }
        IPU_OSD_CHX_PARA_ALPHA_SET(p, alpha);
        *para = p;
        return ret;
}


// sample param pix fmt same as bk fmt
static int _set_osd_chx_mask(unsigned int *para, int alpha)
{
        //int ret = 0;
        unsigned int p = 0;
        if ((alpha & (0x1 << 8)) > 0) {  //disable gAlphaEn use pixel alpha
                p = 0x020347f9;
                alpha = alpha & 0xff;
        } else {                         //enable gAlphaEn use pixsel alpha * (global alpha/256)
                p = 0x020347fd;
        }
        IPU_OSD_CHX_PARA_CSCCTL_SET(p, 2);
        IPU_OSD_CHX_PARA_PICTYPE_SET(p, 0);
        IPU_OSD_CHX_PARA_ALPHA_SET(p, alpha);
        IPU_OSD_CHX_PARA_MASK_SET(p, 1);
        IPU_OSD_CHX_PARA_ARGBTYPE_SET(p, 0);

        return 0;
}

IHAL_INT32 IHal_OSD_ProcessFrame(IHal_OSD_Handle_t *handle, IHal_OSD_FrameDesc_t *framedesc, IMPP_FrameInfo_t *out)
{
        assert(handle);
        assert(framedesc);
        assert(out);

        osd_ctx_t *ctx = (osd_ctx_t *)handle;
        struct ipu_param frmdsc;
        int ret = 0;
        unsigned int chxparam = 0;
        IHal_OSD_CH_Attr_t *chattr = NULL;


        if (framedesc->bgAttr.fmt != IMPP_PIX_FMT_NV12) {
                IMPP_LOGE(TAG, "the osd bg fmt must set as NV12");
                return -IHAL_RERR;
        }
        pthread_mutex_lock(&ctx->lock);
        frmdsc.cmd = 0xf & framedesc->osd_flags;
        frmdsc.bg_w = framedesc->bgAttr.width;
        frmdsc.bg_h = framedesc->bgAttr.height;
        frmdsc.bg_fmt = _impp_fmt_to_ipu(framedesc->bgAttr.fmt);
        frmdsc.bg_buf_p = framedesc->bgAttr.paddr;

        frmdsc.out_fmt = _impp_fmt_to_ipu(framedesc->bgAttr.fmt);

        if (framedesc->osd_flags & OSD_CH0_EN) {
                chattr = &framedesc->chAttr[0];
                frmdsc.osd_ch0_fmt = _impp_fmt_to_ipu(chattr->fmt);
                ret = _set_osd_chx_para(&chxparam, chattr->fmt, framedesc->bgAttr.fmt, chattr->alpha);
                if (ret) {
                        IMPP_LOGE(TAG, "set osd ch0 param error");
                        goto param_err;
                }
                frmdsc.osd_ch0_para = chxparam;
                frmdsc.osd_ch0_pos_x = chattr->posX;
                frmdsc.osd_ch0_pos_y = chattr->posY;
                frmdsc.osd_ch0_src_w = chattr->width;
                frmdsc.osd_ch0_src_h = chattr->height;
                frmdsc.osd_ch0_buf_p = chattr->paddr;
        }

        if (framedesc->osd_flags & OSD_CH1_EN) {
                chattr = &framedesc->chAttr[1];
                frmdsc.osd_ch1_fmt = _impp_fmt_to_ipu(chattr->fmt);
                ret = _set_osd_chx_para(&chxparam, chattr->fmt, framedesc->bgAttr.fmt, chattr->alpha);
                if (ret) {
                        IMPP_LOGE(TAG, "set osd ch1 param error");
                        goto param_err;
                }
                frmdsc.osd_ch1_para = chxparam;
                frmdsc.osd_ch1_pos_x = chattr->posX;
                frmdsc.osd_ch1_pos_y = chattr->posY;
                frmdsc.osd_ch1_src_w = chattr->width;
                frmdsc.osd_ch1_src_h = chattr->height;
                frmdsc.osd_ch1_buf_p = chattr->paddr;
        }

        if (framedesc->osd_flags & OSD_CH2_EN) {
                chattr = &framedesc->chAttr[2];
                frmdsc.osd_ch2_fmt = _impp_fmt_to_ipu(chattr->fmt);
                ret = _set_osd_chx_para(&chxparam, chattr->fmt, framedesc->bgAttr.fmt, chattr->alpha);
                if (ret) {
                        IMPP_LOGE(TAG, "set osd ch2 param error");
                        goto param_err;
                }
                frmdsc.osd_ch2_para = chxparam;
                frmdsc.osd_ch2_pos_x = chattr->posX;
                frmdsc.osd_ch2_pos_y = chattr->posY;
                frmdsc.osd_ch2_src_w = chattr->width;
                frmdsc.osd_ch2_src_h = chattr->height;
                frmdsc.osd_ch2_buf_p = chattr->paddr;
        }

        if (framedesc->osd_flags & OSD_CH3_EN) {
                chattr = &framedesc->chAttr[3];
                frmdsc.osd_ch3_fmt = _impp_fmt_to_ipu(chattr->fmt);
                ret = _set_osd_chx_para(&chxparam, chattr->fmt, framedesc->bgAttr.fmt, chattr->alpha);
                if (ret) {
                        IMPP_LOGE(TAG, "set osd ch0 param error");
                        goto param_err;
                }
                frmdsc.osd_ch3_para = chxparam;
                frmdsc.osd_ch3_pos_x = chattr->posX;
                frmdsc.osd_ch3_pos_y = chattr->posY;
                frmdsc.osd_ch3_src_w = chattr->width;
                frmdsc.osd_ch3_src_h = chattr->height;
                frmdsc.osd_ch3_buf_p = chattr->paddr;
        }

        ret = ioctl(ctx->fd, IOCTL_IPU_START, &frmdsc);
        if (ret) {
                IMPP_LOGE(TAG, "OSD Process failed");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }
        out->fd = framedesc->bgAttr.fd;
        out->paddr = framedesc->bgAttr.paddr;
        out->vaddr = framedesc->bgAttr.vaddr;
        if (framedesc->bgAttr.fmt == IMPP_PIX_FMT_NV12 || framedesc->bgAttr.fmt == IMPP_PIX_FMT_NV21) {
                out->size = framedesc->bgAttr.width * framedesc->bgAttr.height * 3 / 2;
        } else if (framedesc->bgAttr.fmt == IMPP_PIX_FMT_BGRA_5551 || framedesc->bgAttr.fmt == IMPP_PIX_FMT_RGBA_5551) {
                out->size = framedesc->bgAttr.width * framedesc->bgAttr.height * 2;
        } else {
                out->size = framedesc->bgAttr.width * framedesc->bgAttr.height * 4;
        }
        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
param_err:
        pthread_mutex_unlock(&ctx->lock);
        return -IHAL_RERR;
}


IHAL_INT32 IHal_OSD_FlushCache(IHal_OSD_Handle_t *handle, IPU_DmaBuf_SyncInfo_t *info)
{
        assert(handle);
        assert(info);

        if (DMA_SYNC_FOR_READ != info->sync_type && DMA_SYNC_FOR_WRITE != info->sync_type) {
                IMPP_LOGE(TAG, "IPU DMA sync type is error");
                return -IHAL_RERR;
        }

        osd_ctx_t *ctx = (osd_ctx_t *)handle;
        int ret = ioctl(ctx->fd, IOCTL_IPU_FLUSH_DMA_FD, info);
        if (ret) {
                IMPP_LOGE(TAG, "OSD cache sync failed");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}
