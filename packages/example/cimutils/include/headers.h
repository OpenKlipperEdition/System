#ifndef __CIM_HEADER_H__
#define __CIM_HEADER_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <malloc.h>
#include <linux/i2c.h>
#include "framebuffer.h"
#include "format.h"
#include "pixel.h"
#include "savebmp.h"
#include "savejpeg.h"
#include "jz_cim.h"	//same as kernel/include/media/jz_cim.h !!!
#include "camera.h"
#include "v4l2-jpegenc.h"

typedef unsigned int	__u32;
typedef signed int	__s32;
typedef unsigned short	__u16;
typedef signed short	__s16;
typedef unsigned char	__u8;
typedef signed char	__s8;

#undef cim_dbg
#define cim_dbg(x...)				\
	({					\
	 fprintf(stdout, x);	\
	})
/* #define Debug */
#ifdef Debug
#define _DEBUG  1
#else
#define _DEBUG  0
#endif
#define debug_cond(cond, fmt, args...)      \
	do {                    \
		if (cond)           \
		printf(fmt, ##args);    \
	} while (0)

#define debug(fmt, args...)         \
	debug_cond(_DEBUG, fmt, ##args)

// function declaration

/* process */
extern void process_frame(struct camera_info *camera_inf, int ops);

/* preview_display */
extern void preview_display(struct camera_info *camera_inf, struct fb_info *fb_info);

extern void init_framebuffer(struct fb_info *fb_info);
extern void finish_framebuffer(struct fb_info *fb_info);

extern void fb_set_a_pixel(struct fb_info *fb_info, int line, int col,
		    __u8 red, __u8 green, __u8 blue,
		    __u8 alpha);

extern int display_direct_to_fb(__u8 *frame, struct camera_info *camera_inf, struct fb_info *fb);
extern int display_to_fb(struct camera_info *camera_inf, struct fb_info *fb);

/* savebmp */
extern int convert_yuv_to_rgb24(__u8 *frame, __u8 *rgb, struct camera_info *camera_inf);
extern int convert_RGB565_to_rgb24(__u8 *frame, __u8 *rgb, struct camera_info *camera_inf);
extern int convert_grey_to_rgb24 (__u8 *frame, __u8 *rgb, struct camera_info *camera_inf);
extern int convert_RGB32_to_rgb24(__u8 *frame, __u8 *rgb, struct camera_info *camera_inf);

extern int save_bgr_to_bmp(__u8 *buf, struct camera_info *camera_inf, FILE *fp);

/* saveraw */
#define HEADER_SIZE	128

extern int preview_save_raw_file(FILE *file, const __u8 *frame, struct camera_info *camera_inf);

extern int preview_save_raw(char *filename, const __u8 *frame, struct camera_info *camera_inf);

extern int preview_display_raw(const char *filename, struct fb_info *fb);

extern int convert_packed_2_planar(char *filename);

/* convert */
extern void yCbCr422_normalization(__u32 fmt, struct yuv422_sample *yuv422_samp_in,
				   struct yuv422_sample *yuv422_samp_out);

extern void yCbCr444_normalization(__u32 fmt, struct yuv444_sample *yuv444_samp_in,
				 struct yuv444_sample *yuv444_samp_out);

extern void rgb888_normalization(__u32 fmt, struct rgb888_sample *in_samp,
				 struct rgb888_sample *out_samp);

extern void rgb565_normalization(__u32 fmt, struct rgb565_sample *in_samp,
				 struct rgb565_sample *out_samp);

extern void yCbCr422_pack_to_planar(__u8 *y, __u8 *cb, __u8 *cr, const __u8 *src, struct camera_info *camera_inf);

extern void yCbCr444_pack_to_planar(__u8 *y, __u8 *cb, __u8 *cr,
				  int image_width, int image_height,
				  __u8 *src, __u32 fmt);
/* video */
extern int open_device(struct camera_info *camera_inf);
extern int open_device_video_id(struct camera_info *camera_inf);

extern void init_device(struct camera_info *camera_inf, struct camera_ctl *camera_ctl);
extern void uninit_device(struct camera_ctl *camera_ctl, struct camera_info *camera_inf);

extern int process_framebuf(struct camera_info *camera_inf, struct camera_ctl *camera_ctl);

extern void start_capturing(struct camera_ctl *camera_ctl);
extern void stop_capturing(struct camera_ctl *camera_ctl);
extern void frame_handler_init(void *handle);

#endif // __CIM_HEADER_H__
