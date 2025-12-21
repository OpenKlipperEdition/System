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
#include <impp.h>
#include <display.h>
#include <icamera.h>

#define WIDTH   720
#define HEIGHT  1280

IHal_SampleFB_Handle_t *fb0_handle;
IHal_SampleFB_Handle_t *fb1_handle;
IHAL_CameraHandle_t    *cam_handle;

static int signal_handle(int sig)
{
        IHal_CameraStop(cam_handle);
        IHal_CameraClose(cam_handle);
        IHal_SampleFB_DeInit(fb0_handle);
        IHal_SampleFB_DeInit(fb1_handle);

        printf("###################################### exit ####################################\r\n");
        exit(0);
}

int main(int argc, char **argv)
{
        int ret = 0;
        cam_handle = IHal_CameraOpen("/dev/video4");
        IHAL_CAMERA_PARAMS cam_params = {
                .imageWidth = WIDTH,
                .imageHeight = HEIGHT,
                .imageFmt = IMPP_PIX_FMT_NV12,
        };
        IHal_CameraSetParams(cam_handle, &cam_params);
        IHal_CameraCreateBuffers(cam_handle, IMPP_INTERNAL_BUFFER, 3); // create 3 buf

        IHal_SampleFB_Attr fb_attr;
        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb0");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = WIDTH;
        fb_attr.frame_height = HEIGHT;
        fb_attr.crop_x = 0;
        fb_attr.crop_y = 0;
        fb_attr.crop_w = 0;
        fb_attr.crop_h = 0;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb0_handle = IHal_SampleFB_Init(&fb_attr);
        if (!fb0_handle) {
                printf("init error");
                return -1;
        }
        IHal_SampleFB_SetSrcFrameSize(fb0_handle, WIDTH, HEIGHT);
        IHal_SampleFB_SetTargetFrameSize(fb0_handle, WIDTH, HEIGHT / 2);
        IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        IHal_SampleFB_SetZorder(fb0_handle, Order_2);

        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb1");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = WIDTH;
        fb_attr.frame_height = HEIGHT;
        fb_attr.crop_x = 100;   //初始化crop信息。
        fb_attr.crop_y = 100;
        fb_attr.crop_w = 400;
        fb_attr.crop_h = 400;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb1_handle = IHal_SampleFB_Init(&fb_attr);
        if (!fb1_handle) {
                printf("init error");
                return -1;
        }
        IHal_SampleFB_SetSrcFrameSize(fb1_handle, 720, 1280);
        IHal_SampleFB_SetSrcCrop(fb1_handle, 100, 100, 500, 500);
        IHal_SampleFB_SetTargetFrameSize(fb1_handle, 720, 640);
        IHal_SampleFB_SetTargetPos(fb1_handle, 0, 640);
        IHal_SampleFB_SetZorder(fb1_handle, Order_3);

        //动态调整的参数需要配合IHal_SampleFB_CompRestart()更新配置.
        IHal_SampleFB_CompRestart(fb0_handle);

        IHal_CameraStart(cam_handle);

        signal(SIGINT, signal_handle);

        IMPP_BufferInfo_t cam_buf;
        IMPP_BufferInfo_t fb0_buf;
        IMPP_BufferInfo_t fb1_buf;
        while (1) {
                ret = IHal_Camera_WaitBufferAvailable(cam_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        ret = IHal_CameraDeQueueBuffer(cam_handle, &cam_buf);
                        IHal_SampleFB_GetMem(fb0_handle, &fb0_buf);
                        IHal_SampleFB_GetMem(fb1_handle, &fb1_buf);
                        memcpy(fb0_buf.vaddr, cam_buf.vaddr, cam_buf.size);
                        memcpy(fb1_buf.vaddr, cam_buf.vaddr, cam_buf.size);
                        IHal_SampleFB_Update(fb0_handle, &fb0_buf);
                        IHal_SampleFB_Update(fb1_handle, &fb1_buf);
                        IHal_CameraQueuebuffer(cam_handle, &cam_buf);
                }
        }

        IHal_CameraStop(cam_handle);
        IHal_CameraClose(cam_handle);
        IHal_SampleFB_DeInit(fb0_handle);
        IHal_SampleFB_DeInit(fb1_handle);

        return 0;
}
