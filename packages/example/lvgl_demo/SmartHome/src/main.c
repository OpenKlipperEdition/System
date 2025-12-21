#include "lvgl.h"
#include "display/fbdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "indev/tslib_drv.h"
#include "smarthome_main.h"
#ifdef NV3041_LCD
#include "nv3041_lcd_drv.h"
#endif
#define DISP_BUF_SIZE (720 * 1280)

int main(void)
{
	/*LittlevGL init*/
	lv_init();

#ifdef NV3041_LCD
  int ret = nv3041_lv_disp_init();
  if(ret != 0)
    return -1;
#else
	/*Linux frame buffer device init*/
	fbdev_init();

	/*A small buffer for LittlevGL to draw the screen's content*/
	static lv_color_t buf1[DISP_BUF_SIZE];

	/*Initialize a descriptor for the buffer*/
	static lv_disp_draw_buf_t disp_buf;
	lv_disp_draw_buf_init(&disp_buf, buf1, NULL,DISP_BUF_SIZE);

	/*Initialize and register a display driver*/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.draw_buf   = &disp_buf;
	disp_drv.flush_cb   = fbdev_flush;
	disp_drv.hor_res    = 720;
	disp_drv.ver_res    = 1280;
	disp_drv.direct_mode = 1;
	/*disp_drv.rotated = LV_DISP_ROT_90;*/
	disp_drv.sw_rotate = 0;
	//disp_drv.full_refresh = 1;
	lv_disp_drv_register(&disp_drv);
#endif
	tslib_init();
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type =LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = tslib_read;
	lv_indev_drv_register(&indev_drv);

	/*Call your application entry interface here*/
	application_init();

	/*Handle LitlevGL tasks (tickless mode)*/
	while(1) {
		lv_task_handler();
		usleep(5000);
	}
#ifdef NV3041_LCD
  nv3041_lv_disp_deinit();
#endif
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
