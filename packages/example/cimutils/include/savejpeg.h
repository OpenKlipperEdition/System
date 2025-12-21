#ifndef __SAVEJPEG_H__
#define __SAVEJPEG_H__

#include "headers.h"

struct camera_info;

extern int yuv422_sep_to_jpeg(__u8 *data, int image_width, int image_height, FILE *fp, int quality);
extern int yuv422_packed_to_jpeg(void *handle, const __u8 *frm, FILE *fp, struct camera_info *camera_inf, int quality);

extern int yuv420_sep_to_jpeg(__u8 *data, int mb, int image_width, int image_height, FILE *fp, int quality);

extern int yuv444_sep_to_jpeg(__u8 *data, int image_width, int image_height, FILE *fp, int quality);
extern int yuv444_packed_to_jpeg(__u8 *data, FILE *fp, struct camera_info *camera_inf, int quality);

extern int itu656_rearrange(__u8 *src, __u8 *dest, int fmt, int width, int height);

extern int nv12_or_nv21_packed_to_jpeg(FILE *fp, struct camera_info *camera_inf);
#endif /* __SAVEJPEG_H__ */
