/*
 * @file smarthome_curtain.c - is provided for use with Ingenic products.
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
#include "smarthome_curtain.h"

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
static void setting_curtain_changed(lv_event_t * e);      //调控滑块改变温度
static void curtain_switch_clicked(lv_event_t* e);        //开关键事件
static void resize_curtain_page(int progress);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* curtain_window;
static lv_style_t style_container;
static lv_style_t style_switch_group;

static const char* s_switch_screen_window[] = {
    "开",
    "关"
};

static const char* s_switch_curtain[] = {
    "关",
    "半开",
    "全开"
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_curtain_window(lv_event_t* e)
{
	start_ring(RING_TONE);
    destroy_curtain_window();
}

static void curtain_switch_clicked(lv_event_t* e)
{
    int i = 0;
    lv_obj_t* current_btn = lv_event_get_target(e);
    lv_obj_t* parent = lv_obj_get_parent(current_btn);
    for (i == 0; i < lv_obj_get_child_cnt(parent); i++) {
        lv_obj_t* child = lv_obj_get_child(parent, i);
        lv_imgbtn_set_state(child, LV_IMGBTN_STATE_RELEASED);
        lv_obj_t* label_text = lv_obj_get_child(child, 0);
        lv_obj_set_style_text_color(label_text, lv_color_hex(0x000000), LV_PART_MAIN);
    }
    lv_imgbtn_set_state(current_btn, LV_IMGBTN_STATE_CHECKED_RELEASED);
    lv_obj_t* label_text_checked = lv_obj_get_child(current_btn, 0);
    lv_obj_set_style_text_color(label_text_checked, lv_color_hex(0xffffff), LV_PART_MAIN);
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
    lv_label_set_text(label, "窗帘");
}

static void create_switch_item(lv_obj_t* parent, const char* text)
{
    static lv_ft_info_t font_text;
    font_text.name = FONT_PATH;
    font_text.weight = 18;
    font_text.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_text);

    lv_obj_t* label_text = lv_label_create(parent);
    lv_obj_set_size(label_text, 60, 25);
    lv_obj_set_pos(label_text, 6, 2);

    lv_obj_set_style_text_font(label_text, font_text.font, LV_PART_MAIN);
    lv_obj_set_style_text_align(label_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_text(label_text, text);
    lv_state_t parent_state = lv_obj_get_state(parent);
     if (parent_state == LV_STATE_CHECKED) {
         lv_obj_set_style_text_color(label_text, lv_color_hex(0xffffff), LV_PART_MAIN);
     } else {
         lv_obj_set_style_text_color(label_text, lv_color_hex(0x000000), LV_PART_MAIN);
     }
}

static void create_screen_window_switch_group(lv_obj_t* parent)
{
    lv_style_init(&style_switch_group);
    lv_style_set_radius(&style_switch_group, 0);
    lv_style_set_border_width(&style_switch_group, 0);
    lv_style_set_bg_opa(&style_switch_group, LV_OPA_TRANSP);
    lv_style_set_flex_flow(&style_switch_group, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_layout(&style_switch_group, LV_LAYOUT_FLEX);

    lv_obj_t* switch_group_container = lv_obj_create(parent);
    lv_obj_set_size(switch_group_container, 190, 38);
    lv_obj_set_pos(switch_group_container, 223, 75);
    lv_obj_add_style(switch_group_container, &style_switch_group, LV_PART_MAIN);
    lv_obj_clear_flag(switch_group_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(switch_group_container, 0, LV_PART_MAIN);   //clear all padding of container, default padding value is 16
    lv_obj_set_style_pad_column(switch_group_container, 34, LV_PART_MAIN);

    uint32_t i;
    for (i = 0; i < 2; i++) {
        lv_obj_t* btn_switch = lv_imgbtn_create(switch_group_container);
        lv_obj_set_size(btn_switch, 78, 38);
        lv_obj_set_style_pad_all(btn_switch, 0, LV_PART_MAIN);
        lv_obj_add_flag(btn_switch, LV_OBJ_FLAG_CHECKABLE);
        lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_RELEASED, NULL, "../assets/images/curtain_switch_default.png", NULL);
        lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, "../assets/images/curtain_switch_checked.png", NULL);
        if (i == 0) lv_imgbtn_set_state(btn_switch, LV_IMGBTN_STATE_CHECKED_RELEASED);
        create_switch_item(btn_switch, s_switch_screen_window[i]);
        lv_obj_add_event_cb(btn_switch, curtain_switch_clicked, LV_EVENT_CLICKED, NULL);
    }

}

static void create_curtain_switch_group(lv_obj_t* parent)
{
    lv_obj_t* switch_group_container = lv_obj_create(parent);
    lv_obj_set_size(switch_group_container, 302, 38);
    lv_obj_set_pos(switch_group_container, 111, 167);
    lv_obj_add_style(switch_group_container, &style_switch_group, LV_PART_MAIN);
    lv_obj_clear_flag(switch_group_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(switch_group_container, 0, LV_PART_MAIN);   //clear all padding of container, default padding value is 16
    lv_obj_set_style_pad_column(switch_group_container, 34, LV_PART_MAIN);

    uint32_t i;
    for (i = 0; i < 3; i++) {
        lv_obj_t* btn_switch = lv_imgbtn_create(switch_group_container);
        lv_obj_set_size(btn_switch, 78, 38);
        lv_obj_set_style_pad_all(btn_switch, 0, LV_PART_MAIN);
        lv_obj_add_flag(btn_switch, LV_OBJ_FLAG_CHECKABLE);
        lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_RELEASED, NULL, "../assets/images/curtain_switch_default.png", NULL);
        lv_imgbtn_set_src(btn_switch, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, "../assets/images/curtain_switch_checked.png", NULL);
        if (i == 2) lv_imgbtn_set_state(btn_switch, LV_IMGBTN_STATE_CHECKED_RELEASED);
        create_switch_item(btn_switch, s_switch_curtain[i]);
        lv_obj_add_event_cb(btn_switch, curtain_switch_clicked, LV_EVENT_CLICKED, NULL);
    }
}

static void create_display_area(lv_obj_t* parent)
{
    static lv_ft_info_t font_title;
    font_title.name = FONT_PATH;
    font_title.weight = 20;
    font_title.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_title);

    static lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_font(&style_text, font_title.font);
    lv_style_set_text_color(&style_text, lv_color_hex(0x000000));

    lv_obj_t* curtain_container = lv_obj_create(parent);
    lv_obj_set_pos(curtain_container, 26, 68);
    lv_obj_set_size(curtain_container, 428, 232);
    lv_obj_add_style(curtain_container, &style_container, LV_PART_MAIN);
    lv_obj_set_style_radius(curtain_container, 10, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(curtain_container, "../assets/images/curtain_bg.png", LV_PART_MAIN);
    lv_obj_set_style_pad_all(curtain_container, 0, LV_PART_MAIN);

    lv_obj_t* label_room = lv_label_create(curtain_container);
    lv_obj_set_size(label_room, 100, 30);
    lv_obj_set_pos(label_room, 164, 5);
    lv_obj_add_style(label_room, &style_text, LV_PART_MAIN);
    lv_label_set_text(label_room, "卧室窗帘");
    lv_obj_set_style_text_align(label_room, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_t* label_screen_window = lv_label_create(curtain_container);
    lv_obj_set_size(label_screen_window, 50, 30);
    lv_obj_set_pos(label_screen_window, 25, 76);
    lv_obj_add_style(label_screen_window, &style_text, LV_PART_MAIN);
    lv_label_set_text(label_screen_window, "纱窗");

    create_screen_window_switch_group(curtain_container);

    lv_obj_t* label_curtain = lv_label_create(curtain_container);
    lv_obj_set_size(label_curtain, 50, 30);
    lv_obj_set_pos(label_curtain, 25, 166);
    lv_obj_add_style(label_curtain, &style_text, LV_PART_MAIN);
    lv_label_set_text(label_curtain, "窗帘");

    create_curtain_switch_group(curtain_container);
}


void destroy_curtain_window()
{
    lv_obj_del(curtain_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, curtain_window);
}

void create_curtain_window(lv_obj_t* screen)
{
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);

    curtain_window = lv_obj_create(screen);
    lv_obj_add_style(curtain_window, &style_container, LV_PART_MAIN);
    lv_obj_set_size(curtain_window, 480, 320);
    lv_obj_clear_flag(curtain_window, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(curtain_window, 0, LV_PART_MAIN);

    create_title_bar(curtain_window);

    lv_obj_t* btn_return = lv_imgbtn_create(curtain_window);
    lv_obj_set_pos(btn_return, 420, 4);
    lv_obj_set_size(btn_return, 56, 52);
    lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
    lv_obj_add_event_cb(btn_return, quit_curtain_window, LV_EVENT_CLICKED, NULL);
    create_display_area(curtain_window);
}
