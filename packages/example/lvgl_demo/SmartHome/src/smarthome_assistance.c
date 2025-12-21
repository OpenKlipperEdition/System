/*
 * @file smarthome_assistance.c - is provided for use with Ingenic products.
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

#include "lvgl.h"
#include "smarthome_assistance.h"

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH     "../../assets/fonts/sourcehan.ttf"
#define RING_TONE "../assets/songs/btn_sound.wav"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void on_device_switch_btn_clicked(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/
static const char* s_switch_text[] = {
    "客厅空调",
    "卧室空调",
    "客厅空净",
    "客厅灯",
    "次卧灯",
    "一键搞定"
};

static lv_obj_t* assist_window;
/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_assistance_window(lv_event_t* e)
{
    destroy_assistance_window();
	start_ring(RING_TONE);
}

static void on_device_switch_btn_clicked(lv_event_t* e)
{
	int i = 0;
	lv_obj_t* current_btn = lv_event_get_target(e);
	lv_obj_t* parent = lv_obj_get_parent(current_btn);
	int index =  (int)lv_event_get_user_data(e);
	if (index == lv_obj_get_child_cnt(parent) - 1) {
		for (i == 0; i < lv_obj_get_child_cnt(parent); i++) {
			lv_obj_t* child = lv_obj_get_child(parent, i);
			lv_imgbtn_set_state(child, LV_IMGBTN_STATE_RELEASED);
		}
	}
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

	lv_obj_t* title_con = lv_obj_create(parent);
	lv_obj_set_pos(title_con, 0, 0);
	lv_obj_set_size(title_con, 480, 48);
	lv_obj_add_style(title_con, &style_title_bg, LV_PART_MAIN);
	lv_obj_clear_flag(title_con, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t* label = lv_label_create(title_con);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_pos(label, 0, 4);
	lv_obj_set_size(label, 480, 30);
	lv_obj_add_style(label, &style_title, LV_PART_MAIN);
	lv_label_set_text(label, "小娜提示");
}

static void create_device_switch_item(lv_obj_t* container, uint32_t index)
{
	static lv_ft_info_t font_text;
	font_text.name = FONT_PATH;
	font_text.weight = 20;
	font_text.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_text);

	lv_obj_t* label_text = lv_label_create(container);
	lv_obj_set_size(label_text, 100, 30);
	lv_obj_set_pos(label_text, 20, 75);
	lv_obj_set_style_text_color(label_text, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_text_font(label_text, font_text.font, LV_PART_MAIN);
	lv_obj_set_style_text_align(label_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_label_set_text(label_text, s_switch_text[index]);
}

static void create_device_switch_area(lv_obj_t* parent)
{
	static lv_style_t style_switch;
	lv_style_init(&style_switch);
	lv_style_set_radius(&style_switch, 0);
	lv_style_set_border_width(&style_switch, 0);
	lv_style_set_bg_opa(&style_switch, LV_OPA_TRANSP);
	lv_style_set_flex_flow(&style_switch, LV_FLEX_FLOW_ROW_WRAP);
	lv_style_set_layout(&style_switch, LV_LAYOUT_FLEX);

	lv_obj_t* switch_container = lv_obj_create(parent);
	lv_obj_set_size(switch_container, 450, 239);
	lv_obj_set_pos(switch_container, 15, 65);
	lv_obj_add_style(switch_container, &style_switch, LV_PART_MAIN);
	lv_obj_clear_flag(switch_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(switch_container, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_row(switch_container, 15, LV_PART_MAIN);
	lv_obj_set_style_pad_column(switch_container, 15, LV_PART_MAIN);

	uint32_t i;
	for (i = 0; i < 6; i++) {
		lv_obj_t* btn_switch = lv_imgbtn_create(switch_container);
		lv_obj_set_size(btn_switch, 140, 112);
		if (i ==5) {
			lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_RELEASED, NULL, "../assets/images/xiaona_delete_unchecked.png", NULL);
		} else {
			lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_RELEASED, NULL, "../assets/images/xiaona_switch_unchecked.png", NULL);
			lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, "../assets/images/xiaona_switch_checked.png", NULL);
			lv_obj_add_flag(btn_switch, LV_OBJ_FLAG_CHECKABLE);
		}
		lv_obj_set_style_pad_all(btn_switch, 0, LV_PART_MAIN);
		create_device_switch_item(btn_switch, i);
		lv_obj_add_event_cb(btn_switch, on_device_switch_btn_clicked, LV_EVENT_CLICKED, (void*)i);
	}
}
void destroy_assistance_window()
{
	lv_obj_del(assist_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, assist_window);
}

void create_assistance_window(lv_obj_t* screen)
{
	static lv_style_t style_container;
	lv_style_init(&style_container);
	lv_style_set_radius(&style_container, 0);
	lv_style_set_border_width(&style_container, 0);

	assist_window = lv_obj_create(screen);

	lv_obj_add_style(assist_window, &style_container, LV_PART_MAIN);
	lv_obj_set_size(assist_window, 480, 320);
	lv_obj_clear_flag(assist_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(assist_window, 0, LV_PART_MAIN);

	create_title_bar(assist_window);
	create_device_switch_area(assist_window);

	lv_obj_t* btn_return = lv_imgbtn_create(assist_window);
	lv_obj_set_pos(btn_return, 420, 4);
	lv_obj_set_size(btn_return, 56, 52);
	lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
	lv_obj_add_event_cb(btn_return, quit_assistance_window, LV_EVENT_CLICKED, NULL);
}
