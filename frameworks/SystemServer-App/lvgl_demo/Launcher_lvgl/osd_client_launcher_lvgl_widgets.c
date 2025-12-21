#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "lvgl/lvgl.h"
#include "lvgl/lv_conf.h"
#include "indev/tslib_drv.h"                    // tslib
#include "iss_osd.h"

#define TEST_FPS			0
#define DISP_BUF_SIZE		(720 * 1280)		// in pixels


/* A small buffer for LittlevGL to draw the screen's content */
static lv_color_t buf1[DISP_BUF_SIZE];
static lv_color_t buf2[DISP_BUF_SIZE];
static struct OsdClient *clt = NULL;
pthread_mutex_t g_ui_lock = PTHREAD_MUTEX_INITIALIZER;


void launcher_widgets(void);


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

	tslib_set_file("/dev/input/event0");
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type =LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = tslib_read;
	lv_indev_drv_register(&indev_drv);

	launcher_widgets();

	while(1) {
		pthread_mutex_lock(&g_ui_lock);
		lv_timer_handler();
		pthread_mutex_unlock(&g_ui_lock);
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
