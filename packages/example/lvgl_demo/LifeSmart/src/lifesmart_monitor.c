/*
 * @file lifesmart_monitor.c
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
#include "lifesmart_monitor.h"

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
static void on_monitor_temperature_changed(lv_event_t * e);      //调控滑块改变灯的亮度
static void on_monitor_switch_clicked(lv_event_t* e);     //开关键事件
/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* monitor_window;

static const char* sFunc[] = {
	"红外探头",
	"监控摄像头",
	"玻璃击碎传感器",
	"水浸传感器"
};

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_monitor_window(lv_event_t* e)
{
	close_monitor_window();
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
	lv_obj_add_event_cb(btn_return, quit_monitor_window, LV_EVENT_CLICKED, NULL);

	static lv_style_t style_title;
	lv_style_init(&style_title);
	lv_style_set_text_font(&style_title, font_title.font);
	lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

	lv_obj_t* label = lv_label_create(title_con);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_pos(label, 210, 12);
	lv_obj_set_size(label, 300, 50);
	lv_obj_add_style(label, &style_title, LV_PART_MAIN);
	lv_label_set_text(label, "布防");
}

static void create_switch_list_item(lv_obj_t* container, int index)
{
	static lv_ft_info_t font_title;
	font_title.name = FONT_PATH;
	font_title.weight = 40;
	font_title.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_title);

	static lv_style_t style_label_func;
	lv_style_init(&style_label_func);
	lv_style_set_text_font(&style_label_func, font_title.font);
	lv_style_set_text_color(&style_label_func, lv_color_hex(0xffffff));

	lv_obj_t* label_func = lv_label_create(container);
	lv_obj_set_size(label_func, 400, 70);
	lv_obj_set_pos(label_func, 30, 20);
	lv_label_set_text(label_func, sFunc[index]);
	lv_obj_add_style(label_func, &style_label_func, LV_PART_MAIN);

	lv_obj_t* switch_btn = lv_switch_create(container);
	lv_obj_set_size(switch_btn, 130, 60);
	lv_obj_set_pos(switch_btn, 525, 30);
	lv_state_t switch_state = lv_obj_get_state(switch_btn);
	lv_obj_set_style_outline_width(switch_btn, 0, switch_state);
    lv_obj_set_style_bg_color(switch_btn, lv_color_hex(0xe7e7e7), LV_PART_MAIN);
	lv_obj_set_style_bg_color(switch_btn, lv_color_hex(0xe07e6b), LV_PART_INDICATOR | LV_STATE_CHECKED);
	if (index == 0 || index == 3) lv_obj_add_state(switch_btn, LV_STATE_CHECKED);    //初始化第一个和第四个按键开关状态为CHECKED
}


static void create_switch_list(lv_obj_t* parent)
{
	static lv_style_t style_list;
	lv_style_init(&style_list);
	lv_style_set_radius(&style_list, 0);
	lv_style_set_border_width(&style_list, 0);
	lv_style_set_bg_opa(&style_list, LV_OPA_0);

	lv_obj_t* list_switch = lv_list_create(parent);
	lv_obj_set_size(list_switch, 720, 1184);
	lv_obj_set_pos(list_switch, 0, 96);
	lv_obj_set_style_pad_all(list_switch, 0, LV_PART_MAIN);
	lv_obj_add_style(list_switch, &style_list, LV_PART_MAIN);


	int i;
	for (i = 0; i < 4; i++) {
	    lv_obj_t* list_item = lv_obj_create(list_switch);
		lv_obj_set_size(list_item, 720, 120);
		lv_obj_set_style_radius(list_item, 0, LV_PART_MAIN);
		lv_obj_set_style_border_width(list_item, 0, LV_PART_MAIN);
		lv_obj_set_style_bg_opa(list_item, LV_OPA_0, LV_PART_MAIN);
		lv_obj_clear_flag(list_item, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_pad_all(list_item, 0, LV_PART_MAIN);
		lv_obj_set_style_border_width(list_item, 1, LV_PART_MAIN);
		lv_obj_set_style_border_side(list_item, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
		create_switch_list_item(list_item, i);
	}
}

void close_monitor_window()
{
	lv_obj_del(monitor_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, monitor_window);
}

void create_monitor_window(lv_obj_t* screen)
{
	monitor_window = lv_obj_create(screen);
	lv_obj_set_size(monitor_window, 720, 1280);
	lv_obj_clear_flag(monitor_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(monitor_window, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(monitor_window, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(monitor_window, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(monitor_window, lv_color_hex(0x2f3949), LV_PART_MAIN);

	create_title_bar(monitor_window);
	create_switch_list(monitor_window);
}
