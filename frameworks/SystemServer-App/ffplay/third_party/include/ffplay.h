#ifndef __FFPLAY_H__
#define __FFPLAY_H__

#include <stdint.h>
#include <stdbool.h>

#define FFPLAY_SHOW_PTS
#define FFPLAY_FB_BUFFER_COUNT 4// 解码器输出使用的buffer最大数量，需要大于等于内核的配置，最小为2
#define FFPLAY_NOEND          ((int64_t)UINT64_C(0x8000000000000000))

typedef struct ImppRect_{
	int x;
	int y;
	int w;
	int h;
}ImppRect;

/** ffplayer video callback struct */
typedef void (*update_pos_fun_t) (void*context, ImppRect *vd_rect,
		     ImppRect *trg_rect, ImppRect *crop_rect, int *scaler,
		     int64_t duration_us);
typedef int (*video_eof_fun_t)(void*context, int is_loop);

typedef struct FfplayCallback_{
  /**
   * @brief 在视屏解析完成回调

   * @param context  :  定义的回调参数， 根据实际情况可自定义;
   * @param vd_rect  ： 视屏源实际尺寸;
   * @param trg_rect ： 目标显示尺寸和位置,默认尺寸为屏幕尺寸,按照需求可更改.
   * @param crop_rect： 基于视频源上 crop 位置和尺寸, 默认为 (0，0, vd_rect.w, vd_rect.h);
   * @param scaler   ： 是否允许 SLCD 做 scaler，1: enable, 0: disable. 如果 enable，视频源尺寸 crop_rect 会 scaler 到 trg_rect
   * @param duration_us: 视屏时长
   * @return   : null
   */
  void (*update_pos)(void*context, ImppRect *vd_rect,
		     ImppRect *trg_rect, ImppRect *crop_rect, int *scaler,
		     int64_t duration_us);
  /**
   * @brief 在视屏播放完后回调

   * @param is_loop  : 当前视屏 is_loop 状态, 即是否是循环播出状态.
   * @return : 预留接口, 当前返回值没有实际作用
   */
  int (*video_eof)(void*context, int is_loop);
  /**
   * @brief 在同步播放时期望启动播放的时间戳，即gettimeofday的sec值
   */
  unsigned int sync_start_time_sec;
  int reserved[7];

  void *context;
  int is_pipe;
}FfplayCallback;



typedef union {
	struct {
		/**
		 *  - low bit
		 *   8: order
		 *   4: display model
		 *   4: reserved1;
		 *   8: fbidx;
		 *   8: reserved2;
		 *   - hight bit
		 */
		unsigned int order : 8;
		unsigned int displaymode : 4;
		unsigned int reserved1 : 4;
		unsigned int fbidx : 8;
		unsigned int reserved2 : 8;
	} param;
	int32_t value;
} DisplayParam;

// ffplay获取显示帧率
int ffplay_get_fps(void);

// ffplay初始化，必须首先调用。
int ffplay_init(void);
int ffplay_init_detl(int param);

// ffplay反初始化，退出前调用。
int ffplay_deinit(void);

int ffplay_get_video_width_and_height(int *width, int *height);

typedef int (*timestamp_callback_t)(void *args, double timestamp);
int ffplay_register_timestamp_callback(timestamp_callback_t func, void *args);

// 设置播放的屏幕位置。
int ffplay_set_video_pos(int x, int y, int w, int h);

// 设置为全屏模式。
int ffplay_set_video_fullsize(void);

// 设置音量
int ffplay_set_audio_volume(int volume); // 0 ~ 99

// 视频播放完毕后，自动将视频层设置为黑色。
int ffplay_set_autoclean(bool st);

// 设置循环播放。
int ffplay_set_play_loop(bool st);

// 关闭当前视频流。
int ffplay_play_state_stop(void);

// 设置待播放文件，不会自动播放。
int ffplay_set_play_file(char *file);

// 设置循环播放区间，单位毫秒，当time_end为FFPLAY_NOEND时，播放到文件尾。
int ffplay_set_play_time(int64_t time_start, int64_t time_end);

// 设置播放位置，单位毫秒。
int ffplay_set_play_seek(int64_t ms);
/* int set_play_seek(int64_t frame); */

// 设置播放状态，true为暂停，false为播放。
int ffplay_play_state_pause(bool pause);

// 设置UI层和VIDEO层的顺序，1为UI在上，0为VIDEO在上。
int ffplay_set_layer_order(int UI_up_VIDEO_down);

// 隐藏视频层。
int ffplay_hide_video_layer(void);

// 显示视频层。
int ffplay_show_video_layer(void);

// 回调函数等待资源准备完毕后通知UI更新
typedef void (*ffplay_callback)(void);
void ffplay_async_callback(ffplay_callback callback);

//设置视频旋转角度
int ffplay_set_video_rotate(int rot); // 0, 90, 270, 180

// 不支持接口
int ffplay_set_video_alpha(int alpha); // 0 ~ 255
int ffplay_set_single_frame(int frame_n);
void ffplay_register_callback(void*);

/* int get_playtime(void); */
/* int wait_playend(void); */

enum {
  FFPLAY_INIT = 0,
  FFPLAY_DEINIT,
  SET_PLAY_FILE,
  SET_LAYER_ORDER,
  SET_PLAY_LOOP,
  SET_AUDIO_VOLUME,
  PLAY_STATE_PAUSE,
  PLAY_STATE_STOP,
  REGISTER_CALLBACK,
  CALLBACK_UPDATE_POS,
  CALLBACK_VIDEO_EOF,
  ALL_CMD,
};

static __attribute__ ((unused))char const *get_cmd_type[] = {
  "ffplay_init",
  "ffplay_deinit",
  "ffplay_set_play_file",
  "ffplay_set_layer_order",
  "ffplay_set_play_loop",
  "ffplay_set_audio_volume",
  "ffplay_play_state_pause",
  "ffplay_play_state_stop",
  "ffplay_register_callback",
  "callback_update_pos",
  "callback_video_eof",
  "ffplay_set_video_rotate",
};

struct callbackParam {
  FfplayCallback callback;
  ImppRect vd_rect;
  ImppRect trg_rect;
  ImppRect crop_rect;
  int scaler;
  int64_t duration_us;
  int is_loop;
};

struct msg_context {
  char type[32];
  int need_return;
  int retval;
  union {
    int param;
    struct callbackParam cbparam;
    char filename[128];
  } ;
};

enum {
  UNWAIT_RETURN = 0,
  WAIT_RETURN = 1
};

__attribute__((weak))int callback_process(int type, struct callbackParam *cb_param, int wait_return);
#endif /* __FFPLAY_H__ */
