/**
 * @file
 * simple media player based on the FFmpeg libraries.
 * Support player video mode and  pipe video mode.
 * player_video_mode: support directory play one video, show how call libffplay interface.
 * pipe_video_mode : can as a client connect with main process with FIIO(name pipe).
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ffplay.h"
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static int write_pipe, read_pipe;
static pthread_mutex_t mutex;

static pthread_cond_t cond_ctx;
static pthread_mutex_t mutex_ctx;
static struct msg_context global_gctx;
static int order;
static int video_angle;
static int rspon_cnt;

extern void video_reset_param(int w, int h);

#define flog_v(format, ...) printf("- [ffplayer]:" format, ##__VA_ARGS__)


int player_video_eof_proc(void *context, int is_loop)
{
	printf("once video eof, callback!\n");
	if (!is_loop){
	*(int *)context = 1;
	}
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

int start_player_mode(char *video_path)
{
	FfplayCallback cback_info;
	static volatile int video_end;

  /* use hardware layer1 */

	ffplay_init_detl(0);

  memset(&cback_info, 0, sizeof(cback_info));
  cback_info.context = &video_end;
  cback_info.video_eof = player_video_eof_proc;
  cback_info.update_pos = player_video_begin_proc;

  ffplay_register_callback(&cback_info);
  int n = ffplay_set_play_file(video_path);

  if (n >= 0) {
    ffplay_set_play_loop(1);
    ffplay_set_audio_volume(12);
    ffplay_play_state_pause(0);
  } else {
    printf("player video fail\n");
    return -1;
  }

  while(!video_end) ;

  ffplay_play_state_stop();
  return 0;
}


int main(int argc, char **argv)
{
  char opt;
  char *wname= NULL, *rname = NULL, *videopath = NULL;
  /* flog_v("argc %d %s %s %s\n", argc, argv[0], argv[1],argv[2]); */

  while ((opt = getopt(argc, argv, "f:x:y:s")) != -1)
  {
    switch (opt)
    {
    case 'f':
      videopath = optarg;
      break;
    case 'x':
      trg_x = atoi(optarg);
      break;
    case 'y':
      trg_y = atoi(optarg);
      break;
    case 's':
      enable_scale = 1;
      break;
    default:
      flog_v("unknown option %c\n",(char)opt);
      return 0;

    }
  }
  if (videopath) {
    flog_v("start player mode \n", videopath);
    start_player_mode(videopath);
  } else {
    printf("usage: \n");
    printf("%-25s %-40s\n", "-f <filename>", "direct play video");
    printf("%-25s %-40s\n", "-x <target x>", "display target pos x, default 0");
    printf("%-25s %-40s\n", "-y <target y>", "display target pos y, default 0");
    printf("%-25s %-40s\n", "-s", "enable scale, default disabled");
  }

  return 0;
}
