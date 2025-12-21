/**
 * @brief 本用例是将Camera输出的图像数据经Rotater旋转90°之后再CSC转换为BGRA8888格式与一张720x1280的格式BGRA8888格式的图片经DPU进行OSD操作
 *        在进行叠加时将CSC输出的数据缩放为720x1080
 *        camera(1280x720)  ---->  Rotater 90°  ---->  CSC  ---->  Layer0(720x1080)  \
 *                                                                                    ---->  DPU OSD
 *        720x1280 BGRA8888图片  ------------------------------->  Layer1            /
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <icamera.h>
#include <ipu.h>
#include <rotater.h>
#include <dpu.h>
#include <rotater.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>

#define CAMERA_BUFFER_NUM 2

static IHAL_CameraHandle_t *camera_handle = NULL;
static IHAL_CAMERA_PARAMS   camera_params;
static int num_buffers = 0;
static IHal_CSC_Handle_t *csc_handle = NULL;
static IHal_Rot_Handle_t *rotater_handle = NULL;
static int uifd = -1;
static IHal_Dpu_InitStruct_t dpu_initStruct;
static IHal_Dpu_Handle_t *dpu_handle = NULL;
static IMPP_BufferInfo_t dpu_displaybuf;

pthread_t cam_rotater_thid = 0;
pthread_t rotater_csc_thid = 0;

void *camera_rot_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        IMPP_FrameInfo_t srcframe;

        while (1) {
                memset(&camera_buf, 0, sizeof(IMPP_BufferInfo_t));
                memset(&srcframe, 0, sizeof(IMPP_FrameInfo_t));

                ret = IHal_Camera_WaitBufferAvailable(camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_CameraDeQueueBuffer(camera_handle, &camera_buf);
                        srcframe.fd = camera_buf.fd;
                        srcframe.index = camera_buf.index;
                        ret = IHal_Rot_ProcessFrame(rotater_handle, &srcframe, ROT_PROCESS_ROTATE, ROT_ANGLE_90);
                        if (!ret) {
                                IHal_CameraQueuebuffer(camera_handle, &camera_buf);
                        }
                } else {
                        usleep(5000);
                }
        }
}

void *rotater_csc_thread(void *arg)
{
        int ret = 0;
        IMPP_FrameInfo_t dst;

        while (1) {
                memset(&dst, 0, sizeof(IMPP_FrameInfo_t));

                ret = IHal_Rot_GetFrame(rotater_handle, &dst);
                if (!ret) {
                        dst.fmt = IMPP_PIX_FMT_NV12;
                        IHal_CSC_FrameProcess(csc_handle, &dst);
                        IHal_Rot_ReleaseFrame(rotater_handle, &dst);
                } else {
                        usleep(10000);
                }
        }

}

static int signal_handle(int sig)
{
        pthread_cancel(cam_rotater_thid);
        pthread_join(cam_rotater_thid, NULL);
        pthread_cancel(rotater_csc_thid);
        pthread_join(rotater_csc_thid, NULL);

        IHal_CameraStop(camera_handle);

        IHal_Dpu_Deinit(dpu_handle);

        close(uifd);

        IHal_CSC_DestroyChan(csc_handle);

        IHal_Rot_DestroyChan(rotater_handle);

        IHal_CameraClose(camera_handle);

        printf("####################### end end ###############################\r\n");
        exit(0);
}

int main(int argc, char **argv)
{
        camera_handle = IHal_CameraOpen("/dev/video4");
        if (NULL == camera_handle) {
                printf("Camera open error!\r\n");
                return -1;
        }

        memset(&camera_params, 0, sizeof(IHAL_CAMERA_PARAMS));
        camera_params.imageWidth = 1280;
        camera_params.imageHeight = 720;
        camera_params.imageFmt = IMPP_PIX_FMT_NV12;
        camera_params.scale_out_crop_posx = 0;
        camera_params.scale_out_crop_posy = 0;
        camera_params.scale_out_crop_width = 0;
        camera_params.scale_out_crop_height = 0;
        IHal_CameraSetParams(camera_handle, &camera_params);

        num_buffers = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, CAMERA_BUFFER_NUM);
        if (num_buffers <= 0) {
                printf("Camera create buffers error!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        IHal_Rot_ChanAttr_t rotater_attr;
        memset(&rotater_attr, 0, sizeof(IHal_Rot_ChanAttr_t));
        rotater_attr.sWidth = camera_params.imageWidth;
        rotater_attr.sHeight = camera_params.imageHeight;
        rotater_attr.srcBufType = IMPP_EXT_DMABUFFER;
        rotater_attr.dstBufType = IMPP_INTERNAL_BUFFER;
        rotater_attr.numSrcBuf = num_buffers;
        rotater_attr.numDstBuf = num_buffers;
        rotater_attr.srcFmt = camera_params.imageFmt;
        rotater_handle = IHal_Rot_CreateChan(&rotater_attr);
        if (NULL == rotater_handle) {
                printf("Create rotater handle error!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        IHal_CSC_ChanAttr_t csc_chan_attr;
        memset(&csc_chan_attr, 0, sizeof(IHal_CSC_ChanAttr_t));
        csc_chan_attr.sWidth = rotater_attr.sHeight;
        csc_chan_attr.sHeight = rotater_attr.sWidth;
        csc_chan_attr.isSrcUseExtDmaBuf = 1;
        csc_chan_attr.isDstUseExtDmaBuf = 0;
        csc_chan_attr.numSrcBuf = num_buffers;
        csc_chan_attr.numDstBuf = num_buffers;
        csc_chan_attr.srcFmt = rotater_attr.srcFmt;
        csc_chan_attr.dstFmt = IMPP_PIX_FMT_BGRA_8888;
        csc_handle = IHal_CSC_ChanCreate(&csc_chan_attr);
        if (NULL == csc_handle) {
                printf("Create CSC handle error!\r\n");
                IHal_Rot_DestroyChan(rotater_handle);
                IHal_CameraClose(camera_handle);
                return -1;
        }

        IMPP_BufferInfo_t share;
        /* 共享Camera和Rotater的Buf : Camera ----> Rotater */
        for (int i = 0; i < num_buffers; i++) {
                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_GetCameraBuffers(camera_handle, i, &share);
                IHal_Rot_SetExtSrcBuffer(rotater_handle, &share, i);
        }

        /* 共享Rotater和CSC的Buf : Rotater ----> CSC */
        for (int i = 0; i < num_buffers; i++) {
                memset(&share, 0, sizeof(IMPP_BufferInfo_t));
                IHal_Rot_DstDmaBufGet(rotater_handle, &share, i);
                IHal_CSC_SetSrcExtBuf(csc_handle, &share, i);
        }

        uifd = open("ffmpeg_720x1280_bgra.rgb", O_RDWR);
        if (-1 == uifd) {
                printf("Open ui file error!\r\n");
                IHal_CSC_DestroyChan(csc_handle);
                IHal_Rot_DestroyChan(rotater_handle);
                IHal_CameraClose(camera_handle);
                return -1;
        }

        memset(&dpu_initStruct, 0, sizeof(IHal_Dpu_InitStruct_t));
        dpu_initStruct.num_outmem = 4;
        dpu_handle = IHal_Dpu_Init(&dpu_initStruct);
        if (NULL == dpu_handle) {
                printf("Init dpu error!\r\n");
                close(uifd);
                IHal_CSC_DestroyChan(csc_handle);
                IHal_Rot_DestroyChan(rotater_handle);
                IHal_CameraClose(camera_handle);
                return -1;
        }

        /* fill picture */
        IMPP_BufferInfo_t dpu_uibuf;
        memset(&dpu_uibuf, 0, sizeof(IMPP_BufferInfo_t));
        dpu_uibuf.vaddr = malloc(720 * 1280 * 4);
        read(uifd, dpu_uibuf.vaddr, 720 * 1280 * 4);
        lseek(uifd, 0, SEEK_SET);

        IHal_Dpu_FrameDesc_t dpu_frame;
        memset(&dpu_frame, 0, sizeof(IHal_Dpu_FrameDesc_t));
        dpu_frame.dpu_func = DPU_OSD_Func;
        dpu_frame.func_desc.osd_frame.osd_flags = Dpu_Layer0 | Dpu_Layer1;
        dpu_frame.func_desc.osd_frame.outWidth = 720;
        dpu_frame.func_desc.osd_frame.outHeight = 1280;
        dpu_frame.func_desc.osd_frame.wback_enable = 1;
        dpu_frame.func_desc.osd_frame.wback_fmt = IMPP_PIX_FMT_BGRA_8888;

        dpu_frame.func_desc.osd_frame.layer[0].srcFmt = IMPP_PIX_FMT_BGRA_8888;
        dpu_frame.func_desc.osd_frame.layer[0].srcWidth = 720;
        dpu_frame.func_desc.osd_frame.layer[0].srcHeight = 1280;
        dpu_frame.func_desc.osd_frame.layer[0].scale_enable = 1;
        dpu_frame.func_desc.osd_frame.layer[0].scaleWidth = 720;
        dpu_frame.func_desc.osd_frame.layer[0].scaleHeight = 1080;
        dpu_frame.func_desc.osd_frame.layer[0].srcCropx = 0;
        dpu_frame.func_desc.osd_frame.layer[0].srcCropy = 0;
        dpu_frame.func_desc.osd_frame.layer[0].srcCropw = 0;
        dpu_frame.func_desc.osd_frame.layer[0].srcCroph = 0;
        dpu_frame.func_desc.osd_frame.layer[0].osd_posX = 0;
        dpu_frame.func_desc.osd_frame.layer[0].osd_posY = 0;
        dpu_frame.func_desc.osd_frame.layer[0].osd_order = DPU_OSD_Order3;
        dpu_frame.func_desc.osd_frame.layer[0].alpha = 255;
        dpu_frame.func_desc.osd_frame.layer[0].paddr = 0; //layer0buf.paddr;
        dpu_frame.func_desc.osd_frame.layer[0].color_order = DPU_CFG_COLOR_RGB;
        dpu_frame.func_desc.osd_frame.layer[0].vaddr = 0;                                                  /*!< 使用TLB. */

        dpu_frame.func_desc.osd_frame.layer[1].srcFmt = IMPP_PIX_FMT_BGRA_8888;
        dpu_frame.func_desc.osd_frame.layer[1].srcWidth = 720;
        dpu_frame.func_desc.osd_frame.layer[1].srcHeight = 1280;
        dpu_frame.func_desc.osd_frame.layer[1].srcCropx = 0;
        dpu_frame.func_desc.osd_frame.layer[1].srcCropy = 0;
        dpu_frame.func_desc.osd_frame.layer[1].srcCropw = 0;
        dpu_frame.func_desc.osd_frame.layer[1].srcCroph = 0;
        dpu_frame.func_desc.osd_frame.layer[1].scale_enable = 0;
        dpu_frame.func_desc.osd_frame.layer[1].scaleWidth = 0;
        dpu_frame.func_desc.osd_frame.layer[1].scaleHeight = 0;
        dpu_frame.func_desc.osd_frame.layer[1].osd_posX = 0;
        dpu_frame.func_desc.osd_frame.layer[1].osd_posY = 0;
        dpu_frame.func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order2;
        dpu_frame.func_desc.osd_frame.layer[1].alpha = 255;
        dpu_frame.func_desc.osd_frame.layer[1].paddr = 0; //layer1buf.paddr;
        dpu_frame.func_desc.osd_frame.layer[1].color_order = DPU_CFG_COLOR_RGB;
        dpu_frame.func_desc.osd_frame.layer[1].vaddr = dpu_uibuf.vaddr;                                    /*!< 使用TLB */

        IHal_CameraStart(camera_handle);

        pthread_create(&cam_rotater_thid, NULL, camera_rot_thread,  NULL);
        pthread_create(&rotater_csc_thid, NULL, rotater_csc_thread,  NULL);
        // signal(SIGINT, signal_handle);

        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        IMPP_FrameInfo_t  rot_srcframe;
        IMPP_FrameInfo_t  rot_dstframe;
        IMPP_FrameInfo_t  csc_dframe;
        while (1) {
                memset(&csc_dframe, 0, sizeof(IMPP_FrameInfo_t));

                ret = IHal_CSC_GetDstFrame(csc_handle, &csc_dframe, IMPP_WAIT_FOREVER);
                dpu_frame.func_desc.osd_frame.layer[0].vaddr = csc_dframe.vaddr;                          /*!< 使用TLB. */
                dpu_frame.func_desc.osd_frame.layer[1].vaddr = dpu_uibuf.vaddr;                           /*!< 使用TLB */

                /* 从显示获取一个Frame Buffer.*/
                IHal_Dpu_RDMA_GetFrame(dpu_handle, &dpu_displaybuf);

                /* 设置DPU 写回的Buffer 为显存，一旦更新绘图之后，可以直接显示 */
                IHal_Dpu_Composer_Set_WbackBuffers(dpu_handle, &dpu_displaybuf, dpu_displaybuf.index);

                /* 更新绘图 */
                IHal_Dpu_Composer_Process(dpu_handle, &dpu_frame);

                /* 上屏显示 */
                IHal_Dpu_RDMA_PutFrame(dpu_handle, &dpu_displaybuf);

                IHal_CSC_ReleaseDstFrame(csc_handle, &csc_dframe);
        }

        IHal_CameraStop(camera_handle);

        close(uifd);

        IHal_Dpu_Deinit(dpu_handle);

        IHal_CSC_DestroyChan(csc_handle);

        IHal_Rot_DestroyChan(rotater_handle);

        IHal_CameraClose(camera_handle);

        return 0;
}
