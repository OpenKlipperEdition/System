#ifndef __ISP_TUNING_H__
#define __ISP_TUNING_H__

#include "isp_tuning_api.h"

enum options {
	opt_width = 'w',
	opt_height = 'h',
	opt_video_id = 'i',
	opt_count = 'n',
	opt_hflip = 'H',
	opt_vflip = 'V',
	opt_sharpness = 's',
	opt_contrast = 'c',
	opt_saturation = 'S',
	opt_brightness = 'b',
	opt_exposure = 'e',
	opt_gain = 'g',
	opt_antiflicker = 'f',
	opt_module = 'm',
	opt_day_or_night = 'd',
	opt_luma = 'l',
	opt_red = 'R',
	opt_blue = 'B',
	opt_hilightdepress = 'D',
	opt_preset_wb = 128,
	opt_roi_ae = 256,
	opt_auto_wb,
	opt_help,
	opt_last,
};


struct isp_tuning_para_t{
        int width;
        int height;
        int video_id;
        int sharpness;
        int contrast;
        int saturation;
        int brightness;
        int hflip;
        int vflip;
        int frequency;
        int modules;
        int dn;
        int exp;
        int gain;
        int auto_wb;
        int preset_wb;
        int red_value;
        int blue_value;
        int hilightdepress;
	tisp_area_t roi_ae_attr;
};

#endif //__ISP_TUNING_H__
