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
#include "impp.h"
#include "display.h"
#include "icamera.h"

#define WIDTH   480
#define HEIGHT  640

IHal_SampleFB_Handle_t *fb0_handle;
IHal_SampleFB_Handle_t *fb1_handle;
IHAL_CameraHandle_t    *cam0_handle;
IHAL_CameraHandle_t    *cam1_handle;

pthread_t cam0_tid;
pthread_t cam1_tid;

int fb0_nr_buffers;
int fb1_nr_buffers;


#if 0
使用基于 FrameBuffer composer 模式.

camera 使用DPU 的buffer 作为输出.

前提条件:

1. 内核 CONFIG_NR_FRAME_BUFFERS = 3
                                    2. dts 配置

status = "okay";
ingenic, disable - rdma - fb = <1>;
/*Defines the init state of composer fb export infomations.*/
ingenic, layer - exported = <1 1 0 0>;
ingenic, layer - frames   = <3 3 2 2>; <= == == == == == == == = layer - frames:>= 3
ingenic, layer - framesize = <720 1280>, <720 1280>, <320 240>, <320 240>; /*Max framesize for each layer.*/
layer, color_mode        = <0 0 0 0>;                                   /*src fmt,*/
layer, src - size          = <720 1280>, <720 1280>, <320 240>, <240 200>; /*Layer src size should smaller than framesize*/
layer, target - size       = <720 1280>, <720 640>, <160 240>, <240 200>; /*Target Size should smaller than src_size.*/
layer, target - pos        = <0 0>, <0 640>, <340 480>, <100 980>; /*target pos , the start point of the target panel.*/
layer, enable            = <1 1 1 1>;                                   /*layer enabled or disabled.*/
ingenic, logo - pan - layer  = <0>;                                     /*on which layer should init logo pan on.*/
port {
dpu_out_ep:
        endpoint {
                remote - endpoint = <&panel_fw050_ep>;
        };
};
#endif

static unsigned long long get_time_ms(void)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);

        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void *cam0_thread(void *arg)
{
        IMPP_BufferInfo_t cam_buf;
        IMPP_BufferInfo_t fb_buf;
        int ret = 0;
        printf("start cam0 preview thread!\n");
        for (;;) {
                ret = IHal_Camera_WaitBufferAvailable(cam0_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        ret = IHal_CameraDeQueueBuffer(cam0_handle, &cam_buf);
                        if (!ret) {

                                int vsync = 0;
                                unsigned long long t = get_time_ms();

                                /*更新cam_buf到fb,此时buf 被 fb 占有.
                                  camera 不应该再使用该buffer，否则会造成屏幕撕裂.
                                */

                                fb_buf.index = cam_buf.index;
                                IHal_SampleFB_Update(fb0_handle, &fb_buf);

                                //printf("--cam0---Update cost: %lld ms\n", get_time_ms() - t);

                                //IHal_SampleFB_WaitForVsync(fb0_handle, &vsync);
                                static int cam0_buffer_dqueued[5] = { -1, -1, -1, -1, -1};
                                cam0_buffer_dqueued[cam_buf.index] = 1;
                                int next_to_render = (cam_buf.index + 1) % fb0_nr_buffers;
                                if (cam0_buffer_dqueued[next_to_render] == 1) {


                                        cam_buf.index = next_to_render;
                                        IHal_CameraQueuebuffer(cam0_handle, &cam_buf);

                                        cam0_buffer_dqueued[next_to_render] = -1;

                                }
                        }
                } else {
                        usleep(5000);
                }
        }
}

void *cam1_thread(void *arg)
{
        IMPP_BufferInfo_t cam_buf;
        IMPP_BufferInfo_t fb_buf;
        int ret = 0;
        printf("start cam1 preview thread!\n");
        for (;;) {
                ret = IHal_Camera_WaitBufferAvailable(cam1_handle, IMPP_WAIT_FOREVER);
                if (!ret || 1) {
                        ret = IHal_CameraDeQueueBuffer(cam1_handle, &cam_buf);

                        if (!ret || 1) {

                                unsigned long long t = get_time_ms();

                                int vsync = 0;

                                fb_buf.index = cam_buf.index;
                                IHal_SampleFB_Update(fb1_handle, &fb_buf);

//				IHal_SampleFB_WaitForVsync(fb1_handle, &vsync);
//				需要做两个线程:
//				WaitForVsync
//				CameraQueueBuffer(cam1_handle, &cam_buf)
//				如果帧率相当，就不要WaitForVsync，错帧显示就可以达到防止裂屏的效果.
//
                                //printf("--cam1---Update cost: %lld ms\n", get_time_ms() - t);

#if 1
                                static int cam1_buffer_dqueued[5] = { -1, -1, -1, -1, -1};
                                cam1_buffer_dqueued[cam_buf.index] = 1;
                                int next_to_render = (cam_buf.index + 1) % fb1_nr_buffers;
                                if (cam1_buffer_dqueued[next_to_render] == 1) {

                                        cam_buf.index = next_to_render;
                                        IHal_CameraQueuebuffer(cam1_handle, &cam_buf);

                                        cam1_buffer_dqueued[next_to_render] = -1;

                                }
#endif
                        }
                } else {
                        usleep(5000);
                }
        }
}

static int signal_handle(int sig)
{
        pthread_cancel(cam0_tid);
        pthread_join(cam0_tid, NULL);
        pthread_cancel(cam1_tid);
        pthread_join(cam1_tid, NULL);

        IHal_CameraStop(cam1_handle);
        IHal_CameraStop(cam0_handle);
        IHal_CameraClose(cam0_handle);
        IHal_CameraClose(cam1_handle);
        IHal_SampleFB_DeInit(fb0_handle);
        IHal_SampleFB_DeInit(fb1_handle);

        printf("###################################### exit ####################################\r\n");
        exit(0);
}

int main(int argc, char **argv)
{
        int ret = 0;

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

#if 0
        IHal_SampleFB_SetSrcFrameSize(fb1_handle, WIDTH, HEIGHT);
        IHal_SampleFB_SetSrcCrop(fb1_handle, 100, 100, 500, 500);
        IHal_SampleFB_SetTargetFrameSize(fb1_handle, 720, 640);
        IHal_SampleFB_SetTargetPos(fb1_handle, 0, 640);
        IHal_SampleFB_SetZorder(fb1_handle, Order_3);
        IHal_SampleFB_CompRestart(fb1_handle);
#endif

        fb0_nr_buffers = IHal_SampleFB_GetBufferNumbers(fb0_handle);

        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb1");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = WIDTH;
        fb_attr.frame_height = HEIGHT;
        fb_attr.crop_x = 0;   //初始化crop信息。
        fb_attr.crop_y = 0;
        fb_attr.crop_w = 0;
        fb_attr.crop_h = 0;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb1_handle = IHal_SampleFB_Init(&fb_attr);
        if (!fb1_handle) {
                printf("init error");
                return -1;
        }

#if 0
        IHal_SampleFB_SetSrcFrameSize(fb0_handle, WIDTH, HEIGHT);
        IHal_SampleFB_SetTargetFrameSize(fb0_handle, WIDTH, HEIGHT / 2);
        IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        IHal_SampleFB_SetZorder(fb0_handle, Order_2);
        //动态调整的参数需要配合IHal_SampleFB_CompRestart()更新配置.
        IHal_SampleFB_CompRestart(fb0_handle);
#endif

        fb1_nr_buffers = IHal_SampleFB_GetBufferNumbers(fb1_handle);




        cam0_handle = IHal_CameraOpen("/dev/video4");
        cam1_handle = IHal_CameraOpen("/dev/video8");
        IHAL_CAMERA_PARAMS cam_params = {
                .imageWidth = WIDTH,
                .imageHeight = HEIGHT,
                .imageFmt = IMPP_PIX_FMT_NV12,
        };
        IHal_CameraSetParams(cam0_handle, &cam_params);
        IHal_CameraSetParams(cam1_handle, &cam_params);
        IHal_CameraCreateBuffers(cam0_handle, IMPP_EXT_USERBUFFER, fb0_nr_buffers); // create 3 buf
        IHal_CameraCreateBuffers(cam1_handle, IMPP_EXT_USERBUFFER, fb1_nr_buffers); // create 3 buf


        IMPP_BufferInfo_t cam_buf;
        IMPP_BufferInfo_t cam1_buf;
        int i = 0;

	IMPP_BufferInfo_t fb_buf;

        /*从 framebuffer 获取分配好的内存，用于camera 的输出. */
        for (i = 0; i < fb0_nr_buffers; i++) {
                IHal_SampleFB_GetMem(fb0_handle, &fb_buf);
                /*fb0_buf -> cma_buf*/
                memcpy(&cam_buf, &fb_buf, sizeof(IMPP_BufferInfo_t));

                IHal_CameraSetBuffers(cam0_handle, fb_buf.index, &cam_buf);
        }

        for (i = 0; i < fb1_nr_buffers; i++) {
                IHal_SampleFB_GetMem(fb1_handle, &fb_buf);
                /*fb1_buf -> cam1_buf*/
                memcpy(&cam1_buf, &fb_buf, sizeof(IMPP_BufferInfo_t));

                IHal_CameraSetBuffers(cam1_handle, fb_buf.index, &cam1_buf);
        }

        IHal_CameraStart(cam0_handle);
        IHal_CameraStart(cam1_handle);


        pthread_create(&cam0_tid, NULL, cam0_thread, NULL);
        pthread_create(&cam1_tid, NULL, cam1_thread, NULL);
        signal(SIGINT, signal_handle);

        while (1) {
                printf("main thread running!\n");
                sleep(5);
        };

        pthread_cancel(cam0_tid);
        pthread_join(cam0_tid, NULL);
        pthread_cancel(cam1_tid);
        pthread_join(cam1_tid, NULL);

        IHal_CameraStop(cam0_handle);
        IHal_CameraStop(cam1_handle);
        IHal_CameraClose(cam0_handle);
        IHal_CameraClose(cam1_handle);
        IHal_SampleFB_DeInit(fb0_handle);
        IHal_SampleFB_DeInit(fb1_handle);

        return 0;
}

