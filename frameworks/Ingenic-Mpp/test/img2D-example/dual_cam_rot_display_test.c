/**
 * @file dual_cam_rotater_display_test.c
 * @brief 双路Camera+Rotater+Display。将video4节点的图像旋转90°后在fb0上显示，video8节点的图像旋转180°在fb1上显示。
 * @version 0.1
 * @date 2022-10-08
 */
#include <stdio.h>
#include <string.h>
#include <icamera.h>
#include <rotater.h>
#include <display.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define CAMERA1_WIDTH 1280
#define CAMERA1_HEIGHT 720
#define CAMERA2_WIDTH 720
#define CAMERA2_HEIGHT 1280

IHAL_CameraHandle_t *cam_handle1 = NULL;
IHAL_CameraHandle_t *cam_handle2 = NULL;
IHal_Rot_Handle_t *rot_handle1 = NULL;
IHal_Rot_Handle_t *rot_handle2 = NULL;
IHal_SampleFB_Handle_t *fb0_handle = NULL;
IHal_SampleFB_Handle_t *fb1_handle = NULL;

pthread_t cam_rotater_thid1 = 0;
pthread_t cam_rotater_thid2 = 0;
pthread_t rot_display_thid1 = 0;
pthread_t rot_display_thid2 = 0;

void *camera_rot_thread1(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        IMPP_FrameInfo_t srcframe;

        while (1) {
                memset(&camera_buf, 0, sizeof(IMPP_BufferInfo_t));
                memset(&srcframe, 0, sizeof(IMPP_FrameInfo_t));

                ret = IHal_Camera_WaitBufferAvailable(cam_handle1, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_CameraDeQueueBuffer(cam_handle1, &camera_buf);
                        srcframe.fd = camera_buf.fd;
                        srcframe.index = camera_buf.index;
                        ret = IHal_Rot_ProcessFrame(rot_handle1, &srcframe, ROT_PROCESS_ROTATE, ROT_ANGLE_90);
                        if (!ret) {
                                IHal_CameraQueuebuffer(cam_handle1, &camera_buf);
                        }
                } else {
                        usleep(5000);
                }
        }
}

void *camera_rot_thread2(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        IMPP_FrameInfo_t srcframe;

        while (1) {
                memset(&camera_buf, 0, sizeof(IMPP_BufferInfo_t));
                memset(&srcframe, 0, sizeof(IMPP_FrameInfo_t));

                ret = IHal_Camera_WaitBufferAvailable(cam_handle2, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_CameraDeQueueBuffer(cam_handle2, &camera_buf);
                        srcframe.fd = camera_buf.fd;
                        srcframe.index = camera_buf.index;
                        ret = IHal_Rot_ProcessFrame(rot_handle2, &srcframe, ROT_PROCESS_ROTATE, ROT_ANGLE_180);
                        if (!ret) {
                                IHal_CameraQueuebuffer(cam_handle2, &camera_buf);
                        }
                } else {
                        usleep(5000);
                }
        }
}

void *rot_display_thread1(void *arg)
{
        int ret = 0;
        IMPP_FrameInfo_t dst;
        IMPP_BufferInfo_t fb_buf;

        while (1) {
                memset(&dst, 0, sizeof(IMPP_FrameInfo_t));
                memset(&fb_buf, 0, sizeof(IMPP_BufferInfo_t));

                ret = IHal_Rot_GetFrame(rot_handle1, &dst);
                if (!ret) {
                        fb_buf.index = dst.index;
                        IHal_SampleFB_Update(fb0_handle, &fb_buf);
                        IHal_Rot_ReleaseFrame(rot_handle1, &dst);
                } else {
                        usleep(10000);
                }
        }
}

void *rot_display_thread2(void *arg)
{
        int ret = 0;
        IMPP_FrameInfo_t dst;
        IMPP_BufferInfo_t fb_buf;

        while (1) {
                memset(&dst, 0, sizeof(IMPP_FrameInfo_t));
                memset(&fb_buf, 0, sizeof(IMPP_BufferInfo_t));

                ret = IHal_Rot_GetFrame(rot_handle2, &dst);
                if (!ret) {
                        fb_buf.index = dst.index;
                        IHal_SampleFB_Update(fb1_handle, &fb_buf);
                        IHal_Rot_ReleaseFrame(rot_handle2, &dst);
                } else {
                        usleep(10000);
                }
        }
}

static int signal_handle(int sig)
{
        pthread_cancel(cam_rotater_thid1);
        pthread_join(cam_rotater_thid1, NULL);
        pthread_cancel(cam_rotater_thid2);
        pthread_join(cam_rotater_thid2, NULL);
        pthread_cancel(rot_display_thid1);
        pthread_join(rot_display_thid1, NULL);
        pthread_cancel(rot_display_thid2);
        pthread_join(rot_display_thid2, NULL);

        /* 停止Camera */
        IHal_CameraStop(cam_handle2);
        IHal_CameraStop(cam_handle1);

        /* 去初始化Display */
        IHal_SampleFB_DeInit(fb1_handle);
        IHal_SampleFB_DeInit(fb0_handle);

        /* 销毁Rotater */
        IHal_Rot_DestroyChan(rot_handle2);
        IHal_Rot_DestroyChan(rot_handle1);

        /* 关闭Camera节点 */
        IHal_CameraClose(cam_handle2);
        IHal_CameraClose(cam_handle1);

        printf("####################### end end ###############################\r\n");
        exit(0);
}

int main(int argc, char **argv)
{
        /* 打开Camera节点 */
        cam_handle1 = IHal_CameraOpen("/dev/video4");
        cam_handle2 = IHal_CameraOpen("/dev/video8");
        if (NULL == cam_handle1 || NULL == cam_handle2) {
                printf("Camera open error!\r\n");
                return -1;
        }

        /* 设置Camera的基本参数 */
        IHAL_CAMERA_PARAMS cam_params;
        memset(&cam_params, 0, sizeof(IHAL_CAMERA_PARAMS));
        cam_params.imageWidth = CAMERA1_WIDTH;
        cam_params.imageHeight = CAMERA1_HEIGHT;
        cam_params.imageFmt = IMPP_PIX_FMT_NV12;
        IHal_CameraSetParams(cam_handle1, &cam_params);
        memset(&cam_params, 0, sizeof(IHAL_CAMERA_PARAMS));
        cam_params.imageWidth = CAMERA2_WIDTH;
        cam_params.imageHeight = CAMERA2_HEIGHT;
        cam_params.imageFmt = IMPP_PIX_FMT_NV12;
        IHal_CameraSetParams(cam_handle2, &cam_params);

        /* Camera创建Buffer */
        IHal_CameraCreateBuffers(cam_handle1, IMPP_INTERNAL_BUFFER, 3);
        IHal_CameraCreateBuffers(cam_handle2, IMPP_INTERNAL_BUFFER, 3);

        /* 创建Rotater */
        IHal_Rot_ChanAttr_t rotater_attr;
        memset(&rotater_attr, 0, sizeof(IHal_Rot_ChanAttr_t));
        rotater_attr.sWidth = CAMERA1_WIDTH;
        rotater_attr.sHeight = CAMERA1_HEIGHT;
        rotater_attr.srcBufType = IMPP_EXT_DMABUFFER;
        rotater_attr.dstBufType = IMPP_EXT_USERBUFFER;
        rotater_attr.numSrcBuf = 3;
        rotater_attr.numDstBuf = 3;
        rotater_attr.srcFmt = IMPP_PIX_FMT_NV12;
        rot_handle1 = IHal_Rot_CreateChan(&rotater_attr);
        memset(&rotater_attr, 0, sizeof(IHal_Rot_ChanAttr_t));
        rotater_attr.sWidth = CAMERA2_WIDTH;
        rotater_attr.sHeight = CAMERA2_HEIGHT;
        rotater_attr.srcBufType = IMPP_EXT_DMABUFFER;
        rotater_attr.dstBufType = IMPP_EXT_USERBUFFER;
        rotater_attr.numSrcBuf = 3;
        rotater_attr.numDstBuf = 3;
        rotater_attr.srcFmt = IMPP_PIX_FMT_NV12;
        rot_handle2 = IHal_Rot_CreateChan(&rotater_attr);
        if (NULL == rot_handle1 || NULL == rot_handle2) {
                printf("Create rotater handle error!\r\n");
                IHal_CameraClose(cam_handle1);
                IHal_CameraClose(cam_handle2);
                return -1;
        }

        /* 初始化Display */
        IHal_SampleFB_Attr fb_attr;
        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb0");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = CAMERA1_HEIGHT;                                          // Rotater旋转90°后将数据给到Display
        fb_attr.frame_height = CAMERA1_WIDTH;                                          // Rotater旋转90°后将数据给到Display
        fb_attr.crop_x = 0;
        fb_attr.crop_y = 0;
        fb_attr.crop_w = 0;
        fb_attr.crop_h = 0;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb0_handle = IHal_SampleFB_Init(&fb_attr);
        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb1");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = CAMERA2_WIDTH;                                          // Rotater旋转180°后将数据给到Display
        fb_attr.frame_height = CAMERA2_HEIGHT;                                        // Rotater旋转180°后将数据给到Display
        fb_attr.crop_x = 0;
        fb_attr.crop_y = 0;
        fb_attr.crop_w = 0;
        fb_attr.crop_h = 0;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb1_handle = IHal_SampleFB_Init(&fb_attr);
        if (NULL == fb0_handle || NULL == fb1_handle) {
                printf("Init SampleFB error!\r\n");
                IHal_Rot_DestroyChan(rot_handle2);
                IHal_Rot_DestroyChan(rot_handle1);
                IHal_CameraClose(cam_handle2);
                IHal_CameraClose(cam_handle1);
                return -1;
        }

        IHal_SampleFB_SetSrcFrameSize(fb0_handle, CAMERA1_HEIGHT, CAMERA1_WIDTH);
        // IHal_SampleFB_SetTargetFrameSize(fb0_handle, CAMERA1_HEIGHT, CAMERA1_WIDTH / 2);                             // 目标大小 : 720x640
        IHal_SampleFB_SetTargetFrameSize(fb0_handle, 720, 640);                         // 目标大小 : 720x640
        IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        IHal_SampleFB_SetZorder(fb0_handle, Order_2);

        IHal_SampleFB_SetSrcFrameSize(fb1_handle, CAMERA2_WIDTH, CAMERA2_HEIGHT);
        // IHal_SampleFB_SetTargetFrameSize(fb1_handle, CAMERA2_WIDTH, CAMERA2_HEIGHT / 2);                             // 目标大小 : 720x640
        IHal_SampleFB_SetTargetFrameSize(fb1_handle, 720, 640);                         // 目标大小 : 720x640
        IHal_SampleFB_SetTargetPos(fb1_handle, 0, 640);
        IHal_SampleFB_SetZorder(fb1_handle, Order_3);

        /* 动态调整的参数需要配合IHal_SampleFB_CompRestart()更新配置. */
        IHal_SampleFB_CompRestart(fb0_handle);

        /* 共享Camera和Rotater的Buf : Camera ----> Rotater */
        IMPP_BufferInfo_t share;
        for (int i = 0; i < 3; i++) {
                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_GetCameraBuffers(cam_handle1, i, &share);
                IHal_Rot_SetExtSrcBuffer(rot_handle1, &share, i);

                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_GetCameraBuffers(cam_handle2, i, &share);
                IHal_Rot_SetExtSrcBuffer(rot_handle2, &share, i);
        }

        for (int i = 0; i < 3; i++) {
                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_SampleFB_GetMem(fb0_handle, &share);
                IHal_Rot_SetExtDstBuffer(rot_handle1, &share, i);

                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_SampleFB_GetMem(fb1_handle, &share);
                IHal_Rot_SetExtDstBuffer(rot_handle2, &share, i);
        }

        /* 启动Camera */
        IHal_CameraStart(cam_handle1);
        IHal_CameraStart(cam_handle2);

        pthread_create(&cam_rotater_thid1, NULL, camera_rot_thread1, NULL);
        pthread_create(&cam_rotater_thid2, NULL, camera_rot_thread2, NULL);
        pthread_create(&rot_display_thid1, NULL, rot_display_thread1, NULL);
        pthread_create(&rot_display_thid2, NULL, rot_display_thread2, NULL);
        // signal(SIGINT, signal_handle);

        while (1) {};

        /* 停止Camera */
        IHal_CameraStop(cam_handle2);
        IHal_CameraStop(cam_handle1);

        /* 去初始化Display */
        IHal_SampleFB_DeInit(fb1_handle);
        IHal_SampleFB_DeInit(fb0_handle);

        /* 销毁Rotater */
        IHal_Rot_DestroyChan(rot_handle2);
        IHal_Rot_DestroyChan(rot_handle1);

        /* 关闭Camera节点 */
        IHal_CameraClose(cam_handle2);
        IHal_CameraClose(cam_handle1);

        return 0;
}
