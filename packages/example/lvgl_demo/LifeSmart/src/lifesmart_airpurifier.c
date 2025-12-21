/*
 * @file lifesmart_airpurifier.c
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
#include "lifesmart_airpurifier.h"
#include <stdio.h>
/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "/assets/fonts/sourcehan.ttf"
#define GEAR_LEVEL    3
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void on_airpurifier_level_changed(lv_event_t * e);      //调控滑块改变灯的亮度
static void on_airpurifier_switch_clicked(lv_event_t* e);     //开关键事件
/**********************
 *  STATIC VARIABLES
 **********************/
static const char* sImgGearOn[] = {
	"/assets/images/gear_1_on.png",
	"/assets/images/gear_2_on.png",
	"/assets/images/gear_3_on.png",
};

static const char* sImgGearOff[] = {
	"/assets/images/gear_1_off.png",
	"/assets/images/gear_2_off.png",
	"/assets/images/gear_3_off.png",
};

static const char* sImgGearDisable[] = {
	"/assets/images/gear_1_off.png",
	"/assets/images/gear_2_off.png",
	"/assets/images/gear_3_off.png",
};

static const char* sGearText[] = {
	"1挡",
	"2挡",
	"3挡"
};

static lv_obj_t* airpurifier_window;
static lv_obj_t* gear_container;
static lv_obj_t* controller_container;
static lv_obj_t* label_level;

static int level;
static int airpurifier_gear;
/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void update_gear_state(lv_obj_t* obj, int gear)
{
	int i = 0;
	int gear_count = lv_obj_get_child_cnt(obj);   //获取档位容器数
	for (i; i < gear_count; i++) {
		lv_obj_t* gear_container = lv_obj_get_child(obj, i);
		lv_obj_t* gear_btn = lv_obj_get_child(gear_container, 0);
		if (gear == -1) { //空调为关，所有模式按键不可用
			lv_imgbtn_set_state(gear_btn, LV_IMGBTN_STATE_DISABLED);
		} else if (i == gear) {
			lv_imgbtn_set_state(gear_btn, LV_IMGBTN_STATE_CHECKED_RELEASED);
		} else {
			lv_imgbtn_set_state(gear_btn, LV_IMGBTN_STATE_RELEASED);
		}
	}
}

static void quit_airpurifier_window(lv_event_t* e)
{
	close_airpurifier_window();
}

static void on_gear_btn_clicked(lv_event_t* e)
{
	airpurifier_gear =  (int)lv_event_get_user_data(e);
	lv_obj_t* current_btn = lv_event_get_target(e);
	lv_obj_t* btn_parent = lv_obj_get_parent(current_btn);  //获取到档位按键父容器，父容器包括label和imgbtn两个控件
	lv_obj_t* gear_group = lv_obj_get_parent(btn_parent);   //档位组容器，包括4个obj container

	update_gear_state(gear_group, airpurifier_gear);
}

static void on_airpurifier_level_changed(lv_event_t* e)
{
	char str_level[1024];
	lv_obj_t * slider = lv_event_get_target(e);
	level = (int)lv_slider_get_value(slider);
	sprintf(str_level, "%02d%%", level);
	lv_label_set_text(label_level, str_level);
}

static void on_airpurifier_switch_clicked(lv_event_t* e)
{
	lv_obj_t * button = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_VALUE_CHANGED) {
		if(lv_obj_has_state(button, LV_STATE_CHECKED)) {   //airconditioner is on
			lv_obj_clear_flag(label_level, LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(controller_container, LV_OBJ_FLAG_HIDDEN);
			update_gear_state(gear_container, airpurifier_gear);
		} else {      //airconditioner is off
			lv_obj_add_flag(label_level, LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(controller_container, LV_OBJ_FLAG_HIDDEN);
			update_gear_state(gear_container, -1);
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
	lv_obj_add_event_cb(btn_return, quit_airpurifier_window, LV_EVENT_CLICKED, NULL);

	static lv_style_t style_title;
	lv_style_init(&style_title);
	lv_style_set_text_font(&style_title, font_title.font);
	lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

	lv_obj_t* label = lv_label_create(title_con);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_pos(label, 210, 12);
	lv_obj_set_size(label, 300, 50);
	lv_obj_add_style(label, &style_title, LV_PART_MAIN);
	lv_label_set_text(label, "空气净化");
}

static void create_display_area(lv_obj_t* parent)
{
	char str_level[1024];

	static lv_ft_info_t font_level;
	font_level.name = FONT_PATH;
	font_level.weight = 65;
	font_level.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_level);

	lv_obj_t* btn_switch = lv_switch_create(parent);
	lv_obj_set_pos(btn_switch, 550, 180);
	lv_obj_set_size(btn_switch, 125, 60);
	lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0xe7e7e7), LV_PART_MAIN);
	lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0x128283), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_state_t switch_state = lv_obj_get_state(btn_switch);
	lv_obj_set_style_outline_width(btn_switch, 0, switch_state);
	lv_obj_add_state(btn_switch, LV_STATE_CHECKED);
	lv_obj_add_event_cb(btn_switch, on_airpurifier_switch_clicked, LV_EVENT_ALL, NULL);

	lv_obj_t* img_device = lv_img_create(parent);
	lv_obj_set_pos(img_device, 230, 300);
	lv_obj_set_size(img_device, 343, 410);
	lv_img_set_src(img_device, "/assets/images/purifier_button.png");

	label_level = lv_label_create(parent);
	lv_obj_set_pos(label_level, 270, 405);
	lv_obj_set_size(label_level, 220, 100);
	lv_obj_set_style_text_align(label_level, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_style_text_font(label_level, font_level.font, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_level, lv_color_hex(0xffffff), LV_PART_MAIN);
	sprintf(str_level, "%02d%%", level);
	lv_label_set_text(label_level, str_level);
}

static void create_gear_item(lv_obj_t* parent, int gear)
{
	static lv_ft_info_t font_gear;
	font_gear.name = FONT_PATH;
	font_gear.weight = 28;
	font_gear.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_gear);

	lv_obj_t* btn_gear = lv_imgbtn_create(parent);
	lv_obj_set_pos(btn_gear, 0, 0);
	lv_obj_set_size(btn_gear, 108, 108);
	lv_imgbtn_set_src(btn_gear, LV_IMGBTN_STATE_RELEASED, NULL, sImgGearOff[gear], NULL);
	lv_imgbtn_set_src(btn_gear, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, sImgGearOn[gear], NULL);
	lv_imgbtn_set_src(btn_gear, LV_IMGBTN_STATE_DISABLED, NULL, sImgGearDisable[gear], NULL);
	lv_obj_add_event_cb(btn_gear, on_gear_btn_clicked, LV_EVENT_CLICKED, (void*)gear);
	if (gear == airpurifier_gear) lv_imgbtn_set_state(btn_gear, LV_IMGBTN_STATE_CHECKED_RELEASED);

	lv_obj_t* label_gear = lv_label_create(parent);
	lv_obj_set_pos(label_gear, 0, 110);
	lv_obj_set_size(label_gear, 108, 70);
	lv_obj_set_style_text_font(label_gear, font_gear.font, LV_PART_MAIN);
	lv_obj_set_style_text_color(label_gear, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_align(label_gear, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

	lv_label_set_text(label_gear, sGearText[gear]);

}

static void create_purifier_gear_area(lv_obj_t* parent)
{
	static lv_style_t style_mode;
	lv_style_init(&style_mode);
	lv_style_set_radius(&style_mode, 0);
	lv_style_set_border_width(&style_mode, 0);
	lv_style_set_bg_opa(&style_mode, LV_OPA_TRANSP);
	lv_style_set_pad_all(&style_mode, 0);

	gear_container = lv_obj_create(parent);
	lv_obj_set_size(gear_container, 636, 180);
	lv_obj_set_pos(gear_container, 132, 825);
	lv_obj_add_style(gear_container, &style_mode, LV_PART_MAIN);
	lv_obj_clear_flag(gear_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_flex_flow(gear_container, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN);
	lv_obj_set_style_layout(gear_container, LV_LAYOUT_FLEX, LV_PART_MAIN);
	lv_obj_set_style_pad_column(gear_container, 66, LV_PART_MAIN);

	int i = 0;
	for (i; i < GEAR_LEVEL; i++) {
		lv_obj_t* container = lv_obj_create(gear_container);
		lv_obj_set_size(container, 108, 180);
		lv_obj_add_style(container, &style_mode, LV_PART_MAIN);
		create_gear_item(container, i);
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

	lv_obj_t* label_min = lv_label_create(controller_container);
	lv_obj_set_pos(label_min, 28, 30);
	lv_obj_set_size(label_min, 80, 30);
	lv_obj_set_style_text_align(label_min, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_min, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_min, "10%");

	lv_obj_t* label_max = lv_label_create(controller_container);
	lv_obj_set_pos(label_max, 620, 30);
	lv_obj_set_size(label_max, 80, 30);
	lv_obj_set_style_text_align(label_max, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_max, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_max, "100%");

	static lv_style_t style_slider;
	lv_style_init(&style_slider);
	lv_style_set_bg_opa(&style_slider, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider, lv_color_hex(0xefefef));

	static lv_style_t style_slider_indicator;
	lv_style_init(&style_slider_indicator);
	lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_indicator, lv_color_hex(0xcdfffd));
	lv_style_set_bg_grad_color(&style_slider_indicator, lv_color_hex(0x117f7f));
	lv_style_set_bg_grad_dir(&style_slider_indicator, LV_GRAD_DIR_HOR);

	static lv_style_t style_slider_knob;
	lv_style_init(&style_slider_knob);
	lv_style_set_pad_all(&style_slider_knob, 20);
	lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_knob, lv_color_hex(0x128283));

	lv_obj_t* slider = lv_slider_create(controller_container);
	lv_obj_set_pos(slider, 132, 40);
	lv_obj_set_size(slider, 460, 30);
	lv_slider_set_range(slider, 10, 100);
	lv_slider_set_value(slider, level, LV_ANIM_OFF);
	lv_obj_add_style(slider, &style_slider, LV_PART_MAIN);
	lv_obj_add_style(slider, &style_slider_indicator,  LV_PART_INDICATOR);
	lv_obj_add_style(slider, &style_slider_knob, LV_PART_KNOB);
	lv_obj_add_event_cb(slider, on_airpurifier_level_changed, LV_EVENT_VALUE_CHANGED, NULL);
}

void close_airpurifier_window()
{
	lv_obj_del(airpurifier_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, airpurifier_window);
}

void create_airpurifier_window(lv_obj_t* screen)
{
	level = 60;
	airpurifier_gear = GEAR_MID;
	airpurifier_window = lv_obj_create(screen);
	lv_obj_set_size(airpurifier_window, 720, 1280);
	lv_obj_clear_flag(airpurifier_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(airpurifier_window, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(airpurifier_window, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(airpurifier_window, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(airpurifier_window, lv_color_hex(0x2f3949), LV_PART_MAIN);

	create_title_bar(airpurifier_window);
	create_display_area(airpurifier_window);
	create_purifier_gear_area(airpurifier_window);
	create_control_bar_area(airpurifier_window);
}
