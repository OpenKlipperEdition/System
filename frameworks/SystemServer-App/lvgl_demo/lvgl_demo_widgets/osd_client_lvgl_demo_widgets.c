#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lvgl/lvgl.h"
#include "lvgl/lv_conf.h"
#include "lvgl/lv_drivers/indev/evdev.h"		// 需要`evdev_xxx()`
#include "lv_demo_widgets.h"
#include "iss_osd.h"

#define TEST_FPS			0
#define DISP_BUF_SIZE		(720 * 1280)		// in pixels


/* A small buffer for LittlevGL to draw the screen's content */
static lv_color_t buf1[DISP_BUF_SIZE];
static lv_color_t buf2[DISP_BUF_SIZE];
static struct OsdClient *clt = NULL;


#if TEST_FPS
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
#endif


static void iss_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
	ISS_Flush(clt, color_p, 720 * 1280 *4);
#if TEST_FPS
	calc_fps();
#endif
	lv_disp_flush_ready(drv);
}

int main()
{
	int ret = 0;
	struct ImageAttr image_attr;

	/* LittlevGL init */
	lv_init();

	clt = ISS_CreateOsdClt(UI);
	if (clt == NULL) {
		printf("ISS_CreateOsdClt failed!!!\n");
		exit(-1);
	}
	printf("ISS_CreateOsdClt sucess!!!\n");

	image_attr.imageFmt = PIX_FMT_BGRA_8888;
	image_attr.imageWidth = 720;
	image_attr.imageHeight = 1280;
	ret = ISS_SetImageAttr(clt, image_attr);
	if (ret != 0) {
		printf("ISS_SetImageAttr failed!!!\n");
		exit(-1);
	}
	printf("ISS_SetImageAttr sucess!!!\n");

	ret = ISS_AllocDispMem(clt, 720 * 1280 * 4, 3);
	if (ret != 0) {
		printf("ISS_AllocDispMem failed!!!\n");
		exit(-1);
	}
	printf("ISS_AllocDispMem sucess!!!\n");

	/* Initialize a descriptor for the buffer */
	static lv_disp_draw_buf_t disp_buf;
	lv_disp_draw_buf_init(&disp_buf, buf1, buf2, DISP_BUF_SIZE);

	/* Initialize and register a display driver */
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf   = &disp_buf;
	disp_drv.flush_cb   = iss_flush;
	disp_drv.hor_res    = 720;
	disp_drv.ver_res    = 1280;
	disp_drv.direct_mode = 0;
	disp_drv.full_refresh = 1;
	lv_disp_drv_register(&disp_drv);

	evdev_init();
	static lv_indev_drv_t indev_drv_1;
	lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
	indev_drv_1.type = LV_INDEV_TYPE_POINTER;

	/*This function will be called periodically (by the library) to get the mouse position and state*/
	indev_drv_1.read_cb = evdev_read;
	lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

	/*Set a cursor for the mouse*/
	LV_IMG_DECLARE(mouse_cursor_icon);
	lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
	lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
	lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/

	lv_demo_widgets();

	while(1) {
		lv_timer_handler();
		usleep(5000);
	}

	return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
