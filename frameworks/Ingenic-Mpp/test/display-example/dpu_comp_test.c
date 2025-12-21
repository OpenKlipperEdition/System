#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdint.h>
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

   1. 从文件读入一张图片.
   2. 设置输入图片的尺寸信息，
   3. 设置DPU 为写回模式.
   4. 输出RGBA文件，其中写回buffer为reserve memory.

   重点:
   使用的地址为 vaddr = malloc 的地址而不是物理地址.
   */

#define RESERVE_MEMOEY_BASE 0x07000000


int main(int argc, char **argv)
{
    IHal_Dpu_InitStruct_t initStruct;
    initStruct.num_outmem = 4;

    IMPP_BufferInfo_t layer1buf;
    IMPP_BufferInfo_t displaybuf;
    IMPP_BufferInfo_t outBuf;
    IMPP_FrameInfo_t outframe;

    IHal_Dpu_Handle_t *dpuHandle = IHal_Dpu_Init(&initStruct);

    int file2 = open("crop.yuv", O_RDWR);
    int file3 = open("osd_out.rgba", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (-1 == file2) {
        printf("Open file failed!\r\n");
        return -1;
    }

    uint32_t *outBufPaddr;
    size_t s0 = (832*1944*4); // align 4K

    int map_fd = open("/dev/mem", O_RDWR);
    if(map_fd < 0)
    {
        printf("cannot open /dev/mem.\n");
        return -1;
    }

    outBufPaddr = mmap(NULL, s0, PROT_READ | PROT_WRITE,
                       MAP_SHARED, map_fd, RESERVE_MEMOEY_BASE);
    if (outBufPaddr == MAP_FAILED) {
        perror("map mem");
        outBufPaddr = NULL;
        return -1;
    }
    outBuf.vaddr = (uint32_t)outBufPaddr;
    outBuf.paddr = RESERVE_MEMOEY_BASE;
    IHal_Dpu_Composer_Set_WbackBuffers(dpuHandle, &outBuf, 0);

    //fill picture
    layer1buf.vaddr = malloc(832 * 1944 * 3 / 2);
    read(file2, layer1buf.vaddr, 832 * 1944 * 3 / 2);

    IHal_Dpu_FrameDesc_t frame;
    memset(&frame, 0, sizeof(IHal_Dpu_FrameDesc_t));
    frame.dpu_func = DPU_OSD_Func;
    frame.func_desc.osd_frame.osd_flags = Dpu_Layer1;
    frame.func_desc.osd_frame.outWidth = 832;
    frame.func_desc.osd_frame.outHeight = 1944;
    frame.func_desc.osd_frame.wback_enable = 1;
    frame.func_desc.osd_frame.wback_fmt = IMPP_PIX_FMT_RGB_888;

    frame.func_desc.osd_frame.layer[1].srcFmt = IMPP_PIX_FMT_NV12;
    frame.func_desc.osd_frame.layer[1].srcWidth = 832;
    frame.func_desc.osd_frame.layer[1].srcHeight = 1944;
    frame.func_desc.osd_frame.layer[1].srcCropx = 0;
    frame.func_desc.osd_frame.layer[1].srcCropy = 0;
    frame.func_desc.osd_frame.layer[1].srcCropw = 0;
    frame.func_desc.osd_frame.layer[1].srcCroph = 0;
    frame.func_desc.osd_frame.layer[1].osd_order = DPU_OSD_Order3;
    frame.func_desc.osd_frame.layer[1].alpha = 200;
    frame.func_desc.osd_frame.layer[1].paddr = 0;
    frame.func_desc.osd_frame.layer[1].color_order = DPU_CFG_COLOR_RGB;
    frame.func_desc.osd_frame.layer[1].vaddr = layer1buf.vaddr;


    // 启动DPU.
    IHal_Dpu_Composer_Process(dpuHandle, &frame);

    // 获取写回的数据.
    IHal_Dpu_Composer_GetFrame(dpuHandle, &outframe);

    // 处理写回的数据.
    lseek(file3, 0, SEEK_SET);
    write(file3, outframe.vaddr, outframe.size);

    // 释放写回的数据.
    IHal_Dpu_Composer_ReleaseFrame(dpuHandle, &outframe);

    IHal_Dpu_Deinit(dpuHandle);
    close(file2);
    close(file3);
    munmap(outBufPaddr, s0);

    return 0;
}
