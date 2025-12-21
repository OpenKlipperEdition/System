#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define HOR_RES 480
#define VER_RES 272

static lv_color_t disp_buf1[HOR_RES * VER_RES];

static struct part_rect {
    int left,top;
    int right,bottom;
    uint16_t buffer[HOR_RES * VER_RES];
};

static int disp_fd = -1;
#define DEVNAME "/dev/nv3041_lcd"
struct part_rect framebuffer;
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
int nv3041_lv_disp_init(void)
{
    disp_fd = open(DEVNAME,O_WRONLY);
    if(disp_fd < 0) {
        printf("%s not finded.\n",DEVNAME);
        perror("ERROR: nv3041_lv_disp_init\n");
        return -1;
    }
    static lv_disp_draw_buf_t draw_buf_dsc;
    static lv_disp_drv_t disp_drv;                  /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/
    lv_disp_draw_buf_init(&draw_buf_dsc, disp_buf1, NULL, HOR_RES * VER_RES);
    disp_drv.hor_res = HOR_RES;
    disp_drv.ver_res = VER_RES;
    disp_drv.direct_mode = 1;
	disp_drv.sw_rotate = 0;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc;
    lv_disp_drv_register(&disp_drv);

    memset(framebuffer.buffer,0,sizeof(framebuffer.buffer));

    framebuffer.top = 0;
    framebuffer.left = 0;
    framebuffer.right = HOR_RES - 1;
    framebuffer.bottom = VER_RES - 1;
    write(disp_fd,&framebuffer,sizeof(framebuffer));
    printf("disp init Ok\n");
    return 0;
}

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    int32_t x;
    int32_t y;
    uint16_t pix,prev_pix;
    int top = VER_RES,left = HOR_RES,right = -1,bottom = -1;
    int w;
    int h;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            pix = ((color_p->ch.blue >> 3) << 0)  |
                ((color_p->ch.green >> 2) << 5) |
                ((color_p->ch.red >> 3) << 11);
            pix = ((pix & 0xff) << 8) | (pix >> 8);
            prev_pix = framebuffer.buffer[y * HOR_RES + x];
            if(prev_pix != pix) {
                if(top > y)
                    top = y;
                if(left > x)
                    left = 0;
                if(right < x)
                    right = x;
                if(bottom < y)
                    bottom = y;
                framebuffer.buffer[y * HOR_RES + x] = pix;
            }
            color_p++;
        }
    }

    if(right > 0 && bottom > 0) {
        //printf("Rect: %d %d %d %d\n",left,top,right,bottom);
        w = right - left + 1;
        h = bottom - top + 1;
        left = left / 8 * 8;
        right = (right + 8) / 8 * 8 - 1;
        top = top / 4 * 4;
        bottom = (bottom + 4) / 4 * 4 - 1;
        framebuffer.top = top;
        framebuffer.left = left;
        framebuffer.right = right;
        framebuffer.bottom = bottom;
        //printf("Aligned Rect: %d %d %d %d\n",left,top,right,bottom);
        write(disp_fd,&framebuffer,sizeof(framebuffer));
    }
    /* IMPORTANT!!!
     * Inform the graphics library that you are ready with the flushing
     */
    lv_disp_flush_ready(disp_drv);
}
void nv3041_lv_disp_deinit(void)
{
    if(disp_fd)
        close(disp_fd);
}
