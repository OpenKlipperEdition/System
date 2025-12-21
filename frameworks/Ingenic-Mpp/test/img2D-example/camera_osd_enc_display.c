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
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>

#include "log.h"
#include "icamera.h"
#include "codec.h"
#include "display.h"
#include "ipu.h"


#define UI_PICTURE0     "/mnt/x2500/720x1280-3.rgba"
#define UI_PICTURE1     "/mnt/x2500/720x1280-2.rgba"
//#define UI_PICTURE1   "/mnt/x2500/720pR90.nv12"

#define UI_WIDTH        720
#define UI_HEIGHT       1280

#define CAM_WIDTH       640
#define CAM_HEIGHT      480
#define CAM_NODE        "/dev/video4"



static pthread_t osd_server_tid;
static pthread_t enc_tid;
static pthread_t camera_tid;
static pthread_t ui_write_tid;
static pthread_t lcd_update_tid;


static IMPP_BufferInfo_t osd_out[3];    // NV12
static IMPP_BufferInfo_t osd_chn0[3];   // rgba
static IMPP_BufferInfo_t osd_chn1[3];   // import from camera dmabuf ---> NV12
static IMPP_BufferInfo_t cam_buf[3];
static IMPP_BufferInfo_t fb0_buf[2];

static impp_fifo_t osd_src_fifo_ui;
static impp_fifo_t osd_src_fifo_camera;
static impp_fifo_t osd_dst_fifo;
static impp_fifo_t encoder_src_fifo;
static impp_fifo_t ui_src_fifo;
static impp_fifo_t fb_buffer_fifo;

static IHAL_CameraHandle_t *camera_handle = NULL;
static IHal_CodecHandle_t  *encoder_handle = NULL;
static IHal_OSD_Handle_t   *osd_handle = NULL;
static IHal_SampleFB_Handle_t *fb0_handle = NULL;

static unsigned int fb_cnt = 0;

static int uifd0 = -1;
static int uifd1 = -1;
static int savefd;

static char *ui_src = NULL;

static int signal_handle(int sig);

double get_wall_time()
{
        struct timeval time;
        if (gettimeofday(&time, NULL)) {
                return 0;
        }
        return (double)time.tv_sec * 1000.0 + (double)time.tv_usec / 1000.0;
}

void *lcd_update_thread(void *arg)
{
        IMPP_BufferInfo_t *fb;
        for (;;) {
                fb = impp_fifo_dequeue(&fb_buffer_fifo, IMPP_WAIT_FOREVER);
                IHal_SampleFB_Update(fb0_handle, fb);
        }
}

void *osd_server_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t *src_ui;
        IMPP_BufferInfo_t *src_cam;
        IMPP_BufferInfo_t *osd_out;
        IHal_OSD_FrameDesc_t frame;
        IMPP_FrameInfo_t outframe;
        for (;;) {
                src_ui = impp_fifo_dequeue(&osd_src_fifo_ui, IMPP_WAIT_FOREVER);
                src_cam = impp_fifo_dequeue(&osd_src_fifo_camera, IMPP_WAIT_FOREVER);
                osd_out = impp_fifo_dequeue(&osd_dst_fifo, IMPP_WAIT_FOREVER);

                //printf("src_ui paddr = 0x%x \r\n",src_ui->paddr);
                //printf("src_cam paddr = 0x%x index = %d \r\n",src_cam->paddr,src_cam->index);
                //printf("osd_out paddr = 0x%x \r\n",osd_out->paddr);
#if 1
                frame.osd_flags = OSD_CH0_EN | OSD_CH1_EN;// | OSD_CH2_EN | OSD_CH3_EN;
                frame.bgAttr.width = UI_WIDTH;
                frame.bgAttr.height = UI_HEIGHT;
                frame.bgAttr.paddr = osd_out->paddr;   // bg is output addr
                frame.bgAttr.vaddr = osd_out->vaddr;
                frame.bgAttr.fmt = IMPP_PIX_FMT_NV12;   //最终输出格式

                frame.chAttr[0].width = UI_WIDTH;
                frame.chAttr[0].height = UI_HEIGHT;
                frame.chAttr[0].alpha = 128;
                frame.chAttr[0].posX = 0;
                frame.chAttr[0].posY = 0;
                frame.chAttr[0].paddr = src_ui->paddr;
                frame.chAttr[0].fmt = IMPP_PIX_FMT_RGBA_8888;
                //frame.chAttr[0].fmt = IMPP_PIX_FMT_NV12;

                frame.chAttr[1].width = CAM_WIDTH;
                frame.chAttr[1].height = CAM_HEIGHT;
                frame.chAttr[1].alpha = 255;
                frame.chAttr[1].posX = 0;
                frame.chAttr[1].posY = 0;
                frame.chAttr[1].paddr = src_cam->paddr;
                frame.chAttr[1].fmt = IMPP_PIX_FMT_NV12;
#endif
                ret = IHal_OSD_ProcessFrame(osd_handle, &frame, &outframe);
                if (ret) {
                        printf("osd process frame failed \r\n");
                }
                IHal_CameraQueuebuffer(camera_handle, src_cam);
                //printf("%s %d #############\r\n",__func__,__LINE__);
                // 传输数据到framebuffer
                /* IHal_SampleFB_GetMem(fb0_handle,&fb0_buf[0]); */
                /* memcpy(fb0_buf[0].vaddr,outframe.vaddr,outframe.size); */
#if 1
                IHal_SampleFB_GetMem(fb0_handle, &fb0_buf[fb_cnt % 2]);
                frame.osd_flags = OSD_CH0_EN ;//| OSD_CH1_EN;// | OSD_CH2_EN | OSD_CH3_EN;
                frame.bgAttr.width = UI_WIDTH;
                frame.bgAttr.height = UI_HEIGHT;
                frame.bgAttr.paddr = fb0_buf[fb_cnt % 2].paddr;   // bg is output addr
                frame.bgAttr.vaddr = fb0_buf[fb_cnt % 2].vaddr;
                frame.bgAttr.fmt = IMPP_PIX_FMT_NV12;   //最终输出格式

                frame.chAttr[0].width = UI_WIDTH;
                frame.chAttr[0].height = UI_HEIGHT;
                frame.chAttr[0].alpha = 255;
                frame.chAttr[0].posX = 0;
                frame.chAttr[0].posY = 0;
                frame.chAttr[0].paddr = osd_out->paddr;
                frame.chAttr[0].fmt = IMPP_PIX_FMT_NV12;
                ret = IHal_OSD_ProcessFrame(osd_handle, &frame, &outframe);
                if (ret) {
                        printf("osd transmit data process frame failed \r\n");
                }
#endif
                impp_fifo_queue(&fb_buffer_fifo, &fb0_buf[fb_cnt % 2], IMPP_WAIT_FOREVER);
                fb_cnt++;
                impp_fifo_queue(&ui_src_fifo, src_ui, IMPP_WAIT_FOREVER);
                //impp_fifo_queue(&encoder_src_fifo,osd_out,IMPP_WAIT_FOREVER);
                //impp_fifo_queue(&osd_dst_fifo,osd_out,IMPP_WAIT_FOREVER);
                IHal_Codec_QueueSrcBuffer(encoder_handle, osd_out);

        }
}

void *encoder_thread(void *arg)
{
        IMPP_BufferInfo_t enc_src_buf;
        int ret = 0;
        for (;;) {
                ret = IHal_Codec_WaitSrcAvailable(encoder_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueSrcBuffer(encoder_handle, &enc_src_buf);
                        impp_fifo_queue(&osd_dst_fifo, &osd_out[enc_src_buf.index], IMPP_WAIT_FOREVER);
                }
        }

}

void *camera_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        IHal_CameraStart(camera_handle);
        for (;;) {
                ret = IHal_Camera_WaitBufferAvailable(camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        // dequeue buffer from camera
                        IHal_CameraDeQueueBuffer(camera_handle, &camera_buf);
                        //printf("get camera buffer ok !!!\r\n");
                        // add buffer to osd src fifo
                        impp_fifo_queue(&osd_src_fifo_camera, &osd_chn1[camera_buf.index], IMPP_WAIT_FOREVER);
                } else {
                        usleep(15000);
                }
        }
}

void *ui_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t *ui_buf;
        unsigned int cnt = 0;

        int src_flag = 0;
        double start_time;
        double end_time;
        IPU_DmaBuf_SyncInfo_t sync;
        for (;;) {
                ui_buf = (IMPP_BufferInfo_t *)impp_fifo_dequeue(&ui_src_fifo, IMPP_WAIT_FOREVER);
                if (ui_buf) {
                        if (0 == cnt % 120) {
                                if (src_flag == 0) {
                                        lseek(uifd0, 0, SEEK_SET);
                                        read(uifd0, ui_src, UI_WIDTH * UI_HEIGHT * 4);
                                        src_flag = 1;
                                } else {
                                        lseek(uifd1, 0, SEEK_SET);
                                        read(uifd1, ui_src, UI_WIDTH * UI_HEIGHT * 4);
                                        src_flag = 0;
                                }
                                cnt = 0;
                        }
                        cnt++;
                        start_time = get_wall_time();
                        memcpy((void *)ui_buf->vaddr, ui_src, UI_WIDTH * UI_HEIGHT * 4);
                        sync.sync_type = DMA_SYNC_FOR_WRITE;
                        sync.fd = ui_buf->fd;
                        IHal_OSD_FlushCache(osd_handle, &sync);
                        end_time = get_wall_time();
                        //printf("copy  time use d %.4f ms ######## \r\n",end_time - start_time);
                        impp_fifo_queue(&osd_src_fifo_ui, ui_buf, IMPP_WAIT_FOREVER);
                }
        }
}


int main(int argc, char **argv)
{
        int ret = 0;
        int i = 0;
        unsigned int times = 500;
        IHal_CodecParam enc_param;
        IHal_SampleFB_Attr fb_attr;
        IHAL_CAMERA_PARAMS camera_params;
        IHAL_CodecStreamInfo_t stream;
        IHal_OSD_ChanAttr_t attr;

        impp_fifo_init(&ui_src_fifo, 3);
        impp_fifo_init(&osd_src_fifo_ui, 3);
        impp_fifo_init(&osd_src_fifo_camera, 3);
        impp_fifo_init(&osd_dst_fifo, 3);
        impp_fifo_init(&encoder_src_fifo, 3);
        impp_fifo_init(&fb_buffer_fifo, 2);

        /*camera 初始化*/
        camera_handle = IHal_CameraOpen(CAM_NODE);
        if (!camera_handle) {
                printf("open camera node failed\r\n");
                return -1;
        }

        memset(&camera_params, 0, sizeof(IHAL_CAMERA_PARAMS));
        camera_params.imageWidth = CAM_WIDTH;
        camera_params.imageHeight = CAM_HEIGHT;
        camera_params.imageFmt = IMPP_PIX_FMT_NV12;

        ret = IHal_CameraSetParams(camera_handle, &camera_params);
        if (ret) {
                printf("camera set params failed\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        ret = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, 3);
        if (ret < 3) {
                printf("Camera create buffers failed!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        for (i = 0; i < ret; i++) {
                IHal_GetCameraBuffers(camera_handle, i, &osd_chn1[i]);
                osd_chn1[i].index = i;
        }
        /* memcpy(osd_chn1,cam_buf,sizeof(cam_buf)); */

        /* osd 初始化 */
        memset(&attr, 0, sizeof(IHal_OSD_ChanAttr_t));
        attr.maxFifoNum = 10000;
        osd_handle = IHal_OSD_ChanCreate(&attr);
        if (!osd_handle) {
                printf("create osd failed !!!!\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        // 创建buffer ---> OSD输出
        for (i = 0; i < 3; i++) {
                memset(&osd_out[i], 0, sizeof(IMPP_BufferInfo_t));
                IHal_OSD_CHx_BufCreate(osd_handle, &osd_out[i], UI_HEIGHT * UI_WIDTH *  3 / 2);
                //printf("osd_out %d  paddr = 0x%x fd = %d \r\n",i,osd_out[i].paddr,osd_out[i].fd);
                //printf("osd_chn0 %d  paddr = 0x%x fd = %d \r\n",i,osd_chn0[i].paddr,osd_chn0[i].fd);
                //printf("\r\n");
                //usleep(100000);
        }
        for (i = 0; i < 3; i++) {
                impp_fifo_queue(&osd_dst_fifo, &osd_out[i], IMPP_WAIT_FOREVER);
        }

        // 创建buffer ---> CHN0 输入，UI将写到这些buffer上
        for (i = 0; i < 3; i++) {
                memset(&osd_chn0[i], 0, sizeof(IMPP_BufferInfo_t));
                IHal_OSD_CHx_BufCreate(osd_handle, &osd_chn0[i], UI_HEIGHT * UI_WIDTH * 4);
                printf("osd_chn0 %d  paddr = 0x%x fd = %d \r\n", i, osd_chn0[i].paddr, osd_chn0[i].fd);
        }
        // 在最开始,将chn0buffer加入到ui的src fifo
        for (i = 0; i < 3; i++) {
                impp_fifo_queue(&ui_src_fifo, &osd_chn0[i], IMPP_WAIT_FOREVER);
        }

        // 将camera buffer作为OSD通道1的输入，此时需要通过importDmabuf来获取物理地址
        for (i = 0; i < 3; i++) {
                IHal_OSD_ImportDmaBuf(osd_handle, &osd_chn1[i]);
                //printf("osd_chn1 %d  paddr = 0x%x \r\n",i,osd_chn1[i].paddr);
        }

        /* encoder 初始化 */
        ret = IHal_CodecInit();
        if (ret) {
                printf("codec init error\n");
                return -1;
        }

        encoder_handle = IHal_CodecCreate(H264_ENC);
        if (!encoder_handle) {
                printf("codec create failed\n");
                return -1;
        }

        memset(&enc_param, 0, sizeof(IHal_CodecParam));
        enc_param.codec_type = H264_ENC;
        enc_param.codecparam.h264e_param.rc_mode = IMPP_ENC_RC_MODE_VBR;
        enc_param.codecparam.h264e_param.target_bitrate = 2000;
        enc_param.codecparam.h264e_param.max_bitrate    = 3000;
        enc_param.codecparam.h264e_param.gop_len        = 30;
        enc_param.codecparam.h264e_param.initial_Qp     = 10;
        enc_param.codecparam.h264e_param.mini_Qp        = 10;
        enc_param.codecparam.h264e_param.max_Qp         = 20;
        enc_param.codecparam.h264e_param.level          = 20;
        enc_param.codecparam.h264e_param.maxPSNR        = 40;
        enc_param.codecparam.h264e_param.freqIDR        = 30;

        enc_param.codecparam.h264e_param.frameRateNum   = 30;
        enc_param.codecparam.h264e_param.frameRateDen   = 1;
        enc_param.codecparam.h264e_param.enc_width      = UI_WIDTH;
        enc_param.codecparam.h264e_param.enc_height     = UI_HEIGHT;
        enc_param.codecparam.h264e_param.src_width      = UI_WIDTH;
        enc_param.codecparam.h264e_param.src_height     = UI_HEIGHT;
        enc_param.codecparam.h264e_param.src_fmt        = IMPP_PIX_FMT_NV12;

        ret =  IHal_Codec_SetParams(encoder_handle, &enc_param);
        if (ret) {
                printf("set codec param failed\n");
                return -1;
        }

        ret = IHal_Codec_CreateSrcBuffers(encoder_handle, IMPP_EXT_DMABUFFER, 3);       // Codec的 src建议使用EXT_DMABUF
        if (ret < 1) {
                printf("src buffer create failed\r\n");
                return -1;
        }

        // 将OSD输出buf预先注册到Encoder中
        for (i = 0; i < 3; i++) {
                ret = IHal_Codec_SetSrcBuffer(encoder_handle, i, &osd_out[i]);
                if (ret) {
                        printf("set h264 encoder src buffer failed \r\n");
                        return -1;
                }
        }

        // 创建encoder输出buffer,只能使用内部
        ret = IHal_Codec_CreateDstBuffer(encoder_handle, IMPP_INTERNAL_BUFFER, 3);
        if (ret < 3) {
                printf("dst buffer create failed\r\n");
                return -1;
        }

        /* LCD初始化  */
        memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
        sprintf(&fb_attr.node[0], "%s", "/dev/fb0");
        fb_attr.mode = Composer_Mode;
        fb_attr.frame_width = UI_WIDTH;
        fb_attr.frame_height = UI_HEIGHT;
        fb_attr.frame_fmt    = IMPP_PIX_FMT_NV12;
        fb_attr.alpha = 255;
        fb0_handle = IHal_SampleFB_Init(&fb_attr);
        if (!fb0_handle) {
                printf("init error\r\n");
                return -1;
        }
        IHal_SampleFB_SetSrcFrameSize(fb0_handle, UI_WIDTH, UI_HEIGHT);
        IHal_SampleFB_SetTargetFrameSize(fb0_handle, UI_WIDTH, UI_HEIGHT);
        IHal_SampleFB_SetTargetPos(fb0_handle, 0, 0);
        IHal_SampleFB_SetZorder(fb0_handle, Order_3);
        IHal_SampleFB_CompRestart(fb0_handle);

        uifd0 = open(UI_PICTURE0, O_RDONLY);
        uifd1 = open(UI_PICTURE1, O_RDONLY);
        if (-1 == uifd0 || -1 == uifd1) {
                printf("Open ui file failed!\r\n");
                return -1;
        }

        ui_src = (char *)malloc(UI_WIDTH * UI_HEIGHT * 4);
        if (ui_src == NULL) {
                printf("malloc ui_src failed &&&&&&&&&&&&&& \r\n");
                return -1;
        }

        /* 任务线程创建  */
        pthread_create(&enc_tid, NULL, encoder_thread, NULL);
        pthread_create(&ui_write_tid, NULL, ui_thread, NULL);
        pthread_create(&camera_tid, NULL, camera_thread, NULL);
        pthread_create(&osd_server_tid, NULL, osd_server_thread, NULL);
        pthread_create(&lcd_update_tid, NULL, lcd_update_thread, NULL);

        IHal_Codec_Start(encoder_handle);
        signal(SIGINT, signal_handle);
        savefd = open("/mnt/x2500/osd_save.h264", O_RDWR | O_CREAT | O_TRUNC, 0666);
        while (times--) {
                memset(&stream, 0, sizeof(IHAL_CodecStreamInfo_t));
                ret = IHal_Codec_WaitDstAvailable(encoder_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueDstBuffer(encoder_handle, &stream);
                        //write(savefd,stream.vaddr,stream.size);
                        //printf("get stream ok !!! pack num : %d \r\n",stream.pack.packcnt);
                        for (i = 0; i < stream.pack.packcnt; i++) {
                                write(savefd, (void *)(stream.vaddr + stream.pack.pack[i].offset), stream.pack.pack[i].length);
                                printf("stream-silce type = %d naltype = %d\r\n", stream.pack.pack[i].sliceType, stream.pack.pack[i].nalType);
                        }
                        IHal_Codec_QueueDstBuffer(encoder_handle, &stream);
                } else {
                        usleep(10000);
                }
        }
        signal_handle(0);
        return 0;
}


static int signal_handle(int sig)
{
        pthread_cancel(camera_tid);
        pthread_cancel(ui_write_tid);
        pthread_cancel(osd_server_tid);
        pthread_cancel(enc_tid);
        pthread_cancel(lcd_update_tid);

        pthread_join(camera_tid, NULL);
        pthread_join(ui_write_tid, NULL);
        pthread_join(osd_server_tid, NULL);
        pthread_join(enc_tid, NULL);
        pthread_join(lcd_update_tid, NULL);

        close(uifd0);
        close(uifd1);

        IHal_CameraStop(camera_handle);
        IHal_CameraClose(camera_handle);

        IHal_Codec_Stop(encoder_handle);
        IHal_CodecDestroy(encoder_handle);
        IHal_CodecDeInit();

        IHal_SampleFB_DeInit(fb0_handle);

        int i = 0;

        for (i = 0; i < 3; i++) {
                IHal_OSD_CHx_BufFree(osd_handle, &osd_chn0[i]);
        }

        for (i = 0; i < 3; i++) {
                IHal_OSD_CHx_BufFree(osd_handle, &osd_out[i]);
        }

        IHal_OSD_DestroyChan(osd_handle);
        free(ui_src);
        close(savefd);

        exit(0);
}
