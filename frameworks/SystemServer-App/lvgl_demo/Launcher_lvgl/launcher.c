#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

#include "lvgl/lvgl.h"



static void launcher_icon_create(lv_obj_t * par);
static void launcher_static_icon_create(lv_obj_t * par);


//#define SHADOW_LABEL_EN     0

static int i_launcher_page = 3;
static bool b_virtual_btn = true;
static int i_icon_language = 1;


static lv_obj_t * img_vir_btn[3];
static lv_obj_t * led_icon_page[3];

#define LED_ICON_PAGE_DIS 40

#define MOVE_ICONS_PER_ROW 4      // 每行图标数，不建议修改
#define ROWS_OF_MOVE_ICONS 2      // 每页的总行数，可以修改，目前建议最大为4行
#define MAX_MOVE_ICONS_PER_PAGE (MOVE_ICONS_PER_ROW * ROWS_OF_MOVE_ICONS) // 计算每页最大图标数
#define MAX_STATIC_ICONS_NUM	3			// 静态图标的最大个数

#define FONT_PATH                    "/assets/fonts/sourcehan.ttf"

static lv_obj_t * launcher_bg;
static lv_obj_t * launcher_page[3];
static lv_obj_t * launcher_tab_icon;
static lv_obj_t * tab[3];
static lv_obj_t * launcher_taskbar;
static lv_ft_info_t font_icon_name;


LV_IMG_DECLARE(launcher_background_image)

LV_IMG_DECLARE(icon_031)
LV_IMG_DECLARE(icon_071)
LV_IMG_DECLARE(icon_087)

typedef struct app_icon_info {
	lv_obj_t * icon_imgbtn;
	const lv_img_dsc_t * icon_img_desc;
	lv_obj_t * icon_name_label;
	char *icon_name;
} app_icon_info_t;


static app_icon_info_t move_app_icon[] = {
	{
		.icon_img_desc = &icon_031,
		.icon_name = "时钟",
	},
	{
		.icon_img_desc = &icon_071,
		.icon_name = "相机",
	},
	{
		.icon_img_desc = &icon_087,
		.icon_name = "视频",
	},
};

static app_icon_info_t static_app_icon[] = {};


#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


static int create_app(const char *filename, const char *symbol)
{
	void * handle;
	void (*app_create_func)(lv_obj_t *, void *);
	char * error;

	handle = dlopen(filename, RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "%s\n", dlerror());
		return -1;
	}

	dlerror();    /* Clear any existing error */

	app_create_func = (void (*)(lv_obj_t *, void *)) dlsym(handle, symbol);

	error = dlerror();
	if (error != NULL) {
		fprintf(stderr, "%s\n", error);
		return -1;
	}

	(*app_create_func)(lv_scr_act(), handle);

	return 0;
}


static void icon_click_event_cb(lv_event_t * e)
{
	int i = 0;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * icon = lv_event_get_user_data(e);
	int move_app_icon_num = ARRAY_SIZE(move_app_icon);
	int static_app_icon_num = ARRAY_SIZE(static_app_icon);

	if (code == LV_EVENT_SHORT_CLICKED) {
		for(i=0; i < move_app_icon_num; i++) {
			if(icon == move_app_icon[i].icon_imgbtn) {
				printf("[%s : %d] icon num:%d\n", __func__, __LINE__, i);

				switch(i) {
					case 0:		/* clock */
						create_app("libclockapp.so", "app_calendar_create");
						break;
					case 1:		/* camera */
						create_app("libcameraapp.so", "app_camera_create");
						break;
					case 2:
						create_app("libplayerapp.so", "app_player_create");
						break;
				}
			}
		}

		for(i=0; i < static_app_icon_num; i++) {
			if(icon == static_app_icon[i].icon_imgbtn) {
				printf("[%s : %d] icon num:%d\n", __func__, __LINE__, i);

				/* TODO... */
			}
		}
	}

	if (code == LV_EVENT_LONG_PRESSED) {
		/* TODO... */
	}
}


static void launcher_taskbar_create(lv_obj_t * par)
{
    if(launcher_taskbar == NULL)
    {
        launcher_taskbar = lv_label_create(par);
    }
	lv_label_set_text(launcher_taskbar, "China Mobile Sat. 2020.08.31");
    lv_obj_align_to(launcher_taskbar, launcher_page[0], LV_ALIGN_LEFT_MID, LV_HOR_RES >> 5, 0);
}

static void tab_move_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * tab = lv_event_get_user_data(e);

    if (tab == launcher_tab_icon) {
        if (code == LV_EVENT_VALUE_CHANGED) {
            printf("page change\n");
        }
    }
}


static void launcher_icon_create(lv_obj_t * par)
{
	int i = 0;
#if SHADOW_LABEL_EN
    lv_obj_t * shadow_label;
#endif
	int move_app_icon_num = ARRAY_SIZE(move_app_icon);

	if (move_app_icon_num > MAX_MOVE_ICONS_PER_PAGE * 3/* 3页 */) {
		fprintf(stderr, "The number of movable icons exceeds the maximum limit.(%d > %d)\n", \
				move_app_icon_num, MAX_MOVE_ICONS_PER_PAGE * 3);
		exit(EXIT_FAILURE);
	}

	if(launcher_tab_icon == NULL)
	{
		launcher_tab_icon = lv_tabview_create(par, LV_DIR_TOP, 50);

	    tab[0] = lv_tabview_add_tab(launcher_tab_icon, "A PAGE");
	    tab[1] = lv_tabview_add_tab(launcher_tab_icon, "B PAGE");
	    tab[2] = lv_tabview_add_tab(launcher_tab_icon, "C PAGE");

		lv_btnmatrix_set_btn_ctrl_all(lv_tabview_get_tab_btns(launcher_tab_icon), LV_BTNMATRIX_CTRL_HIDDEN);
		lv_obj_set_style_bg_opa(lv_tabview_get_tab_btns(launcher_tab_icon), LV_OPA_0, LV_STATE_DEFAULT);

		lv_obj_set_style_bg_opa(launcher_tab_icon, LV_OPA_0, LV_STATE_DEFAULT);
		lv_obj_set_size(launcher_tab_icon, lv_obj_get_width(par) - 40, lv_obj_get_height(par) - 40);
		lv_obj_align_to(launcher_tab_icon, par, LV_ALIGN_TOP_LEFT, 0, 0);
	}

	for(i = 0; i < move_app_icon_num; i++) {
		if (move_app_icon[i].icon_imgbtn == NULL) {
			move_app_icon[i].icon_imgbtn = lv_imgbtn_create(tab[i / MAX_MOVE_ICONS_PER_PAGE]);
			lv_obj_set_size(move_app_icon[i].icon_imgbtn, 100, 100);
			lv_obj_add_event_cb(move_app_icon[i].icon_imgbtn, icon_click_event_cb, LV_EVENT_ALL, move_app_icon[i].icon_imgbtn);
		}

		lv_imgbtn_set_src(move_app_icon[i].icon_imgbtn, LV_IMGBTN_STATE_RELEASED, move_app_icon[i].icon_img_desc, NULL, NULL);
		lv_imgbtn_set_src(move_app_icon[i].icon_imgbtn, LV_IMGBTN_STATE_PRESSED, move_app_icon[i].icon_img_desc, NULL, NULL);

		if (move_app_icon[i].icon_name_label == NULL) {
			move_app_icon[i].icon_name_label = lv_label_create(tab[i / MAX_MOVE_ICONS_PER_PAGE]);
		}

		lv_obj_set_style_text_font(move_app_icon[i].icon_name_label, font_icon_name.font, LV_PART_MAIN);
		lv_label_set_text(move_app_icon[i].icon_name_label, move_app_icon[i].icon_name);
        lv_obj_set_style_text_color(move_app_icon[i].icon_name_label, lv_color_hex(0x000000), LV_STATE_DEFAULT);

		if ((i % MAX_MOVE_ICONS_PER_PAGE) == 0) {
			lv_obj_align_to(move_app_icon[i].icon_imgbtn, tab[i / MAX_MOVE_ICONS_PER_PAGE], LV_ALIGN_TOP_LEFT, 20, 0);
		} else if((i % 4) == 0) {
			lv_obj_align_to(move_app_icon[i].icon_imgbtn, move_app_icon[i - 4].icon_imgbtn, LV_ALIGN_OUT_BOTTOM_MID, 0, 100);
		} else {
			lv_obj_align_to(move_app_icon[i].icon_imgbtn, move_app_icon[i - 1].icon_imgbtn, LV_ALIGN_OUT_RIGHT_MID, 64, 0);
		}

		lv_obj_align_to(move_app_icon[i].icon_name_label, move_app_icon[i].icon_imgbtn, LV_ALIGN_OUT_BOTTOM_MID, -1, 5);

#if SHADOW_LABEL_EN
    shadow_label = lv_label_create(par,icon_name[i]);
    lv_obj_set_style_local_text_color(shadow_label,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLACK);
    lv_obj_align(shadow_label,icon_name[i],LV_ALIGN_CENTER,-1,-1);
#endif
	}

	lv_obj_add_event_cb(launcher_tab_icon, tab_move_event_cb, LV_EVENT_ALL, launcher_tab_icon);
}



LV_IMG_DECLARE(key_back)
LV_IMG_DECLARE(key_home)
LV_IMG_DECLARE(key_tool)

static const lv_img_dsc_t * img_array_vir_key[3] =
{
    &key_back,
    &key_home,
    &key_tool
};

static void launcher_static_icon_create(lv_obj_t * par)
{
    int i;
#if SHADOW_LABEL_EN
    lv_obj_t * shadow_label;
#endif
	int static_app_icon_num = ARRAY_SIZE(static_app_icon);

	if (static_app_icon_num > MAX_STATIC_ICONS_NUM) {
		fprintf(stderr, "The number of static icons exceeds the maximum limit.(%d > %d)\n", \
				static_app_icon_num, MAX_STATIC_ICONS_NUM);
		exit(EXIT_FAILURE);
	}

	if (b_virtual_btn == true) {
		for (i = 0; i < 3; i++) {
			if (img_vir_btn[i] == NULL) {
				img_vir_btn[i] = lv_imgbtn_create(par);
				lv_obj_set_size(img_vir_btn[i], 40, 40);
			}

			lv_imgbtn_set_src(img_vir_btn[i], LV_IMGBTN_STATE_RELEASED, img_array_vir_key[i], NULL, NULL);
			lv_imgbtn_set_src(img_vir_btn[i], LV_IMGBTN_STATE_PRESSED, img_array_vir_key[i], NULL, NULL);

			lv_obj_align_to(img_vir_btn[i], par, LV_ALIGN_BOTTOM_MID, \
					(1 - i) * lv_obj_get_width(par) >> 2, -lv_obj_get_height(par) >> 4);

			lv_obj_set_style_img_recolor(img_vir_btn[i], lv_palette_main(LV_PALETTE_GREY), LV_STATE_DEFAULT);
			lv_obj_set_style_img_recolor_opa(img_vir_btn[i], LV_OPA_COVER, LV_STATE_DEFAULT);			// 颜色生效

			if (lv_obj_has_flag(img_vir_btn[i], LV_OBJ_FLAG_HIDDEN))
				lv_obj_clear_flag(img_vir_btn[i], LV_OBJ_FLAG_HIDDEN);
		}
	} else {
		for (i = 0; i < 3; i++) {
			if (!lv_obj_has_flag(img_vir_btn[i], LV_OBJ_FLAG_HIDDEN))
				lv_obj_add_flag(img_vir_btn[i], LV_OBJ_FLAG_HIDDEN);
		}
	}

	for (i=0; i < static_app_icon_num; i++) {
		if (static_app_icon[i].icon_name_label == NULL) {
			static_app_icon[i].icon_name_label = lv_label_create(par);
		}

		lv_obj_set_style_text_font(static_app_icon[i].icon_name_label, font_icon_name.font, LV_PART_MAIN);
		lv_label_set_text(static_app_icon[i].icon_name_label, static_app_icon[i].icon_name);
        lv_obj_set_style_text_color(static_app_icon[i].icon_name_label, lv_color_hex(0x000000), LV_STATE_DEFAULT);

		if (b_virtual_btn == true) {
			lv_obj_align_to(static_app_icon[i].icon_name_label, \
					par, LV_ALIGN_BOTTOM_MID, (i - 1) * lv_obj_get_width(par) >> 2, \
					-((lv_obj_get_height(par) >> 4) + lv_obj_get_height(img_vir_btn[0])) - 25);
		} else {
			lv_obj_align_to(static_app_icon[i].icon_name_label, \
					par, LV_ALIGN_BOTTOM_MID, (i - 1) * lv_obj_get_width(par) >> 2, \
					-(lv_obj_get_height(par) >> 4) - 10);
		}
#if SHADOW_LABEL_EN
    shadow_label = lv_label_create(par,icon_name[MAX_ICON_NUM - i_launcher_sta_icon + i]);
    lv_obj_set_style_local_text_color(shadow_label,LV_LABEL_PART_MAIN,LV_STATE_DEFAULT,LV_COLOR_BLACK);
    lv_obj_align(shadow_label,icon_name[MAX_ICON_NUM - i_launcher_sta_icon + i],\
            LV_ALIGN_CENTER,-1,-1);
#endif
	}

	for (i = 0; i < static_app_icon_num; i++) {
		if (static_app_icon[i].icon_imgbtn == NULL) {
			static_app_icon[i].icon_imgbtn = lv_imgbtn_create(par);
			lv_obj_set_size(static_app_icon[i].icon_imgbtn, 100, 100);
			lv_obj_add_event_cb(static_app_icon[i].icon_imgbtn, icon_click_event_cb, \
					LV_EVENT_ALL, static_app_icon[i].icon_imgbtn);
		}

		lv_imgbtn_set_src(static_app_icon[i].icon_imgbtn, LV_IMGBTN_STATE_RELEASED, static_app_icon[i].icon_img_desc, NULL, NULL);
		lv_imgbtn_set_src(static_app_icon[i].icon_imgbtn, LV_IMGBTN_STATE_PRESSED, static_app_icon[i].icon_img_desc, NULL, NULL);

		if (true) {
			lv_obj_align_to(static_app_icon[i].icon_imgbtn, \
					static_app_icon[i].icon_name_label, \
					LV_ALIGN_OUT_TOP_MID, 3, -(lv_obj_get_height(par) >> 5));
		}
	}
}

static void launcher_led_create(lv_obj_t * par)
{
	int i;

	for(i = 0; i < i_launcher_page; i++) {
		if(i == 0) {
			if (led_icon_page[i] == NULL) {
				led_icon_page[i] = lv_led_create(par);
			}
			lv_led_set_color(led_icon_page[0], lv_color_hex(0xffffff));
 			lv_obj_set_style_bg_opa(led_icon_page[0], LV_OPA_100, LV_STATE_DEFAULT);
			lv_obj_set_size(led_icon_page[0], 12, 12);
			lv_led_set_brightness(led_icon_page[0], LV_LED_BRIGHT_MAX);
		} else {
			if (led_icon_page[i] == NULL) {
				led_icon_page[i] = lv_led_create(par);
				lv_led_set_color(led_icon_page[i], lv_color_hex(0xffffff));
			}
			lv_obj_set_size(led_icon_page[i], 6, 6);
			lv_led_set_brightness(led_icon_page[i], LV_LED_BRIGHT_MIN);
		}

		lv_obj_align_to(led_icon_page[i], par, LV_ALIGN_TOP_MID, (i - 1) * LED_ICON_PAGE_DIS, 6);
	}
}

static void led_icon_page_handle(void)
{
	int i = 0;

	lv_coord_t coordsx = tab[1]->coords.x1;

	for (i = 0; i < i_launcher_page; i++) {
		lv_obj_align_to(led_icon_page[0], lv_obj_get_parent(led_icon_page[0]), LV_ALIGN_TOP_MID, \
				-(coordsx * LED_ICON_PAGE_DIS / LV_HOR_RES), 6);

		if (tab[1]->coords.x1 < 0) {
			lv_obj_align_to(led_icon_page[2], lv_obj_get_parent(led_icon_page[2]), LV_ALIGN_TOP_MID, \
					 (coordsx * LED_ICON_PAGE_DIS / LV_HOR_RES) + LED_ICON_PAGE_DIS, 6);
		}

		if (tab[1]->coords.x1 > 0) {
			lv_obj_align_to(led_icon_page[1], lv_obj_get_parent(led_icon_page[1]), LV_ALIGN_TOP_MID, \
					(coordsx * LED_ICON_PAGE_DIS / LV_HOR_RES) - LED_ICON_PAGE_DIS, 6);
		}
	}
}

static void user_timer_xcb(lv_timer_t * timer)
{
	char buf[52];
    sprintf(buf, "China Mobile Sat. 2020.12.27 %2d:%02d:%02d", lv_tick_get() / 3600000 + 23, lv_tick_get() / 60000 % 60, lv_tick_get() / 1000 % 60);
    lv_label_set_text(launcher_taskbar, buf);

    led_icon_page_handle();
}



void launcher_widgets(void)
{
	int i;
	lv_obj_t * par_scr;
	par_scr = lv_scr_act();

	launcher_bg = lv_img_create(par_scr);
	lv_img_set_src(launcher_bg, &launcher_background_image);
	lv_obj_set_size(launcher_bg, LV_HOR_RES, LV_VER_RES);

	for(i=0; i<3; i++) {
		launcher_page[i] = lv_obj_create(par_scr);
		lv_obj_set_style_bg_opa(launcher_page[i], LV_OPA_0, LV_STATE_DEFAULT);
		lv_obj_set_style_radius(launcher_page[i], 0, LV_STATE_DEFAULT);
		lv_obj_set_style_border_width(launcher_page[i], 0, LV_STATE_DEFAULT);
		lv_obj_set_width(launcher_page[i], LV_HOR_RES);
	}

	lv_obj_set_height(launcher_page[0], (LV_VER_RES >> 5) + 10);
	lv_refr_now(NULL);
	lv_obj_set_height(launcher_page[2], (LV_VER_RES >> 2) - 10);
	lv_refr_now(NULL);
	lv_obj_set_height(launcher_page[1], LV_VER_RES - lv_obj_get_height(launcher_page[0]) - lv_obj_get_height(launcher_page[2]));
	lv_refr_now(NULL);

	lv_obj_align_to(launcher_page[0], par_scr, LV_ALIGN_TOP_LEFT, 0 ,0);
	lv_obj_align_to(launcher_page[2], par_scr, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	lv_obj_align_to(launcher_page[1], launcher_page[0], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

	font_icon_name.name = FONT_PATH;
	font_icon_name.weight = 20;
	font_icon_name.style = FT_FONT_STYLE_NORMAL;
	lv_ft_font_init(&font_icon_name);

	launcher_taskbar_create(launcher_page[0]);
	launcher_icon_create(launcher_page[1]);
	launcher_static_icon_create(launcher_page[2]);
	launcher_led_create(launcher_page[2]);
	lv_timer_create(user_timer_xcb, 10, NULL);
}
