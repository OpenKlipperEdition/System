#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>
#include "display.h"
#include "log.h"

#include "uapi_ingenicfb.h"

#define TAG "SampleFB"
#define DPU_FS_PATH "/sys/devices/platform/ahb0/13050000.dpu/"


struct fb_info {
        int fd;
        char dev_name[32];
        unsigned int xoff;
        unsigned int yoff;
        unsigned int smem_len;
        unsigned int single_len;
        unsigned int fb_mem;
        int num_buf;
        struct fb_var_screeninfo var_info;
        struct fb_fix_screeninfo fix_info;
	struct ingenicfb_var_screeninfo_extended var_info_ext;
};

typedef struct {
        struct fb_info fb;
        unsigned long long cur_frame;
        unsigned int is_fb_init;
        IHal_SampleFB_Attr attr;
        pthread_mutex_t lock;
} fb_ctx_t;

static int __devnode_to_layerIndex(char *devname)
{
        int ret = 0;
        if (!strncmp(devname, "/dev/fb0", strlen("/dev/fb0"))) {
                ret = 0;
        } else if (!strncmp(devname, "/dev/fb1", strlen("/dev/fb1"))) {
                ret = 1;
        } else if (!strncmp(devname, "/dev/fb2", strlen("/dev/fb2"))) {
                ret = 2;
        } else if (!strncmp(devname, "/dev/fb3", strlen("/dev/fb3"))) {
                ret = 3;
        } else {
                ret = -1;
        }
        return ret;
}

static int __imppfmt_to_layerfmt(IMPP_PIX_FMT fmt)
{
        int ret = 0;
        switch (fmt) {
        case IMPP_PIX_FMT_BGR_888:
                ret = 0;
                break;
        case IMPP_PIX_FMT_BGRA_8888:
                ret = 1;
                break;
        case IMPP_PIX_FMT_RGB_555:
                ret = 2;
                break;
        case IMPP_PIX_FMT_ARGB_1555:
                ret = 3;
                break;
        case IMPP_PIX_FMT_RGB_565:
                ret = 4;
                break;
        case IMPP_PIX_FMT_YUYV:
                ret = 5;
                break;
        case IMPP_PIX_FMT_NV12:
                ret = 6;
                break;
        case IMPP_PIX_FMT_NV21:
                ret = 7;
                break;
        default:
                ret = -1;
        }
        return ret;
}

static int __imppfmt_to_layerfmt_extend(IMPP_PIX_FMT fmt)
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
                ret = -1;
        }
        return ret;
}

static int _set_dpu_layerx_enable(char *devname, int enable)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/enable", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open enable node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", enable);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set enable error");
                return -2;
        }
        close(fd);
        return 0;
}

static int _set_dpu_layerx_fmt(char *devname, IMPP_PIX_FMT fmt)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        int layerfmt = __imppfmt_to_layerfmt(fmt);
        if (layerfmt < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/src_fmt", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open src_fmt node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", layerfmt);
        printf(" %s str = %s \r\n", __func__, str);
        ret = write(fd, str, 1);
        if (ret < 0) {
                IMPP_LOGE(TAG, "set fmt error ret = %d ", ret);
                return -2;
        }
        close(fd);
        return 0;

}

static int _set_dpu_layerx_frame_size(char *devname, int width, int height)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        //return 0;   ????
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/frame_size", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open frame_size node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%dx%d", width, height);
        printf(" %s str = %s len = %d \r\n", __func__, str, strlen(str));
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set frame size error ret = %d", ret);
                return -2;
        }
        close(fd);
        return 0;
}

static int _set_dpu_layerx_src_size(char *devname, int width, int height)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/src_size", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open src_size node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%dx%d", width, height);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set src size error");
                return -2;
        }
        close(fd);

        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/stride", DPU_FS_PATH, layerIdx);
        fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open src_size node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", width);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set src size error");
                return -2;
        }
        close(fd);

        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/uv_stride", DPU_FS_PATH, layerIdx);
        fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open src_size node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", width);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set src size error");
                return -2;
        }
        close(fd);
        return 0;
}

static int _set_dpu_layerx_stride_size(char *devname, int stride, int uv_stride)
{
        char str[128];
        int ret = 0;
        int fd  = -1;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }

        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/stride", DPU_FS_PATH, layerIdx);
        fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open stride node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", stride);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set stride error");
                return -2;
        }
        close(fd);

        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/uv_stride", DPU_FS_PATH, layerIdx);
        fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open uv_stride node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", uv_stride);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set uv_stride error");
                return -2;
        }
        close(fd);
        return 0;
}

static int _set_dpu_layerx_src_crop_size(char *devname, int crop_x, int crop_y, int crop_w, int crop_h)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/src_crop_size", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open src_crop_size node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%dx%d", crop_w, crop_h);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set src crop size error");
                return -2;
        }
        close(fd);

        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/src_crop_pos", DPU_FS_PATH, layerIdx);
        fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open src_crop_pos node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%dx%d", crop_x, crop_y);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set src crop pos error");
                return -2;
        }
        close(fd);

        return 0;
}

static int _set_dpu_layerx_global_alpha(char *devname, int alpha)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/global_alpha", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open global_alpha node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", alpha);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set global_alpha error");
                return -2;
        }
        close(fd);

        return 0;
}

static int _set_dpu_layerx_target_size(char *devname, int width, int height)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/target_size", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open target_size node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%dx%d", width, height);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set target size error");
                return -2;
        }
        close(fd);
        return 0;
}

static int _set_dpu_layerx_target_pos(char *devname, int x, int y)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/target_pos", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open target_pos node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%dx%d", x, y);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set target pos error");
                return -2;
        }
        close(fd);
        return 0;
}



static int _set_dpu_layerx_zorder(char *devname, int order)
{
        char str[128];
        int ret = 0;
        int layerIdx = __devnode_to_layerIndex(devname);
        if (layerIdx < 0) {
                return -1;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/layer%d/z_order", DPU_FS_PATH, layerIdx);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open z_order node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", order);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set z_order error");
                return -2;
        }
        close(fd);
        return 0;

}

static int _dpu_comp_update(void)
{
        char str[128];
        int ret = 0;
        memset(str, 0, sizeof(str));
        sprintf(str, "%s/comp_update", DPU_FS_PATH);
        int fd = open(str, O_RDWR);
        if (fd < 0) {
                IMPP_LOGE(TAG, "open z_order node failed");
                return -2;
        }
        memset(str, 0, sizeof(str));
        sprintf(str, "%d", 1);
        ret = write(fd, str, strlen(str));
        if (ret < 0) {
                IMPP_LOGE(TAG, "set z_order error");
                return -2;
        }
        close(fd);
        return 0;
}


static int _dpu_update_fb_info(fb_ctx_t *ctx)
{

        struct fb_var_screeninfo var_info;
        struct fb_fix_screeninfo fix_info;
        int ret = 0;
        int need_map = 0;

        ret = ioctl(ctx->fb.fd, FBIOGET_FSCREENINFO, &fix_info);
        if (ret) {
                IMPP_LOGE(TAG, "get fb fix info failed");
                return -1;
        }
        memcpy(&ctx->fb.fix_info, &fix_info, sizeof(struct fb_fix_screeninfo));
        ret = ioctl(ctx->fb.fd, FBIOGET_VSCREENINFO, &var_info);
        if (ret) {
                IMPP_LOGE(TAG, "get fb var info failed");
                return -1;
        }
        memcpy(&ctx->fb.var_info, &var_info, sizeof(struct fb_var_screeninfo));
        var_info.activate |= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;

        /* munmap old memory, remap buffers below. */
        if (ctx->fb.fb_mem != NULL && ctx->fb.smem_len != fix_info.smem_len) {
                need_map = 1;
                munmap(ctx->fb.fb_mem, ctx->fb.smem_len);
        } else {
                if (ctx->fb.fb_mem == NULL) {
                        need_map = 1;
                }
        }

        if (need_map) {
                ctx->fb.smem_len = fix_info.smem_len;
                ctx->fb.num_buf  = var_info.yres_virtual / var_info.yres;
                ctx->fb.single_len = fix_info.line_length * var_info.yres;

                ctx->fb.fb_mem = (unsigned int)mmap(0, fix_info.smem_len,
                                                    0x01 | 0x02, MAP_SHARED, ctx->fb.fd, 0);
                if (ctx->fb.fb_mem == MAP_FAILED) {
                        IMPP_LOGE(TAG, "mmap fb mem failed");
                        return -1;
                }
        }


        return 0;

}

int _init_framebuffer(fb_ctx_t *ctx)
{
        int ret = 0;
        IHal_SampleFB_Attr *attr = &ctx->attr;
        if (attr->mode == Composer_Mode) {
                ret = _set_dpu_layerx_fmt(attr->node, attr->frame_fmt);
                if (ret) {
                        IMPP_LOGE(TAG, "set layer%d fmt failed ", __devnode_to_layerIndex(attr->node));
                        return -1;
                }
                ret =  _set_dpu_layerx_src_size(ctx->fb.dev_name, attr->frame_width, attr->frame_height);
                if (ret) {
                        return -1;
                }
                ret =  _set_dpu_layerx_src_crop_size(ctx->fb.dev_name, attr->crop_x, attr->crop_y, attr->crop_w, attr->crop_h);
                if (ret) {
                        return -1;
                }

                ret =  _set_dpu_layerx_global_alpha(ctx->fb.dev_name, attr->alpha);
                if (ret) {
                        return -1;
                }

                ret =  _set_dpu_layerx_enable(ctx->fb.dev_name, 0);
                if (ret) {
                        return -1;
                }
                ret = _dpu_comp_update();
                if (ret) {
                        return -1;
                }

                ret = _dpu_update_fb_info(ctx);
                if (ret < 0) {
                        return ret;
                }
        } else if (attr->mode == RDma_Mode) {
        } else {
                IMPP_LOGE(TAG, "work mode error");
                return -1;
        }


        // NV12
        if (attr->mode == Composer_Mode && attr->frame_fmt == IMPP_PIX_FMT_NV12) {
                memset(ctx->fb.fb_mem, 0x80, ctx->fb.smem_len);
        }

        // enable 1
        ret =  _set_dpu_layerx_enable(ctx->fb.dev_name, 1);
        if (ret) {
                return -1;
        }
        // update
        ret = _dpu_comp_update();
        if (ret) {
                return -1;
        }

        // fill frame buffer black
	ctx->cur_frame = ctx->fb.var_info.yoffset / ctx->fb.var_info.yres;
        ctx->is_fb_init = 1;

        return 0;
}

IHal_SampleFB_Handle_t *IHal_SampleFB_Init(IHal_SampleFB_Attr *attr)
{
        int ret = 0;

        if (attr->mode != Composer_Mode && attr->mode != RDma_Mode) {
                IMPP_LOGE(TAG, "work mode error");
                return IHAL_RNULL;
        }

        fb_ctx_t *ctx = (fb_ctx_t *)calloc(1, sizeof(fb_ctx_t));
        if (!ctx) {
                IMPP_LOGE(TAG, "malloc run ctx failed");
                return IHAL_RNULL;
        }
        memcpy(&ctx->attr, attr, sizeof(IHal_SampleFB_Attr));
        memcpy(ctx->fb.dev_name, attr->node, strlen(attr->node));
        ctx->cur_frame = 0;
        ctx->is_fb_init = 0;
        pthread_mutex_init(&ctx->lock, NULL);

        ret = open(attr->node, O_RDWR);
        if (ret < 0) {
                IMPP_LOGE(TAG, "open dev node (%s) failed", attr->node);
                return -2;
        }
        ctx->fb.fd = ret;

#if 1
        ret = _init_framebuffer(ctx);
        if (ret) {
                close(ctx->fb.fd);
                return NULL;
        }
#endif

        return (IHal_SampleFB_Handle_t *)ctx;
}

IHal_SampleFB_Handle_t *IHal_SimpleFB_Init(IHal_SampleFB_LayerAttr_t *attr)
{
        int ret = 0;

        fb_ctx_t *ctx = (fb_ctx_t *)calloc(1, sizeof(fb_ctx_t));
        if (!ctx) {
                IMPP_LOGE(TAG, "malloc run ctx failed");
                return IHAL_RNULL;
        }
        memcpy(ctx->fb.dev_name, attr->node, strlen(attr->node));
        pthread_mutex_init(&ctx->lock, NULL);

        ret = open(attr->node, O_RDWR);
        if (ret < 0) {
                IMPP_LOGE(TAG, "open dev node (%s) failed", attr->node);
                return -2;
        }
	ctx->fb.fd = ret;

	if (attr->update_fbinfo) {
		ret = _set_dpu_layerx_fmt(attr->node, attr->srcFmt);
		if (ret) {
			return -1;
		}
		ret =  _set_dpu_layerx_src_size(ctx->fb.dev_name, attr->srcHeight, attr->srcWidth);
		if (ret) {
			return -1;
		}
		ret =  _set_dpu_layerx_enable(ctx->fb.dev_name, 0);
		if (ret) {
			return -1;
		}
		ret = _dpu_comp_update();
		if (ret) {
			return -1;
		}
	}

	ret = _dpu_update_fb_info(ctx);
	if (ret < 0) {
		close(ctx->fb.fd);
		return NULL;
	}
	ctx->cur_frame = ctx->fb.var_info.yoffset / ctx->fb.var_info.yres;
	ctx->is_fb_init = 1;

        return (IHal_SampleFB_Handle_t *)ctx;
}

IHAL_INT32 IHal_SampleFB_GetBufferNumbers(IHal_SampleFB_Handle_t *handle)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
	assert(ctx->is_fb_init == 1);

	return ctx->fb.num_buf;
}

IHAL_INT32 IHal_SampleFB_GetMem(IHal_SampleFB_Handle_t *handle, IMPP_BufferInfo_t *buf)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        int ret = 0;
        int buf_no = -1;
        pthread_mutex_lock(&ctx->lock);
        if (ctx->is_fb_init == 0) {
                ret = _init_framebuffer(ctx);
                if (ret) {
                        pthread_mutex_unlock(&ctx->lock);
                        return -IHAL_RERR;
                }
        }
	buf_no = (ctx->cur_frame + 1) % ctx->fb.num_buf;
        buf->vaddr = (IHAL_UINT32)(ctx->fb.fb_mem + buf_no * ctx->fb.single_len);
        buf->paddr = (IHAL_UINT32)(ctx->fb.fix_info.smem_start + buf_no * ctx->fb.single_len);

        buf->size = ctx->fb.single_len;
        buf->index = buf_no;
        ctx->cur_frame++;
        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_DeInit(IHal_SampleFB_Handle_t *handle)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        int ret = 0;
        int layerIdx = 0;

        layerIdx = __devnode_to_layerIndex(ctx->fb.dev_name);
        if (0 > layerIdx) {
                IMPP_LOGE(TAG, "Get layer index failed.");
                return -IHAL_RERR;
        }

        if (Composer_Mode == ctx->attr.mode) {
                switch (layerIdx) {
                case 0:
                        ret = _set_dpu_layerx_zorder(ctx->fb.dev_name, Order_3);
                        if (ret) {
                                IMPP_LOGE(TAG, "Reset dpu layer%d zorder failed.", layerIdx);
                                return -IHAL_RERR;
                        }
                        break;
                case 1:
                        ret = _set_dpu_layerx_zorder(ctx->fb.dev_name, Order_2);
                        if (ret) {
                                IMPP_LOGE(TAG, "Reset dpu layer%d zorder failed.", layerIdx);
                                return -IHAL_RERR;
                        }
                        break;
                case 2:
                        ret = _set_dpu_layerx_zorder(ctx->fb.dev_name, Order_1);
                        if (ret) {
                                IMPP_LOGE(TAG, "Reset dpu layer%d zorder failed.", layerIdx);
                                return -IHAL_RERR;
                        }
                        break;
                case 3:
                        ret = _set_dpu_layerx_zorder(ctx->fb.dev_name, Order_0);
                        if (ret) {
                                IMPP_LOGE(TAG, "Reset dpu layer%d zorder failed.", layerIdx);
                                return -IHAL_RERR;
                        }
                        break;
                default:
                        IMPP_LOGE(TAG, "Dpu layer index error.");
                        return -IHAL_RERR;
                }
        }

        ret =  _set_dpu_layerx_enable(ctx->fb.dev_name, 0);
        if (ret) {
                IMPP_LOGE(TAG, "Disable dpu layer%d failed.", layerIdx);
                return -1;
        }

        ret = _dpu_comp_update();
        if (ret) {
                IMPP_LOGE(TAG, "Dpu composer update failed.");
                return -IHAL_RERR;
        }

        close(ctx->fb.fd);
        munmap(ctx->fb.fb_mem, ctx->fb.smem_len);
        free(ctx);
        return IHAL_ROK;
}

IHAL_INT32 IHal_SimpleFB_DeInit(IHal_SampleFB_Handle_t *handle)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;

        close(ctx->fb.fd);
        munmap(ctx->fb.fb_mem, ctx->fb.smem_len);
        free(ctx);
        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_Update(IHal_SampleFB_Handle_t *handle, IMPP_BufferInfo_t *buffer)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        pthread_mutex_lock(&ctx->lock);
	struct ingenicfb_var_screeninfo_extended *var_info_ext = &ctx->fb.var_info_ext;

        ctx->fb.var_info.yoffset = ctx->fb.var_info.yres * buffer->index;

	/* 与 UpdateWithAttr 互斥，取消use_extended*/
	var_info_ext->magic = 0;
	var_info_ext->use_extended = 0;

	ctx->fb.var_info.reserved[0] = var_info_ext;

        int ret = ioctl(ctx->fb.fd, FBIOPAN_DISPLAY, &ctx->fb.var_info);
        if (ret) {
                IMPP_LOGE(TAG, "update framebuffer failed");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }

        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_SimpleFB_UpdateWithAttr(IHal_SampleFB_Handle_t *handle, IMPP_BufferInfo_t *buffer, IHal_SampleFB_LayerAttr_t *attr)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;

        pthread_mutex_lock(&ctx->lock);
	struct ingenicfb_var_screeninfo_extended *var_info_ext = &ctx->fb.var_info_ext;
	struct ingenicfb_lay_cfg *lay_cfg = &var_info_ext->lay_cfg;
        ctx->fb.var_info.yoffset = ctx->fb.var_info.yres * buffer->index;

	var_info_ext->magic = VAR_INFO_EXT_MAGIC;
	var_info_ext->use_extended = 1;

	lay_cfg->lay_en 	= attr->lay_en;
	lay_cfg->format 	= __imppfmt_to_layerfmt_extend(attr->srcFmt);
	lay_cfg->source_w 	= attr->srcWidth;
	lay_cfg->source_h	= attr->srcHeight;
	lay_cfg->src_crop_x	= attr->srcCropx;
	lay_cfg->src_crop_y	= attr->srcCropy;
	lay_cfg->src_crop_w	= attr->srcCropw;
	lay_cfg->src_crop_h	= attr->srcCroph;
	lay_cfg->scale_w	= attr->scale_enable == 1 ? attr->scaleWidth : 0;
	lay_cfg->scale_h	= attr->scale_enable == 1 ? attr->scaleHeight : 0;
	lay_cfg->disp_pos_x	= attr->osd_posX;
	lay_cfg->disp_pos_y	= attr->osd_posY;
	lay_cfg->g_alpha_en	= attr->alpha != 255 ? 1 : 0;
	lay_cfg->g_alpha_val	= attr->alpha;
	lay_cfg->lay_z_order	= attr->osd_order;
	lay_cfg->color		= attr->color;
	lay_cfg->version	= attr->version;

	ctx->fb.var_info.reserved[0] = var_info_ext;

        int ret = ioctl(ctx->fb.fd, FBIOPAN_DISPLAY, &ctx->fb.var_info);
        if (ret) {
                IMPP_LOGE(TAG, "update framebuffer failed");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }

        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_WaitForVsync(IHal_SampleFB_Handle_t *handle, int *vsync)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        pthread_mutex_lock(&ctx->lock);
        int ret = ioctl(ctx->fb.fd, FBIO_WAITFORVSYNC, vsync);
        if (ret) {
                IMPP_LOGE(TAG, "wait for vsync failed");
                pthread_mutex_unlock(&ctx->lock);
                return -IHAL_RERR;
        }

        pthread_mutex_unlock(&ctx->lock);
        return IHAL_ROK;
}


// if not set format , alloc as rgba
IHAL_INT32 IHal_SampleFB_SetSrcFrameSize(IHal_SampleFB_Handle_t *handle, IHAL_INT32 width, IHAL_INT32 height)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_src_size(ctx->fb.dev_name, width, height);
        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

// if not set, use as frame size.
IHAL_INT32 IHal_SampleFB_SetSrcStrideSize(IHal_SampleFB_Handle_t *handle, IHAL_INT32 stride, IHAL_INT32 uv_stride)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_stride_size(ctx->fb.dev_name, stride, uv_stride);
        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_SetSrcCrop(IHal_SampleFB_Handle_t *handle, IHAL_INT32 crop_x, IHAL_INT32 crop_y, IHAL_INT32 crop_w, IHAL_INT32 crop_h)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_src_crop_size(ctx->fb.dev_name, crop_x, crop_y, crop_w, crop_h);
        if (ret) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_SetTargetFrameSize(IHal_SampleFB_Handle_t *handle, IHAL_INT32 width, IHAL_INT32 height)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_target_size(ctx->fb.dev_name, width, height);
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_SetTargetPos(IHal_SampleFB_Handle_t *handle, IHAL_INT32 posx, IHAL_INT32 posy)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                IMPP_LOGE(TAG, "this API only support composer");
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_target_pos(ctx->fb.dev_name, posx, posy);
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;

}

IHAL_INT32 IHal_SampleFB_SetAlpha(IHal_SampleFB_Handle_t *handle, IHAL_INT32 alpha)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                IMPP_LOGE(TAG, "this API only support composer");
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_global_alpha(ctx->fb.dev_name, alpha);
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;

}


IHAL_INT32 IHal_SampleFB_SetZorder(IHal_SampleFB_Handle_t *handle, FB_Composer_Zorder_t order)
{
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        if (ctx->attr.mode != Composer_Mode) {
                IMPP_LOGE(TAG, "this API only support composer");
                return -IHAL_RERR;
        }
        int ret =  _set_dpu_layerx_zorder(ctx->fb.dev_name, order);
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_CompRestart(IHal_SampleFB_Handle_t *handle)
{
        assert(handle);

        int ret = _dpu_comp_update();
        if (ret) {
                return -IHAL_RERR;
        }

        fb_ctx_t *ctx = (fb_ctx_t *)handle;
        ret = _dpu_update_fb_info(ctx);

        if (ret < 0) {
                return -IHAL_RERR;
        }

        return IHAL_ROK;
}

IHAL_INT32 IHal_SampleFB_Layer_Enable(IHal_SampleFB_Handle_t *handle, int enable)
{
        int ret = -1;
        assert(handle);
        fb_ctx_t *ctx = (fb_ctx_t *)handle;

        char devname[32];
        strcpy(devname, ctx->fb.dev_name);
        if (devname == NULL || strlen(devname) == 0) {
                IMPP_LOGE(TAG, "devname is empty");
                return -IHAL_RERR;
        }
        ret = _set_dpu_layerx_enable(devname, enable);
        if (ret < 0) {
                IMPP_LOGE(TAG, "layerenable failed");
                return -IHAL_RERR;
        }
        ret = _dpu_comp_update();
        if (ret) {
                return -IHAL_RERR;
        }
        return IHAL_ROK;

}

