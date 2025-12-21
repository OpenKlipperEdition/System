/*
 * @file lifesmart_airconditioner.c
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
#include "lifesmart_airconditioner.h"
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "/assets/fonts/sourcehan.ttf"
#define WORK_MODE_COUNT  4

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void on_airconditioner_temperature_changed(lv_event_t * e);      //调控滑块改变灯的亮度
static void on_airconditioner_switch_clicked(lv_event_t* e);     //开关键事件
static void on_airconditioner_mode_changed(lv_event_t* e);       //制冷模式、制热模式、除湿模式和送风模式切换
/**********************
 *  STATIC VARIABLES
 **********************/
static const char* sImgModeOn[] = {
	"/assets/images/cold_mode_on.png",
	"/assets/images/heat_mode_on.png",
	"/assets/images/dehumify_mode_on.png",
	"/assets/images/wind_mode_on.png"
};

static const char* sImgModeOff[] = {
	"/assets/images/cold_mode_off.png",
	"/assets/images/heat_mode_off.png",
	"/assets/images/dehumify_mode_off.png",
	"/assets/images/wind_mode_off.png"
};

static const char* sImgModeDisable[] = {
	"/assets/images/cold_mode_off.png",
	"/assets/images/heat_mode_off.png",
	"/assets/images/dehumify_mode_off.png",
	"/assets/images/wind_mode_off.png"
};

static lv_obj_t* airconditioner_window;
static lv_obj_t* mode_container;
static lv_obj_t* label_temp;
static lv_obj_t* img_airconditioner;
static lv_obj_t* controller_container;

static int temperature;
static int work_mode;
/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void update_mode_state(lv_obj_t* obj, int mode)
{
	int i = 0;
	for (i; i < lv_obj_get_child_cnt(obj); i++) {
		lv_obj_t* child = lv_obj_get_child(obj, i);
		if (mode == -1) { //空调为关，所有模式按键不可用
			lv_imgbtn_set_state(child, LV_IMGBTN_STATE_DISABLED);
		} else if (i == mode) {
			lv_imgbtn_set_state(child, LV_IMGBTN_STATE_CHECKED_RELEASED);
		} else {
			lv_imgbtn_set_state(child, LV_IMGBTN_STATE_RELEASED);
		}
	}
}

static void quit_airconditioner_window(lv_event_t* e)
{
	close_airconditioner_window();
}

static void on_airconditioner_temperature_changed(lv_event_t* e)
{
	char str_temp[20];
	lv_obj_t * slider = lv_event_get_target(e);
	temperature = (int)lv_slider_get_value(slider);
	sprintf(str_temp, "%02d℃", temperature);
	lv_label_set_text(label_temp, str_temp);
}

static void on_airconditioner_mode_changed(lv_event_t* e)
{
	work_mode =  (int)lv_event_get_user_data(e);
	lv_obj_t* current_btn = lv_event_get_target(e);
	lv_obj_t* parent = lv_obj_get_parent(current_btn);

	update_mode_state(parent, work_mode);
}

static void on_airconditioner_switch_clicked(lv_event_t* e)
{
	lv_obj_t * button = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_VALUE_CHANGED) {
		if(lv_obj_has_state(button, LV_STATE_CHECKED)) {   //airconditioner is on
			lv_img_set_src(img_airconditioner, "/assets/images/airconditioner_on.png");
			lv_obj_clear_flag(label_temp, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(controller_container, LV_OBJ_FLAG_HIDDEN);
			update_mode_state(mode_container, work_mode);
		} else {      //airconditioner is off
			lv_img_set_src(img_airconditioner, "/assets/images/airconditioner_off.png");
			lv_obj_add_flag(label_temp, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(controller_container, LV_OBJ_FLAG_HIDDEN);
			update_mode_state(mode_container, -1);
		}
	}
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
	lv_obj_add_event_cb(btn_return, quit_airconditioner_window, LV_EVENT_CLICKED, NULL);

	static lv_style_t style_title;
	lv_style_init(&style_title);
	lv_style_set_text_font(&style_title, font_title.font);
	lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

	lv_obj_t* label = lv_label_create(title_con);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_pos(label, 210, 12);
	lv_obj_set_size(label, 300, 50);
	lv_obj_add_style(label, &style_title, LV_PART_MAIN);
	lv_label_set_text(label, "空调");
}

static void create_display_area(lv_obj_t* parent)
{
	char str_temp[1024];

	static lv_ft_info_t font_temp;
	font_temp.name = FONT_PATH;
	font_temp.weight = 80;
	font_temp.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_temp);

	static lv_style_t style_label_temp;
	lv_style_init(&style_label_temp);
	lv_style_set_text_font(&style_label_temp, font_temp.font);
	lv_style_set_text_color(&style_label_temp, lv_color_hex(0xffffff));

	lv_obj_t* btn_switch = lv_switch_create(parent);
	lv_obj_set_pos(btn_switch, 550, 180);
	lv_obj_set_size(btn_switch, 125, 60);
	lv_state_t switch_state = lv_obj_get_state(btn_switch);
	lv_obj_set_style_outline_width(btn_switch, 0, switch_state);
	lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0xe7e7e7), LV_PART_MAIN);
	lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0x64b1f3), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_add_state(btn_switch, LV_STATE_CHECKED);

	lv_obj_add_event_cb(btn_switch, on_airconditioner_switch_clicked, LV_EVENT_ALL, NULL);

	img_airconditioner = lv_img_create(parent);
	lv_obj_set_pos(img_airconditioner, 143, 395);
	lv_obj_set_size(img_airconditioner, 434, 280);
	lv_img_set_src(img_airconditioner, "/assets/images/airconditioner_on.png");

	label_temp = lv_label_create(parent);
	lv_obj_set_pos(label_temp, 260, 436);
	lv_obj_set_size(label_temp, 200, 110);
	lv_obj_set_style_text_align(label_temp, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_temp, &style_label_temp, 0);
	sprintf(str_temp, "%02d℃", temperature);
	lv_label_set_text(label_temp, str_temp);
}

static void create_work_mode_area(lv_obj_t* parent)
{
	static lv_style_t style_mode;
	lv_style_init(&style_mode);
	lv_style_set_radius(&style_mode, 0);
	lv_style_set_border_width(&style_mode, 0);
	lv_style_set_bg_opa(&style_mode, LV_OPA_TRANSP);
	lv_style_set_flex_flow(&style_mode, LV_FLEX_FLOW_ROW_WRAP);
	lv_style_set_layout(&style_mode, LV_LAYOUT_FLEX);

	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style, 10);
	lv_style_set_shadow_width(&btn_style, 0);

	mode_container = lv_obj_create(parent);
	lv_obj_set_size(mode_container, 636, 120);
	lv_obj_set_pos(mode_container, 42, 855);
	lv_obj_add_style(mode_container, &style_mode, LV_PART_MAIN);
	lv_obj_clear_flag(mode_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(mode_container, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_column(mode_container, 52, LV_PART_MAIN);

	uint32_t i;
	for (i = 0; i < WORK_MODE_COUNT; i++) {
		lv_obj_t* btn_mode = lv_imgbtn_create(mode_container);
		lv_obj_set_size(btn_mode, 120, 120);
//		lv_obj_add_style(btn_mode, &btn_style, LV_PART_MAIN);
		lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_RELEASED, NULL, sImgModeOff[i], NULL);
		lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, sImgModeOn[i], NULL);
		lv_imgbtn_set_src(btn_mode, LV_IMGBTN_STATE_DISABLED, NULL, sImgModeDisable[i], NULL);
		if (work_mode == i) lv_imgbtn_set_state(btn_mode, LV_IMGBTN_STATE_CHECKED_RELEASED);
		//lv_state_t btn_state = lv_obj_get_state(btn_mode);
		lv_obj_add_event_cb(btn_mode, on_airconditioner_mode_changed, LV_EVENT_CLICKED, (void*)i);
	}
}

static void create_control_bar_area(lv_obj_t* parent)
{
	static lv_ft_info_t font_range;
	font_range.name = FONT_PATH;
	font_range.weight = 26;
	font_range.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_range);

	static lv_style_t style_label_range;
	lv_style_init(&style_label_range);
	lv_style_set_text_font(&style_label_range, font_range.font);
	lv_style_set_text_color(&style_label_range, lv_color_hex(0xffffff));

	controller_container = lv_obj_create(parent);
	lv_obj_set_size(controller_container, 720, 100);
	lv_obj_set_pos(controller_container, 0, 1026);
	lv_obj_clear_flag(controller_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(controller_container, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(controller_container, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(controller_container, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(controller_container, LV_OPA_0, LV_PART_MAIN);

	lv_obj_t* label_temp_min = lv_label_create(controller_container);
	lv_obj_set_pos(label_temp_min, 28, 30);
	lv_obj_set_size(label_temp_min, 80, 30);
	lv_obj_set_style_text_align(label_temp_min, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_temp_min, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_temp_min, "18℃");

	lv_obj_t* label_temp_max = lv_label_create(controller_container);
	lv_obj_set_pos(label_temp_max, 620, 30);
	lv_obj_set_size(label_temp_max, 80, 30);
	lv_obj_set_style_text_align(label_temp_max, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_temp_max, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_temp_max, "32℃");

	static lv_style_t style_slider;
	lv_style_init(&style_slider);
	lv_style_set_bg_opa(&style_slider, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider, lv_color_hex(0xefefef));

	static lv_style_t style_slider_indicator;
	lv_style_init(&style_slider_indicator);
	lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_indicator, lv_color_hex(0xbae8fa));
	lv_style_set_bg_grad_color(&style_slider_indicator, lv_color_hex(0x6dcef2));
	lv_style_set_bg_grad_dir(&style_slider_indicator, LV_GRAD_DIR_HOR);

	static lv_style_t style_slider_knob;
	lv_style_init(&style_slider_knob);
	lv_style_set_pad_all(&style_slider_knob, 20);
	lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_knob, lv_color_hex(0x64b1f3));

	lv_obj_t* slider = lv_slider_create(controller_container);
	lv_obj_set_pos(slider, 132, 40);
	lv_obj_set_size(slider, 460, 30);
	lv_slider_set_range(slider, 18, 32);
	lv_slider_set_value(slider, temperature, LV_ANIM_OFF);
	lv_obj_add_style(slider, &style_slider, LV_PART_MAIN);
	lv_obj_add_style(slider, &style_slider_indicator,  LV_PART_INDICATOR);
	lv_obj_add_style(slider, &style_slider_knob, LV_PART_KNOB);
	lv_obj_add_event_cb(slider, on_airconditioner_temperature_changed, LV_EVENT_VALUE_CHANGED, NULL);
}

void close_airconditioner_window()
{
	lv_obj_del(airconditioner_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, airconditioner_window);
}

void create_airconditioner_window(lv_obj_t* screen)
{
	temperature = 24;
	airconditioner_window = lv_obj_create(screen);
	lv_obj_set_size(airconditioner_window, 720, 1280);
	lv_obj_clear_flag(airconditioner_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(airconditioner_window, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(airconditioner_window, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(airconditioner_window, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(airconditioner_window, lv_color_hex(0x2f3949), LV_PART_MAIN);

	create_title_bar(airconditioner_window);
	create_display_area(airconditioner_window);
	create_work_mode_area(airconditioner_window);
	create_control_bar_area(airconditioner_window);
}
