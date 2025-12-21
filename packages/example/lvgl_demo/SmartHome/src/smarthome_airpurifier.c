/*
 * @file smarthome_airpurifier.c - is provided for use with Ingenic products.
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
#include "smarthome_airpurifier.h"

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH     "../assets/fonts/sourcehan.ttf"
#define RING_TONE "../assets/songs/btn_sound.wav"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void on_mode_purifier(lv_event_t* e);
static void on_mode_dehumify(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_style_t style_switch_knob;
static lv_style_t style_switch;

static lv_obj_t* airpurifier_window;
static lv_obj_t* mode_purifier;
static lv_obj_t* mode_dehumify;

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_airpurifier_window(lv_event_t* e)
{
	start_ring(RING_TONE);
    destroy_airpurifier_window();
}

static void on_mode_purifier(lv_event_t* e)
{
    lv_imgbtn_set_state(mode_purifier, LV_IMGBTN_STATE_CHECKED_RELEASED);
    lv_imgbtn_set_state(mode_dehumify, LV_IMGBTN_STATE_RELEASED);
	start_ring(RING_TONE);
}

static void on_mode_dehumifier(lv_event_t* e)
{
    lv_imgbtn_set_state(mode_purifier, LV_IMGBTN_STATE_RELEASED);
    lv_imgbtn_set_state(mode_dehumify, LV_IMGBTN_STATE_CHECKED_RELEASED);
	start_ring(RING_TONE);
}

static void on_switch_triggered(lv_event_t* e)
{
	start_ring(RING_TONE);
}

static void create_title_bar(lv_obj_t* parent)
{
    static lv_ft_info_t font_title;
    font_title.name = FONT_PATH;
    font_title.weight = 22;
    font_title.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_title);

    static lv_style_t style_title_bg;
    lv_style_init(&style_title_bg);
    lv_style_set_bg_color(&style_title_bg, lv_color_hex(0x000000));
    lv_style_set_bg_opa(&style_title_bg, 191);
    lv_style_set_radius(&style_title_bg, 0);
    lv_style_set_border_width(&style_title_bg, 0);
    lv_style_set_pad_all(&style_title_bg, 0);

    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_title.font);
    lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

    lv_style_init(&style_switch_knob);
    lv_style_set_bg_color(&style_switch_knob, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_switch_knob, lv_color_hex(0xe7e7e7));
    lv_style_set_bg_grad_dir(&style_switch_knob, LV_GRAD_DIR_VER); // 设置样式的颜色渐变方向为垂直方向
    lv_style_set_border_color(&style_switch_knob, lv_color_hex(0xefefef));
    lv_style_set_border_width(&style_switch_knob, 1);

    lv_style_init(&style_switch);
    lv_style_set_bg_color(&style_switch, lv_color_hex(0xffffff));
    lv_style_set_border_color(&style_switch, lv_color_hex(0x000000));
    lv_style_set_border_opa(&style_switch, 48);
    lv_style_set_border_width(&style_switch, 1);
    lv_style_set_border_side(&style_switch, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP);

    lv_obj_t* title_con = lv_obj_create(parent);
    lv_obj_set_pos(title_con, 0, 0);
    lv_obj_set_size(title_con, 480, 48);
    lv_obj_add_style(title_con, &style_title_bg, LV_PART_MAIN);
    lv_obj_clear_flag(title_con, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_switch = lv_switch_create(parent);
    lv_obj_set_pos(btn_switch, 10, 9);
    lv_obj_set_size(btn_switch, 62, 30);
    lv_obj_add_style(btn_switch, &style_switch_knob, LV_PART_KNOB);
    lv_obj_add_style(btn_switch, &style_switch, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0x12b1d1), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_state_t switch_state = lv_obj_get_state(btn_switch);
    lv_obj_set_style_outline_width(btn_switch, 0, switch_state);
    lv_obj_add_state(btn_switch, LV_STATE_CHECKED);
    lv_obj_add_event_cb(btn_switch, on_switch_triggered, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* label = lv_label_create(title_con);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(label, 0, 4);
    lv_obj_set_size(label, 480, 30);
    lv_obj_add_style(label, &style_title, LV_PART_MAIN);
    lv_label_set_text(label, "空气净化");
}

static void create_display_area(lv_obj_t* parent)
{
    static lv_style_t style_label;

    static lv_ft_info_t font_label;
    font_label.name = FONT_PATH;
    font_label.weight = 16;
    font_label.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_label);

    lv_style_init(&style_label);
    lv_style_set_text_font(&style_label, font_label.font);
    lv_style_set_text_color(&style_label, lv_color_hex(0x000000));

    lv_obj_t* img_purifier = lv_img_create(parent);
    lv_obj_set_size(img_purifier, 300, 264);
    lv_obj_set_pos(img_purifier, 90, 55);
    lv_img_set_src(img_purifier, "../assets/images/air_purifier_bg.png");

    lv_obj_t* label_temperature = lv_label_create(parent);
    lv_obj_set_pos(label_temperature, 370, 70);
    lv_obj_set_size(label_temperature, 100, 25);
    lv_obj_add_style(label_temperature, &style_label, LV_PART_MAIN);
    lv_label_set_text(label_temperature, "温度：25℃");

    lv_obj_t* label_humidity = lv_label_create(parent);
    lv_obj_set_pos(label_humidity, 370, 95);
    lv_obj_set_size(label_humidity, 100, 25);
    lv_obj_add_style(label_humidity, &style_label, LV_PART_MAIN);
    lv_label_set_text(label_humidity, "湿度：50%");

}

static void create_control_bar_area(lv_obj_t* parent)
{
    mode_purifier = lv_imgbtn_create(parent);
    lv_obj_set_size(mode_purifier, 72, 72);
    lv_obj_set_pos(mode_purifier, 18, 235);
    lv_imgbtn_set_src(mode_purifier, LV_IMGBTN_STATE_RELEASED, NULL, "../assets/images/purifier_switch_off.png", NULL);
    lv_imgbtn_set_src(mode_purifier, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, "../assets/images/purifier_switch_on.png", NULL);
    lv_obj_add_flag(mode_purifier, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(mode_purifier, on_mode_purifier, LV_EVENT_CLICKED, NULL);
    lv_imgbtn_set_state(mode_purifier, LV_IMGBTN_STATE_CHECKED_RELEASED);

    mode_dehumify = lv_imgbtn_create(parent);
    lv_obj_set_size(mode_dehumify, 72, 72);
    lv_obj_set_pos(mode_dehumify, 390, 235);
    lv_imgbtn_set_src(mode_dehumify, LV_IMGBTN_STATE_RELEASED, NULL, "../assets/images/humidify_switch_off.png", NULL);
    lv_imgbtn_set_src(mode_dehumify, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, "../assets/images/humidify_switch_on.png", NULL);
    lv_obj_add_flag(mode_dehumify, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(mode_dehumify, on_mode_dehumifier, LV_EVENT_CLICKED, NULL);
}

void destroy_airpurifier_window()
{
    lv_obj_del(airpurifier_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, airpurifier_window);
}

void create_airpurifier_window(lv_obj_t* screen)
{
    static lv_style_t style_container;
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);

    airpurifier_window = lv_obj_create(screen);
    lv_obj_add_style(airpurifier_window, &style_container, LV_PART_MAIN);
    lv_obj_set_size(airpurifier_window, 480, 320);
    lv_obj_clear_flag(airpurifier_window, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(airpurifier_window, 0, LV_PART_MAIN);

    create_title_bar(airpurifier_window);
    create_display_area(airpurifier_window);
    create_control_bar_area(airpurifier_window);

    lv_obj_t* btn_return = lv_imgbtn_create(airpurifier_window);
    lv_obj_set_pos(btn_return, 420, 4);
    lv_obj_set_size(btn_return, 56, 52);
    lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
    lv_obj_add_event_cb(btn_return, quit_airpurifier_window, LV_EVENT_CLICKED, NULL);
}
