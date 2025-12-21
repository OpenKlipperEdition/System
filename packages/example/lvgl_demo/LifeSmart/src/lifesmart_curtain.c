/*
 * @file lifesmart_curtain.c
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
#include "lifesmart_curtain.h"

/*********************
 *      DEFINES
 *********************/
#define FONT_PATH "/assets/fonts/sourcehan.ttf"
#define CURTAIN_MAX_WIDTH    242
#define CURTAIN_MIN_WIDTH      20

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void setting_curtain_changed(lv_event_t * e);      //调控滑块改变窗帘布间隔
static void resize_curtain_page(int progress);
/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* curtain_window;
static lv_obj_t* img_curtain_left;
static lv_obj_t* img_curtain_right;
static lv_obj_t* slider;

static int value;
static int left_curtain_x;
static int right_curtain_x;
/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_curtain_window(lv_event_t* e)
{
    close_curtain_window();
}

static void setting_curtain_changed(lv_event_t * e)
{
    lv_obj_t* slider = lv_event_get_target(e);
    value = (int)lv_slider_get_value(slider);
    resize_curtain_page(value);
}

static void resize_curtain_page(int progress)
{
    left_curtain_x = value - CURTAIN_MAX_WIDTH;
    right_curtain_x = CURTAIN_MAX_WIDTH * 2 - value;
    lv_obj_set_x(img_curtain_left, left_curtain_x);
    lv_obj_set_x(img_curtain_right, right_curtain_x);
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
    lv_obj_add_event_cb(btn_return, quit_curtain_window, LV_EVENT_CLICKED, NULL);

    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, font_title.font);
    lv_style_set_text_color(&style_title, lv_color_hex(0xffffff));

    lv_obj_t* label = lv_label_create(title_con);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(label, 210, 12);
    lv_obj_set_size(label, 300, 50);
    lv_obj_add_style(label, &style_title, LV_PART_MAIN);
    lv_label_set_text(label, "窗帘");

}

static void create_display_area(lv_obj_t* parent)
{
    left_curtain_x = 0;
    right_curtain_x = 484 - CURTAIN_MAX_WIDTH;
    lv_obj_t* img_curtain_bar = lv_img_create(parent);
    lv_obj_set_size(img_curtain_bar, 553, 13);
    lv_obj_set_pos(img_curtain_bar, 83, 398);
    lv_img_set_src(img_curtain_bar, "/assets/images/curtain_bar.png");

    static lv_style_t style_mask;
    lv_style_init(&style_mask);
    lv_style_set_radius(&style_mask, 0);
    lv_style_set_border_width(&style_mask, 0);
    lv_style_set_bg_opa(&style_mask, LV_OPA_TRANSP);

    lv_obj_t* container_mask = lv_obj_create(parent);
    lv_obj_set_size(container_mask, 484, 345);
    lv_obj_set_pos(container_mask, 118, 411);
    lv_obj_add_style(container_mask, &style_mask, LV_PART_MAIN);
    lv_obj_clear_flag(container_mask, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(container_mask, 0, LV_PART_MAIN);

    img_curtain_left = lv_img_create(container_mask);
    lv_obj_set_size(img_curtain_left, 242, 345);
    lv_obj_set_pos(img_curtain_left, left_curtain_x, 0);
    lv_img_set_src(img_curtain_left, "/assets/images/curtain_left.png");

    img_curtain_right = lv_img_create(container_mask);
    lv_obj_set_size(img_curtain_right, 242, 345);
    lv_obj_set_pos(img_curtain_right, right_curtain_x, 0);
    lv_img_set_src(img_curtain_right, "/assets/images/curtain_right.png");

    resize_curtain_page(value);
}

static void create_controller_area(lv_obj_t* parent)
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

	lv_obj_t* label_on = lv_label_create(parent);
	lv_obj_set_pos(label_on, 28, 1025);
	lv_obj_set_size(label_on, 80, 30);
	lv_obj_set_style_text_align(label_on, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_on, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_on, "全开");

	lv_obj_t* label_off = lv_label_create(parent);
	lv_obj_set_pos(label_off, 615, 1025);
	lv_obj_set_size(label_off, 80, 30);
	lv_obj_set_style_text_align(label_off, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_add_style(label_off, &style_label_range, LV_PART_MAIN);
	lv_label_set_text(label_off, "全关");

	static lv_style_t style_slider;
	lv_style_init(&style_slider);
	lv_style_set_bg_opa(&style_slider, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider, lv_color_hex(0xefefef));

	static lv_style_t style_slider_indicator;
	lv_style_init(&style_slider_indicator);
	lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
	lv_style_set_bg_color(&style_slider_indicator, lv_color_hex(0x13c38f));
	lv_style_set_bg_grad_color(&style_slider_indicator, lv_color_hex(0x175a78));
	lv_style_set_bg_grad_dir(&style_slider_indicator, LV_GRAD_DIR_HOR);

	static lv_style_t style_slider_knob;
	lv_style_init(&style_slider_knob);
	lv_style_set_bg_opa(&style_slider_knob, LV_OPA_COVER);
	lv_style_set_pad_all(&style_slider_knob, 20);
	lv_style_set_bg_color(&style_slider_knob, lv_color_hex(0x13bf8e));

	slider = lv_slider_create(parent);
	lv_obj_set_pos(slider, 130, 1035);
	lv_obj_set_size(slider, 460, 30);
	lv_slider_set_range(slider, CURTAIN_MIN_WIDTH, CURTAIN_MAX_WIDTH);
	lv_slider_set_value(slider, value, LV_ANIM_OFF);
	lv_obj_add_style(slider, &style_slider, LV_PART_MAIN);
	lv_obj_add_style(slider, &style_slider_indicator,  LV_PART_INDICATOR);
	lv_obj_add_style(slider, &style_slider_knob, LV_PART_KNOB);
	lv_obj_add_event_cb(slider, setting_curtain_changed, LV_EVENT_VALUE_CHANGED, NULL);

	/*
	   lv_obj_t* lv_img_open = lv_img_create(parent);
	   lv_obj_set_size(lv_img_open, 96, 63);
	   lv_obj_set_pos(lv_img_open, 20, 1017);
	   lv_img_set_src(lv_img_open, "/assets/images/curtain_open.png");

	   lv_obj_t* lv_img_close = lv_img_create(parent);
	   lv_obj_set_size(lv_img_close, 96, 63);
	   lv_obj_set_pos(lv_img_close, 608, 1017);
	   lv_img_set_src(lv_img_close, "/assets/images/curtain_close.png");
	   */
}

void close_curtain_window()
{
	lv_obj_del(curtain_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, curtain_window);
}

void create_curtain_window(lv_obj_t* screen)
{
	value = 68;
	static lv_style_t style_container;
	lv_style_init(&style_container);
	lv_style_set_radius(&style_container, 0);
	lv_style_set_border_width(&style_container, 0);
	lv_style_set_bg_color(&style_container, lv_color_hex(0x2f3949));

	curtain_window = lv_obj_create(screen);
	lv_obj_add_style(curtain_window, &style_container, LV_PART_MAIN);
	lv_obj_set_size(curtain_window, 720, 1280);
	lv_obj_clear_flag(curtain_window, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_pad_all(curtain_window, 0, LV_PART_MAIN);
	/*
	   lv_obj_t* img_bg = lv_img_create(curtain_window);
	   lv_obj_set_size(img_bg, 720, 1184);
	   lv_obj_set_pos(img_bg, 0, 96);
	   lv_img_set_src(img_bg, "/assets/images/device_bg.png");
	   */
	create_title_bar(curtain_window);
	create_display_area(curtain_window);
	create_controller_area(curtain_window);
}
