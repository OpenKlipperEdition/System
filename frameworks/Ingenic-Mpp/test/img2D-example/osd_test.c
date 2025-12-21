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
#include <ipu.h>

// basic test

#define FOUR_CHANNELS_OSD_ENABLE        0

int main(int argc, char **argv)
{
        int ret = 0;
        IPU_DmaBuf_SyncInfo_t syncinfo;

        IHal_OSD_Handle_t *osd_handle;
        IHal_OSD_ChanAttr_t attr;
        osd_handle = IHal_OSD_ChanCreate(&attr);
        if (!osd_handle) {
                printf("create osd failed !!!!\r\n");
                return -1;
        }

        /* 填充背景图片 : NV12格式 */
        int osd_bgfd = -1;
        char *osd_bgfilename = "osd_bg_3840x2160.nv12";
        osd_bgfd = open(osd_bgfilename, O_RDWR);
        if (-1 == osd_bgfd) {
                printf("Open osd bgfile failed!\r\n");
                goto destroy_osdchan;
        }
        unsigned int osd_bgsize = 3840 * 2160 * 3 / 2;
        IMPP_BufferInfo_t osd_bgbuf;
        memset(&osd_bgbuf, 0, sizeof(osd_bgbuf));
        ret = IHal_OSD_CHx_BufCreate(osd_handle, &osd_bgbuf, osd_bgsize);
        if (ret) {
                printf("Create bgbuf failed!\r\n");
                goto close_bgfd;
        }
        printf("bg-paddr = %#x vaddr = %#x fd = %d size = %d\r\n", osd_bgbuf.paddr, osd_bgbuf.vaddr, osd_bgbuf.fd, osd_bgbuf.size);
        read(osd_bgfd, (void *)osd_bgbuf.vaddr, osd_bgsize);
        memset(&syncinfo, 0, sizeof(syncinfo));
        syncinfo.fd = osd_bgbuf.fd;
        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
        IHal_OSD_FlushCache(osd_handle, &syncinfo);

        /* 填充通道0的图片数据 : RGBA8888格式 */
        int osd_ch0fd = -1;
        char *osd_ch0filename = "osd_ch0_1920x1080.rgba8888";
        osd_ch0fd = open(osd_ch0filename, O_RDWR);
        if (-1 == osd_ch0fd) {
                printf("Open osd ch0file failed!\r\n");
                goto free_bgbuf;
        }
        unsigned int osd_ch0size = 1920 * 1080 * 4;
        IMPP_BufferInfo_t osd_ch0buf;
        memset(&osd_ch0buf, 0, sizeof(osd_ch0buf));
        ret = IHal_OSD_CHx_BufCreate(osd_handle, &osd_ch0buf, osd_ch0size);
        if (ret) {
                printf("Create ch0buf failed!\r\n");
                goto close_ch0fd;
        }
        printf("ch0-paddr = %#x vaddr = %#x fd = %d size = %d\r\n", osd_ch0buf.paddr, osd_ch0buf.vaddr, osd_ch0buf.fd, osd_ch0buf.size);
        read(osd_ch0fd, (void *)osd_ch0buf.vaddr, osd_ch0size);
        memset(&syncinfo, 0, sizeof(syncinfo));
        syncinfo.fd = osd_ch0buf.fd;
        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
        IHal_OSD_FlushCache(osd_handle, &syncinfo);

        /* 填充通道1的图片数据 : BGRA8888格式 */
        int osd_ch1fd = -1;
        char *osd_ch1filename = "osd_ch1_64x64.bgra8888";
        osd_ch1fd = open(osd_ch1filename, O_RDWR);
        if (-1 == osd_ch1fd) {
                printf("Open osd ch1file failed!\r\n");
                goto free_ch0buf;
        }
        unsigned int osd_ch1size = 64 * 64 * 4;
        IMPP_BufferInfo_t osd_ch1buf;
        memset(&osd_ch1buf, 0, sizeof(osd_ch1buf));
        ret = IHal_OSD_CHx_BufCreate(osd_handle, &osd_ch1buf, osd_ch1size);
        if (-1 == ret) {
                printf("Create ch1buf failed!\r\n");
                goto close_ch1fd;
        }
        printf("ch1-paddr = %#x vaddr = %#x fd = %d size = %d\r\n", osd_ch1buf.paddr, osd_ch1buf.vaddr, osd_ch1buf.fd, osd_ch1buf.size);
        read(osd_ch1fd, (void *)osd_ch1buf.vaddr, osd_ch1size);
        memset(&syncinfo, 0, sizeof(syncinfo));
        syncinfo.fd = osd_ch1buf.fd;
        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
        IHal_OSD_FlushCache(osd_handle, &syncinfo);

#if FOUR_CHANNELS_OSD_ENABLE
        /* 填充通道2的图片数据 : NV21格式 */
        int osd_ch2fd = -1;
        char *osd_ch2filename = "osd_ch2_352x288_colorbar.nv21";
        osd_ch2fd = open(osd_ch2filename, O_RDWR);
        if (-1 == osd_ch2fd) {
                printf("Open osd ch2file failed!\r\n");
                goto free_ch1buf;
        }
        unsigned int osd_ch2size = 352 * 288 * 3 / 2;
        IMPP_BufferInfo_t osd_ch2buf;
        memset(&osd_ch2buf, 0, sizeof(osd_ch2buf));
        ret = IHal_OSD_CHx_BufCreate(osd_handle, &osd_ch2buf, osd_ch2size);
        if (ret) {
                printf("Create ch2buf failed!\r\n");
                goto close_ch2fd;
        }
        printf("ch2-paddr = %#x vaddr = %#x fd = %d size = %d\r\n", osd_ch2buf.paddr, osd_ch2buf.vaddr, osd_ch2buf.fd, osd_ch2buf.size);
        read(osd_ch2fd, (void *)osd_ch2buf.vaddr, osd_ch2size);
        memset(&syncinfo, 0, sizeof(syncinfo));
        syncinfo.fd = osd_ch2buf.fd;
        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
        IHal_OSD_FlushCache(osd_handle, &syncinfo);

        /* 填充通道3的图片数据 : RGBA5551 */
        int osd_ch3fd = -1;
        char *osd_ch3filename = "osd_ch3_352x288.rgba5551";
        osd_ch3fd = open(osd_ch3filename, O_RDWR);
        if (-1 == osd_ch3fd) {
                printf("Open osd ch3file failed!\r\n");
                goto free_ch2buf;
        }
        unsigned int osd_ch3size = 352 * 288 * 2;
        IMPP_BufferInfo_t osd_ch3buf;
        memset(&osd_ch3buf, 0, sizeof(osd_ch3buf));
        ret = IHal_OSD_CHx_BufCreate(osd_handle, &osd_ch3buf, osd_ch3size);
        if (ret) {
                printf("Create ch3buf failed!\r\n");
                goto close_ch3fd;
        }
        read(osd_ch3fd, (void *)osd_ch3buf.vaddr, osd_ch3size);
        memset(&syncinfo, 0, sizeof(syncinfo));
        syncinfo.fd = osd_ch3buf.fd;
        syncinfo.sync_type = DMA_SYNC_FOR_WRITE;
        IHal_OSD_FlushCache(osd_handle, &syncinfo);
#endif

        IHal_OSD_FrameDesc_t osd_inputFrame;
        memset(&osd_inputFrame, 0, sizeof(osd_inputFrame));
#if FOUR_CHANNELS_OSD_ENABLE
        osd_inputFrame.osd_flags = OSD_CH0_EN | OSD_CH1_EN | OSD_CH2_EN | OSD_CH3_EN;
#else
        osd_inputFrame.osd_flags = OSD_CH0_EN | OSD_CH1_EN;
#endif
        osd_inputFrame.bgAttr.width = 3840;
        osd_inputFrame.bgAttr.height = 2160;
        osd_inputFrame.bgAttr.paddr = osd_bgbuf.paddr;
        osd_inputFrame.bgAttr.vaddr = osd_bgbuf.vaddr;
        osd_inputFrame.bgAttr.fmt = IMPP_PIX_FMT_NV12;

        osd_inputFrame.chAttr[0].width = 1920;
        osd_inputFrame.chAttr[0].height = 1080;
        osd_inputFrame.chAttr[0].alpha = 255;
        osd_inputFrame.chAttr[0].posX = 600;
        osd_inputFrame.chAttr[0].posY = 500;
        osd_inputFrame.chAttr[0].paddr = osd_ch0buf.paddr;
        osd_inputFrame.chAttr[0].fmt = IMPP_PIX_FMT_RGBA_8888;

        osd_inputFrame.chAttr[1].width = 64;
        osd_inputFrame.chAttr[1].height = 64;
        osd_inputFrame.chAttr[1].alpha = 255;
        osd_inputFrame.chAttr[1].posX = 2800;
        osd_inputFrame.chAttr[1].posY = 2000;
        osd_inputFrame.chAttr[1].paddr = osd_ch1buf.paddr;
        osd_inputFrame.chAttr[1].fmt = IMPP_PIX_FMT_BGRA_8888;

#if FOUR_CHANNELS_OSD_ENABLE
        osd_inputFrame.chAttr[2].width = 352;
        osd_inputFrame.chAttr[2].height = 288;
        osd_inputFrame.chAttr[2].alpha = 255;
        osd_inputFrame.chAttr[2].posX = 2000;
        osd_inputFrame.chAttr[2].posY = 1500;
        osd_inputFrame.chAttr[2].paddr = osd_ch2buf.paddr;
        osd_inputFrame.chAttr[2].fmt = IMPP_PIX_FMT_NV21;

        osd_inputFrame.chAttr[3].width = 352;
        osd_inputFrame.chAttr[3].height = 288;
        osd_inputFrame.chAttr[3].alpha = 255;
        osd_inputFrame.chAttr[3].posX = 2300;
        osd_inputFrame.chAttr[3].posY = 400;
        osd_inputFrame.chAttr[3].paddr = osd_ch3buf.paddr;
        osd_inputFrame.chAttr[3].fmt = IMPP_PIX_FMT_RGBA_5551;
#endif


        int osd_outfd = -1;
        char *osd_outfilename = "osd_out_3840x2160.nv12";
        IMPP_FrameInfo_t osd_outputFrame;
        memset(&osd_outputFrame, 0, sizeof(osd_outputFrame));
        osd_outfd = open(osd_outfilename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (-1 == osd_outfd) {
                printf("Open osd outfile failed!\r\n");
#if FOUR_CHANNELS_OSD_ENABLE
                goto free_ch3buf;
#else
                goto free_ch1buf;
#endif
        }
        ret = IHal_OSD_ProcessFrame(osd_handle, &osd_inputFrame, &osd_outputFrame);
        if (ret) {
                printf("OSD process frame failed!\r\n");
                goto close_outfd;
        }
        printf("out framesize = %d \r\n", osd_outputFrame.size);
        write(osd_outfd, (void *)osd_outputFrame.vaddr, osd_outputFrame.size);


        close(osd_outfd);
#if FOUR_CHANNELS_OSD_ENABLE
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch3buf);
        close(osd_ch3fd);
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch2buf);
        close(osd_ch2fd);
#endif
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch1buf);
        close(osd_ch1fd);
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch0buf);
        close(osd_ch0fd);
        IHal_OSD_CHx_BufFree(osd_handle, &osd_bgbuf);
        close(osd_bgfd);
        IHal_OSD_DestroyChan(osd_handle);

        return 0;

close_outfd:
        close(osd_outfd);
#if FOUR_CHANNELS_OSD_ENABLE
free_ch3buf:
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch3buf);
close_ch3fd:
        close(osd_ch3fd);
free_ch2buf:
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch2buf);
close_ch2fd:
        close(osd_ch2fd);
#endif
free_ch1buf:
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch1buf);
close_ch1fd:
        close(osd_ch1fd);
free_ch0buf:
        IHal_OSD_CHx_BufFree(osd_handle, &osd_ch0buf);
close_ch0fd:
        close(osd_ch0fd);
free_bgbuf:
        IHal_OSD_CHx_BufFree(osd_handle, &osd_bgbuf);
close_bgfd:
        close(osd_bgfd);
destroy_osdchan:
        IHal_OSD_DestroyChan(osd_handle);

        return -1;
}
