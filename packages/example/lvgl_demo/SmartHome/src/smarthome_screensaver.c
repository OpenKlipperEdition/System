/*
 * @file smarthome_screensaver.c - is provided for use with Ingenic products.
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
 * Created: 2022/05/09
 * Updated:
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "smarthome_screensaver.h"
#include "smarthome_assistance.h"
#include "date_time.h"
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "../assets/fonts/sourcehan.ttf"
#define RING_TONE "../assets/songs/btn_sound.wav"
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* label_time;
static lv_obj_t* label_date;
static lv_obj_t* label_week;
static lv_obj_t* screensaver;
static lv_obj_t* assist;    //新窗口的screen
static lv_timer_t* timer_clock;
static lv_style_t style_container;
static lv_style_t style_label_date;

static const char *s_weeks[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};

/*******************
 *    MACROS
 *******************/
/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void on_assistance_btn_clicked(lv_event_t* e)
{
	start_ring(RING_TONE);
	lv_scr_load(assist);
	create_assistance_window(assist);
}

static void quit_screensaver_window(lv_event_t* e)
{
	destroy_screensaver_window();
}

static void on_window_destory(lv_event_t* e)
{
	lv_obj_t* screen_other = lv_obj_get_screen(screensaver);
	lv_scr_load(screen_other);
}

static void update_system_time(lv_timer_t *t)
{
    date_time_t dt;
    date_time_init(&dt);
    char time[1024];
    sprintf(time, "%02d:%02d", dt.hour, dt.minute);
    char date[1024];
    sprintf(date, "%04d年%02d月%02d日", dt.year, dt.month, dt.day);

    lv_label_set_text(label_time, time);
    lv_label_set_text(label_date, date);
    lv_label_set_text(label_week, s_weeks[dt.wday]);
}

static void create_assistance_button(lv_obj_t* parent)
{
	lv_obj_t* btn_assistant = lv_imgbtn_create(parent);
	lv_obj_set_size(btn_assistant, 80, 80);
	lv_obj_set_pos(btn_assistant,  364, 207);
	lv_imgbtn_set_src(btn_assistant, LV_IMGBTN_STATE_RELEASED, "./assets/images/standby_xiaona.png", NULL, NULL);
	lv_obj_add_event_cb(btn_assistant, on_assistance_btn_clicked, LV_EVENT_CLICKED, NULL);
}

static void create_digital_clock(lv_obj_t *parent)
{
    timer_clock = lv_timer_create(update_system_time, 1000, NULL);
    lv_timer_set_repeat_count(timer_clock, -1);

    date_time_t dt;
    date_time_init(&dt);

    char sys_time[10];
    sprintf(sys_time, "%02d:%02d", dt.hour, dt.minute);
    char sys_date[30];
    sprintf(sys_date, "%04d年%02d月%02d日", dt.year, dt.month, dt.day);

	static lv_ft_info_t font_date;
    font_date.name = FONT_PATH;
    font_date.weight = 25;
    font_date.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_date);

	static lv_ft_info_t font_time;
	font_time.name = FONT_PATH;
	font_time.weight = 90;
	font_time.style = FT_FONT_STYLE_BOLD;
	lv_ft_font_init(&font_time);

	static lv_ft_info_t font_week;
	font_week.name = FONT_PATH;
	font_week.weight = 22;
	font_week.style = FT_FONT_STYLE_BOLD;
	lv_ft_font_init(&font_week);

    lv_style_init(&style_label_date);
    lv_style_set_text_color(&style_label_date, lv_color_hex(0xffffff));
    lv_style_set_text_font(&style_label_date, font_date.font);

    label_date = lv_label_create(parent);
    lv_obj_set_pos(label_date, 46, 30);
    lv_obj_set_size(label_date, 200, 40);
    lv_obj_add_style(label_date, &style_label_date, LV_PART_MAIN);

    label_time = lv_label_create(parent);
    lv_obj_set_pos(label_time, 25, 45);
    lv_obj_set_size(label_time, 300, 120);
    lv_obj_add_style(label_time, &style_label_date, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_time, font_time.font, LV_PART_MAIN);

    label_week = lv_label_create(parent);
    lv_obj_set_pos(label_week, 95, 200);
    lv_obj_set_size(label_week, 80, 30);
    lv_obj_add_style(label_week, &style_label_date, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_week, font_week.font, LV_PART_MAIN);

    lv_label_set_text(label_date, sys_date);
    lv_label_set_text(label_time, sys_time);
    lv_label_set_text(label_week, s_weeks[dt.wday]);
}

static void create_weather_display_area(lv_obj_t* parent)
{
	static lv_ft_info_t font_weather;
    font_weather.name = FONT_PATH;
    font_weather.weight = 25;
    font_weather.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_weather);

	lv_obj_t* label_temp = lv_label_create(parent);
    lv_obj_set_pos(label_temp, 340, 90);
    lv_obj_set_size(label_temp, 120, 50);
    lv_obj_add_style(label_temp, &style_label_date, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_temp, font_weather.font, LV_PART_MAIN);
	lv_label_set_text(label_temp, "12~20℃");

	lv_obj_t* label_weather = lv_label_create(parent);
    lv_obj_set_pos(label_weather, 340, 126);
    lv_obj_set_size(label_weather, 120, 50);
    lv_obj_add_style(label_weather, &style_label_date, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_weather, font_weather.font, LV_PART_MAIN);
	lv_label_set_text(label_weather, "多云转晴");

}

void destroy_screensaver_window()
{
	lv_timer_del(timer_clock);
	lv_obj_del(assist);
	lv_obj_del(screensaver);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, screensaver);
}

void create_screensaver_window(lv_obj_t* screen)
{
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);
    lv_style_set_pad_all(&style_container, 0);

    assist = lv_obj_create(NULL);
	lv_obj_add_event_cb(assist, on_window_destory, LV_EVENT_DELETE, NULL);

	screensaver = lv_obj_create(screen);
    lv_obj_set_size(screensaver, 480, 320);
    lv_obj_add_style(screensaver, &style_container, LV_PART_MAIN);
    lv_obj_clear_flag(screensaver, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_img_src(screensaver, "../assets/images/standby_background.png", LV_PART_MAIN);
    lv_obj_add_event_cb(screensaver, quit_screensaver_window, LV_EVENT_CLICKED, NULL);

    create_digital_clock(screensaver);
	create_weather_display_area(screensaver);
	create_assistance_button(screensaver);
}

