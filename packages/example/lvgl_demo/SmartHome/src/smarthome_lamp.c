/*
 * @file smarthome_lamp.c - is provided for use with Ingenic products.
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

/*******************
 *     INCLUDES   *
 *******************/
#include "lvgl.h"
#include "smarthome_lamp.h"

/*******************
 *     DEFINES    *
 *******************/
#define FONT_PATH     "../assets/fonts/sourcehan.ttf"
#define RING_TONE "../assets/songs/btn_sound.wav"
/*******************
 *     TYPEDFS
 *******************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t* lamp_window;

static lv_style_t style_switch_knob;
static lv_style_t style_switch;
static lv_style_t style_container;

/*******************
 *    MACROS
 *******************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void quit_lamp_window(lv_event_t* e)
{
	start_ring(RING_TONE);
    destroy_lamp_window();
}

static void lamp_switch_clicked(lv_event_t* e)
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

    lv_style_init(&style_switch_knob);
    lv_style_set_bg_color(&style_switch_knob, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_switch_knob, lv_color_hex(0xe7e7e7));
    lv_style_set_bg_grad_dir(&style_switch_knob, LV_GRAD_DIR_VER); // 设置样式的颜色渐变方向为垂直方向
    lv_style_set_border_color(&style_switch_knob, lv_color_hex(0xefefef));
    lv_style_set_border_width(&style_switch_knob, 1);

    lv_style_init(&style_switch);
    lv_style_set_bg_color(&style_switch, lv_color_hex(0xffffff));
    lv_style_set_border_color(&style_switch, lv_color_hex(0x000000));
    lv_style_set_border_opa(&style_switch, 48);
    lv_style_set_border_width(&style_switch, 1);
    lv_style_set_border_side(&style_switch, LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_TOP);

    lv_obj_t* title_con = lv_obj_create(parent);
    lv_obj_set_pos(title_con, 0, 0);
    lv_obj_set_size(title_con, 480, 48);
    lv_obj_add_style(title_con, &style_title_bg, LV_PART_MAIN);
    lv_obj_clear_flag(title_con, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* btn_switch = lv_switch_create(parent);
    lv_obj_set_pos(btn_switch, 10, 9);
    lv_obj_set_size(btn_switch, 62, 30);
    lv_obj_add_style(btn_switch, &style_switch_knob, LV_PART_KNOB);
    lv_obj_add_style(btn_switch, &style_switch, LV_PART_MAIN);
	lv_obj_add_event_cb(btn_switch, lamp_switch_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0x12b1d1), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_state_t switch_state = lv_obj_get_state(btn_switch);
    lv_obj_set_style_outline_width(btn_switch, 0, switch_state);
    lv_obj_add_state(btn_switch, LV_STATE_CHECKED);

    lv_obj_t* label = lv_label_create(title_con);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(label, 0, 4);
    lv_obj_set_size(label, 480, 30);
    lv_obj_add_style(label, &style_title, LV_PART_MAIN);
    lv_label_set_text(label, "灯光控制");
}

static void create_container_room_item(lv_obj_t* parent, char* room_name, bool state)
{
    static lv_ft_info_t font_name;
    font_name.name = FONT_PATH;
    font_name.weight = 18;
    font_name.style = FT_FONT_STYLE_NORMAL;
    lv_ft_font_init(&font_name);

    lv_obj_t* label_room_name = lv_label_create(parent);
    lv_obj_set_size(label_room_name, 100, 30);
    lv_obj_set_pos(label_room_name, 20, 1);
    lv_obj_set_style_text_font(label_room_name, font_name.font, LV_PART_MAIN);
    lv_label_set_text(label_room_name, room_name);

    lv_obj_t* btn_switch = lv_switch_create(parent);
    lv_obj_set_pos(btn_switch, 395, 3);
    lv_obj_set_size(btn_switch, 62, 30);
    lv_obj_set_style_bg_color(btn_switch, lv_color_hex(0x12b1d1), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(btn_switch, &style_switch_knob, LV_PART_KNOB);
    lv_obj_add_style(btn_switch, &style_switch, LV_PART_MAIN);
	lv_obj_add_event_cb(btn_switch, lamp_switch_clicked, LV_EVENT_CLICKED, NULL);

    lv_state_t switch_state = lv_obj_get_state(btn_switch);
    lv_obj_set_style_outline_width(btn_switch, 0, switch_state);
    if (state)
        lv_obj_add_state(btn_switch, LV_STATE_CHECKED);
}

static void create_controller_area(lv_obj_t* parent, uint32_t value)
{
    static lv_style_t style_slider;
    lv_style_init(&style_slider);
    lv_style_set_bg_opa(&style_slider, LV_OPA_30);
    lv_style_set_bg_color(&style_slider, lv_color_hex(0x000000));

    static lv_style_t style_slider_indicator;
    lv_style_init(&style_slider_indicator);
    lv_style_set_bg_opa(&style_slider_indicator, LV_OPA_COVER);
    lv_style_set_bg_color(&style_slider_indicator, lv_color_hex(0x12b1d1));

    static lv_style_t style_slider_knob;
    lv_style_init(&style_slider_knob);
    lv_style_set_pad_all(&style_slider_knob, 10); /*Makes the knob larger*/
    lv_style_set_bg_color(&style_slider_knob, lv_color_hex(0xffffff));
    lv_style_set_bg_grad_color(&style_slider_knob, lv_color_hex(0xe7e7e7));
    lv_style_set_bg_grad_dir(&style_slider_knob, LV_GRAD_DIR_VER); // 设置样式的颜色渐变方向为垂直方向
    lv_style_set_border_color(&style_slider_knob, lv_color_hex(0xc2c2c2));
    lv_style_set_border_width(&style_slider_knob, 1);

    lv_obj_t* slider = lv_slider_create(parent);
    lv_obj_set_pos(slider, 67, 30);
    lv_obj_set_size(slider, 345, 3);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    lv_obj_add_style(slider, &style_slider, LV_PART_MAIN);
    lv_obj_add_style(slider, &style_slider_indicator,  LV_PART_INDICATOR);
    lv_obj_add_style(slider, &style_slider_knob, LV_PART_KNOB);
}

static void create_display_area(lv_obj_t* parent)
{
    static lv_style_t style_room;
    lv_style_init(&style_room);
    lv_style_set_bg_color(&style_room, lv_color_hex(0x000000));
    lv_style_set_radius(&style_room, 0);
    lv_style_set_border_width(&style_room, 0);
    lv_style_set_pad_all(&style_room, 0);

    lv_obj_t* container_room_1 = lv_obj_create(parent);
    lv_obj_set_pos(container_room_1, 0, 48);
    lv_obj_set_size(container_room_1, 480, 36);
    lv_obj_add_style(container_room_1, &style_room, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_room_1, LV_OPA_10, LV_PART_MAIN);
    lv_obj_clear_flag(container_room_1, LV_OBJ_FLAG_SCROLLABLE);
    create_container_room_item(container_room_1, "客厅", true);

    lv_obj_t* container_controller_1 = lv_obj_create(parent);
    lv_obj_clear_flag(container_controller_1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(container_controller_1, 0, 84);
    lv_obj_set_size(container_controller_1, 480, 64);
    lv_obj_set_style_bg_opa(container_controller_1, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(container_controller_1, "../assets/images/lights_bar_bg.png", LV_PART_MAIN);
    lv_obj_add_style(container_controller_1, &style_room, LV_PART_MAIN);
    create_controller_area(container_controller_1, 60);

    lv_obj_t* container_room_2 = lv_obj_create(parent);
    lv_obj_set_pos(container_room_2, 0, 160);
    lv_obj_set_size(container_room_2, 480, 36);
    lv_obj_add_style(container_room_2, &style_room, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_room_2, LV_OPA_10, LV_PART_MAIN);
    lv_obj_clear_flag(container_room_2, LV_OBJ_FLAG_SCROLLABLE);

    create_container_room_item(container_room_2, "主卧", false);

    lv_obj_t* container_controller_2 = lv_obj_create(parent);
    lv_obj_clear_flag(container_controller_2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(container_controller_2, 0, 196);
    lv_obj_set_size(container_controller_2, 480, 64);
    lv_obj_set_style_bg_opa(container_controller_2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_img_src(container_controller_2, "../assets/images/lights_bar_bg.png", LV_PART_MAIN);
    lv_obj_add_style(container_controller_2, &style_room, LV_PART_MAIN);
    create_controller_area(container_controller_2, 30);

    lv_obj_t* container_room_3 = lv_obj_create(parent);
    lv_obj_set_pos(container_room_3, 0, 272);
    lv_obj_set_size(container_room_3, 480, 36);
    lv_obj_add_style(container_room_3, &style_room, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_room_3, LV_OPA_10, LV_PART_MAIN);
    lv_obj_clear_flag(container_room_3, LV_OBJ_FLAG_SCROLLABLE);

    create_container_room_item(container_room_3, "次卧", false);
}

void destroy_lamp_window()
{
    lv_obj_del(lamp_window);
	lv_event_send(lv_scr_act(), LV_EVENT_DELETE, lamp_window);
}

void create_lamp_window(lv_obj_t* screen)
{
    lv_style_init(&style_container);
    lv_style_set_radius(&style_container, 0);
    lv_style_set_border_width(&style_container, 0);
    lv_style_set_pad_all(&style_container, 0);

    lamp_window = lv_obj_create(screen);
    lv_obj_add_style(lamp_window, &style_container, LV_PART_MAIN);
    lv_obj_set_size(lamp_window, 480, 320);
    lv_obj_clear_flag(lamp_window, LV_OBJ_FLAG_SCROLLABLE);

    create_title_bar(lamp_window);
    create_display_area(lamp_window);

    lv_obj_t* btn_return = lv_imgbtn_create(lamp_window);
    lv_obj_set_pos(btn_return, 420, 4);
    lv_obj_set_size(btn_return, 56, 52);
    lv_imgbtn_set_src(btn_return, LV_IMGBTN_STATE_RELEASED, "../assets/images/back_home.png", NULL, NULL);
    lv_obj_add_event_cb(btn_return, quit_lamp_window, LV_EVENT_CLICKED, NULL);

}
