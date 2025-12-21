#include "headers.h"

/*
 * calc_frame_size
 *
 * @width:
 * @height:
 * @fmt:	fourcc code
 *
 */
static int calc_frame_size(__u32 width, __u32 height, int fmt)
{
	int y_len = width * height;
	int cb_len = 0;
	int cr_len = 0;

	printf("====>%s:%d img_width = %d img_height = %d fmt = %#0x\n",
	       __FUNCTION__, __LINE__, width, height, fmt);

	if (DF_IS_YCbCr420(fmt)) {
		printf("420..................\n");
		cb_len = y_len / 4;
		cr_len = y_len / 4;

	} else if (DF_IS_YCbCr422(fmt)) {
		printf("422..................\n");
		cb_len = y_len / 2;
		cr_len = y_len / 2;
	} else if (DF_IS_GREY(fmt)) {
		printf("grey..................\n");
		cb_len = 0;
		cr_len = 0;
	} else if (DF_IS_RGB565(fmt)) {
		printf("rgb5565..................\n");
		cb_len = y_len / 2;
		cr_len = y_len / 2;
	} else {
		printf("444....................\n");
		cb_len = y_len;
		cr_len = y_len;
	}

	return y_len + cb_len + cr_len;
}

int preview_save_raw_file(FILE *file, const __u8 *frame, struct camera_info *camera_inf)
{
	int total_len = calc_frame_size(camera_inf->param.width, camera_inf->param.height, camera_inf->fmt.fourcc);
	char padding_code[HEADER_SIZE];	// 0x55
	int padding_size = HEADER_SIZE - sizeof(struct camera_info);
	int ret = 0;

	ret = fwrite(frame, sizeof(__u8),  total_len, file);
	if (ret != total_len) {
		fprintf(stderr, "%s() L%d: write error!!!\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

