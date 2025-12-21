/**
 * 【注意事项】
 *       在使用这个示例时，需要将内核中的CONFIG_FB_INGENIC_NR_FRAMES=1修改为2或3，设备树的也应进行修改，因为这个和设备树取最小的frames
 *       CONFIG_FB_INGENIC_NR_FRAMES=1时流转不起来，所以测试时现象有问题
 */
#include <stdio.h>
#include <string.h>
#include <icamera.h>
#include <ipu.h>
#include <display.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>


#define CAMERA_BUFFER_NUM 3

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

int main(int argc, char **argv)
{
        int ret = 0;
        IHAL_CameraHandle_t *camera_handle = NULL;
        IHal_OSD_Handle_t *osd_handle = NULL;
        IHal_SampleFB_Handle_t *fb0_handle = NULL;
        int osd_chn0fd = -1;
        IMPP_BufferInfo_t osd_chn0buf;
        IHAL_CAMERA_PARAMS   camera_params;
        int num_buffers = 0;
        IHal_OSD_ChanAttr_t osd_chan_attr;
        IHal_OSD_FrameDesc_t osd_inputFrame;
        IPU_DmaBuf_SyncInfo_t syncinfo;
        IMPP_FrameInfo_t osd_outputFrame;
        unsigned int osd_chn0size = 0;
        IMPP_BufferInfo_t camera_buf;
        IHal_SampleFB_Attr fb_attr;
        IMPP_BufferInfo_t fb0_buf;

        camera_handle = IHal_CameraOpen("/dev/video4");
        if (NULL == camera_handle) {
                printf("Camera open error!\r\n");
                return -1;
        }

        memset(&camera_params, 0, sizeof(IHAL_CAMERA_PARAMS));
        camera_params.imageWidth = 1920;
        camera_params.imageHeight = 1080;
        camera_params.imageFmt = IMPP_PIX_FMT_NV12;
        camera_params.scale_out_crop_posx = 0;
        camera_params.scale_out_crop_posy = 0;
        camera_params.scale_out_crop_width = 720;                               /* 裁剪后进行缩放 */
        camera_params.scale_out_crop_height = 1080;
        IHal_CameraSetParams(camera_handle, &camera_params);

        num_buffers = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, CAMERA_BUFFER_NUM);
        if (num_buffers <= 0) {
                printf("Camera create buffers error!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        memset(&osd_chan_attr, 0, sizeof(osd_chan_attr));
        osd_chan_attr.maxFifoNum = 1000;
        osd_handle = IHal_OSD_ChanCreate(&osd_chan_attr);
        if (NULL == osd_handle) {
                printf("Create osd chan error!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        osd_chn0size = 720 * 1280 * 3 / 2;
        memset(&osd_chn0buf, 0, sizeof(osd_chn0buf));
        IHal_OSD_CHx_BufCreate(osd_handle, &osd_chn0buf, osd_chn0size);

        osd_chn0fd = open("720x1280.nv12", O_RDWR);
        if (-1 == osd_chn0fd) {
                printf("Failed to open the image!\r\n");
                IHal_OSD_CHx_BufFree(osd_handle, &osd_chn0buf);
                IHal_OSD_DestroyChan(osd_handle);
                IHal_CameraClose(camera_handle);
                return -1;
        }
        read(osd_chn0fd, (void *)osd_chn0buf.vaddr, osd_chn0size);

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
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;       /* OSD输出格式为NV12 */
        fb0_handle = IHal_SampleFB_Init(&fb_attr);
        if (!fb0_handle) {
                printf("SampleFB Init failed!\r\n");
                close(osd_chn0fd);
                IHal_OSD_CHx_BufFree(osd_handle, &osd_chn0buf);
                IHal_OSD_DestroyChan(osd_handle);
                IHal_CameraClose(camera_handle);
                return -1;
        }

        IHal_SampleFB_SetSrcFrameSize(fb0_handle, 720, 1280);
        IHal_SampleFB_SetTargetFrameSize(fb0_handle, 720, 1280);
        IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        IHal_SampleFB_SetZorder(fb0_handle, Order_3);
        IHal_SampleFB_CompRestart(fb0_handle);

        IHal_CameraStart(camera_handle);

        for (int i = 0; i < 1000; i++) {
                ret = IHal_Camera_WaitBufferAvailable(camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        memset(&camera_buf, 0, sizeof(IMPP_BufferInfo_t));

                        IHal_CameraDeQueueBuffer(camera_handle, &camera_buf);

                        IHal_OSD_ImportDmaBuf(osd_handle, &camera_buf);

                        /* OSD DMA-BUF Cache刷新 */
                        memset(&syncinfo, 0, sizeof(syncinfo));
                        syncinfo.fd = osd_chn0buf.fd;
                        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
                        IHal_OSD_FlushCache(osd_handle, &syncinfo);
                        memset(&syncinfo, 0, sizeof(syncinfo));
                        syncinfo.fd = camera_buf.fd;
                        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
                        IHal_OSD_FlushCache(osd_handle, &syncinfo);

                        memset(&fb0_buf, 0, sizeof(IMPP_BufferInfo_t));
                        IHal_SampleFB_GetMem(fb0_handle, &fb0_buf);

                        memset(&osd_inputFrame, 0, sizeof(osd_inputFrame));
                        osd_inputFrame.osd_flags = OSD_CH0_EN | OSD_CH1_EN;
                        osd_inputFrame.bgAttr.width = 720;
                        osd_inputFrame.bgAttr.height = 1280;
                        osd_inputFrame.bgAttr.paddr = fb0_buf.paddr;
                        osd_inputFrame.bgAttr.vaddr = fb0_buf.vaddr;
                        osd_inputFrame.bgAttr.fmt = IMPP_PIX_FMT_NV12;
                        /* 设置通道0的属性 : 叠加图片的图像数据 */
                        osd_inputFrame.chAttr[0].width = 720;
                        osd_inputFrame.chAttr[0].height = 1280;
                        osd_inputFrame.chAttr[0].alpha = 255;
                        osd_inputFrame.chAttr[0].posX = 0;
                        osd_inputFrame.chAttr[0].posY = 0;
                        osd_inputFrame.chAttr[0].paddr = osd_chn0buf.paddr;
                        osd_inputFrame.chAttr[0].fmt = IMPP_PIX_FMT_NV12;
                        /* 设置通道1的属性 : 叠加Camera的图像数据 */
                        osd_inputFrame.chAttr[1].width = camera_params.scale_out_crop_width;              /* Camera缩放后的宽 : 720 */
                        osd_inputFrame.chAttr[1].height = camera_params.scale_out_crop_height;            /* Camera缩放后的高 : 1080 */
                        osd_inputFrame.chAttr[1].alpha = 255;
                        osd_inputFrame.chAttr[1].posX = 0;
                        osd_inputFrame.chAttr[1].posY = 0;
                        osd_inputFrame.chAttr[1].paddr = camera_buf.paddr;                                /* Camera_buf的Paddr */
                        osd_inputFrame.chAttr[1].fmt = camera_params.imageFmt;

                        /* 对一帧图像数据进行图像叠加处理 */
                        memset(&osd_outputFrame, 0, sizeof(osd_outputFrame));
                        IHal_OSD_ProcessFrame(osd_handle, &osd_inputFrame, &osd_outputFrame);

                        /* 刷新FrameBuffer显示数据 */
                        IHal_SampleFB_Update(fb0_handle, &fb0_buf);

                        calc_fps();

                        /* 将Camera数据缓冲区放回到视频缓存队列中  */
                        IHal_CameraQueuebuffer(camera_handle, &camera_buf);
                } else {
                        usleep(5000);
                }
        }

        IHal_SampleFB_DeInit(fb0_handle);

        IHal_CameraStop(camera_handle);

        close(osd_chn0fd);

        IHal_OSD_CHx_BufFree(osd_handle, &osd_chn0buf);

        IHal_OSD_DestroyChan(osd_handle);

        IHal_CameraClose(camera_handle);

        return 0;
}
