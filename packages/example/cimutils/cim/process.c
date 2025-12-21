/*
 * Process frame buffer
 */
#include "headers.h"

void process_frame(struct camera_info *camera_inf, int ops)
{
	FILE *file;

	int width = camera_inf->param.width;
	int height = camera_inf->param.height;
	int format = camera_inf->fmt.fourcc;
	void *frame = camera_inf->buf_vaddr;
	void *handle = camera_inf->jz_jpeg;
	char *filename = camera_inf->file_name;

	__u8 *rgb_frame = NULL;

	file = fopen(filename, "w");
	if (!file) {
		fprintf(stderr, "--%s() L%d: open %s failed!!!\n", __func__, __LINE__, filename);
		return;
	}

	switch (ops) {
	case OPS_SAVE_BMP:
		//fixme
		rgb_frame = (__u8 *)malloc(width * height * 3);	//rgb888 bpp=24
		if (rgb_frame == NULL) {
			fprintf(stderr, "-->%s() L%d: malloc failed!\n", __func__, __LINE__);
			return;
		}
		switch (camera_inf->fmt.fourcc) {
		case V4L2_PIX_FMT_YUYV:
			convert_yuv_to_rgb24(frame, rgb_frame, camera_inf);
			break;
		case V4L2_PIX_FMT_RGB32:
			convert_RGB32_to_rgb24(frame, rgb_frame, camera_inf);
			break;
		case V4L2_PIX_FMT_RGB565:
			convert_RGB565_to_rgb24(frame, rgb_frame, camera_inf);
			break;
		case V4L2_PIX_FMT_GREY:
			convert_grey_to_rgb24(frame, rgb_frame, camera_inf);
			break;
		default:
			printf("%s %d not support format!\n",__func__,__LINE__);
			return;
		}

		save_bgr_to_bmp(rgb_frame, camera_inf, file);

		free(rgb_frame);

		break;
	case OPS_SAVE_JPG:
		//fixme
		if (DF_IS_YCbCr444(format)){
			printf("now not support yuv444!!!!\n");
			break;
		}
		else if (DF_IS_YCbCr422(format))
		{
			printf("==>%s() L%d: yuv422!\n", __func__, __LINE__);
			yuv422_packed_to_jpeg(handle, frame, file, camera_inf, 0);
		}
		else if (DF_IS_NV21_OR_NV12(format)) {
			printf("==>%s() L%d: nv12 or nv21!\n", __func__, __LINE__);
			nv12_or_nv21_packed_to_jpeg(file, camera_inf);
		}
		else
			printf("==>%s() L%d: error format!\n", __func__, __LINE__);
		break;
	case OPS_SAVE_RAW:
		//fixme
		preview_save_raw_file(file, frame, camera_inf);
		break;
	default:
		fprintf(stderr, "-->%s() L%d: unsupported operation!\n", __func__, __LINE__);
		break;
	};

	fclose(file);
}


