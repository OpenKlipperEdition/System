/*
 * @file smarthome_scene.c - is provided for use with Ingenic products.
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
#include "smarthome_scene.h"

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH     "../../assets/fonts/sourcehan.ttf"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void on_scene_btn_clicked(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/
static const char* s_scene_text[] = {
    "工作",
    "回家",
    "睡眠",
    "晚宴",
    "派对",
    "度假"
};
static const char* s_scene_default_image[] = {
    "../assets/images/scene_work_default.png",
    "../assets/images/scene_home_default.png",
    "../assets/images/scene_sleep_default.png",
    "../assets/images/scene_dinner_default.png",
    "../assets/images/scene_party_default.png",
    "../assets/images/scene_rest_default.png"
};
static const char* s_scene_checked_image[] = {
    "../assets/images/scene_work_checked.png",
    "../assets/images/scene_home_checked.png",
    "../assets/images/scene_sleep_checked.png",
    "../assets/images/scene_dinner_checked.png",
    "../assets/images/scene_party_checked.png",
    "../assets/images/scene_rest_checked.png"
};

static lv_obj_t* scene_window;

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_scene_window(lv_event_t* e)
{
    destroy_scene_window();
}

static void play_music(uint32_t index)
{
	stop_music();
	switch (index) {
		case SCENE_AT_WORK:
			start_music(MUSIC_SCENE_AT_WORK);
			break;
		case SCENE_AT_HOME:
			start_music(MUSIC_SCENE_AT_HOME);
			break;
		case SCENE_SLEEP:
			start_music(MUSIC_SCENE_SLEEP);
			break;
		case SCENE_AT_DINNER:
			start_music(MUSIC_SCENE_AT_DINNER);
			break;
		case SCENE_AT_PARTY:
			start_music(MUSIC_SCENE_AT_PARTY);
			break;
		case SCENE_ON_VACATION:
			start_music(MUSIC_SCENE_ON_VACATION);
			break;
	}
}

static void on_scene_btn_clicked(lv_event_t* e)
{
	int i = 0;
	lv_obj_t* current_btn = lv_event_get_target(e);
	lv_obj_t* parent = lv_obj_get_parent(current_btn);
	int index =  (int)lv_event_get_user_data(e);
	play_music(index);
	for (i == 0; i < lv_obj_get_child_cnt(parent); i++) {
		lv_obj_t* child = lv_obj_get_child(parent, i);
		lv_imgbtn_set_state(child, LV_IMGBTN_STATE_RELEASED);
	}
	lv_imgbtn_set_state(current_btn, LV_IMGBTN_STATE_CHECKED_RELEASED);
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
	lv_label_set_text(label, "场景");
}

static void create_scene_item(lv_obj_t* container, uint32_t index)
{
	static lv_ft_info_t font_text;
	font_text.name = FONT_PATH;
	font_text.weight = 20;
	font_text.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_text);

	lv_obj_t* label_text = lv_label_create(container);
	lv_obj_set_size(label_text, 100, 30);
	lv_obj_set_pos(label_text, 20, 75);
	lv_obj_set_style_text_color(label_text, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(label_text, font_text.font, LV_PART_MAIN);
	lv_obj_set_style_text_align(label_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_label_set_text(label_text, s_scene_text[index]);
}

static void create_scene_area(lv_obj_t* parent)
{
	static lv_style_t style_scene;
	lv_style_init(&style_scene);
	lv_style_set_radius(&style_scene, 0);
	lv_style_set_border_width(&style_scene, 0);
	lv_style_set_bg_opa(&style_scene, LV_OPA_TRANSP);
	lv_style_set_flex_flow(&style_scene, LV_FLEX_FLOW_ROW_WRAP);
	lv_style_set_layout(&style_scene, LV_LAYOUT_FLEX);

	lv_obj_t* scene_container = lv_obj_create(parent);
	lv_obj_set_size(scene_container, 448, 360);
	lv_obj_set_pos(scene_container, 16, 66);
	lv_obj_add_style(scene_container, &style_scene, LV_PART_MAIN);
	lv_obj_clear_flag(scene_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(scene_container, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_row(scene_container, 14, LV_PART_MAIN);
	lv_obj_set_style_pad_column(scene_container, 14, LV_PART_MAIN);

	uint32_t i;
	for (i = 0; i < 6; i++) {
		lv_obj_t* btn_scene = lv_imgbtn_create(scene_container);
		lv_obj_set_size(btn_scene, 140, 115);
		lv_imgbtn_set_src(btn_scene, LV_IMGBTN_STATE_RELEASED, NULL, s_scene_default_image[i], NULL);
		lv_imgbtn_set_src(btn_scene, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, s_scene_checked_image[i], NULL);
		lv_obj_set_style_pad_all(btn_scene, 0, LV_PART_MAIN);
		lv_obj_add_flag(btn_scene, LV_OBJ_FLAG_CHECKABLE);
		if (i == 0) {
			lv_imgbtn_set_state(btn_scene, LV_IMGBTN_STATE_CHECKED_RELEASED);
			stop_music();
			start_music(MUSIC_SCENE_AT_WORK);
		}
		create_scene_item(btn_scene, i);
		lv_obj_add_event_cb(btn_scene, on_scene_btn_clicked, LV_EVENT_CLICKED, (void*)i);
	}
}

void destroy_scene_window()
{
	stop_music();
	lv_obj_del(scene_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, scene_window);
}

void create_scene_window(lv_obj_t* screen)
{
	static lv_style_t style_container;
	lv_style_init(&style_container);
	lv_style_set_radius(&style_container, 0);
	lv_style_set_border_width(&style_container, 0);
	lv_style_set_bg_color(&style_container, lv_color_hex(0x6a777f));

	scene_window = lv_obj_create(screen);
	lv_obj_add_style(scene_window, &style_container, LV_PART_MAIN);
	lv_obj_set_size(scene_window, 480, 320);
	lv_obj_clear_flag(scene_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(scene_window, 0, LV_PART_MAIN);

	create_title_bar(scene_window);
	create_scene_area(scene_window);

	lv_obj_t* btn_return = lv_imgbtn_create(scene_window);
	lv_obj_set_pos(btn_return, 420, 4);
	lv_obj_set_size(btn_return, 56, 52);
	lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
	lv_obj_add_event_cb(btn_return, quit_scene_window, LV_EVENT_CLICKED, NULL);
}
