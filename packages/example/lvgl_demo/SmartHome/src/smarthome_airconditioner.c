/*
 * @file smarthome_airconditioner.c - is provided for use with Ingenic products.
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
#include "smarthome_airconditioner.h"
#include <stdio.h>

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
static void setting_temperature_changed(lv_event_t * e);      //调控滑块改变温度
static void airconditioner_switch_clicked(lv_event_t* e);     //开关键事件
static void airconditioner_mode_changed(lv_event_t* e);       //制冷模式、制热模式、除湿模式和送风模式切换

/**********************
 *  STATIC VARIABLES
 **********************/
static const char* s_mode_icon_default[] = {
    "../assets/images/air_conditioner_cold.png",
    "../assets/images/air_conditioner_warm.png",
    "../assets/images/air_conditioner_exchange.png",
    "../assets/images/air_conditioner_wet.png"
};
static const char* s_mode_icon_checked[] = {
    "../assets/images/air_conditioner_cold_on.png",
    "../assets/images/air_conditioner_warm_on.png",
    "../assets/images/air_conditioner_exchange_on.png",
    "../assets/images/air_conditioner_wet_on.png"
};
static const char* s_wind_gear_default[] = {
    "../assets/images/air_conditioner_wind_small.png",
    "../assets/images/air_conditioner_wind_mid.png",
    "../assets/images/air_conditioner_wind_big.png",
};
static const char* s_wind_gear_checked[] = {
     "../assets/images/air_conditioner_wind_small_on.png",
    "../assets/images/air_conditioner_wind_mid_on.png",
    "../assets/images/air_conditioner_wind_big_on.png",
};

static int temperature;

static lv_obj_t* airconditioner_window;
static lv_obj_t* label_temp;

static lv_style_t style_flex_layouttainer;
static lv_style_t style_flex_layout;


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_airconditioner_window(lv_event_t* e)
{
	start_ring(RING_TONE);
    destroy_airconditioner_window();
}

static void setting_temperature_changed(lv_event_t* e)
{
    char str_temp[10];
    lv_obj_t * slider = lv_event_get_target(e);
    temperature = (int)lv_arc_get_value(slider);
    sprintf(str_temp, "%02d", temperature);
    lv_label_set_text(label_temp, str_temp);
}

static void on_airconditioner_switch_triggered(lv_event_t* e)
{
	start_ring(RING_TONE);
}

static void on_mode_changed(lv_event_t* e)
{
    int i = 0;
    lv_obj_t* current_btn = lv_event_get_target(e);
    lv_obj_t* parent = lv_obj_get_parent(current_btn);
    for (i == 0; i < lv_obj_get_child_cnt(parent); i++) {
        lv_obj_t* child = lv_obj_get_child(parent, i);
        lv_imgbtn_set_state(child, LV_IMGBTN_STATE_RELEASED);
    }
    lv_imgbtn_set_state(current_btn, LV_IMGBTN_STATE_CHECKED_RELEASED);
	start_ring(RING_TONE);
}

static void on_wind_gear_changed(lv_event_t* e)
{
    int i = 0;
    lv_obj_t* current_btn = lv_event_get_target(e);
    lv_obj_t* parent = lv_obj_get_parent(current_btn);
    for (i == 0; i < lv_obj_get_child_cnt(parent); i++) {
        lv_obj_t* child = lv_obj_get_child(parent, i);
        lv_imgbtn_set_state(child, LV_IMGBTN_STATE_RELEASED);
    }
    lv_imgbtn_set_state(current_btn, LV_IMGBTN_STATE_CHECKED_RELEASED);
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

    static lv_style_t style_switch_knob;
    lv_style_init(&style_switch_knob);
    lv_style_set_bg_color(&style_switch_knob, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_switch_knob, lv_color_hex(0xe7e7e7));
    lv_style_set_bg_grad_dir(&style_switch_knob, LV_GRAD_DIR_VER); // 设置样式的颜色渐变方向为垂直方向
    lv_style_set_border_color(&style_switch_knob, lv_color_hex(0xefefef));
    lv_style_set_border_width(&style_switch_knob, 1);

    static lv_style_t style_switch;
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
	lv_obj_add_event_cb(btn_switch, on_airconditioner_switch_triggered, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* label = lv_label_create(title_con);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(label, 0, 4);
    lv_obj_set_size(label, 480, 30);
    lv_obj_add_style(label, &style_title, LV_PART_MAIN);
    lv_label_set_text(label, "空调");
}

static void create_mode_btn_group(lv_obj_t* parent)
{
    lv_style_init(&style_flex_layout);
    lv_style_set_radius(&style_flex_layout, 0);
    lv_style_set_border_width(&style_flex_layout, 0);
    lv_style_set_bg_opa(&style_flex_layout, LV_OPA_TRANSP);
    lv_style_set_flex_flow(&style_flex_layout, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_layout(&style_flex_layout, LV_LAYOUT_FLEX);

    lv_obj_t *mode_container = lv_obj_create(parent);
    lv_obj_set_size(mode_container, 46, 230);
    lv_obj_set_pos(mode_container, 25, 72);
    lv_obj_add_style(mode_container, &style_flex_layout, LV_PART_MAIN);
    lv_obj_clear_flag(mode_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(mode_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(mode_container, 15, LV_PART_MAIN);

    uint16_t i;
    for (i = 0; i < 4; i++) {
        lv_obj_t* btn_mode = lv_imgbtn_create(mode_container);
        lv_obj_set_size(btn_mode, 46, 46);
        lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_RELEASED, NULL, s_mode_icon_default[i], NULL);
        lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, s_mode_icon_checked[i], NULL);
        lv_obj_add_flag(btn_mode, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(btn_mode, on_mode_changed, LV_EVENT_CLICKED, NULL);
        if (i == 0) lv_obj_add_state(btn_mode, LV_STATE_CHECKED);
    }
}

static void create_wind_gear_btn_group(lv_obj_t* parent)
{
    lv_obj_t *wind_container = lv_obj_create(parent);
    lv_obj_set_size(wind_container, 46, 194);
    lv_obj_set_pos(wind_container, 409, 86);
    lv_obj_add_style(wind_container, &style_flex_layout, LV_PART_MAIN);
    lv_obj_clear_flag(wind_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(wind_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(wind_container, 28, LV_PART_MAIN);

    uint16_t i;
    for (i = 0; i < 3; i++) {
        lv_obj_t* btn_mode = lv_imgbtn_create(wind_container);
        lv_obj_set_size(btn_mode, 46, 46);
        lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_RELEASED, NULL, s_wind_gear_default[i], NULL);
        lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, s_wind_gear_checked[i], NULL);
        lv_obj_add_flag(btn_mode, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_add_event_cb(btn_mode, on_wind_gear_changed, LV_EVENT_CLICKED, NULL);
        if (i == 0) lv_obj_add_state(btn_mode, LV_STATE_CHECKED);
    }

}

static void create_display_area(lv_obj_t* parent)
{
    char str_temp[10];

    static lv_ft_info_t font_temp;
    font_temp.name = FONT_PATH;
    font_temp.weight = 70;
    font_temp.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_temp);

    static lv_ft_info_t font_unit;
    font_unit.name = FONT_PATH;
    font_unit.weight = 26;
    font_temp.style = FT_FONT_STYLE_BOLD;
    lv_ft_font_init(&font_unit);

    static lv_ft_info_t font_marker;
    font_marker.name = FONT_PATH;
    font_marker.weight = 40;
    font_marker.style = FT_FONT_STYLE_BOLD;
    lv_ft_font_init(&font_marker);

    static lv_ft_info_t font_range;
    font_range.name = FONT_PATH;
    font_range.weight = 16;
    font_range.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_range);

    static lv_ft_info_t font_text;
    font_text.name = FONT_PATH;
    font_text.weight = 14;
    font_text.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_text);

    static lv_style_t style_label_temp;
    lv_style_init(&style_label_temp);
    lv_style_set_text_color(&style_label_temp, lv_color_hex(0x000000));

    static lv_style_t style_slider_knob;
    lv_style_init(&style_slider_knob);
    lv_style_set_pad_all(&style_slider_knob, 8); /*Makes the knob larger*/
    lv_style_set_bg_color(&style_slider_knob, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_slider_knob, lv_color_hex(0xe7e7e7));
    lv_style_set_bg_grad_dir(&style_slider_knob, LV_GRAD_DIR_VER); // 设置样式的颜色渐变方向为垂直方向
    lv_style_set_border_color(&style_slider_knob, lv_color_hex(0xc2c2c2));
    lv_style_set_border_width(&style_slider_knob, 1);

    lv_obj_t * arc = lv_arc_create(parent);
    lv_obj_set_size(arc, 210, 210);
    lv_obj_set_pos(arc, 135, 80);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_value(arc, temperature);
    lv_arc_set_range(arc, 18, 32);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x12b1d1), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_add_style(arc, &style_slider_knob, LV_PART_KNOB);
    lv_obj_add_event_cb(arc, setting_temperature_changed, LV_EVENT_VALUE_CHANGED, NULL);

    create_mode_btn_group(parent);
    create_wind_gear_btn_group(parent);

    label_temp = lv_label_create(parent);
    lv_obj_set_size(label_temp, 120, 120);
    lv_obj_set_pos(label_temp, 180, 102);
    lv_obj_set_style_text_align(label_temp, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_temp, font_temp.font, LV_PART_MAIN);
    lv_obj_add_style(label_temp, &style_label_temp, LV_PART_MAIN);
    sprintf(str_temp, "%02d", temperature);
    lv_label_set_text(label_temp, str_temp);

    lv_obj_t* label_unit = lv_label_create(parent);
    lv_obj_set_size(label_unit, 50, 40);
    lv_obj_set_pos(label_unit, 292, 148);
    lv_obj_set_style_text_font(label_unit, font_unit.font, LV_PART_MAIN);
    lv_obj_add_style(label_unit, &style_label_temp, LV_PART_MAIN);
    lv_label_set_text(label_unit, "C");

    lv_obj_t* label_marker = lv_label_create(parent);
    lv_obj_set_size(label_marker, 40, 40);
    lv_obj_set_pos(label_marker, 280, 114);
    lv_obj_set_style_text_font(label_marker, font_marker.font, LV_PART_MAIN);
    lv_obj_add_style(label_marker, &style_label_temp, LV_PART_MAIN);
    lv_label_set_text(label_marker, "°");

    lv_obj_t* label_text = lv_label_create(parent);
    lv_obj_set_size(label_text, 100, 30);
    lv_obj_set_pos(label_text, 190, 225);
    lv_obj_set_style_text_align(label_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_text, font_text.font, LV_PART_MAIN);
    lv_obj_add_style(label_text, &style_label_temp, LV_PART_MAIN);
    lv_label_set_text(label_text, "设定温度");

    lv_obj_t* label_range_min = lv_label_create(parent);
    lv_obj_set_size(label_range_min, 60, 30);
    lv_obj_set_pos(label_range_min, 165, 262);
    lv_obj_add_style(label_range_min, &style_label_temp, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_range_min, font_range.font, LV_PART_MAIN);
    lv_label_set_text(label_range_min, "18℃");

    lv_obj_t* label_range_max = lv_label_create(parent);
    lv_obj_set_size(label_range_max, 60, 30);
    lv_obj_set_pos(label_range_max, 300, 262);
    lv_obj_add_style(label_range_max, &style_label_temp, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_range_max, font_range.font, LV_PART_MAIN);
    lv_label_set_text(label_range_max, "32℃");
}

void destroy_airconditioner_window()
{
    lv_obj_del(airconditioner_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, airconditioner_window);
}

void create_airconditioner_window(lv_obj_t* screen)
{
    temperature = 26;

    lv_style_init(&style_flex_layouttainer);
    lv_style_set_radius(&style_flex_layouttainer, 0);
    lv_style_set_border_width(&style_flex_layouttainer, 0);
    lv_style_set_pad_all(&style_flex_layouttainer, 0);
    airconditioner_window = lv_obj_create(screen);
    lv_obj_add_style(airconditioner_window, &style_flex_layouttainer, LV_PART_MAIN);
    lv_obj_set_size(airconditioner_window, 480, 320);
    lv_obj_clear_flag(airconditioner_window, LV_OBJ_FLAG_SCROLLABLE);

    create_title_bar(airconditioner_window);
    create_display_area(airconditioner_window);

    lv_obj_t* btn_return = lv_imgbtn_create(airconditioner_window);
    lv_obj_set_pos(btn_return, 420, 4);
    lv_obj_set_size(btn_return, 56, 52);
    lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
    lv_obj_add_event_cb(btn_return, quit_airconditioner_window, LV_EVENT_CLICKED, NULL);

}
