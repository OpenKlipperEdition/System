/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

static lv_obj_t * main_page;
static lv_obj_t * close_btn;
static lv_obj_t * calendar;

static lv_timer_t * timer;
static void user_timer_xcb(lv_timer_t * timer);

static void calendar_close_event_cb1(lv_event_t * e)
{
	void * handle = lv_event_get_user_data(e);
	lv_event_code_t event = lv_event_get_code(e);

    if (event == LV_EVENT_SHORT_CLICKED) {
        lv_obj_del(main_page);
        lv_timer_del(timer);
		dlclose(handle);
    }
}

LV_IMG_DECLARE(clock_bg)
LV_IMG_DECLARE(clock_hou)
LV_IMG_DECLARE(clock_min)
LV_IMG_DECLARE(clock_sec)
static const lv_img_dsc_t * clock_img_array_l[4] =
{
    &clock_bg,
    &clock_hou,
    &clock_min,
    &clock_sec,
};
static lv_obj_t * clock[4];

static void user_timer_xcb(lv_timer_t * timer)
{
    lv_img_set_angle(clock[3], lv_tick_get() * 3 / 50 % 3600);
    lv_img_set_angle(clock[2], lv_tick_get() * 3 / 3000 % 3600);
    lv_img_set_angle(clock[1], lv_tick_get() * 3 / 36000 % 3600);
}

void app_calendar_create(lv_obj_t * parent, void *handle)
{
    int i;
	main_page = lv_obj_create(parent);
    lv_obj_set_size(main_page, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(main_page, LV_OPA_100, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(main_page, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(main_page, 0, LV_STATE_DEFAULT);
    lv_obj_align_to(main_page, parent, LV_ALIGN_CENTER, 0, 0);

    close_btn = lv_btn_create(main_page);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_60, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(close_btn, 3, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(close_btn, 1, LV_STATE_DEFAULT);
    lv_obj_set_size(close_btn, 55, 35);
    lv_obj_align_to(close_btn, main_page, LV_ALIGN_TOP_RIGHT, -15, 15);

    lv_obj_t * label = lv_label_create(close_btn);
    lv_label_set_text(label, "Exit");
    lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
	lv_obj_align_to(label, close_btn, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_event_cb(close_btn, calendar_close_event_cb1, LV_EVENT_ALL, handle);

    for (i = 0; i < 4; i++) {
        clock[i] = lv_img_create(main_page);
        lv_img_set_src(clock[i], clock_img_array_l[i]);
        lv_obj_align_to(clock[i], main_page, LV_ALIGN_CENTER, 0, 0);
    }

	timer = lv_timer_create(user_timer_xcb, 30, NULL);
}

