/*
 *  叠加显示图片，设置上层layer的局部alpha来显示底层图片
 *  320x800的图像，中间有一个200x700的全透明区域
 */


#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdint.h>
#include <impp.h>
#include <display.h>
#include <assert.h>


IHal_SampleFB_Handle_t *fb0_handle;
IHal_SampleFB_Handle_t *fb1_handle;

static int signal_handle(int sig)
{
    IHal_SimpleFB_DeInit(fb0_handle);
    IHal_SimpleFB_DeInit(fb1_handle);

    printf("###################################### exit ####################################\r\n");
    exit(0);
}

#define WIDTH 320
#define HEIGHT 800

int main(int argc, char **argv)
{
    int ret = 0;
    IHal_SampleFB_LayerAttr_t g_attr_layer0;
    IHal_SampleFB_LayerAttr_t g_attr_layer1;
    uint8_t *buffer0Ptr = NULL;
    void *buffer1Ptr = NULL;
    uint32_t buffer0Size = WIDTH * HEIGHT * 4;
    uint32_t buffer1Size = WIDTH * HEIGHT * 3 / 2;

    memset(&g_attr_layer0, 0, sizeof(IHal_SampleFB_LayerAttr_t));
    sprintf(&g_attr_layer0.node[0],"%s","/dev/fb0");
    g_attr_layer0.lay_en = 1;
    g_attr_layer0.srcWidth = WIDTH;
    g_attr_layer0.srcHeight = HEIGHT;
    g_attr_layer0.scaleWidth = WIDTH;
    g_attr_layer0.scaleHeight = HEIGHT;
    g_attr_layer0.scale_enable = 1;
    g_attr_layer0.alpha = 255;
    g_attr_layer0.osd_posX = 80;
    g_attr_layer0.osd_posY = 0;
    g_attr_layer0.srcCropx = 0;
    g_attr_layer0.srcCropy = 0;
    g_attr_layer0.srcCropw = 0;
    g_attr_layer0.srcCroph = 0;
    g_attr_layer0.version = 0;
    // g_attr_layer0.srcFmt = IMPP_PIX_FMT_NV12;
    g_attr_layer0.srcFmt = IMPP_PIX_FMT_BGRA_8888;
    g_attr_layer0.osd_order = Order_3;
    fb0_handle = IHal_SimpleFB_Init(&g_attr_layer0);
    if(!fb0_handle){
        printf("init error");
        return -1;
    }

    memset(&g_attr_layer1, 0, sizeof(IHal_SampleFB_LayerAttr_t));
    sprintf(&g_attr_layer1.node[0],"%s","/dev/fb1");
    g_attr_layer1.lay_en = 1;
    g_attr_layer1.srcWidth = WIDTH;
    g_attr_layer1.srcHeight = HEIGHT;
    g_attr_layer1.scaleWidth = WIDTH;
    g_attr_layer1.scaleHeight = HEIGHT;
    g_attr_layer1.scale_enable = 1;
    g_attr_layer1.alpha = 255;
    g_attr_layer1.osd_posX = 80;
    g_attr_layer1.osd_posY = 0;
    g_attr_layer1.srcCropx = 0;
    g_attr_layer1.srcCropy = 0;
    g_attr_layer1.srcCropw = 0;
    g_attr_layer1.srcCroph = 0;
    g_attr_layer1.srcFmt = IMPP_PIX_FMT_NV12;
    g_attr_layer1.osd_order = Order_2;
    fb1_handle = IHal_SimpleFB_Init(&g_attr_layer1);
    if(!fb1_handle){
        printf("init error");
        return -1;
    }

    int file1 = open("/storage/scale1.yuv", O_RDONLY);
    if (-1 == file1) {
        printf("Open file failed!\r\n");
        return -1;
    }


    buffer0Ptr = (uint8_t*)malloc(buffer0Size);
    // 设置透明度
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int index = (y * WIDTH + x) * 4;
            if ((x >= 50 && x <= 250) && (y >= 50 && y <= 749)) {
                buffer0Ptr[index] = 0x00;
                buffer0Ptr[index+1] = 0x00;
                buffer0Ptr[index+2] = 0x00;
                buffer0Ptr[index+3] = 0x00;
            } else {
                buffer0Ptr[index] =  rand() %0xff;
                buffer0Ptr[index+1] = rand() %0xff;
                buffer0Ptr[index+2] = rand() %0xff;
                buffer0Ptr[index+3] = rand() %0xff;
            }
        }

    }

    buffer1Ptr = malloc(buffer1Size);
    read(file1, buffer1Ptr, buffer1Size);

    signal(SIGINT, signal_handle);

    close(file1);

    IMPP_BufferInfo_t fb0_buf;
    IMPP_BufferInfo_t fb1_buf;
    while (1) {
        IHal_SampleFB_GetMem(fb0_handle, &fb0_buf);
        memcpy(fb0_buf.vaddr, buffer0Ptr, buffer0Size);
        IHal_SimpleFB_UpdateWithAttr(fb0_handle, &fb0_buf, &g_attr_layer0);

        IHal_SampleFB_GetMem(fb1_handle, &fb1_buf);
        memcpy(fb1_buf.vaddr, buffer1Ptr, buffer1Size);
        IHal_SimpleFB_UpdateWithAttr(fb1_handle, &fb1_buf, &g_attr_layer1);
        usleep(800000);
    }

    IHal_SimpleFB_DeInit(fb0_handle);
    IHal_SimpleFB_DeInit(fb1_handle);

    return 0;
}
