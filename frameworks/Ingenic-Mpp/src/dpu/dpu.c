/**
 * @file dpu.c
 * @author
 * @brief
 * @version 0.1
 * @date 2022-06-14
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
#include <linux/fb.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/types.h>
#include <semaphore.h>

#include "dpu.h"
#include "log.h"
#include "impp_fifo.h"
#include "uapi_ingenicfb.h"


#define TAG "DPU"

#define RDMA_FB_PATH    "/dev/fb0"

struct fb_info {
        int fd;
        //char dev_name[32];
        unsigned int xoff;
        unsigned int yoff;
        unsigned int smem_len;
        unsigned int single_len;
        unsigned int fb_mem;
        int num_buf;
        unsigned long long cur_frame;
        struct fb_var_screeninfo var_info;
        struct fb_fix_screeninfo fix_info;
};

typedef struct {
        struct fb_info rdmafb;
        struct fb_info layerfb[4];
        IMPP_BufferInfo_t outmem[4];
        int layer_open_status;
        impp_fifo_t outEmptyFifo;
        impp_fifo_t outFullFifo;
        unsigned int outheight;
        unsigned int outwidth;
        pthread_mutex_t lock;
        unsigned int tlb_base;
} dpu_ctx_t;

// 0-->fb0 rdma
// 1-->fb1 layer0
// 2-->fb2 layer1
// 3-->fb3 layer2
// 4-->fb4 layer3
static int _init_dpufb(dpu_ctx_t *ctx, int index)
{
        assert(ctx);
        char fbid[12];
        char fbnode[12];
        struct fb_info *info = NULL;
        int ret = 0;
        int fd = 0;
        struct fb_var_screeninfo var_info;
        struct fb_fix_screeninfo fix_info;

        if (index == 0) {
                info = &ctx->rdmafb;
                sprintf(fbid, "ingenicfb");
                sprintf(fbnode, "/dev/fb0");
        } else {
                sprintf(fbid, "layer%d", (index - 1));
                sprintf(fbnode, "/dev/fb%d", index);
                info = &ctx->layerfb[index - 1];
        }

        info->fd = -1;
        fd = open(fbnode, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open fb node (%s) failed", fbnode);
                return -1;
        }

        ret = ioctl(fd, FBIOGET_FSCREENINFO, &fix_info);
        if (ret < 0) {
                IMPP_LOGE(TAG, "get screen fix info failed");
                goto close_fb;
        }

        // check fbid
        if (strncmp(fix_info.id, fbid, strlen(fbid))) {
                IMPP_LOGE(TAG, "/dev/fb0 is not rdma ,please check rdma-status in dts");
                goto close_fb;
        }
        IMPP_LOGD(TAG, "fb id = %s ######## \r\n", fix_info.id);
        memcpy(&info->fix_info, &fix_info, sizeof(struct fb_fix_screeninfo));

        ret = ioctl(fd, FBIOGET_VSCREENINFO, &var_info);
        if (ret < 0) {
                info = &ctx->rdmafb;
                IMPP_LOGE(TAG, "get screen var info failed");
                goto close_fb;
        }
        var_info.activate |= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;
        memcpy(&info->var_info, &var_info, sizeof(struct fb_var_screeninfo));

        info->smem_len = fix_info.smem_len;
        info->num_buf  = var_info.yres_virtual / var_info.yres;
        info->single_len = fix_info.line_length * var_info.yres;
        info->fb_mem = (unsigned int)mmap(0, fix_info.smem_len,
                                          0x01 | 0x02, MAP_SHARED, fd, 0);
        if (info->fb_mem == MAP_FAILED) {
                IMPP_LOGE(TAG, "mmap fb mem failed");
                goto close_fb;
        }
        IMPP_LOGD(TAG, "fb buf num %d single_len = %d #####\r\n", info->num_buf,info->single_len);
        info->fd = fd;

        return 0;
close_fb:
        close(fd);
        return -1;
}

static void _deinit_dpufb(dpu_ctx_t *ctx, int index)
{
        struct fb_info *info = NULL;
        if (index == 0) {
                info = &ctx->rdmafb;
				ioctl(info->fd,JZFB_DMMU_UNMAP_ALL,0);	// release dmmu
        } else {
                info = &ctx->layerfb[index - 1];
        }
        if (info->fd) {
                munmap(info->fb_mem, info->smem_len);
                close(info->fd);
                IMPP_LOGD(TAG, "%s close fb end \r\n", __func__);
        }
}

static int __imppfmt_to_layerfmt(IMPP_PIX_FMT fmt)
{
        int ret = 0;
        switch (fmt) {
        case IMPP_PIX_FMT_BGR_888:
                ret = LAYER_CFG_FORMAT_RGB888;
                break;
        case IMPP_PIX_FMT_BGRA_8888:
                ret = LAYER_CFG_FORMAT_ARGB8888;
                break;
        case IMPP_PIX_FMT_RGB_555:
                ret = LAYER_CFG_FORMAT_RGB555;
                break;
        case IMPP_PIX_FMT_ARGB_1555:
                ret = LAYER_CFG_FORMAT_ARGB1555;
                break;
        case IMPP_PIX_FMT_RGB_565:
                ret = LAYER_CFG_FORMAT_RGB565;
                break;
        case IMPP_PIX_FMT_YUYV:
                ret = LAYER_CFG_FORMAT_YUV422;
                break;
        case IMPP_PIX_FMT_NV12:
                ret = LAYER_CFG_FORMAT_NV12;
                break;
        case IMPP_PIX_FMT_NV21:
                ret = LAYER_CFG_FORMAT_NV21;
                break;
        default:
                IMPP_LOGD(TAG,  "Please check the layer format\n"
				"The following formats are supported:\n"
				"IMPP_PIX_FMT_BGR_888\n"
				"IMPP_PIX_FMT_BGRA_8888\n"
				"IMPP_PIX_FMT_RGB_555\n"
				"IMPP_PIX_FMT_ARGB_1555\n"
				"IMPP_PIX_FMT_RGB_565\n"
				"IMPP_PIX_FMT_YUYV\n"
				"IMPP_PIX_FMT_NV12\n"
				"IMPP_PIX_FMT_NV21\n"
				);
                ret = -1;
        }
        return ret;
}

static int __imppfmt_to_wbfmt(IMPP_PIX_FMT fmt)
{
        int ret = 0;
        switch (fmt) {
        case IMPP_PIX_FMT_BGR_888:
                ret = DC_WB_FORMAT_888;
                break;
        case IMPP_PIX_FMT_BGRA_8888:
                ret = DC_WB_FORMAT_8888;
                break;
        case IMPP_PIX_FMT_RGB_555:	// rgb555le
                ret = DC_WB_FORMAT_555;
                break;
        case IMPP_PIX_FMT_RGB_565:	// rgb565le
                ret = DC_WB_FORMAT_565;
                break;
        default:
                IMPP_LOGD(TAG,  "Please check the wback format\n"
				"The following formats are supported:\n"
				"IMPP_PIX_FMT_BGR_888\n"
				"IMPP_PIX_FMT_BGRA_8888\n"
				"IMPP_PIX_FMT_RGB_555\n"
				"IMPP_PIX_FMT_RGB_565\n"
				);
                ret = -1;
        }
        return ret;
}

static int __dpu_osd_fill_layerinfo(struct ingenicfb_lay_cfg *layer_cfg, IHal_DpuOSD_LayerAttr_t *attr)
{
        layer_cfg->lay_en = 1;
	if(attr->vaddr && attr->paddr == 0) {
		layer_cfg->tlb_en = 1;
	} else {
		layer_cfg->tlb_en = 0;
	}

        layer_cfg->lay_scale_en = 0;
        if (attr->scale_enable) {
                layer_cfg->lay_scale_en = 1;
                layer_cfg->scale_w   = attr->scaleWidth;
                layer_cfg->scale_h   = attr->scaleHeight;
        }
        layer_cfg->lay_z_order = attr->osd_order;
        layer_cfg->format = __imppfmt_to_layerfmt(attr->srcFmt);
        if (layer_cfg->format < 0) {
                return -1;
        }
        layer_cfg->color = attr->color_order;
        layer_cfg->g_alpha_en = 1;
        layer_cfg->g_alpha_val = attr->alpha;
        layer_cfg->source_w = attr->srcWidth;
        layer_cfg->source_h = attr->srcHeight;
        layer_cfg->src_crop_x = attr->srcCropx;
        layer_cfg->src_crop_y = attr->srcCropy;
        layer_cfg->src_crop_w = attr->srcCropw;
        layer_cfg->src_crop_h = attr->srcCroph;

        layer_cfg->stride   = attr->srcWidth;
        if (attr->srcFmt == IMPP_PIX_FMT_NV12 || attr->srcFmt == IMPP_PIX_FMT_NV21) {
                layer_cfg->uv_stride = attr->srcWidth;
        }
        layer_cfg->disp_pos_x  = attr->osd_posX;
        layer_cfg->disp_pos_y  = attr->osd_posY;
	unsigned int addr = !layer_cfg->tlb_en ? attr->paddr:attr->vaddr;
	layer_cfg->addr[0]     = addr;
        if (attr->srcFmt == IMPP_PIX_FMT_NV12 || attr->srcFmt == IMPP_PIX_FMT_NV21) {
                layer_cfg->uv_addr[0] = attr->uv_vaddr;
        }
        return 0;
}

static int __dpu_csc_fill_layerinfo(struct ingenicfb_lay_cfg *layer_cfg, IHal_DpuCSC_Attr_t *attr)
{

}

static int __dpu_csc_sacle_process(dpu_ctx_t *ctx, IHal_DpuCSC_FrameDesc_t *desc)
{

}

static int _dpu_osd_process(dpu_ctx_t *ctx, IHal_DpuOSD_FrameDesc_t *desc)
{
        struct ingenicfb_frm_cfg frm_cfg;
        int ret = 0;
        IMPP_BufferInfo_t *buf  = NULL;
        memset(&frm_cfg, 0, sizeof(struct ingenicfb_frm_cfg));
        // fill frm_cfg
        if (desc->osd_flags & Dpu_Layer0) {
                __dpu_osd_fill_layerinfo(&frm_cfg.lay_cfg[0], &desc->layer[0]);
                //IMPP_LOGD(TAG,"layer0 paddr = 0x%x !!!!!!!",frm_cfg.lay_cfg[0].addr[0]);
        }

        if (desc->osd_flags & Dpu_Layer1) {
                __dpu_osd_fill_layerinfo(&frm_cfg.lay_cfg[1], &desc->layer[1]);
                //IMPP_LOGD(TAG,"layer1 paddr = 0x%x !!!!!!!",frm_cfg.lay_cfg[1].addr[0]);
        }

        if (desc->osd_flags & Dpu_Layer2) {
                if (desc->layer[2].srcFmt != IMPP_PIX_FMT_NV12) {
                        __dpu_osd_fill_layerinfo(&frm_cfg.lay_cfg[2], &desc->layer[2]);
                } else {
                        IMPP_LOGD(TAG, "Only layer 0 and 1 support NV12 format!\r\n");
                        return -1;
                }
        }

        if (desc->osd_flags & Dpu_Layer3) {
                if (desc->layer[3].srcFmt != IMPP_PIX_FMT_NV12) {
                        __dpu_osd_fill_layerinfo(&frm_cfg.lay_cfg[3], &desc->layer[3]);
                } else {
                        IMPP_LOGD(TAG, "Only layer 0 and 1 support NV12 format!\r\n");
                        return -1;
                }
        }
        frm_cfg.width = desc->outWidth;
        frm_cfg.height = desc->outHeight;
        // TODO
        ctx->outheight = desc->outHeight;
        ctx->outwidth  = desc->outWidth;
        if (desc->wback_enable) {
                buf = impp_fifo_dequeue(&ctx->outEmptyFifo, IMPP_WAIT_FOREVER);
                frm_cfg.wback_info.en = 1;
                frm_cfg.wback_info.fmt = __imppfmt_to_wbfmt(desc->wback_fmt);
                frm_cfg.wback_info.addr = buf->paddr;
                //IMPP_LOGD(TAG,"wback paddr = 0x%x !!!!!!!",buf->paddr);
                frm_cfg.wback_info.stride = desc->outWidth;
                frm_cfg.wback_info.dither_en = 0;
        }
        ret = ioctl(ctx->rdmafb.fd, JZFB_PUT_FRM_CFG, &frm_cfg);
        if (ret) {
                impp_fifo_queue(&ctx->outEmptyFifo, buf, IMPP_NO_WAIT);
                IMPP_LOGE(TAG, "%s failed", __func__);
                return -1;
        }
        if (desc->wback_enable) {
                if (desc->wback_fmt == IMPP_PIX_FMT_ARGB_8888) {
                        buf->size = desc->outWidth * desc->outHeight * 4;
                }
                if (desc->wback_fmt == IMPP_PIX_FMT_RGB_888) {
                        buf->size = desc->outWidth * desc->outHeight * 4;
                }
                if (desc->wback_fmt == IMPP_PIX_FMT_RGB_565 || desc->wback_fmt == IMPP_PIX_FMT_RGB_555) {
                        buf->size = desc->outWidth * desc->outHeight * 2;
                }
                impp_fifo_queue(&ctx->outFullFifo, buf, IMPP_NO_WAIT);
        }

        return 0;
}

IHal_Dpu_Handle_t *IHal_Dpu_Init(IHal_Dpu_InitStruct_t *init)
{
        int ret = 0;
        dpu_ctx_t *ctx = (dpu_ctx_t *)malloc(sizeof(dpu_ctx_t));
        if (!ctx) {
                IMPP_LOGE(TAG, "init dpu ctx failed");
                return IHAL_RNULL;
        }
        ret = _init_dpufb(ctx, 0);
        if (ret) {
                free(ctx);
                return IHAL_RNULL;
        }
        pthread_mutex_init(&ctx->lock, NULL);
        impp_fifo_init(&ctx->outFullFifo, init->num_outmem);
        impp_fifo_init(&ctx->outEmptyFifo, init->num_outmem);
        return (IHal_Dpu_Handle_t *)ctx;
}

IHAL_INT32 IHal_Dpu_WaitForVsync(IHal_Dpu_Handle_t *handle, int *vsync)
{
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        pthread_mutex_lock(&ctx->lock);
        int ret = ioctl(ctx->rdmafb.fd, FBIO_WAITFORVSYNC, vsync);
        if (ret) {
                IMPP_LOGE(TAG, "wait for vsync failed");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }

        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_RDMA_GetFrame(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf)
{
        int ret = 0;
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        int buf_no = -1;
        int index = -1;
        struct fb_info *info = NULL;
        pthread_mutex_lock(&ctx->lock);
        info = &ctx->rdmafb;
        buf_no = info->cur_frame % info->num_buf;
        buf->vaddr = (IHAL_UINT32)(info->fb_mem + buf_no * info->single_len);
        buf->paddr = (IHAL_UINT32)(info->fix_info.smem_start + buf_no * info->single_len);
        buf->size = info->single_len;
        buf->index = buf_no;
        info->cur_frame++;
        pthread_mutex_unlock(&ctx->lock);

        return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_RDMA_Get_BufferNums(IHal_Dpu_Handle_t *handle)
{
        int ret = 0;
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        int buf_no = -1;
        int index = -1;
        struct fb_info *info = NULL;
        info = &ctx->rdmafb;

        return info->num_buf;
}

IHAL_INT32 IHal_Dpu_RDMA_PutFrame(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf)
{
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        ctx->rdmafb.var_info.yoffset = ctx->rdmafb.var_info.yres * buf->index;
        int ret = ioctl(ctx->rdmafb.fd, FBIOPAN_DISPLAY, &ctx->rdmafb.var_info);
        if (ret) {
                IMPP_LOGE(TAG, "update display framebuffer failed");
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_Composer_Set_WbackBuffers(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index)
{
        int ret = 0;
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        unsigned int arg;
        IMPP_BufferInfo_t *dpu_buf = &ctx->outmem[index];
        //TODO
        if (buf->paddr == 0 && buf->fd == 0) {
                return -IHAL_RERR;
        }
        if (buf->paddr == 0 && buf->fd > 2) {
                arg = buf->fd;
                ret = ioctl(ctx->rdmafb.fd, JZFB_GET_DMA_PHY, &arg);
                if (ret) {
                        IMPP_LOGE(TAG, "get dma buf paddr for wb failed");
                        return -IHAL_RERR;
                }
                buf->paddr = arg;
                //IMPP_LOGD(TAG,"dma paddr = 0x%x ########\r\n",arg);
        }
        memcpy(dpu_buf, buf, sizeof(IMPP_BufferInfo_t));
        dpu_buf->index = index;
        impp_fifo_queue(&ctx->outEmptyFifo, dpu_buf, IMPP_NO_WAIT);
        return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_ImportDmabuf(IHal_Dpu_Handle_t *handle, IMPP_BufferInfo_t *buf)
{
        // TODO
}

IHAL_INT32 IHal_Dpu_Composer_Process(IHal_Dpu_Handle_t *handle, IHal_Dpu_FrameDesc_t *frame)
{
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        int ret = 0;
        if (frame->dpu_func == DPU_OSD_Func) {
                ret = _dpu_osd_process(ctx, &frame->func_desc.osd_frame);
        } else {
                IMPP_LOGE(TAG, "CSC function not support yet");
                return -IHAL_RERR;
        }
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_Composer_GetFrame(IHal_Dpu_Handle_t *handle, IMPP_FrameInfo_t *frame)
{
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        IMPP_BufferInfo_t *buf = impp_fifo_dequeue(&ctx->outFullFifo, IMPP_WAIT_FOREVER);
        if (buf) {
                frame->width = ctx->outwidth;
                frame->height = ctx->outheight;
                frame->paddr = buf->paddr;
                frame->vaddr = buf->vaddr;
                frame->size = buf->size;
                frame->index = buf->index;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_Composer_ReleaseFrame(IHal_Dpu_Handle_t *handle, IMPP_FrameInfo_t *frame)
{
        assert(handle);
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        int ret = 0;
        ret = impp_fifo_queue(&ctx->outEmptyFifo, &ctx->outmem[frame->index], IMPP_NO_WAIT);
        return IHAL_ROK;
}


IHAL_INT32 IHal_Dpu_Deinit(IHal_Dpu_Handle_t *handle)
{
        dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
        pthread_mutex_destroy(&ctx->lock);
        impp_fifo_deinit(&ctx->outFullFifo);
        impp_fifo_deinit(&ctx->outEmptyFifo);
        // TODO deinit fb
        if (ctx->layer_open_status & Dpu_Layer0) {
                _deinit_dpufb(ctx, 1);
        }
        if (ctx->layer_open_status & Dpu_Layer1) {
                _deinit_dpufb(ctx, 2);
        }
        if (ctx->layer_open_status & Dpu_Layer2) {
                _deinit_dpufb(ctx, 3);
        }
        if (ctx->layer_open_status & Dpu_Layer3) {
                _deinit_dpufb(ctx, 4);
        }
        _deinit_dpufb(ctx, 0);

        free(ctx);
        return IHAL_ROK;
}


IHAL_INT32 IHal_Dpu_DmmuOps(IHal_Dpu_Handle_t *handle,IMPP_BufferInfo_t *buf,bool map)
{
    assert(handle);
    dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
	struct {
		unsigned int addr;
		unsigned int len;
	}map_info;
	map_info.addr = buf->vaddr;
	map_info.len = buf->size;
	int ret = 0;
	if(map)
		ret = ioctl(ctx->rdmafb.fd,JZFB_DMMU_MAP, &map_info);
	else
		ret = ioctl(ctx->rdmafb.fd,JZFB_DMMU_UNMAP, &map_info);
	if(ret)
		return -IHAL_RERR;
	return IHAL_ROK;
}

IHAL_INT32 IHal_Dpu_DmmuReleaseAll(IHal_Dpu_Handle_t *handle)
{
    assert(handle);
    dpu_ctx_t *ctx = (dpu_ctx_t *)handle;
	int ret = ioctl(ctx->rdmafb.fd,JZFB_DMMU_UNMAP_ALL,NULL);
	if(ret)
		return -IHAL_RERR;
	else
		return IHAL_ROK;
}
