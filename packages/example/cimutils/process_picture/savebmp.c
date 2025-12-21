#include "savebmp.h"

BITMAPFILEHEADER  bfile = {
	.bfType = 0x4d42,	//"BM"
	.bfSize = 800*600*3+0x36,	//frame size + headers
	.bfReserved1 = 0,
	.bfReserved2 = 0,
	.bfoffBits = 0x36
};

BITMAPINFOHEADER  binfo = {
	.biSize = 0x28,		//sizeof(BITMAPINFOHEADER)
	.biWidth = 800,
	.biHeight = 600,
	.biPlanes = 1,
	.biBitCount = 0x18,	//24-bit true color
	.biCompress = 0,	//no compress
	.biSizeImage = 0,	//?
	.biXPelsPerMeter = 0x0b40,	//?
	.biYPelsPerMeter = 0x0b40,	//?
	.biClrUsed = 0,		//?
	.biClrImportant = 0	//?
};

int save_bgr_to_bmp(__u8 *buf, struct camera_info *camera_inf, FILE *fp)
{
	int len = 0;
	int ret = 0;

	debug("sizeof bfile = %d\n", sizeof(BITMAPFILEHEADER));
	debug("sizeof binfo = %d\n", sizeof(BITMAPINFOHEADER));

	bfile.bfSize = camera_inf->param.width * camera_inf->param.height * 3;
	binfo.biWidth = camera_inf->param.width;
	binfo.biHeight = camera_inf->param.height;

	len = bfile.bfSize;

	binfo.biXPelsPerMeter = 0;
	binfo.biYPelsPerMeter = 0;

	ret = fwrite(&bfile, sizeof(BITMAPFILEHEADER), 1, fp);
	if (ret != 1) {
		printf("%s:%d write error!!!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = fwrite(&binfo, sizeof(BITMAPINFOHEADER), 1, fp);
	if (ret != 1) {
		printf("%s:%d write error!!!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = fwrite(buf, sizeof(__u8), len, fp);
	if (ret != len) {
		printf("%s:%d write error!!!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}
int convert_RGB32_to_rgb24(__u8 *frame, __u8 *rgb, struct camera_info *camera_inf)
{
	int w = camera_inf->param.width;
	int h = camera_inf->param.height;
	int i;

	__u8 *tmp = rgb;
	for(i = 0; i < w * h * 4; i++) {
		if(i%4 == 3) {
			continue;
		}
		*tmp++ = frame[i];
	}
	return 0;
}
int convert_RGB565_to_rgb24(__u8 *frame, __u8 *rgb, struct camera_info *camera_inf)
{
	int w = camera_inf->param.width;
	int h = camera_inf->param.height;
	int i;

	__u16 * src_a = (__u16 *)frame;
	__u8 *tmp = rgb;
	for(i = 0; i < w * h; i++) {
		*tmp++ = (src_a[i]  & 0x1f) << 3;
		*tmp++ = ((src_a[i] >> 5) & 0x3f) << 2;
		*tmp++ = ((src_a[i] >> 11) & 0x1f) << 3;
	}
	return 0;
}

//#define BMP_DBG	1
int convert_yuv_to_rgb24(__u8 *frame, __u8 *rgb, struct camera_info *camera_inf)
{
	int fourcc = camera_inf->fmt.fourcc;
	int format = camera_inf->fmt.format;

	int line, col;
	int colorR, colorG, colorB;
	struct yuv422_sample yuv422_samp;
	struct yuv422_sample *yuv422_pixel =  NULL;
	struct yuv444_sample yuv444_samp;
	struct yuv444_sample *yuv444_pixel = NULL;
	__u8 y0, y1, u, v;
	__u8 *y = NULL;
	__u8 *cb = NULL;
	__u8 *cr = NULL;

	struct bmp_bgr *bgr = (struct bmp_bgr *)rgb;

#if BMP_DBG
	__u8 *bmp_bgr_buf = rgb;
	__u8 *yuv_buf = frame;

	FILE *bmp_bgr_raw = NULL;
	FILE *yuv_raw = NULL;
	int w = camera_inf->param.width;
	int h = camera_inf->param.height;
	int i;

	yuv_raw = fopen("yuv.raw", "w");
	if (!yuv_raw)
		printf("==>%s: fail to open yuv.raw!!\n", __func__);
	else {
		i = fwrite(yuv_buf, sizeof(__u8), w*h*2, yuv_raw);
		if (i != w*h*2)
			printf("==>%s: fail to write yuv.raw!!\n", __func__);
		fclose(yuv_raw);
	}

#endif

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

	debug("=======================>yoff= %d \n",camera_inf->yoff);
	for (line = camera_inf->yoff; (line < (camera_inf->yoff + camera_inf->param.height)); line++)
	{
		bgr = (struct bmp_bgr *)(rgb + camera_inf->param.width * (camera_inf->param.height - line - 1) * 3);

		if (!camera_inf->param.planar) {
			if (DF_IS_YCbCr422(fourcc)) {
				yuv422_pixel = (struct yuv422_sample *)
					(frame + line * camera_inf->param.width * 2 + camera_inf->xoff * 2);
			} else {
				yuv444_pixel = (struct yuv444_sample *)
					(frame + line * camera_inf->param.width * 3 + camera_inf->xoff * 3);
			}
		}
		for (col = camera_inf->xoff; (col < (camera_inf->xoff + camera_inf->param.width)); col += 2)
		{
			if (!camera_inf->param.planar) {
				if (DF_IS_YCbCr422(fourcc)) {
					yCbCr422_normalization(format, yuv422_pixel, &yuv422_samp);
					if (camera_inf->param.yuv422_format == F_YUYV) {
						y0 = yuv422_samp.b1;
						u = yuv422_samp.b2;
						y1 = yuv422_samp.b3;
						v = yuv422_samp.b4;
					} else if (camera_inf->param.yuv422_format == F_YVYU) {
						y0 = yuv422_samp.b1;
						v = yuv422_samp.b2;
						y1 = yuv422_samp.b3;
						u = yuv422_samp.b4;
					} else if (camera_inf->param.yuv422_format == F_UYVY) {
						u = yuv422_samp.b1;
						y0 = yuv422_samp.b2;
						v = yuv422_samp.b3;
						y1 = yuv422_samp.b4;
					} else if (camera_inf->param.yuv422_format == F_VYUY) {
						v = yuv422_samp.b1;
						y0 = yuv422_samp.b2;
						u = yuv422_samp.b3;
						y1 = yuv422_samp.b4;
					}
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

			//Windows BitMap BGR
			bgr->blue = colorB;
			bgr->green = colorG;
			bgr->red = colorR;
			bgr++;

			colorB = y1 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y1 -
				((179 * (v - 128) +
				  86 * (u - 128)) >> 8);

			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y1 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			//Windows BitMap BGR
			bgr->blue = colorB;
			bgr->green = colorG;
			bgr->red = colorR;
			bgr++;
		}
	}

#if BMP_DBG
	bmp_bgr_raw = fopen("bmp_bgr.raw", "w");
	if (!bmp_bgr_raw)
		printf("==>%s: fail to open bmp_bgr.raw!!\n", __func__);
	else {
		i = fwrite(bmp_bgr_buf, sizeof(__u8), w*h*3, bmp_bgr_raw);
		if (i != w*h*3)
			printf("==>%s: fail to write bmp_bgr.raw!!\n", __func__);
		fclose(bmp_bgr_raw);
	}
#endif

	return 0;
}

int convert_grey_to_rgb24 (__u8 *frame, __u8 *rgb, struct camera_info *camera_inf)
{
	int line, col;
	int colorR, colorG, colorB;
	__u8 y0, y1, u, v;
	__u8 *y = NULL;

	struct bmp_bgr *bgr = (struct bmp_bgr *)rgb;

	u = 128;
	v = 128;
	y = frame;
	for (line = camera_inf->yoff; (line < (camera_inf->yoff + camera_inf->param.height)); line++)
	{
		bgr = (struct bmp_bgr *)(rgb + camera_inf->param.width * (camera_inf->param.height - line - 1) * 3);

		for (col = camera_inf->xoff; (col < (camera_inf->xoff + camera_inf->param.width)); col += 2)
		{
			y0 = *(y + line * camera_inf->param.width + col);
			y1 = *(y + line * camera_inf->param.width + col + 1);

			colorB = y0 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y0 -
				((179 * (v - 128) +
				  86 * (u - 128)) >> 8);

			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y0 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			//Windows BitMap BGR
			bgr->blue = colorB;
			bgr->green = colorG;
			bgr->red = colorR;
			bgr++;

			colorB = y1 + ((443 * (u - 128)) >> 8);
			colorB = (colorB < 0) ? 0 : ((colorB > 255 ) ? 255 : colorB);

			colorG = y1 -
				((179 * (v - 128) +
				  86 * (u - 128)) >> 8);

			colorG = (colorG < 0) ? 0 : ((colorG > 255 ) ? 255 : colorG);

			colorR = y1 + ((351 * (v - 128)) >> 8);
			colorR = (colorR < 0) ? 0 : ((colorR > 255 ) ? 255 : colorR);

			//Windows BitMap BGR
			bgr->blue = colorB;
			bgr->green = colorG;
			bgr->red = colorR;
			bgr++;
		}
	}

	return 0;
}
