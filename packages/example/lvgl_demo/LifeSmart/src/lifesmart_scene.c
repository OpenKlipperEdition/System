/*
 * @file lifesmart_scene.c
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
#include "lifesmart_scene.h"

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "/assets/fonts/sourcehan.ttf"
#define SCENE_TYPE_COUNT      8
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static const char* s_scene_text[] = {
	"工作",
	"学习",
	"晚宴",
	"派对",
	"清晨",
	"夜晚",
	"运动",
	"休息"
};

static const char* s_scene_icon[] = {
	"/assets/images/scene_work.png",
	"/assets/images/scene_study.png",
	"/assets/images/scene_dinner.png",
	"/assets/images/scene_party.png",
	"/assets/images/scene_morning.png",
	"/assets/images/scene_night.png",
	"/assets/images/scene_sport.png",
	"/assets/images/scene_rest.png"
};

static lv_obj_t* scene_window;
static uint32_t scene_mode;

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void play_music(uint32_t index)
{
	stop_music();
	switch (index) {
		case SCENE_AT_WORK:
			start_music(MUSIC_SCENE_AT_WORK);
			break;
		case SCENE_STUDYING:
			start_music(MUSIC_SCENE_STUDYING);
			break;
		case SCENE_AT_DINNER:
			start_music(MUSIC_SCENE_AT_DINNER);
			break;
		case SCENE_AT_PARTY:
			start_music(MUSIC_SCENE_AT_PARTY);
			break;
		case SCENE_MORNING:
			start_music(MUSIC_SCENE_MORNING);
			break;
		case SCENE_AT_NIGHT:
			start_music(MUSIC_SCENE_AT_NIGHT);
			break;
		case SCENE_DO_SPORTS:
			start_music(MUSIC_SCENE_DO_SPORTS);
			break;
		case SCENE_HAVE_REST:
			start_music(MUSIC_SCENE_HAVE_REST);
			break;
	}
}

static void quit_scene_window(lv_event_t* e)
{
	close_scene_window();
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
		lv_obj_t* gif = lv_obj_get_child(child, 2);
		if (child == current_btn) {
			lv_obj_clear_flag(gif, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(gif, LV_OBJ_FLAG_HIDDEN);
		}
	}

}

static void create_scene_item(lv_obj_t* parent, int index)
{
	static lv_ft_info_t font_text;
	font_text.name = FONT_PATH;
	font_text.weight = 40;
	font_text.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_text);

	lv_obj_t* img_icon = lv_img_create(parent);
	lv_obj_set_size(img_icon, 90, 90);
	lv_obj_set_pos(img_icon, 25, 17);
	lv_img_set_src(img_icon, s_scene_icon[index]);

	lv_obj_t* label_text = lv_label_create(parent);
	lv_obj_set_size(label_text, 150, 60);
	lv_obj_set_pos(label_text, 100, 20);
	lv_obj_set_style_text_color(label_text, lv_color_hex(0xffffff), LV_PART_MAIN);
	lv_obj_set_style_text_font(label_text, font_text.font, LV_PART_MAIN);
	lv_obj_set_style_text_align(label_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_label_set_text(label_text, s_scene_text[index]);

	lv_obj_t* gif_play = lv_gif_create(parent);
	lv_obj_set_size(gif_play, 22, 21);
	lv_obj_set_pos(gif_play, 245, 52);
	lv_gif_set_src(gif_play, "A:/assets/images/play_anim.gif");
	lv_obj_add_flag(gif_play, LV_OBJ_FLAG_HIDDEN);
}

static void create_scene_mode_area(lv_obj_t* parent)
{
	static lv_style_t style_scene;
	lv_style_init(&style_scene);
	lv_style_set_radius(&style_scene, 0);
	lv_style_set_border_width(&style_scene, 0);
	lv_style_set_bg_opa(&style_scene, LV_OPA_TRANSP);
	lv_style_set_flex_flow(&style_scene, LV_FLEX_FLOW_ROW_WRAP);
	lv_style_set_layout(&style_scene, LV_LAYOUT_FLEX);

	static lv_style_t btn_style;
	lv_style_init(&btn_style);
	lv_style_set_radius(&btn_style, 10);
	lv_style_set_shadow_width(&btn_style, 0);

	lv_obj_t* scene_container = lv_obj_create(parent);
	lv_obj_set_size(scene_container, 650, 740);
	lv_obj_set_pos(scene_container, 40, 232);
	lv_obj_add_style(scene_container, &style_scene, LV_PART_MAIN);
	lv_obj_clear_flag(scene_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(scene_container, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_row(scene_container, 80, LV_PART_MAIN);
	lv_obj_set_style_pad_column(scene_container, 60, LV_PART_MAIN);

	uint32_t i;
	for (i = 0; i < SCENE_TYPE_COUNT; i++) {
		lv_obj_t* btn_scene = lv_btn_create(scene_container);
		lv_obj_set_size(btn_scene, 295, 125);
		lv_obj_add_style(btn_scene, &btn_style, LV_PART_MAIN);
		lv_state_t btn_state = lv_obj_get_state(btn_scene);
		lv_obj_set_style_outline_width(btn_scene, 0, btn_state);
		lv_obj_set_style_pad_all(btn_scene, 0, LV_PART_MAIN);
		lv_obj_set_style_radius(btn_scene, 25, LV_PART_MAIN);
		lv_obj_set_style_bg_color(btn_scene, lv_color_hex(0x2bc5b7), LV_PART_MAIN);
		lv_obj_set_style_bg_grad_color(btn_scene, lv_color_hex(0x299fcb), LV_PART_MAIN);
		lv_obj_set_style_bg_grad_dir(btn_scene, LV_GRAD_DIR_HOR, LV_PART_MAIN);
		create_scene_item(btn_scene, i);
		lv_obj_add_event_cb(btn_scene, on_scene_btn_clicked, LV_EVENT_CLICKED, (void*)i);
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
	lv_obj_add_event_cb(btn_return, quit_scene_window, LV_EVENT_CLICKED, NULL);

	static lv_style_t style_title;
	lv_style_init(&style_title);
	lv_style_set_text_font(&style_title, font_title.font);
	lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

	lv_obj_t* label = lv_label_create(title_con);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
	lv_obj_set_pos(label, 210, 12);
	lv_obj_set_size(label, 300, 50);
	lv_obj_add_style(label, &style_title, LV_PART_MAIN);
	lv_label_set_text(label, "场景");
}

void close_scene_window()
{
	lv_obj_del(scene_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, scene_window);
	stop_music();
}

void create_scene_window(lv_obj_t* screen)
{
	scene_window = lv_obj_create(screen);
	lv_obj_set_size(scene_window, 720, 1280);
	lv_obj_clear_flag(scene_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(scene_window, 0, LV_PART_MAIN);
	lv_obj_set_style_radius(scene_window, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(scene_window, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_color(scene_window, lv_color_hex(0x2f3949), LV_PART_MAIN);

	create_title_bar(scene_window);
	create_scene_mode_area(scene_window);
}
