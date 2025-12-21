#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "impp.h"
#include "display.h"


static void print_help(void)
{
        printf("Usage: sample-lcd-pic-example [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -i                 Input image filename.\n"
               " -f                 Image color encoding format string.\n"
               " -w                 width of image.\n"
               " -h                 height of image.\n"
               " -H                 help.\n\n"
               "Camera presets:\n");
}

static int get_fmt_from_fmt_str(const char * const fmt_str)
{
	if (0 == strcasecmp(fmt_str, "BGR888"))
		return IMPP_PIX_FMT_BGR_888;
	else if (0 == strcasecmp(fmt_str, "BGRA8888"))
		return IMPP_PIX_FMT_BGRA_8888;
	else if (0 == strcasecmp(fmt_str, "RGB555")) // rgb555le
		return IMPP_PIX_FMT_RGB_555;
	else if (0 == strcasecmp(fmt_str, "RGB565")) // rgb565le
		return IMPP_PIX_FMT_RGB_565;
	else if (0 == strcasecmp(fmt_str, "YUYV"))
		return IMPP_PIX_FMT_YUYV;
	else if (0 == strcasecmp(fmt_str, "NV12"))
		return IMPP_PIX_FMT_NV12;
	else if (0 == strcasecmp(fmt_str, "NV21"))
		return IMPP_PIX_FMT_NV21;

	return -1;
}

int main(int argc, char **argv)
{
	const char *image_filename = NULL;
	int image_fd = -1;
	IMPP_PIX_FMT image_fmt = -1;
	IHAL_INT32 image_width = 0;
	IHAL_INT32 image_height = 0;
	int opt = 0;
	IHal_SampleFB_Handle_t *fb0_handle;

	while (1) {
		opt = getopt(argc, argv, "i:f:w:h:H");

		if (opt == -1)
			break;

		switch (opt) {
			case 'i':
				image_filename = optarg;
				break;
			case 'f':
				image_fmt = get_fmt_from_fmt_str(optarg);
				if (-1 == image_fmt) {
					printf("The input image color encoding format is not supported.\n");
					return -1;
				}
                break;
			case 'w':
				image_width = atoi(optarg);
				break;
			case 'h':
				image_height = atoi(optarg);
				break;
            case 'H':
				print_help();
				return 0;
		}
	}

    if (NULL == image_filename || -1 == image_fmt || \
			0 == image_width || 0 == image_height) {
		print_help();
		return -1;
	}

	image_fd = open(image_filename, O_RDONLY);
	if (-1 == image_fd) {
		perror("open");
		return -1;
	}

	IHal_SampleFB_Attr fb_attr;
	memset(&fb_attr, 0, sizeof(IHal_SampleFB_Attr));
	sprintf(&fb_attr.node[0], "%s", "/dev/fb0");
	fb_attr.mode = Composer_Mode;
	fb_attr.frame_width = image_width;
	fb_attr.frame_height = image_height;
	fb_attr.crop_x = 0;
	fb_attr.crop_y = 0;
	fb_attr.crop_w = 0;
	fb_attr.crop_h = 0;
	fb_attr.alpha = 255;
	fb_attr.frame_fmt    = image_fmt;
	fb0_handle = IHal_SampleFB_Init(&fb_attr);
	if (!fb0_handle) {
		printf("init error");
		close(image_fd);
		return -1;
	}

	IMPP_BufferInfo_t fb0_bufinfo;
	memset(&fb0_bufinfo, 0, sizeof(IMPP_BufferInfo_t));
	IHal_SampleFB_GetMem(fb0_handle, &fb0_bufinfo);
	read(image_fd, fb0_bufinfo.vaddr, image_width * image_height * 4);
	IHal_SampleFB_Update(fb0_handle, &fb0_bufinfo);

	close(image_fd);
	// IHal_SampleFB_DeInit(fb0_handle);

	return 0;
}
