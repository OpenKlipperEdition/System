#include "headers.h"
union ktime {
	unsigned long long tv64;
	struct {
		unsigned int nsec, sec;
	} tv;
};
typedef union ktime ktime_t;


extern int UYVY_to_RGB888 (void *src_buf, void *dst_buf, size_t src_height ,size_t src_width, size_t dstStride);
extern int YUYV_to_RGB565 (void *src_buf, void *dst_buf, size_t src_height ,size_t src_width, size_t dstStride);
extern int YUYV_to_RGB888 (void *src_buf, void *dst_buf, size_t src_height ,size_t src_width, size_t dstStride);
extern int GREY_to_RGB888 (void *src_buf, void *dst_buf, size_t src_height ,size_t src_width, size_t dstStride);

void init_framebuffer(struct fb_info *fb_info)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	// Open the file for reading and writing
	fb_info->fd = open(fb_info->dev_name, O_RDWR);
	if (!fb_info->fd) {
		fprintf(stderr, "Error: cannot open framebuffer device.\n");
		exit(1);
	}

	// Get fixed screen information
	if (ioctl(fb_info->fd, FBIOGET_FSCREENINFO, &finfo)) {
		fprintf(stderr, "Error reading fixed information.\n");
		exit(2);
	}

	// Get variable screen information
	if (ioctl(fb_info->fd, FBIOGET_VSCREENINFO, &vinfo)) {
		fprintf(stderr, "Error reading variable information.\n");
		exit(3);
	}

	// Put variable screen information
	vinfo.activate |= FB_ACTIVATE_FORCE | FB_ACTIVATE_NOW;

	// Figure out the size of the screen in bytes
	fb_info->xres = vinfo.xres;
	fb_info->yres = vinfo.yres;
	fb_info->bpp = vinfo.bits_per_pixel;
	fb_info->smem_len = finfo.smem_len;
	fb_info->num_buf = vinfo.yres_virtual / vinfo.yres;
	fb_info->index=0;

	fb_info->size_buff=finfo.line_length * vinfo.yres; //一个缓冲区的大小
	debug("lcd fb info: xres=%d, yres=%d bpp=%d, smem_len=%d\n",
		vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, finfo.smem_len);

	// Map the device to memory
	fb_info->fb_mem = (__u8 *)mmap(0,
				       finfo.smem_len,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED,
				       fb_info->fd,
				       0);
	debug("fbp = %p\n", fb_info->fb_mem);
	if (!fb_info->fb_mem) {
		fprintf(stderr, "Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	debug("Open %s successfully.\n", fb_info->dev_name);
}

void finish_framebuffer(struct fb_info *fb)
{
	if (fb->fb_mem) {
		munmap(fb->fb_mem, fb->smem_len);
		fb->fb_mem = NULL;
	}

	if (fb->fd > 0) {
		close(fb->fd);
		fb->fd = -1;
	}
}

void window_cut(struct camera_info *camera_inf, struct fb_info *fb)
{
	//horizontal
	if (camera_inf->param.width > fb->xres)
		camera_inf->xoff = (camera_inf->param.width - fb->xres) / 2;
	else if (camera_inf->param.width < fb->xres)
		fb->xoff = (fb->xres - camera_inf->param.width) / 2;
	else
		camera_inf->xoff = fb->xoff = 0;

	//vertical
	if (camera_inf->param.height > fb->yres)
		camera_inf->yoff = (camera_inf->param.height - fb->yres) / 2;
	else if (camera_inf->param.height < fb->yres)
		fb->yoff = (fb->yres - camera_inf->param.height) / 2;
	else
		camera_inf->yoff = fb->yoff = 0;

}

void fb_set_a_pixel(struct fb_info *fb_info, int line, int col,
		    __u8 red, __u8 green, __u8 blue,
		    __u8 alpha)
{
	switch (fb_info->bpp) {
	case 16:
		{
			struct bpp16_pixel *pixel = (struct bpp16_pixel *)fb_info->fb_mem;
			pixel[line * fb_info->xres + col].red = (red >> 3);
			pixel[line * fb_info->xres + col].green = (green >> 2);
			pixel[line * fb_info->xres + col].blue = (blue >> 3);
		}
		break;
	case 24:
		{
			struct bpp24_pixel *pixel = (struct bpp24_pixel *)fb_info->fb_mem;
			pixel[line * fb_info->xres + col].red = red;
			pixel[line * fb_info->xres + col].green = green;
			pixel[line * fb_info->xres + col].blue = blue;
		}
		break;

	case 32:
		{
			struct bpp32_pixel *pixel = (struct bpp32_pixel *)fb_info->fb_mem;
			pixel[line * fb_info->xres + col].red = red;
			pixel[line * fb_info->xres + col].green = green;
			pixel[line * fb_info->xres + col].blue = blue;
			pixel[line * fb_info->xres + col].alpha = alpha;
		}
		break;
	default:
		break;
	}
}

int display_to_fb(struct camera_info *camera_inf, struct fb_info *fb)
{
	struct fb_var_screeninfo vinfo;
	// 获取可变屏幕信息
	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
		perror("无法获取可变屏幕信息");
		close(fb->fd);
		return -1;
	}
	int src_fourcc = camera_inf->fmt.fourcc;
	int src_height;
	int src_width;
	int dst_fourcc = fb->bpp;
	int num_buf;
	ktime_t timestamp;
	static int flag = 0;
	int ret = 0;
	int buf_nu = 0;
	buf_nu = fb->index % fb->num_buf; //使用第几个缓冲区
	lseek(fb->fd, 0, SEEK_SET);
	__u8 *frame = (__u8 *)camera_inf->buf_vaddr;


	if(camera_inf->do_crop) {
		src_width = camera_inf->crop_w;
		src_height = camera_inf->crop_h;
	} else {
		src_width = camera_inf->param.width;
		src_height = camera_inf->param.height;
	}

	if(src_height > fb->yres)
		src_height = fb->yres;
	memcpy(fb->fb_mem + fb->size_buff * buf_nu, frame, src_width * src_height * 3/2);
	vinfo.yoffset = vinfo.yres * buf_nu;
	if (ioctl(fb->fd, FBIOPAN_DISPLAY, &vinfo) == -1)
	{
		perror("FBIOPAN_DISPLAY");
		exit(EXIT_FAILURE);
	}
	fb->index++;



	return 0;

}
//#define SAVE_RGB_RAW	1
int display_direct_to_fb(__u8 *frame, struct camera_info *camera_inf, struct fb_info *fb)
{
	int fourcc = camera_inf->fmt.fourcc;
	int format = camera_inf->fmt.format;

	int line, col;
	int fb_line;
	int fb_col;
	int colorR, colorG, colorB;
	struct yuv422_sample yuv422_samp;
	struct yuv422_sample *yuv422_pixel =  NULL;
	struct yuv444_sample yuv444_samp;
	struct yuv444_sample *yuv444_pixel = NULL;
	struct rgb888_sample rgb888_samp;
	struct rgb888_sample *rgb888_pixel = NULL;
	__u8 y0, y1, u, v;
	__u8 *y = NULL;
	__u8 *cb = NULL;
	__u8 *cr = NULL;

	//printf("-->%s L%d\n", __func__, __LINE__);
#if SAVE_RGB_RAW
	FILE *rgb_raw = NULL;
	__u8 *rgb_buf = NULL;
	struct bpp24_pixel *pixel = NULL;
	int w = camera_inf->param.width;
	int h = camera_inf->param.height;
	int i;

	rgb_buf = (__u8 *)malloc(w*h*3);
	if (!rgb_buf)
		printf("==>%s: fail to malloc!!\n", __func__);

	pixel = (struct bpp24_pixel *)rgb_buf;
#endif

	// if camera frame is larger or smaller than lcd resolution or not?
	window_cut(camera_inf, fb);

	if (DF_IS_RGB888(fourcc))
	{
		cim_dbg("fmt is rgb888\n");
		for (line = camera_inf->yoff, fb_line = fb->yoff;
		     ( (line < (camera_inf->yoff + camera_inf->param.height)) && (fb_line < (fb->yoff + fb->yres)) );
		     line ++, fb_line++)
		{

			rgb888_pixel = (struct rgb888_sample *)
				(frame + line * camera_inf->param.width * 3 + camera_inf->xoff * 3);

			for (col = camera_inf->xoff, fb_col = fb->xoff;
			     ( (col < (camera_inf->xoff + camera_inf->param.width)) && (fb_col < (fb->yoff + fb->xres)) );
			     col ++, fb_col ++)
			{

				rgb888_normalization(format, rgb888_pixel, &rgb888_samp);
				fb_set_a_pixel(fb, fb_line, fb_col,
					       rgb888_samp.b1, rgb888_samp.b2, rgb888_samp.b3, 0);

				rgb888_pixel ++;
			}
		}

		return 0;
	}

	cim_dbg("planar = %d\n", camera_inf->param.planar);
	if (camera_inf->param.planar) {
		int img_size = camera_inf->param.width * camera_inf->param.height;
		if (DF_IS_YCbCr422(fourcc)) {	//YUV422
			y = frame;
			cb = frame + img_size;
			cr = frame + img_size + img_size / 2;
		} else {	//YUV444
			y = frame;
			cb = frame + img_size;
			cr = cb + img_size;
		}
	}

	cim_dbg("=======>start camera_infplay......\n");
	cim_dbg("-->%s L%d: camera_info: xoff=%d, yoff=%d, width=%d, height=%d, fb: xoff=%d, yoff=%d, xres=%d, yres=%d\n", __func__, __LINE__, camera_inf->xoff, camera_inf->yoff, camera_inf->param.width, camera_inf->param.height, fb->xoff, fb->yoff, fb->xres, fb->yres);
	for (line = camera_inf->yoff, fb_line = fb->yoff;
	     ( (line < (camera_inf->yoff + camera_inf->param.height)) && (fb_line < (fb->yoff + fb->yres)) );
	     line ++, fb_line++)
	{
		if (!camera_inf->param.planar) {
			if (DF_IS_YCbCr422(fourcc)) {
				yuv422_pixel = (struct yuv422_sample *)
					(frame + line * camera_inf->param.width * 2 + camera_inf->xoff * 2);
			} else {
				yuv444_pixel = (struct yuv444_sample *)
					(frame + line * camera_inf->param.width * 3 + camera_inf->xoff * 3);
			}
		}
		for (col = camera_inf->xoff, fb_col = fb->xoff;
		     ( (col < (camera_inf->xoff + camera_inf->param.width)) && (fb_col < (fb->yoff + fb->xres)) );
		     col += 2, fb_col += 2) {
			if (!camera_inf->param.planar) {
				if (DF_IS_YCbCr422(fourcc)) {
					yCbCr422_normalization(format, yuv422_pixel, &yuv422_samp);
					y0 = yuv422_samp.b1;
					u = yuv422_samp.b2;
					y1 = yuv422_samp.b3;
					v = yuv422_samp.b4;

					yuv422_pixel ++;
				} else {
					yCbCr444_normalization(format, yuv444_pixel, &yuv444_samp);
					y0 = yuv444_samp.b1;
					u = yuv444_samp.b2;
					v = yuv444_samp.b3;

					yuv444_pixel++;

					yCbCr444_normalization(format, yuv444_pixel, &yuv444_samp);
					y1 = yuv444_samp.b1;
					u = (yuv444_samp.b2 + u) / 2;
					v = (yuv444_samp.b3 + v) / 2;

					yuv444_pixel++;
				}
			} else {
				if (DF_IS_YCbCr422(fourcc)) {
					y0 = *(y + line * camera_inf->param.width + col);
					y1 = *(y + line * camera_inf->param.width + col + 1);
					u = *(cb + line * camera_inf->param.width / 2 + col / 2);
					v = *(cr + line * camera_inf->param.width / 2 + col / 2);
				} else {
					y0 = *(y + line * camera_inf->param.width + col);
					y1 = *(y + line * camera_inf->param.width + col + 1);
					u = (*(cb + line * camera_inf->param.width + col) + *(cb + line * camera_inf->param.width + col + 1)) / 2;
					v = (*(cr + line * camera_inf->param.width + col) + *(cr + line * camera_inf->param.width + col + 1)) / 2;
				}
			}

//			if (camera_inf->only_y) {
//				fprintf(stdout, "-->%s L%d: ignore uv.\n", __func__, __LINE__);
//				u = 128;
//				v = 128;
//			}

			//convert from YUV domain to RGB domain
			colorB = y0 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y0 -
				((179 * (v - 128) +
				  86 * (u - 128)) >> 8);

			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y0 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			fb_set_a_pixel(fb, fb_line, fb_col, colorB, colorG, colorR, 0);
#if SAVE_RGB_RAW
			if (rgb_buf) {
				pixel->red = colorR;
				pixel->green = colorG;
				pixel->blue = colorB;
				pixel++;
			}
#endif

			colorB = y1 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y1 -
				((179 * (v - 128) +
				  86 * (u - 128)) >> 8);

			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y1 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			fb_set_a_pixel(fb, fb_line, fb_col + 1, colorB, colorG, colorR, 0);
#if SAVE_RGB_RAW
			if (rgb_buf) {
				pixel->red = colorR;
				pixel->green = colorG;
				pixel->blue = colorB;
				pixel++;
			}
#endif

		}
	}

#if SAVE_RGB_RAW
	if (rgb_buf) {
		rgb_raw = fopen("rgb.raw", "w");
		if (!rgb_raw)
			printf("==>%s: fail to open rgb.raw!!\n", __func__);
		else {
			i = fwrite(rgb_buf, sizeof(__u8), w*h*3, rgb_raw);
			if (i != w*h*3)
				printf("==>%s: fail to write rgb.raw!!\n", __func__);
			fclose(rgb_raw);
		}

		free(rgb_buf);
	}
#endif

	return 0;
}
