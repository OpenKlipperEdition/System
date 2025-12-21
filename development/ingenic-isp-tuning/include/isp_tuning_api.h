#ifndef __ISP_TUNING_API_H__
#define __ISP_TUNING_API_H__

#include <linux/videodev2.h>

typedef struct {
        unsigned int enable;
        unsigned int left;
        unsigned int top;
        unsigned int right;
        unsigned int bottom;
        unsigned int target;
} tisp_area_t;

enum isp_image_tuning_private_cmd_id {
	IMAGE_TUNING_CID_MODULE_CONTROL = V4L2_CID_PRIVATE_BASE,
	IMAGE_TUNING_CID_DAY_OR_NIGHT,
	IMAGE_TUNING_CID_AE_LUMA,
	IMAGE_TUNING_CID_HILIGHTDEPRESS,
	IMAGE_TUNING_CID_ROI_AE,
};


int isp_tuning_set_hflip(int fd, int value);
int isp_tuning_get_hflip(int fd);
int isp_tuning_set_vflip(int fd, int value);
int isp_tuning_get_vflip(int fd);
int isp_tuning_set_sharpness(int fd, int value);
int isp_tuning_get_sharpness(int fd);
int isp_tuning_set_contrast(int fd, int value);
int isp_tuning_get_contrast(int fd);
int isp_tuning_set_saturation(int fd, int value);
int isp_tuning_get_saturation(int fd);
int isp_tuning_set_brightness(int fd, int value);
int isp_tuning_get_brightness(int fd);
int isp_tuning_set_frequency(int fd, int value);
int isp_tuning_get_frequency(int fd);
int isp_tuning_set_modules(int fd, int value);
int isp_tuning_get_modules(int fd);
int isp_tuning_set_dn(int fd, int value);
int isp_tuning_get_dn(int fd);
int isp_tuning_get_luma(int fd);
int isp_tuning_set_wb_red(int fd, int value);
int isp_tuning_get_wb_red(int fd);
int isp_tuning_set_wb_blue(int fd, int value);
int isp_tuning_get_wb_blue(int fd);
int isp_tuning_set_preset_wb(int fd, int value, int red_value, int blue_value);
int isp_tuning_get_preset_wb(int fd);
int isp_tuning_set_auto_wb(int fd, int value, int red_value, int blue_value);
int isp_tuning_get_auto_wb(int fd);
int isp_tuning_set_exp(int fd, int value);
int isp_tuning_get_exp(int fd);
int isp_tuning_set_gain(int fd, int value);
int isp_tuning_get_gain(int fd);
int isp_tuning_set_hilightdepress(int fd, int value);
int isp_tuning_get_hilightdepress(int fd);
int isp_tuning_set_roi_ae(int fd, int enable, int top, int bottom, int left, int right);
#endif
