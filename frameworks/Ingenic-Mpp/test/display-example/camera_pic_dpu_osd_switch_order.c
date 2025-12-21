#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <icamera.h>
#include <dpu.h>
#include <stdlib.h>

#define CAMERA_BUFFER_NUM 3

static IHAL_CameraHandle_t *camera_handle = NULL;
static IHAL_CAMERA_PARAMS   camera_params;
static int num_buffers = 0;
static int uifd = -1;
static IHal_Dpu_InitStruct_t dpu_initStruct;
static IHal_Dpu_Handle_t *dpu_handle = NULL;
static IMPP_BufferInfo_t dpu_displaybuf;

static int signal_handle(int sig)
{
        IHal_CameraStop(camera_handle);

        IHal_Dpu_Deinit(dpu_handle);

        close(uifd);

        IHal_CameraClose(camera_handle);

        printf("###################################### exit ####################################\r\n");
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
        camera_params.imageWidth = 720;
        camera_params.imageHeight = 1280;
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

        uifd = open("640x480.rgb565", O_RDWR);
        if (-1 == uifd) {
                printf("Open ui file error!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        memset(&dpu_initStruct, 0, sizeof(IHal_Dpu_InitStruct_t));
        dpu_initStruct.num_outmem = 4;
        dpu_handle = IHal_Dpu_Init(&dpu_initStruct);
        if (NULL == dpu_handle) {
                printf("Init dpu error!\r\n");
                close(uifd);
                IHal_CameraClose(camera_handle);
                return -1;
        }

        /* fill picture */
        IMPP_BufferInfo_t dpu_uibuf;
        memset(&dpu_uibuf, 0, sizeof(IMPP_BufferInfo_t));
        dpu_uibuf.vaddr = malloc(640 * 480 * 2);
        read(uifd, dpu_uibuf.vaddr, 640 * 480 * 2);
        lseek(uifd, 0, SEEK_SET);

        IHal_Dpu_FrameDesc_t dpu_frame;
        memset(&dpu_frame, 0, sizeof(IHal_Dpu_FrameDesc_t));
        dpu_frame.dpu_func = DPU_OSD_Func;
        dpu_frame.func_desc.osd_frame.osd_flags = Dpu_Layer0 | Dpu_Layer1;
        dpu_frame.func_desc.osd_frame.outWidth = 720;
        dpu_frame.func_desc.osd_frame.outHeight = 1280;
        dpu_frame.func_desc.osd_frame.wback_enable = 1;
        dpu_frame.func_desc.osd_frame.wback_fmt = IMPP_PIX_FMT_BGRA_8888;

        dpu_frame.func_desc.osd_frame.layer[0].srcFmt = IMPP_PIX_FMT_NV12;                          /* Camera出的数据NV12 */
        dpu_frame.func_desc.osd_frame.layer[0].srcWidth = 720;
        dpu_frame.func_desc.osd_frame.layer[0].srcHeight = 1280;
        dpu_frame.func_desc.osd_frame.layer[0].scale_enable = 1;
        dpu_frame.func_desc.osd_frame.layer[0].scaleWidth = 720;
        dpu_frame.func_desc.osd_frame.layer[0].scaleHeight = 1280;
        dpu_frame.func_desc.osd_frame.layer[0].srcCropx = 0;
        dpu_frame.func_desc.osd_frame.layer[0].srcCropy = 0;
        dpu_frame.func_desc.osd_frame.layer[0].srcCropw = 0;
        dpu_frame.func_desc.osd_frame.layer[0].srcCroph = 0;
        dpu_frame.func_desc.osd_frame.layer[0].osd_posX = 0;
        dpu_frame.func_desc.osd_frame.layer[0].osd_posY = 0;
        dpu_frame.func_desc.osd_frame.layer[0].osd_order = DPU_OSD_Order1;
        dpu_frame.func_desc.osd_frame.layer[0].alpha = 255;
        dpu_frame.func_desc.osd_frame.layer[0].paddr = 0; //layer0buf.paddr;
        dpu_frame.func_desc.osd_frame.layer[0].color_order = DPU_CFG_COLOR_RGB;
        dpu_frame.func_desc.osd_frame.layer[0].vaddr = 0;                                           /*!< 使用TLB. */

        dpu_frame.func_desc.osd_frame.layer[1].srcFmt = IMPP_PIX_FMT_RGB_565;
        dpu_frame.func_desc.osd_frame.layer[1].srcWidth = 640;
        dpu_frame.func_desc.osd_frame.layer[1].srcHeight = 480;
        dpu_frame.func_desc.osd_frame.layer[1].srcCropx = 0;
        dpu_frame.func_desc.osd_frame.layer[1].srcCropy = 0;
        dpu_frame.func_desc.osd_frame.layer[1].srcCropw = 0;
        dpu_frame.func_desc.osd_frame.layer[1].srcCroph = 0;
        dpu_frame.func_desc.osd_frame.layer[1].scale_enable = 0;
        dpu_frame.func_desc.osd_frame.layer[1].scaleWidth = 0;
        dpu_frame.func_desc.osd_frame.layer[1].scaleHeight = 0;
        dpu_frame.func_desc.osd_frame.layer[1].osd_posX = 50;
        dpu_frame.func_desc.osd_frame.layer[1].osd_posY = 200;
        dpu_frame.func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order3;
        dpu_frame.func_desc.osd_frame.layer[1].alpha = 255;
        dpu_frame.func_desc.osd_frame.layer[1].paddr = 0; //layer1buf.paddr;
        dpu_frame.func_desc.osd_frame.layer[1].color_order = DPU_CFG_COLOR_RGB;
        dpu_frame.func_desc.osd_frame.layer[1].vaddr = dpu_uibuf.vaddr;                              /*!< 使用TLB */

        IHal_CameraStart(camera_handle);

        signal(SIGINT, signal_handle);

        int ret = 0;
        struct timeval start;
        struct timeval end;
        int toggle_flag = 0;                                                                            /* 切换标志位，0 : 表示未切换、1 : 表示切换过 */
        memset(&start, 0, sizeof(struct timeval));
        memset(&end, 0, sizeof(struct timeval));
        IMPP_BufferInfo_t camerabuf;
        while (1) {
                memset(&camerabuf, 0, sizeof(IMPP_BufferInfo_t));

                if (0 == start.tv_sec && 0 == start.tv_usec) {
                        gettimeofday(&start, NULL);
                }

                /* 等待Camera数据有效 */
                ret = IHal_Camera_WaitBufferAvailable(camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        /* 从视频缓存队列中取出一个已填充数据的Camera数据缓冲区 */
                        IHal_CameraDeQueueBuffer(camera_handle, &camerabuf);

                        dpu_frame.func_desc.osd_frame.layer[0].vaddr = camerabuf.vaddr;                          /*!< 使用TLB. */
                        dpu_frame.func_desc.osd_frame.layer[0].uv_vaddr = (camerabuf.vaddr + camera_params.imageWidth * camera_params.imageHeight);                          /*!< 使用TLB. */
                        dpu_frame.func_desc.osd_frame.layer[1].vaddr = dpu_uibuf.vaddr;                              /*!< 使用TLB */

                        gettimeofday(&end, NULL);
                        /* 每2s互换一次order */
                        if ((2 * 1000) <= ((end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000))) {
                                if (0 == toggle_flag) {
                                        dpu_frame.func_desc.osd_frame.layer[0].osd_order = DPU_OSD_Order3;
                                        dpu_frame.func_desc.osd_frame.layer[0].alpha = 150;
                                        dpu_frame.func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order1;
                                        toggle_flag = 1;
                                } else {
                                        dpu_frame.func_desc.osd_frame.layer[0].osd_order = DPU_OSD_Order1;
                                        dpu_frame.func_desc.osd_frame.layer[0].alpha = 255;
                                        dpu_frame.func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order3;
                                        toggle_flag = 0;
                                }
                                memset(&start, 0, sizeof(struct timeval));
                        }

                        /* 从显示获取一个Frame Buffer.*/
                        IHal_Dpu_RDMA_GetFrame(dpu_handle, &dpu_displaybuf);

                        /* 设置DPU 写回的Buffer 为显存，一旦更新绘图之后，可以直接显示 */
                        IHal_Dpu_Composer_Set_WbackBuffers(dpu_handle, &dpu_displaybuf, dpu_displaybuf.index);

                        /* 更新绘图 */
                        IHal_Dpu_Composer_Process(dpu_handle, &dpu_frame);

                        /* 上屏显示 */
                        IHal_Dpu_RDMA_PutFrame(dpu_handle, &dpu_displaybuf);

                        /* 将Camera数据缓冲区放回到视频缓存队列中 */
                        IHal_CameraQueuebuffer(camera_handle, &camerabuf);
                } else {
                        usleep(5000);
                }
        }

        IHal_CameraStop(camera_handle);

        IHal_Dpu_Deinit(dpu_handle);

        close(uifd);

        IHal_CameraClose(camera_handle);

        printf("===========================================================================\r\n");
        return 0;
}
