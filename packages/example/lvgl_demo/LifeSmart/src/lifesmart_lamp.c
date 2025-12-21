/*
 * @file lifesmart_lamp.c
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
#include "lifesmart_lamp.h"

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "/assets/fonts/sourcehan.ttf"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void on_lamp_brightness_changed(lv_event_t * e);      //调控滑块改变灯的亮度
static void on_lamp_switch_clicked(lv_event_t* e);     //开关键事件
/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* lamp_window;
static lv_obj_t* img_light;
static lv_obj_t* controller_container;
static lv_obj_t* slider;
static int brightness;
static int color_type = LIGHT_COLOR_WHITE;   //默认是白色
/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_lamp_window(lv_event_t* e)
{
	close_lamp_window();
}

static void on_lamp_brightness_changed(lv_event_t* e)
{
	lv_obj_t* slider = lv_event_get_target(e);
	brightness = (int)lv_slider_get_value(slider);
	int opacity = 1.0 * (brightness - 20) / (100 - 20) * (255 * 0.7) + 0.3 * 255;   //乘以1.0，使原来的除法得到float型变量
	lv_obj_set_style_img_opa(img_light, opacity, LV_PART_MAIN);
}

static void on_lamp_switch_clicked(lv_event_t* e)
{
	lv_obj_t * button = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_VALUE_CHANGED) {
		if(lv_obj_has_state(button, LV_STATE_CHECKED)) {   //lamp is on
			lv_obj_clear_flag(img_light, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(controller_container, LV_OBJ_FLAG_HIDDEN);
		} else {      //lamp is off
			lv_obj_add_flag(img_light, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(controller_container, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

static void create_controller_area(lv_obj_t* parent)
{
	controller_container = lv_obj_create(parent);
	lv_obj_set_size(controller_container, 720, 100);
	lv_obj_set_pos(controller_container, 0, 1026);
	lv_obj_clear_flag(controller_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(controller_container, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(controller_container, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(controller_container, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(controller_container, LV_OPA_0, LV_PART_MAIN);

	static lv_ft_info_t font_range;
	font_range.name = FONT_PATH;
	font_range.weight = 26;
	font_range.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_range);

	static lv_style_t style_label_range;
	lv_style_init(&style_label_range);
	lv_style_set_text_font(&style_label_range, font_range.font);
	lv_style_set_text_color(&style_label_range, lv_color_hex(0xffffff));

	lv_obj_t* label_lightness_min = lv_label_create(controller_container);
	lv_obj_set_pos(label_lightness_min, 28, 30);
	lv_obj_set_size(label_lightness_min, 80, 30);
	lv_obj_set_style_text_align(label_lightness_min, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_lightness_min, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_lightness_min, "20%");

	lv_obj_t* label_lightness_max = lv_label_create(controller_container);
	lv_obj_set_pos(label_lightness_max, 620, 30);
	lv_obj_set_size(label_lightness_max, 80, 30);
	lv_obj_set_style_text_align(label_lightness_max, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_lightness_max, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_lightness_max, "100%");

	static lv_style_t style_slider;
	lv_style_init(&style_slider);
	lv_style_set_bg_opa(&style_slider, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider, lv_color_hex(0xefefef));

	static lv_style_t style_slider_indicator;
	lv_style_init(&style_slider_indicator);
	lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_indicator, lv_color_hex(0xfff1df));
	lv_style_set_bg_grad_color(&style_slider_indicator, lv_color_hex(0xf7a53b));
	lv_style_set_bg_grad_dir(&style_slider_indicator, LV_GRAD_DIR_HOR);

	static lv_style_t style_slider_knob;
	lv_style_init(&style_slider_knob);
	lv_style_set_pad_all(&style_slider_knob, 20);
	lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_knob, lv_color_hex(0xffd249));

	lv_obj_t* slider = lv_slider_create(controller_container);
	lv_obj_set_pos(slider, 130, 40);
	lv_obj_set_size(slider, 460, 30);
	lv_slider_set_range(slider, 20, 100);
	lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
	lv_obj_add_style(slider, &style_slider, LV_PART_MAIN);
	lv_obj_add_style(slider, &style_slider_indicator,  LV_PART_INDICATOR);
	lv_obj_add_style(slider, &style_slider_knob, LV_PART_KNOB);
	lv_obj_add_event_cb(slider, on_lamp_brightness_changed, LV_EVENT_VALUE_CHANGED, NULL);

}

static void create_display_area(lv_obj_t* parent)
{
	lv_obj_t* btn_switch = lv_switch_create(parent);
	lv_obj_set_pos(btn_switch, 550, 180);
	lv_obj_set_size(btn_switch, 125, 60);
	lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0xe7e7e7), LV_PART_MAIN);
	lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0xffd249), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_state_t switch_state = lv_obj_get_state(btn_switch);
	lv_obj_set_style_outline_width(btn_switch, 0, switch_state);
	lv_obj_add_state(btn_switch, LV_STATE_CHECKED);
	lv_obj_add_event_cb(btn_switch, on_lamp_switch_clicked, LV_EVENT_ALL, NULL);

	lv_obj_t* img_lamp = lv_img_create(parent);
	lv_obj_set_pos(img_lamp, 141, 386);
	lv_obj_set_size(img_lamp, 438, 368);
	lv_img_set_src(img_lamp, "/assets/images/lamp_device.png");

	img_light = lv_img_create(parent);
	lv_obj_set_pos(img_light, 138, 468);
	lv_obj_set_size(img_light, 443, 423);
	lv_img_set_src(img_light, "/assets/images/lamp_shine.png");

	//   lv_obj_set_style_img_recolor(img_light, lv_color_hex(0xffff00), 0);
}

static void create_title_bar(lv_obj_t* parent)
{
	static lv_ft_info_t font_title;
	font_title.name = FONT_PATH;
	font_title.weight = 38;
	font_title.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_title);

	static lv_style_t style_title_bg;
	lv_style_init(&style_title_bg);
	lv_style_set_radius(&style_title_bg, 0);
	lv_style_set_border_width(&style_title_bg, 0);
	lv_style_set_bg_color(&style_title_bg, lv_color_hex(0x151920));

	lv_obj_t* title_con = lv_obj_create(parent);
	lv_obj_set_pos(title_con, 0, 0);
	lv_obj_set_size(title_con, 720, 96);
	lv_obj_add_style(title_con, &style_title_bg, LV_PART_MAIN);
	lv_obj_clear_flag(title_con, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(title_con, 0, LV_PART_MAIN);

	lv_obj_t* btn_return = lv_imgbtn_create(title_con);
	lv_obj_set_pos(btn_return, 15, 0);
	lv_obj_set_size(btn_return, 120, 96);
	lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "/assets/images/back_home.png", NULL, NULL);
	lv_obj_add_event_cb(btn_return, quit_lamp_window, LV_EVENT_CLICKED, NULL);

	static lv_style_t style_title;
	lv_style_init(&style_title);
	lv_style_set_text_font(&style_title, font_title.font);
	lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

	lv_obj_t* label = lv_label_create(title_con);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_pos(label, 210, 12);
	lv_obj_set_size(label, 300, 50);
	lv_obj_add_style(label, &style_title, LV_PART_MAIN);
	lv_label_set_text(label, "灯光");
}

void close_lamp_window()
{
	lv_obj_del(lamp_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, lamp_window);
}

void create_lamp_window(lv_obj_t* screen)
{
	brightness = 100;
	lamp_window = lv_obj_create(screen);
	lv_obj_set_size(lamp_window, 720, 1280);
	lv_obj_clear_flag(lamp_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(lamp_window, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(lamp_window, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(lamp_window, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(lamp_window, lv_color_hex(0x2f3949), LV_PART_MAIN);

	create_title_bar(lamp_window);
	create_display_area(lamp_window);
	create_controller_area(lamp_window);
}
