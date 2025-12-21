#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>

#include <drawbox.h>
#include <log.h>

#define BOX_DEV_PATH  "/dev/dbox"
#define TAG           "DRAWBOX"

#define JZDBOX_IOC_MAGIC                'X'
#define IOCTL_DBOX_START                _IO(JZDBOX_IOC_MAGIC, 106)
#define IOCTL_DBOX_RES_PBUFF            _IO(JZDBOX_IOC_MAGIC, 114)
#define IOCTL_DBOX_GET_PBUFF            _IO(JZDBOX_IOC_MAGIC, 115)
#define IOCTL_DBOX_BUF_LOCK             _IO(JZDBOX_IOC_MAGIC, 116)
#define IOCTL_DBOX_BUF_UNLOCK           _IO(JZDBOX_IOC_MAGIC, 117)
#define IOCTL_DBOX_BUF_FLUSH_CACHE      _IO(JZDBOX_IOC_MAGIC, 118)
#define IOCTL_DBOX_GET_DMA_PHY          _IO(JZDBOX_IOC_MAGIC, 119)
#define IOCTL_DBOX_BUF_FLUSH_CACHE_PHY  _IO(JZDBOX_IOC_MAGIC, 120)

struct dbox_ram {
        unsigned int        box_x;
        unsigned int        box_y;
        unsigned int        box_w;
        unsigned int        box_h;
        unsigned int        line_w;
        unsigned int        line_l;
        unsigned int        box_mode;
        unsigned int        color_mode;
};

struct dbox_param {
        unsigned int        box_pbuf;
        unsigned int        img_w;
        unsigned int        img_h;
        unsigned int        boxs_num;
        unsigned int        is_rgba;
        struct dbox_ram     ram_para[24];  //kernel stack overflow
};


typedef struct {
        int fd;
        pthread_mutex_t lock;
        int register_sta;
} draw_box_ctx_t;

static draw_box_ctx_t boxCh[2];

IHAL_INT32 IHal_DrawBox_Init(IHAL_INT32 ch)
{
        int ret = 0;
        if (ch > 1) {
                IMPP_LOGE(TAG, "ch only support [0-1]");
                return -IHAL_RERR;
        }
        ret = open(BOX_DEV_PATH, O_RDWR);
        if (ret < 0) {
                IMPP_LOGE(TAG, "init draw box failed");
                return -IHAL_RERR;
        }
        boxCh[ch].fd = ret;
        pthread_mutex_init(&boxCh[ch].lock, NULL);

        return IHAL_ROK;
}

IHAL_INT32 IHal_DrawBox_DeInit(IHAL_INT32 ch)
{
        int ret = 0;
        if (ch > 1) {
                IMPP_LOGE(TAG, "ch only support [0-1]");
                return -IHAL_RERR;
        }
        pthread_mutex_destroy(&boxCh[ch].lock);
        close(boxCh[ch].fd);

        return IHAL_ROK;
}

// call this api ,will return paddr of dma-buf
IHAL_UINT32 IHal_DrawBox_ImportDmaBuf(IHAL_INT32 ch, IMPP_BufferInfo_t *buf)
{
        unsigned int arg;
        if (ch > 1) {
                IMPP_LOGE(TAG, "ch only support [0-1]");
                return -IHAL_RERR;
        }
        int dboxfd = boxCh[ch].fd;
        if (buf->fd <= 0) {
                IMPP_LOGE(TAG, "dma-buf fd error");
                return -IHAL_RERR;
        }
        arg = buf->fd;
        int ret = ioctl(dboxfd, IOCTL_DBOX_GET_DMA_PHY, &arg);
        if (ret) {
                return 0;
        } else {
                return arg;
        }
}

IHAL_INT32 IHal_DrawBox_Process(IHAL_INT32 ch, IHal_DrawBoxInfo_t *boxinfo)
{
        int boxnum = 0;
        struct dbox_param inparam;
        if (ch > 1) {
                IMPP_LOGE(TAG, "ch only support [0-1]");
                return -IHAL_RERR;
        }
        if (boxinfo->img_fmt != IMPP_PIX_FMT_NV12) {
                IMPP_LOGE(TAG, "pix fmt only support nv12");
                return -IHAL_RERR;
        }
        if (boxinfo->box_num > 20) {
                IMPP_LOGE(TAG, "box max num is 20");
                return -IHAL_RERR;
        }
        inparam.box_pbuf = boxinfo->img_paddr;
        inparam.img_w = boxinfo->img_width;
        inparam.img_h = boxinfo->img_height;
        inparam.boxs_num = boxinfo->box_num;
        inparam.is_rgba = 0;
        int x0, y0, x1, y1;
        for (int i = 0; i < boxinfo->box_num; i++) {
                x0 = boxinfo->box[i].x0;
                x1 = boxinfo->box[i].x1;
                y0 = boxinfo->box[i].y0;
                y1 = boxinfo->box[i].y1;
                assert(x1 > x0);
                assert(y1 > y0);
                inparam.ram_para[i].box_x = x0;
                inparam.ram_para[i].box_y = y0;
                inparam.ram_para[i].box_w = x1 - x0;
                inparam.ram_para[i].box_h = y1 - y0;
                inparam.ram_para[i].line_w = boxinfo->line_w;
                inparam.ram_para[i].line_l = boxinfo->line_l;
                inparam.ram_para[i].box_mode = boxinfo->boxtype;
                inparam.ram_para[i].color_mode = boxinfo->color;

        }
        int ret = ioctl(boxCh[ch].fd, IOCTL_DBOX_START, &inparam);
        if (ret) {
                IMPP_LOGE(TAG, "draw box failed");
                return -IHAL_RERR;
        }
        return IHAL_ROK;

}

