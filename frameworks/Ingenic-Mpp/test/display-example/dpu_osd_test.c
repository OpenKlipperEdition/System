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
#include <stdbool.h>
#include "dpu.h"
#include "impp.h"



/*
        示例说明:

        1. 从文件读入两张图片.
        2. 设置输入图片的尺寸信息，
        3. 设置DPU 为写回模式.
        4. 循环改变DPU输入信息，观察屏幕现象.

        重点:
        使用的地址为 vaddr = malloc 的地址而不是物理地址.
*/


int main(int argc, char **argv)
{
        IHal_Dpu_InitStruct_t initStruct;
        initStruct.num_outmem = 4;

        IMPP_BufferInfo_t layer0buf;
        IMPP_BufferInfo_t layer1buf;
        IMPP_BufferInfo_t displaybuf;

        IHal_Dpu_Handle_t *dpuHandle = IHal_Dpu_Init(&initStruct);

        IHal_Dpu_RDMA_GetFrame(dpuHandle, &displaybuf);
        displaybuf.fd = 0;
        IHal_Dpu_Composer_Set_WbackBuffers(dpuHandle, &displaybuf, 0);

        int file1 = open("ffmpeg_720x1280_bgra.rgb", O_RDWR);
        int file2 = open("640x480.nv12", O_RDWR);
        // int file3 = open("osd_out.rgba", O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (-1 == file1 || -1 == file2) {
                printf("Open file failed!\r\n");
                return -1;
        }

        //fill picture
        layer0buf.vaddr = malloc(720 * 1280 * 4);
        read(file1, layer0buf.vaddr, 720 * 1280 * 4);
        lseek(file1, 0, SEEK_SET);

        read(file1, displaybuf.vaddr, 720 * 1280 * 4);
        IHal_Dpu_RDMA_PutFrame(dpuHandle, &displaybuf);

        layer1buf.vaddr = malloc(640 * 480 * 3 / 2);
        read(file2, layer1buf.vaddr, 640 * 480 * 3 / 2);

        IHal_Dpu_FrameDesc_t frame;
        memset(&frame, 0, sizeof(IHal_Dpu_FrameDesc_t));
        frame.dpu_func = DPU_OSD_Func;
        frame.func_desc.osd_frame.osd_flags = Dpu_Layer0 | Dpu_Layer1;
        frame.func_desc.osd_frame.outWidth = 720;
        frame.func_desc.osd_frame.outHeight = 1280;
        frame.func_desc.osd_frame.wback_enable = 1;
        frame.func_desc.osd_frame.wback_fmt = IMPP_PIX_FMT_BGRA_8888;

        frame.func_desc.osd_frame.layer[0].srcFmt = IMPP_PIX_FMT_BGRA_8888;
        frame.func_desc.osd_frame.layer[0].srcWidth = 720;
        frame.func_desc.osd_frame.layer[0].srcHeight = 1280;
        frame.func_desc.osd_frame.layer[0].scale_enable = 1;
        frame.func_desc.osd_frame.layer[0].scaleWidth = 720;
        frame.func_desc.osd_frame.layer[0].scaleHeight = 1280;
        frame.func_desc.osd_frame.layer[0].srcCropx = 200;
        frame.func_desc.osd_frame.layer[0].srcCropy = 600;
        frame.func_desc.osd_frame.layer[0].srcCropw = 500;
        frame.func_desc.osd_frame.layer[0].srcCroph = 500;
        frame.func_desc.osd_frame.layer[0].osd_posX = 0;
        frame.func_desc.osd_frame.layer[0].osd_posY = 0;
        frame.func_desc.osd_frame.layer[0].osd_order = DPU_OSD_Order1;
        frame.func_desc.osd_frame.layer[0].alpha = 255;
        frame.func_desc.osd_frame.layer[0].paddr = 0; //layer0buf.paddr;
        frame.func_desc.osd_frame.layer[0].color_order = DPU_CFG_COLOR_RGB;
        frame.func_desc.osd_frame.layer[0].vaddr = layer0buf.vaddr;     /*!< 使用TLB. */

        frame.func_desc.osd_frame.layer[1].srcFmt = IMPP_PIX_FMT_NV12;
        frame.func_desc.osd_frame.layer[1].srcWidth = 640;
        frame.func_desc.osd_frame.layer[1].srcHeight = 480;
        frame.func_desc.osd_frame.layer[1].srcCropx = 0;
        frame.func_desc.osd_frame.layer[1].srcCropy = 0;
        frame.func_desc.osd_frame.layer[1].srcCropw = 0;
        frame.func_desc.osd_frame.layer[1].srcCroph = 0;
        frame.func_desc.osd_frame.layer[1].scale_enable = 1;
        frame.func_desc.osd_frame.layer[1].scaleWidth = 320;
        frame.func_desc.osd_frame.layer[1].scaleHeight = 240;
        frame.func_desc.osd_frame.layer[1].osd_posX = 100;
        frame.func_desc.osd_frame.layer[1].osd_posY = 200;
        frame.func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order3;
        frame.func_desc.osd_frame.layer[1].alpha = 200;
        frame.func_desc.osd_frame.layer[1].paddr = 0; //layer1buf.paddr;
        frame.func_desc.osd_frame.layer[1].color_order = DPU_CFG_COLOR_RGB;
        frame.func_desc.osd_frame.layer[1].vaddr = layer1buf.vaddr;
        frame.func_desc.osd_frame.layer[1].uv_vaddr = (layer1buf.vaddr + 640 * 480);

        int i = 0;
        int count = 3000;

        unsigned int src_w = frame.func_desc.osd_frame.layer[0].srcWidth;
        unsigned int src_h = frame.func_desc.osd_frame.layer[0].srcHeight;
        IMPP_FrameInfo_t outframe;
        while (count--) {
                unsigned int cropx = 100 + i;
                unsigned int cropy = 100 + i;
                unsigned int cropw = 100 + i;
                unsigned int croph = 100 + i;

                cropx %= src_w;
                cropy %= src_h;
                cropw %= src_w;
                croph %= src_h;

                /*iterate crop size of layer 0*/
                frame.func_desc.osd_frame.layer[0].srcCropx = cropx;
                frame.func_desc.osd_frame.layer[0].srcCropy = cropy;
                frame.func_desc.osd_frame.layer[0].srcCropw = cropw;
                frame.func_desc.osd_frame.layer[0].srcCroph = croph;

                printf("src crop[%d, %d, %d, %d]\n", cropx, cropy, cropw, croph);
                frame.func_desc.osd_frame.layer[1].osd_posX = i % 720;
                frame.func_desc.osd_frame.layer[1].osd_posY = i % 1280;

                // 启动DPU.
                IHal_Dpu_Composer_Process(dpuHandle, &frame);

                // 获取写回的数据.
                IHal_Dpu_Composer_GetFrame(dpuHandle, &outframe);

                // 处理写回的数据.
                //lseek(file3, 0, SEEK_SET);
                //write(file3, outframe.vaddr, outframe.size);

                // 释放写回的数据.
                IHal_Dpu_Composer_ReleaseFrame(dpuHandle, &outframe);

                i ++;
        }

        /* printf("process end out vaddr = 0x%x paddr = 0x%x size = %d \r\n",outframe.vaddr,outframe.paddr,outframe.size); */
        IHal_Dpu_Deinit(dpuHandle);
        close(file1);
        close(file2);
        // close(file3);

        return 0;
}
