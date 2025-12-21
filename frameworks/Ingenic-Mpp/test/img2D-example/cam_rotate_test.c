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
#include <unistd.h>
#include <rotater.h>
#include <display.h>
#include <icamera.h>
#include <signal.h>

/**
 * 这个用例中是将Camera的数据经Rotater旋转90°后在屏上进行显示，Display的frame_width和frame_height固定为了720x1280
 * 所以，这个地方就先固定为1280x720
 */
#define CAMERA_WIDTH   1280
#define CAMERA_HEIGHGT 720

IHal_SampleFB_Handle_t *fb0_handle;
IHAL_CameraHandle_t    *cam_handle;
IHal_Rot_Handle_t      *rot_handle;
pthread_t    cam_thid;

void *camera_rot_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        IMPP_FrameInfo_t srcframe;
        while (1) {
                ret = IHal_Camera_WaitBufferAvailable(cam_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        // dequeue buffer from camera
                        IHal_CameraDeQueueBuffer(cam_handle, &camera_buf);
                        // rot process
                        srcframe.fd = camera_buf.fd;
                        srcframe.index = camera_buf.index;
                        ret = IHal_Rot_ProcessFrame(rot_handle, &srcframe, ROT_PROCESS_ROTATE, ROT_ANGLE_90);
                        if (!ret) {
                                IHal_CameraQueuebuffer(cam_handle, &camera_buf);
                        }
                } else {
                        usleep(5000);
                }
        }
}

static int signal_handle(int sig)
{
        pthread_cancel(cam_thid);
        pthread_join(cam_thid, NULL);
        IHal_CameraStop(cam_handle);
        IHal_CameraClose(cam_handle);
        IHal_Rot_DestroyChan(rot_handle);
        IHal_SampleFB_DeInit(fb0_handle);
        printf("end end ###########\r\n");
        exit(0);
}


int main(int argc, char **argv)
{
        cam_handle = IHal_CameraOpen("/dev/video4");
        IHAL_CAMERA_PARAMS cam_params = {
                .imageWidth = CAMERA_WIDTH,
                .imageHeight = CAMERA_HEIGHGT,
                .imageFmt = IMPP_PIX_FMT_NV12,
        };
        IHal_CameraSetParams(cam_handle, &cam_params);
        IHal_CameraCreateBuffers(cam_handle, IMPP_INTERNAL_BUFFER, 3); // create 3 buf

        IHal_Rot_ChanAttr_t attr;
        memset(&attr, 0, sizeof(IHal_Rot_ChanAttr_t));
        attr.sWidth = cam_params.imageWidth;
        attr.sHeight = cam_params.imageHeight;
        attr.srcBufType = IMPP_EXT_DMABUFFER;
        attr.dstBufType = IMPP_EXT_USERBUFFER;
        attr.numSrcBuf = 3;
        attr.numDstBuf = 3;
        attr.srcFmt = IMPP_PIX_FMT_NV12;
        rot_handle = IHal_Rot_CreateChan(&attr);
        if (!rot_handle) {
                printf("rot handle create error\r\n");
                return -1;
        }

        IHal_SampleFB_Attr fb_attr;
        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb0");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = 720;
        fb_attr.frame_height = 1280;
        fb_attr.crop_x = 0;
        fb_attr.crop_y = 0;
        fb_attr.crop_w = 0;
        fb_attr.crop_h = 0;
        fb_attr.alpha = 255;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb0_handle = IHal_SampleFB_Init(&fb_attr);
        if (!fb0_handle) {
                printf("SampleFB init error\r\n");
                return -1;
        }
        IHal_SampleFB_SetSrcFrameSize(fb0_handle, 720, 1280);
        IHal_SampleFB_SetTargetFrameSize(fb0_handle, 720, 1280);
        IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        IHal_SampleFB_SetZorder(fb0_handle, Order_3);
        IHal_SampleFB_CompRestart(fb0_handle);

        // fill rotater buffers
        IMPP_BufferInfo_t share;
        for (int i = 0; i < attr.numSrcBuf; i++) {
                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_GetCameraBuffers(cam_handle, i, &share);
                IHal_Rot_SetExtSrcBuffer(rot_handle, &share, i);
        }

        for (int i = 0; i < attr.numDstBuf; i++) {
                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_SampleFB_GetMem(fb0_handle, &share);
                IHal_Rot_SetExtDstBuffer(rot_handle, &share, i);
        }
        IHal_CameraStart(cam_handle);
        pthread_create(&cam_thid, NULL, camera_rot_thread, NULL);

        signal(SIGINT, signal_handle);
        int ret = 0;
        IMPP_FrameInfo_t dst;
        IMPP_BufferInfo_t fb0_buf;
        while (1) {
                ret = IHal_Rot_GetFrame(rot_handle, &dst);
                if (!ret) {
                        fb0_buf.index = dst.index;
                        IHal_SampleFB_Update(fb0_handle, &fb0_buf);
                        IHal_Rot_ReleaseFrame(rot_handle, &dst);
                        // printf("display ok -----\r\n");
                } else {
                        usleep(10000);
                }
        }
        IHal_CameraStop(cam_handle);
        IHal_CameraClose(cam_handle);
        IHal_Rot_DestroyChan(rot_handle);
        IHal_SampleFB_DeInit(fb0_handle);
        return 0;
}
