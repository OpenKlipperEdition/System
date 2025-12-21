
/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <dlfcn.h>
#include "ffplay.h"

#define VIDEO_PATH "/media/video"

static lv_obj_t * main_page;
static lv_obj_t * close_btn;
struct OsdClient *player_clt = NULL;
int video_start_flag = 0;
int video_file_count = 0;
int video_name_array[30] = { 0 };
int current_video_pos = 0;
extern pthread_mutex_t g_ui_lock;
static int last_hh = 0;
static int last_mm = 0;
static int last_ss = 0;

LV_IMG_DECLARE(play_pause)
LV_IMG_DECLARE(previous_video)
LV_IMG_DECLARE(next_video)

int player_video_eof_proc(void *context, int is_loop)
{
	printf("once video eof, callback!\n");
	if (!is_loop){
	*(int *)context = 1;
	}
	last_hh = 0;
	last_mm = 0;
	last_ss = 0;
	return 0;
}
int trg_x = -1, trg_y = -1, trg_w = -1, trg_h = -1;
int crop_x = -1, crop_y = -1, crop_w = -1, crop_h = -1;
int enable_scale = 0;
/* video start call back! */
void player_video_begin_proc(void *context, ImppRect *vd_rect, ImppRect *trg_rect,
		ImppRect *crop_rect, int *scaler, int64_t duration_us)
{
  int target_w, target_h;

  printf("(x,y,w,h): video (%d,%d,%d,%d), screen/target (%d,%d,%d,%d), "
	 "crop of source (%d,%d,%d,%d), scaler %d\n",
	 vd_rect->x, vd_rect->y, vd_rect->w, vd_rect->h,
	 trg_rect->x, trg_rect->y, trg_rect->w, trg_rect->h,
	 crop_rect->x, crop_rect->y, crop_rect->w, crop_rect->h,
	 scaler);

  /* set video source crop rect.
   .....   */
  /* display position and size setted by lv_obj_set_pos and lv_obj_set_size
   ..... */
  trg_rect->w = trg_w == -1 ? vd_rect->w : trg_w;
  trg_rect->h = trg_h == -1 ? vd_rect->h : trg_h;
  trg_rect->x = trg_x == -1? 0 : trg_x;
  trg_rect->y = trg_y == -1? 0 : trg_y;

  crop_rect->w = crop_w == -1 ? vd_rect->w : crop_w;
  crop_rect->h = crop_h == -1 ? vd_rect->h : crop_h;
  crop_rect->x = crop_x == -1? 0 : crop_x;
  crop_rect->y = crop_y == -1? 0 : crop_y;
  /* Is do scaler from video size to display size */
  *scaler = enable_scale;

  /* reset video decoder OUT Buffer and CAPTURE Buffer numbers */
//  video_reset_param(vd_rect->w, vd_rect->h);

  /* dump video duration */
#define US_TIME_BASE  1000000
  {
    int hours, mins, secs, us;
    int64_t duration = duration_us;
    secs  = duration / US_TIME_BASE;
    us    = duration % US_TIME_BASE;
    mins  = secs / 60;
    secs %= 60;
    hours = mins / 60;
    mins %= 60;
    printf("[video duration]: %02d:%02d:%02d.%02d\n", hours, mins, secs,
	   (100 * us) / US_TIME_BASE);
  }
}

static void player_close_event_cb1(lv_event_t * e)
{
	void * handle = lv_event_get_user_data(e);
	lv_event_code_t event = lv_event_get_code(e);

	if (event == LV_EVENT_SHORT_CLICKED) {
		lv_obj_del(main_page);
//        	lv_timer_del(timer);
		ffplay_play_state_pause(1);
		ffplay_play_state_stop();
		ffplay_deinit();
		dlclose(handle);
	}

}

int video_timestamp(void *args, double timestamp)
{
	lv_obj_t * time_lable = args;
	int int_timestamp = (int)timestamp;
	int hh = 0;
	int mm = 0;
	int ss = 0;


	ss = int_timestamp % 60;
	mm = int_timestamp / 60;
	hh = mm /60;
	mm = mm % 60;

	if (ss <= last_ss)
		return 0;

	pthread_mutex_lock(&g_ui_lock);
	lv_label_set_text_fmt(time_lable, "%02d:%02d:%02d", hh, mm, ss);
	pthread_mutex_unlock(&g_ui_lock);

	last_ss = ss;
	last_mm = mm;
	last_hh = hh;
}

int stop_ffplay_video()
{
	ffplay_play_state_pause(1);
	ffplay_play_state_stop();
}

int start_ffplay_video(int index)
{
	int n = 0;
	int width = 0;
	int height = 0;
	int posx = 0;
	int posy = 0;
	char tmp_name[30] = {0};
	sprintf(tmp_name, "%s/%s", VIDEO_PATH, video_name_array[index]);
	n = ffplay_set_play_file(tmp_name);
	ffplay_get_video_width_and_height(&width, &height);
	posx = (LV_HOR_RES - width) / 2;
	posy = ((LV_VER_RES - 100 - 200 - height) / 2 + 100);
	ffplay_set_video_pos(posx, posy, 0, 0);
    	video_start_flag = 0;
//    	ffplay_set_play_loop(1);
	ffplay_play_state_pause(video_start_flag);
	last_hh = 0;
	last_mm = 0;
	last_ss = 0;
}

static void video_btn_click_event_cb(lv_event_t * e)
{
	int which_icon = 0;
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * icon = lv_event_get_user_data(e);

	which_icon = (int)lv_obj_get_user_data(icon);

	if (code == LV_EVENT_SHORT_CLICKED)
	{
		switch(which_icon)
		{
			case 1:
				if (video_file_count != 0)
				{
					{
						current_video_pos -= 1;
						if (current_video_pos < 0)
							current_video_pos = video_file_count - 1;
					}
					stop_ffplay_video();
					start_ffplay_video(current_video_pos);
				}
				break;
			case 2:
				video_start_flag = !video_start_flag;
				ffplay_play_state_pause(video_start_flag);
				break;
			case 3:
				if (video_file_count != 0)
				{
					{
						current_video_pos += 1;
						if (current_video_pos >= video_file_count)
							current_video_pos = 0;
					}
					stop_ffplay_video();
					start_ffplay_video(current_video_pos);
				}
				break;
		}
	}
}

int scan_video_path()
{
	DIR *dir = opendir(VIDEO_PATH);
	if (dir == NULL)
	{
		return;
	}

	struct dirent *entry;
	char *tmp_video_name = NULL;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] != '.')
		{
			tmp_video_name = malloc(strlen(entry->d_name));
			strcpy(tmp_video_name, entry->d_name);
			video_name_array[video_file_count] = tmp_video_name;
			video_file_count++;
		}
	}

	closedir(dir);
}

void app_player_create(lv_obj_t * parent, void *handle)
{
	int i;
	int n = 0;
	int ret = 0;
	lv_obj_t *top_div = NULL;
	lv_obj_t *bottom_div = NULL;
	lv_obj_t *previous_video_btn = NULL;
	lv_obj_t *play_pause_btn = NULL;
	lv_obj_t *next_video_btn = NULL;
	FfplayCallback cback_info;
	static volatile int video_end;
	int width = 0;
	int height = 0;
	int posx = 0;
	int posy = 0;

	main_page = lv_obj_create(parent);
	lv_obj_set_size(main_page, LV_HOR_RES, LV_VER_RES);
	lv_obj_set_style_bg_color(main_page, lv_color_black(), LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(main_page, LV_OPA_100, LV_STATE_DEFAULT);
	lv_obj_set_style_radius(main_page, 0, LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(main_page, 0, LV_STATE_DEFAULT);
	lv_obj_align_to(main_page, parent, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t * time_lable = lv_label_create(main_page);
	lv_label_set_text(time_lable, "00:00:00");
	lv_label_set_long_mode(time_lable, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_font(time_lable, &lv_font_montserrat_48, 0);
	lv_obj_set_style_text_color(time_lable, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
	lv_obj_align_to(time_lable, main_page, LV_ALIGN_TOP_MID, 0, 25);

	ffplay_register_timestamp_callback(video_timestamp, time_lable);

	close_btn = lv_btn_create(main_page);
	lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(close_btn, LV_OPA_60, LV_STATE_DEFAULT);
	lv_obj_set_style_radius(close_btn, 3, LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(close_btn, 1, LV_STATE_DEFAULT);
	lv_obj_set_size(close_btn, 110, 70);
	lv_obj_align_to(close_btn, main_page, LV_ALIGN_TOP_RIGHT, -15, 15);

	lv_obj_t * label = lv_label_create(close_btn);
	lv_label_set_text(label, "Exit");
	lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
	lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED), LV_STATE_DEFAULT);
	lv_obj_align_to(label, close_btn, LV_ALIGN_CENTER, 0, 0);
	lv_obj_add_event_cb(close_btn, player_close_event_cb1, LV_EVENT_ALL, handle);

	previous_video_btn = lv_imgbtn_create(main_page);
	lv_obj_set_user_data(previous_video_btn, 1);
	lv_obj_set_size(previous_video_btn, 120, 120);
	lv_obj_add_event_cb(previous_video_btn, video_btn_click_event_cb, LV_EVENT_ALL, previous_video_btn);
	lv_imgbtn_set_src(previous_video_btn, LV_IMGBTN_STATE_RELEASED, &previous_video, NULL, NULL);
	lv_obj_align_to(previous_video_btn, main_page, LV_ALIGN_BOTTOM_LEFT, 40, -40);

	play_pause_btn = lv_imgbtn_create(main_page);
	lv_obj_set_user_data(play_pause_btn, 2);
	lv_obj_set_size(play_pause_btn, 120, 120);
	lv_obj_add_event_cb(play_pause_btn, video_btn_click_event_cb, LV_EVENT_ALL, play_pause_btn);
	lv_imgbtn_set_src(play_pause_btn, LV_IMGBTN_STATE_RELEASED, &play_pause, NULL, NULL);
	lv_obj_align_to(play_pause_btn, main_page, LV_ALIGN_BOTTOM_MID, 0, -40);

	next_video_btn = lv_imgbtn_create(main_page);
	lv_obj_set_user_data(next_video_btn, 3);
	lv_obj_set_size(next_video_btn, 120, 120);
	lv_obj_add_event_cb(next_video_btn, video_btn_click_event_cb, LV_EVENT_ALL, next_video_btn);
	lv_imgbtn_set_src(next_video_btn, LV_IMGBTN_STATE_RELEASED, &next_video, NULL, NULL);
	lv_obj_align_to(next_video_btn, main_page, LV_ALIGN_BOTTOM_RIGHT, -40, -40);


	scan_video_path();

	ffplay_register_callback(&cback_info);
	if (video_file_count != 0)
	{
		ffplay_init_detl(0);

  		memset(&cback_info, 0, sizeof(cback_info));
  		cback_info.context = &video_end;
  		cback_info.video_eof = player_video_eof_proc;
  		cback_info.update_pos = player_video_begin_proc;
		start_ffplay_video(current_video_pos);
	}

}
