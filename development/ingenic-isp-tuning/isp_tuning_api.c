#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#include "isp_tuning_api.h"


int isp_tuning_set_hflip(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_HFLIP;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_hflip(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_HFLIP;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_vflip(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_VFLIP;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_vflip(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_VFLIP;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_sharpness(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_SHARPNESS;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_sharpness(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_SHARPNESS;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_contrast(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_CONTRAST;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_contrast(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_CONTRAST;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_saturation(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_SATURATION;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_saturation(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_SATURATION;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_brightness(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_BRIGHTNESS;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_brightness(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_BRIGHTNESS;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_frequency(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
	if(value == 1)
		ctrl.value = V4L2_CID_POWER_LINE_FREQUENCY_50HZ;
	else if(value == 2)
		ctrl.value = V4L2_CID_POWER_LINE_FREQUENCY_60HZ;
	else
		ctrl.value = V4L2_CID_POWER_LINE_FREQUENCY_DISABLED;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_frequency(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_modules(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_MODULE_CONTROL;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_modules(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_MODULE_CONTROL;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_dn(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_DAY_OR_NIGHT;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_dn(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_DAY_OR_NIGHT;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_get_luma(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_AE_LUMA;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_wb_red(int fd, int value)
{
	struct v4l2_control ctrl = {0};
	ctrl.id = V4L2_CID_RED_BALANCE;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		perror("Failed to set wb red_value.\n");
		return -1;
	}

	memset(&ctrl, 0 ,sizeof(struct v4l2_control));
	ctrl.id = V4L2_CID_DO_WHITE_BALANCE;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}

	return 0;
}

int isp_tuning_get_wb_red(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_RED_BALANCE;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_wb_blue(int fd, int value)
{
	struct v4l2_control ctrl = {0};
	ctrl.id = V4L2_CID_BLUE_BALANCE;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		perror("Failed to set wb blue_value.\n");
		return -1;
	}

	memset(&ctrl, 0 ,sizeof(struct v4l2_control));
	ctrl.id = V4L2_CID_DO_WHITE_BALANCE;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}

	return 0;
}

int isp_tuning_get_wb_blue(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_BLUE_BALANCE;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_preset_wb(int fd, int value, int red_value, int blue_value)
{
	struct v4l2_control ctrl;
	int ret = 0;

	/*set white balance*/
	ctrl.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE;
	switch(value) {
	case 0:
		ctrl.value = V4L2_WHITE_BALANCE_MANUAL;
		break;
	case 1:
		ctrl.value = V4L2_WHITE_BALANCE_AUTO;
		break;
	case 2:
		ctrl.value = V4L2_WHITE_BALANCE_INCANDESCENT;
		break;
	case 3:
		ctrl.value = V4L2_WHITE_BALANCE_FLUORESCENT;
		break;
	case 4:
		ctrl.value = V4L2_WHITE_BALANCE_FLUORESCENT_H;
		break;
	case 5:
		ctrl.value = V4L2_WHITE_BALANCE_HORIZON;
		break;
	case 6:
		ctrl.value = V4L2_WHITE_BALANCE_DAYLIGHT;
		break;
	case 8:
		ctrl.value = V4L2_WHITE_BALANCE_CLOUDY;
		break;
	case 9:
		ctrl.value = V4L2_WHITE_BALANCE_SHADE;
		break;
	case 10:
		ctrl.value = 10;
		break;
	default:
		perror("Enter wb num error, the available range of awb is 0-10 and 7 is invalid \n");
		break;
	}
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}

	/*set red blue value manually int this two cases*/
	if(ctrl.value == V4L2_WHITE_BALANCE_MANUAL || ctrl.value == 10) {
		ret = isp_tuning_set_wb_red(fd, red_value);
		if(ret)
			return ret;

		ret = isp_tuning_set_wb_blue(fd, blue_value);
		if(ret)
			return ret;
	}

	return 0;
}

int isp_tuning_get_preset_wb(int fd)
{
	struct v4l2_control ctrl;
	/*get white balance*/
	ctrl.id = V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_auto_wb(int fd, int value, int red_value, int blue_value)
{
	struct v4l2_control ctrl;
	int ret = 0;

	/*set wb*/
	ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}

	/*set red blue value manually int this case*/
	if(ctrl.value == 0) {
		ret = isp_tuning_set_wb_red(fd, red_value);
		if(ret)
			return ret;

		ret = isp_tuning_set_wb_blue(fd, blue_value);
		if(ret)
			return ret;
	}

	return 0;
}

int isp_tuning_get_auto_wb(int fd)
{
	struct v4l2_control ctrl;
	/*get wb*/
	ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_exp(int fd, int value)
{
	struct v4l2_control ctrl;
	/*auto exposure.*/
	ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	if(value) {
		ctrl.value = V4L2_EXPOSURE_MANUAL;
	} else {
		ctrl.value = V4L2_EXPOSURE_AUTO;
	}
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}
	printf("exposure mode : %s\n", ctrl.value == V4L2_EXPOSURE_AUTO ? "auto":(ctrl.value == V4L2_EXPOSURE_MANUAL?"manual":"unknown"));

	/*set manual exposure*/
	ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}

	return 0;
}

int isp_tuning_get_exp(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_gain(int fd, int value)
{
	struct v4l2_control ctrl;
	/*auto again.*/
	ctrl.id = V4L2_CID_AUTOGAIN;
	if(value) {
		ctrl.value = 1;
	} else {
		ctrl.value = 0;
	}
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		return -1;
	}
	printf("again mode : %s\n", ctrl.value == 0?"auto":(ctrl.value == 1)?"manual":"unknown");

	/*set manual gain*/
	ctrl.id = V4L2_CID_GAIN;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)) {
		return -1;
	}

	return 0;
}

int isp_tuning_get_gain(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = V4L2_CID_GAIN;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_hilightdepress(int fd, int value)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_HILIGHTDEPRESS ;
	ctrl.value = value;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

int isp_tuning_get_hilightdepress(int fd)
{
	struct v4l2_control ctrl;
	ctrl.id = IMAGE_TUNING_CID_HILIGHTDEPRESS ;
	if(-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)){
		return -1;
	}

	return ctrl.value;
}

int isp_tuning_set_roi_ae(int fd, int enable, int top,
		int bottom, int left, int right)
{
	struct v4l2_control ctrl;
	tisp_area_t attr;
	attr.enable = enable;
	attr.top = top;
	attr.bottom = bottom;
	attr.left = left;
	attr.right = right;
	ctrl.id = IMAGE_TUNING_CID_ROI_AE;
	ctrl.value = (int)&attr;
	if(-1 == ioctl(fd, VIDIOC_S_CTRL, &ctrl)){
		return -1;
	}

	return 0;
}

