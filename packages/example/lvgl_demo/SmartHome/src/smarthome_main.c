/*
 * @file smarthome_main.c - is provided for use with Ingenic products.
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
 * Created: 2022/04/28
 * Updated:
 */

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"
#include "smarthome_main.h"
#include "smarthome_screensaver.h"
#include "smarthome_curtain.h"
#include "smarthome_lamp.h"
#include "smarthome_monitor.h"
#include "smarthome_airpurifier.h"
#include "smarthome_airconditioner.h"
#include "smarthome_scene.h"
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
static lv_obj_t* screen_main;
static lv_obj_t* screen_other;
static lv_obj_t* label_time;
static lv_obj_t* label_date;
static lv_obj_t* label_week;
static lv_style_t style_container;
static lv_style_t style_label_date;
static lv_timer_t* timer_screensaver;

static lv_ft_info_t font_date;

static const char *s_weeks[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
static const char *s_device_text[] = {
    "空调",
    "灯光",
    "空气净化",
    "窗帘",
    "布防",
    "场景",
};
static const char *s_device_icon[] = {
    "../assets/images/icon_airconditioner.png",
    "../assets/images/icon_lamp.png",
    "../assets/images/icon_purifier.png",
    "../assets/images/icon_curtain.png",
    "../assets/images/icon_monitor.png",
    "../assets/images/icon_scene.png",
};

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void screensaver_triggered(lv_timer_t* t)
{
	lv_timer_pause(timer_screensaver);
	lv_scr_load(screen_other);
	create_screensaver_window(screen_other);
}

void on_window_destory(lv_event_t* e)
{
	lv_timer_resume(timer_screensaver);
	lv_timer_reset(timer_screensaver);
	lv_scr_load(screen_main);
}

static void on_device_btn_clicked(lv_event_t* e)
{
	lv_timer_pause(timer_screensaver);
	lv_scr_load(screen_other);
	start_ring(RING_TONE);

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
    sprintf(date, "%04d年%02d月%02d日", dt.year, dt.month, dt.day);

    lv_label_set_text(label_time, time);
    lv_label_set_text(label_date, date);
    lv_label_set_text(label_week, s_weeks[dt.wday]);
}

static void create_digital_clock(lv_obj_t *parent)
{
    lv_timer_t *timer = lv_timer_create(update_system_time, 1000, NULL);
    lv_timer_set_repeat_count(timer, -1);

    date_time_t dt;
    date_time_init(&dt);

    char sys_time[1024];
    sprintf(sys_time, "%02d:%02d", dt.hour, dt.minute);
    char sys_date[1024];
    sprintf(sys_date, "%04d年%02d月%02d日", dt.year, dt.month, dt.day);

    font_date.name = FONT_PATH;
    font_date.weight = 15;
    font_date.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_date);

    lv_style_init(&style_label_date);
    lv_style_set_text_font(&style_label_date, font_date.font);
    lv_style_set_text_color(&style_label_date, lv_color_hex(0xffffff));

    label_date = lv_label_create(parent);
    lv_obj_set_pos(label_date, 20, 6);
    lv_obj_set_size(label_date, 120, 30);
    lv_obj_add_style(label_date, &style_label_date, LV_PART_MAIN);

    label_week = lv_label_create(parent);
    lv_obj_set_pos(label_week, 140, 6);
    lv_obj_set_size(label_week, 50, 30);
    lv_obj_add_style(label_week, &style_label_date, LV_PART_MAIN);

    label_time = lv_label_create(parent);
    lv_obj_set_pos(label_time, 195, 6);
    lv_obj_set_size(label_time, 50, 30);
    lv_obj_add_style(label_time, &style_label_date, LV_PART_MAIN);

    lv_label_set_text(label_date, sys_date);
    lv_label_set_text(label_time, sys_time);
    lv_label_set_text(label_week, s_weeks[dt.wday]);
}

void create_device_button_item(lv_obj_t *parent, uint16_t index)
{
    static lv_ft_info_t font_device;
    font_device.name = FONT_PATH;
    font_device.weight = 20;
    font_device.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_device);

    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_font(&style_label, font_device.font);
    lv_style_set_text_color(&style_label, lv_color_hex(0xffffff));
    lv_style_set_text_align(&style_label, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *img_icon = lv_img_create(parent);
    lv_img_set_src(img_icon, s_device_icon[index]);
    lv_obj_set_size(img_icon, 68, 68);
    lv_obj_set_pos(img_icon, 36, 15);

    lv_obj_t *label_device = lv_label_create(parent);
    lv_obj_set_pos(label_device, 20, 85);
    lv_obj_set_size(label_device, 100, 40);

    lv_obj_add_style(label_device, &style_label, LV_PART_MAIN);
    lv_label_set_text(label_device, s_device_text[index]);
}

void create_device_button_group(lv_obj_t *parent)
{
    static lv_style_t style_con;
    lv_style_init(&style_con);
    lv_style_set_radius(&style_con, 0);
    lv_style_set_border_width(&style_con, 0);
    lv_style_set_bg_opa(&style_con, LV_OPA_TRANSP);
    lv_style_set_flex_flow(&style_con, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_layout(&style_con, LV_LAYOUT_FLEX);

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 0);
    lv_style_set_bg_opa(&style_btn, 89);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x000000));
    lv_style_set_shadow_width(&style_btn, 0);

    static lv_style_t style_btn_pressed;
    lv_style_init(&style_btn_pressed);
    lv_style_set_radius(&style_btn_pressed, 0);
    lv_style_set_bg_opa(&style_btn_pressed, 191);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0x12b1d1));
    lv_style_set_shadow_width(&style_btn_pressed, 0);

    lv_obj_t *device_container = lv_obj_create(parent);
    lv_obj_set_size(device_container, 448, 268);
    lv_obj_set_pos(device_container, 16, 42);
    lv_obj_add_style(device_container, &style_con, LV_PART_MAIN);
    lv_obj_clear_flag(device_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(device_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(device_container, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_column(device_container, 14, LV_PART_MAIN);

    uint32_t i;
    for (i = 0; i < 6; i++)
    {
        lv_obj_t *btn_device = lv_btn_create(device_container);
        lv_obj_set_size(btn_device, 140, 124);
        lv_obj_add_style(btn_device, &style_btn, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_style(btn_device, &style_btn_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_state_t btn_state = lv_obj_get_state(btn_device);
        lv_obj_set_style_outline_width(btn_device, 0, btn_state);
        lv_obj_set_style_pad_all(btn_device, 0, LV_PART_MAIN);
        create_device_button_item(btn_device, i);
        lv_obj_add_event_cb(btn_device, on_device_btn_clicked, LV_EVENT_CLICKED, (void*)i);
    }
}

void create_main_window(lv_obj_t *parent)
{
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);

    lv_obj_t *main_window = lv_obj_create(parent);
    lv_obj_set_size(main_window, 480, 320);
    lv_obj_add_style(main_window, &style_container, LV_PART_MAIN);
    lv_obj_clear_flag(main_window, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(main_window, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(main_window, "../assets/images/home_bg.png", LV_PART_MAIN);

    create_device_button_group(main_window);
    create_digital_clock(main_window);
	timer_screensaver = lv_timer_create(screensaver_triggered, 5*1000, NULL);
	lv_timer_set_repeat_count(timer_screensaver, -1);
}

void application_init()
{
    lv_freetype_init(64, 1, 0);
    lv_png_init();
	start_ring_thread();
    screen_main = lv_obj_create(NULL);
    screen_other = lv_obj_create(NULL);
	lv_obj_add_event_cb(screen_other, on_window_destory, LV_EVENT_DELETE, NULL);
    create_main_window(screen_main);
}
