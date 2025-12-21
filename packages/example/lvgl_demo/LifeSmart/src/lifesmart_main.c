/*
 * @file lifesmart_main.c
 *
 * No license to Ingenic property rights is granted, Ingenic assumes no
 * liability, provides no warranty either expressed or implied relating
 * to the usage, or intellectual property right infringement except
 * as provided for by Ingenic Terms and Conditions of Sale.
 *
 * All rights reserved by Ingenic Semiconductor CO., LTD.
 *
 * Creator: yflu <yafei.lu@ingenic.com>
 * Maintainer: yflu <yafei.lu@ingenic.com>
 * Created: 2022/07/01
 * Updated:
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "lifesmart_main.h"
#include "lifesmart_airconditioner.h"
#include "lifesmart_curtain.h"
#include "lifesmart_lamp.h"
#include "lifesmart_airpurifier.h"
#include "lifesmart_scene.h"
#include "lifesmart_monitor.h"
#include "date_time.h"
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "/assets/fonts/sourcehan.ttf"
#define DEVICE_COUNT   6

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char *s_weeks[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
static const char *s_device_text[] = {
    "空调",
    "灯光",
    "空气净化",
    "窗帘",
    "布防",
    "场景",
    "视频播放",
};
static const char *s_device_icon[] = {
    "/assets/images/icon_airconditioner.png",
    "/assets/images/icon_lamp.png",
    "/assets/images/icon_airpurifier.png",
    "/assets/images/icon_curtain.png",
    "/assets/images/icon_monitor.png",
    "/assets/images/icon_scene.png",
    "/assets/images/icon_video.png",
};

static lv_style_t style_container;
static lv_obj_t* label_time;
static lv_obj_t* label_date;
static lv_obj_t* label_week;
static lv_obj_t* screen_main;
static lv_obj_t* screen_other;

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void on_window_destory(lv_event_t* e)
{
	lv_scr_load(screen_main);
}

static void on_device_btn_clicked(lv_event_t* e)
{
	lv_scr_load(screen_other);
	int index =  (int)lv_event_get_user_data(e);
	switch (index) {
		case 0:
			create_airconditioner_window(screen_other);
			break;
		case 1:
			create_lamp_window(screen_other);
			break;
		case 2:
			create_airpurifier_window(screen_other);
			break;
		case 3:
			create_curtain_window(screen_other);
			break;
		case 4:
			create_monitor_window(screen_other);
			break;
		case 5:
			create_scene_window(screen_other);
			break;
	}
}

static void update_system_time(lv_timer_t *t)
{
    date_time_t dt;
    date_time_init(&dt);
    char time[1024];
    sprintf(time, "%02d:%02d", dt.hour, dt.minute);
    char date[1024];
    sprintf(date, "%d/%02d/%02d", dt.year, dt.month, dt.day);

    lv_label_set_text(label_time, time);
    lv_label_set_text(label_date, date);
    lv_label_set_text(label_week, s_weeks[dt.wday]);
}

static void create_weather_display_area(lv_obj_t* parent)
{
    static lv_ft_info_t font_weather;
    font_weather.name = FONT_PATH;
    font_weather.weight = 34;
    font_weather.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_weather);

    lv_obj_t* img_weather = lv_img_create(parent);
    lv_obj_set_pos(img_weather, 420, 35);
    lv_obj_set_size(img_weather, 124, 92);
    lv_img_set_src(img_weather, "/assets/images/weather.png");

	lv_obj_t* label_temp = lv_label_create(parent);
    lv_obj_set_pos(label_temp, 555, 30);
    lv_obj_set_size(label_temp, 160, 50);
    lv_obj_set_style_text_color(label_temp, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(label_temp, font_weather.font, LV_PART_MAIN);
	lv_label_set_text(label_temp, "26~33℃");

	lv_obj_t* label_weather = lv_label_create(parent);
    lv_obj_set_pos(label_weather, 555, 75);
    lv_obj_set_size(label_weather, 160, 50);
    lv_obj_set_style_text_color(label_weather, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(label_weather, font_weather.font, LV_PART_MAIN);
	lv_label_set_text(label_weather, "多云转晴");

}

static void create_digital_clock(lv_obj_t* parent)
{
    lv_timer_t *timer = lv_timer_create(update_system_time, 1000, NULL);
    lv_timer_set_repeat_count(timer, -1);

    date_time_t dt;
    date_time_init(&dt);

    char sys_time[1024];
    sprintf(sys_time, "%02d:%02d", dt.hour, dt.minute);
    char sys_date[1024];
    sprintf(sys_date, "%d/%02d/%02d", dt.year, dt.month, dt.day);

    static lv_ft_info_t font_date;
    font_date.name = FONT_PATH;
    font_date.weight = 32;
    font_date.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_date);

	static lv_ft_info_t font_time;
	font_time.name = FONT_PATH;
	font_time.weight = 110;
	font_time.style = FT_FONT_STYLE_BOLD;
	lv_ft_font_init(&font_time);

	static lv_ft_info_t font_week;
	font_week.name = FONT_PATH;
	font_week.weight = 32;
	font_week.style = FT_FONT_STYLE_BOLD;
	lv_ft_font_init(&font_week);

    static lv_style_t style_label_date;
    lv_style_init(&style_label_date);
    lv_style_set_text_font(&style_label_date, font_date.font);
    lv_style_set_text_color(&style_label_date, lv_color_hex(0xffffff));

    label_date = lv_label_create(parent);
    lv_obj_set_pos(label_date, 26, 125);
    lv_obj_set_size(label_date, 200, 50);
    lv_obj_add_style(label_date, &style_label_date, LV_PART_MAIN);

    label_time = lv_label_create(parent);
    lv_obj_set_pos(label_time, 20, -30);
    lv_obj_set_size(label_time, 300, 150);
    lv_obj_add_style(label_time, &style_label_date, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_time, font_time.font, LV_PART_MAIN);

    label_week = lv_label_create(parent);
    lv_obj_set_pos(label_week, 210, 125);
    lv_obj_set_size(label_week, 150, 50);
    lv_obj_add_style(label_week, &style_label_date, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_week, font_week.font, LV_PART_MAIN);

    lv_label_set_text(label_date, sys_date);
    lv_label_set_text(label_time, sys_time);
    lv_label_set_text(label_week, s_weeks[dt.wday]);

}

static void create_desktop_button_item(lv_obj_t* parent, uint32_t index)
{
    static lv_ft_info_t font_text;
    font_text.name = FONT_PATH;
    font_text.weight = 36;
    font_text.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_text);

    lv_obj_t* img_icon = lv_img_create(parent);
    lv_obj_set_pos(img_icon, 20, 31);
    lv_obj_set_size(img_icon, 65, 65);
    lv_img_set_src(img_icon, s_device_icon[index]);

    lv_obj_t* label_txt = lv_label_create(parent);
    lv_obj_set_pos(label_txt, 120, 25);
    lv_obj_set_size(label_txt, 160, 50);
    lv_obj_set_style_text_font(label_txt, font_text.font, LV_PART_MAIN);
    lv_obj_set_style_text_align(label_txt, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_text(label_txt, s_device_text[index]);


}

void create_desktop_button_group(lv_obj_t* parent)
{
    static lv_style_t style_con;
    lv_style_init(&style_con);
    lv_style_set_radius(&style_con, 0);
    lv_style_set_border_width(&style_con, 0);
    lv_style_set_bg_opa(&style_con, LV_OPA_TRANSP);
    lv_style_set_flex_flow(&style_con, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_layout(&style_con, LV_LAYOUT_FLEX);

    lv_obj_t *device_container = lv_obj_create(parent);
    lv_obj_set_size(device_container, 640, 780);
    lv_obj_set_pos(device_container, 40, 235);
    lv_obj_add_style(device_container, &style_con, LV_PART_MAIN);
    lv_obj_clear_flag(device_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(device_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(device_container, 44, LV_PART_MAIN);
    lv_obj_set_style_pad_row(device_container, 30, LV_PART_MAIN);

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 15);
    lv_style_set_shadow_width(&style_btn, 0);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x2b333c));


    static lv_style_t style_btn_pressed;
    lv_style_init(&style_btn_pressed);
    lv_style_set_radius(&style_btn_pressed, 15);
    lv_style_set_shadow_width(&style_btn_pressed, 0);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0x2b333c));

    uint32_t i;
    for (i = 0; i < DEVICE_COUNT; i++) {
         lv_obj_t *btn_device = lv_btn_create(device_container);
         lv_obj_set_size(btn_device, 298, 128);
         lv_obj_add_style(btn_device, &style_btn, LV_PART_MAIN | LV_STATE_DEFAULT);
         lv_obj_add_style(btn_device, &style_btn_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
         lv_state_t btn_state = lv_obj_get_state(btn_device);
         lv_obj_set_style_outline_width(btn_device, 0, btn_state);
         lv_obj_set_style_pad_all(btn_device, 0, LV_PART_MAIN);
         create_desktop_button_item(btn_device, i);
         lv_obj_add_event_cb(btn_device, on_device_btn_clicked, LV_EVENT_CLICKED, (void*)i);
    }
}

void create_main_window(lv_obj_t* screen)
{
	lv_scr_load(screen_main);
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);

    lv_obj_t *main_window = lv_obj_create(screen);
    lv_obj_set_size(main_window, 720, 1280);
    lv_obj_add_style(main_window, &style_container, LV_PART_MAIN);
    lv_obj_clear_flag(main_window, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(main_window, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(main_window, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(main_window, "/assets/images/main_bg.png", LV_PART_MAIN);
    create_digital_clock(main_window);
    create_weather_display_area(main_window);
    create_desktop_button_group(main_window);
}

void application_init()
{
    lv_freetype_init(64, 1, 0);
    lv_png_init();
	start_ring_thread();
	screen_main = lv_obj_create(NULL); //多窗口同时显示需要创建多个screen进行切换。
	screen_other = lv_obj_create(NULL);
	lv_obj_add_event_cb(screen_other, on_window_destory, LV_EVENT_DELETE, NULL);
    create_main_window(screen_main);
}
