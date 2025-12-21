/*
 * @file smarthome_monitor.c - is provided for use with Ingenic products.
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
#include "smarthome_monitor.h"

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

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* monitor_window;

static lv_style_t style_container;

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
	start_ring(RING_TONE);
    destroy_monitor_window();
}

static void on_switch_btn_clicked(lv_event_t* e)
{
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
    lv_label_set_text(label, "布防控制");
}

static void create_switch_list_item(lv_obj_t* container, int index)
{
    static lv_ft_info_t font_title;
    font_title.name = FONT_PATH;
    font_title.weight = 20;
    font_title.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_title);

    static lv_style_t style_label_func;
    lv_style_init(&style_label_func);
    lv_style_set_text_font(&style_label_func, font_title.font);

    static lv_style_t style_switch_knob;
    lv_style_init(&style_switch_knob);
    lv_style_set_bg_color(&style_switch_knob, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_switch_knob, lv_color_hex(0xe7e7e7));
    lv_style_set_bg_grad_dir(&style_switch_knob, LV_GRAD_DIR_VER); // 设置样式的颜色渐变方向为垂直方向
    lv_style_set_border_color(&style_switch_knob, lv_color_hex(0xdfdfdf));
    lv_style_set_border_width(&style_switch_knob, 1);

    static lv_style_t style_switch;
    lv_style_init(&style_switch);
    lv_style_set_bg_color(&style_switch, lv_color_hex(0xf0f0f0));
    lv_style_set_border_color(&style_switch, lv_color_hex(0x000000));
    lv_style_set_border_opa(&style_switch, 10);
    lv_style_set_border_width(&style_switch, 1);

    lv_obj_t* label_func = lv_label_create(container);
    lv_obj_set_size(label_func, 200, 40);
    lv_obj_set_pos(label_func, 20, 14);
    lv_label_set_text(label_func, sFunc[index]);
    lv_obj_add_style(label_func, &style_label_func, LV_PART_MAIN);

    lv_obj_t* switch_btn = lv_switch_create(container);
    lv_obj_set_size(switch_btn, 68, 30);
    lv_obj_set_pos(switch_btn, 380, 19);
    lv_obj_add_style(switch_btn, &style_switch_knob, LV_PART_KNOB);
    lv_obj_add_style(switch_btn, &style_switch, LV_PART_MAIN);
    lv_obj_set_style_bg_color(switch_btn, lv_color_hex(0x12b1d1), LV_PART_INDICATOR | LV_STATE_CHECKED);
	lv_obj_add_event_cb(switch_btn, on_switch_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_state_t switch_state = lv_obj_get_state(switch_btn);
    lv_obj_set_style_outline_width(switch_btn, 0, switch_state);
    if (index == 0 || index == 3) lv_obj_add_state(switch_btn, LV_STATE_CHECKED);    //初始化第一个和第四个按键开关状态为CHECKED
}

static void create_switch_list(lv_obj_t* parent)
{
    static lv_style_t style_list;
    lv_style_init(&style_list);
    lv_style_set_radius(&style_list, 0);
    lv_style_set_border_width(&style_list, 0);

    lv_obj_t* list_switch = lv_list_create(parent);
    lv_obj_set_size(list_switch, 480, 272);
    lv_obj_set_pos(list_switch, 0, 48);
    lv_obj_set_style_pad_all(list_switch, 0, LV_PART_MAIN);
    lv_obj_add_style(list_switch, &style_list, LV_PART_MAIN);

    lv_obj_t* list_item;

    int i;
    for (i = 0; i < 4; i++) {
        list_item = lv_obj_create(list_switch);
        lv_obj_set_size(list_item, 480, 68);
        lv_obj_add_style(list_item, &style_container, LV_PART_MAIN);
        lv_obj_clear_flag(list_item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(list_item, 0, LV_PART_MAIN);
        if (i < 3) {
            lv_obj_set_style_border_width(list_item, 1, LV_PART_MAIN);
            lv_obj_set_style_border_side(list_item, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
        }
        create_switch_list_item(list_item, i);
    }
}

void destroy_monitor_window()
{
    lv_obj_del(monitor_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, monitor_window);
}

void create_monitor_window(lv_obj_t* screen)
{
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);

    monitor_window = lv_obj_create(screen);
    lv_obj_add_style(monitor_window, &style_container, LV_PART_MAIN);
    lv_obj_set_size(monitor_window, 480, 320);
    lv_obj_clear_flag(monitor_window, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(monitor_window, 0, LV_PART_MAIN);

    create_title_bar(monitor_window);
    create_switch_list(monitor_window);

    lv_obj_t* btn_return = lv_imgbtn_create(monitor_window);
    lv_obj_set_pos(btn_return, 420, 4);
    lv_obj_set_size(btn_return, 56, 52);
    lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
    lv_obj_add_event_cb(btn_return, quit_monitor_window, LV_EVENT_CLICKED, NULL);

}
