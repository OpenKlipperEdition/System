#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
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
#include <icamera.h>
#include <dpu.h>

#define CAMERA_BUFFERS 2

static unsigned long long get_time_ms(void)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);

        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int calc_fps(void)
{

        static unsigned long long last_time = 0;
        static int framecount = 0;
        static int fps = 0;
        unsigned long long cur_time = get_time_ms();

        if (last_time == 0) {
                last_time = get_time_ms();
        }

        framecount ++;
        if (cur_time - last_time >= 1000) {
                fps = framecount;
                framecount = 0;
                last_time = cur_time;
                printf("fps:%d\n", fps);
        }

        return fps;
}

static int dpu_frame_desc_init(IHal_Dpu_FrameDesc_t *framedesc)
{

        memset(framedesc, 0, sizeof(IHal_Dpu_FrameDesc_t));

        framedesc->dpu_func = DPU_OSD_Func;
        framedesc->func_desc.osd_frame.osd_flags = Dpu_Layer0 | Dpu_Layer1;
        framedesc->func_desc.osd_frame.outWidth = 720;
        framedesc->func_desc.osd_frame.outHeight = 1280;
        framedesc->func_desc.osd_frame.wback_enable = 1;
        framedesc->func_desc.osd_frame.wback_fmt = IMPP_PIX_FMT_BGRA_8888;

        framedesc->func_desc.osd_frame.layer[0].srcFmt = IMPP_PIX_FMT_BGRA_8888;
        framedesc->func_desc.osd_frame.layer[0].srcWidth = 720;
        framedesc->func_desc.osd_frame.layer[0].srcHeight = 1280;
        framedesc->func_desc.osd_frame.layer[0].scale_enable = 1;
        framedesc->func_desc.osd_frame.layer[0].scaleWidth = 720;
        framedesc->func_desc.osd_frame.layer[0].scaleHeight = 1280;
        framedesc->func_desc.osd_frame.layer[0].srcCropx = 200;
        framedesc->func_desc.osd_frame.layer[0].srcCropy = 600;
        framedesc->func_desc.osd_frame.layer[0].srcCropw = 500;
        framedesc->func_desc.osd_frame.layer[0].srcCroph = 500;
        framedesc->func_desc.osd_frame.layer[0].osd_posX = 0;
        framedesc->func_desc.osd_frame.layer[0].osd_posY = 0;
        framedesc->func_desc.osd_frame.layer[0].osd_order = DPU_OSD_Order1;
        framedesc->func_desc.osd_frame.layer[0].alpha = 255;
        framedesc->func_desc.osd_frame.layer[0].paddr = 0; //layer0buf.paddr;
        framedesc->func_desc.osd_frame.layer[0].color_order = DPU_CFG_COLOR_RGB;
        framedesc->func_desc.osd_frame.layer[0].vaddr = 0;     /*!< 使用TLB. */

        framedesc->func_desc.osd_frame.layer[1].srcFmt = IMPP_PIX_FMT_NV12;
        framedesc->func_desc.osd_frame.layer[1].srcWidth = 640;
        framedesc->func_desc.osd_frame.layer[1].srcHeight = 480;
        framedesc->func_desc.osd_frame.layer[1].srcCropx = 0;
        framedesc->func_desc.osd_frame.layer[1].srcCropy = 0;
        framedesc->func_desc.osd_frame.layer[1].srcCropw = 0;
        framedesc->func_desc.osd_frame.layer[1].srcCroph = 0;
        framedesc->func_desc.osd_frame.layer[1].scale_enable = 1;
        framedesc->func_desc.osd_frame.layer[1].scaleWidth = 320;
        framedesc->func_desc.osd_frame.layer[1].scaleHeight = 240;
        framedesc->func_desc.osd_frame.layer[1].osd_posX = 100;
        framedesc->func_desc.osd_frame.layer[1].osd_posY = 200;
        framedesc->func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order3;
        framedesc->func_desc.osd_frame.layer[1].alpha = 200;
        framedesc->func_desc.osd_frame.layer[1].paddr = 0; //layer1buf.paddr;
        framedesc->func_desc.osd_frame.layer[1].color_order = DPU_CFG_COLOR_RGB;
        framedesc->func_desc.osd_frame.layer[1].vaddr = 0;

        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;

        IHAL_CameraHandle_t *handle = IHal_CameraOpen("/dev/video4");
        if (!handle) {
                return -1;
        }

        IHAL_CameraHandle_t *handle1 = IHal_CameraOpen("/dev/video8");
        if (!handle1) {
                goto close_camera0;
        }

        IHAL_CAMERA_PARAMS cam_params0;
        memset(&cam_params0, 0, sizeof(IHAL_CAMERA_PARAMS));
        cam_params0.imageWidth = 720;
        cam_params0.imageHeight = 1280;
        cam_params0.imageFmt = IMPP_PIX_FMT_NV12;
        ret = IHal_CameraSetParams(handle, &cam_params0);
        if (ret) {
                goto close_camera1;
        }

        IHAL_CAMERA_PARAMS cam_params1;
        memset(&cam_params1, 0, sizeof(IHAL_CAMERA_PARAMS));
        cam_params1.imageWidth = 1280;
        cam_params1.imageHeight = 720;
        cam_params1.imageFmt = IMPP_PIX_FMT_NV12;
        ret = IHal_CameraSetParams(handle1, &cam_params1);
        if (ret) {
                goto close_camera1;
        }

        ret = IHal_CameraCreateBuffers(handle, IMPP_INTERNAL_BUFFER, CAMERA_BUFFERS);
        if (ret < 1) {
                goto close_camera1;
        }

        ret = IHal_CameraCreateBuffers(handle1, IMPP_INTERNAL_BUFFER, CAMERA_BUFFERS);
        if (ret < 1) {
                goto close_camera1;
        }

        ret = IHal_CameraStart(handle);
        if (ret != IHAL_ROK) {
                goto close_camera1;
        }

        ret = IHal_CameraStart(handle1);
        if (ret != IHAL_ROK) {
                goto stop_camera0;
        }

        IHal_Dpu_InitStruct_t initStruct;
        initStruct.num_outmem = 3;
        IHal_Dpu_Handle_t *dpuHandle = IHal_Dpu_Init(&initStruct);
        IMPP_BufferInfo_t displaybuf;
        IHal_Dpu_FrameDesc_t frame;

        dpu_frame_desc_init(&frame);

        IMPP_BufferInfo_t camerabuf;
        IMPP_BufferInfo_t camerabuf1;
        while (1) {
                IHal_Camera_WaitBufferAvailable(handle, IMPP_WAIT_FOREVER);
                IHal_Camera_WaitBufferAvailable(handle1, IMPP_WAIT_FOREVER);
                ret = IHal_CameraDeQueueBuffer(handle, &camerabuf);
                if (ret != IHAL_ROK) {
                        break;
                }

                ret = IHal_CameraDeQueueBuffer(handle1, &camerabuf1);
                if (ret != IHAL_ROK) {
                        break;
                }

                calc_fps();

                frame.func_desc.osd_frame.layer[0].vaddr = camerabuf.vaddr;     /*!< 使用TLB. */
                frame.func_desc.osd_frame.layer[0].uv_vaddr = (camerabuf.vaddr + cam_params0.imageWidth * cam_params0.imageHeight);     /*!< 使用TLB. */
                frame.func_desc.osd_frame.layer[0].srcFmt = cam_params0.imageFmt;
                frame.func_desc.osd_frame.layer[0].srcWidth = cam_params0.imageWidth;
                frame.func_desc.osd_frame.layer[0].srcHeight = cam_params0.imageHeight;

                frame.func_desc.osd_frame.layer[1].vaddr = camerabuf1.vaddr;     /*!< 使用TLB. */
                frame.func_desc.osd_frame.layer[1].uv_vaddr = (camerabuf1.vaddr + cam_params1.imageWidth * cam_params1.imageHeight);     /*!< 使用TLB. */
                frame.func_desc.osd_frame.layer[1].srcFmt = cam_params1.imageFmt;
                frame.func_desc.osd_frame.layer[1].srcWidth = cam_params1.imageWidth;
                frame.func_desc.osd_frame.layer[1].srcHeight = cam_params1.imageHeight;

                /* 从显示获取一个Frame Buffer.*/
                IHal_Dpu_RDMA_GetFrame(dpuHandle, &displaybuf);

                /* 设置DPU 写回的Buffer 为显存，一旦更新绘图之后，可以直接显示.*/
                IHal_Dpu_Composer_Set_WbackBuffers(dpuHandle, &displaybuf, displaybuf.index);

                /*更新绘图.*/
                IHal_Dpu_Composer_Process(dpuHandle, &frame);

                /*上屏显示.*/
                IHal_Dpu_RDMA_PutFrame(dpuHandle, &displaybuf);

                ret = IHal_CameraQueuebuffer(handle, &camerabuf);
                if (ret != IHAL_ROK) {
                        break;
                }

                ret = IHal_CameraQueuebuffer(handle1, &camerabuf1);
                if (ret != IHAL_ROK) {
                        break;
                }
        }

        IHal_CameraStop(handle);
        IHal_CameraStop(handle1);
        IHal_CameraClose(handle);
        IHal_CameraClose(handle1);

        return 0;

stop_camera0:
        IHal_CameraStop(handle);
close_camera1:
        IHal_CameraClose(handle1);
close_camera0:
        IHal_CameraClose(handle);

        return -1;
}
