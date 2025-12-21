#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <jpeglib.h>
#include <jerror.h>

#include <linux/media.h>
#include <linux/videodev2.h>


static struct jpeg_compress_struct cinfo;
static struct jpeg_error_mgr jerr;
static unsigned char *g_yuv = NULL;


static int yuyv_to_jpeg(unsigned char *srcbuffer)
{
	int i = 0,j = 0;
	int indx = 0;
	int width = cinfo.image_width;
	JSAMPROW row_pointer[1];
	while(cinfo.next_scanline < cinfo.image_height){
		indx = 0;
		for(i = 0; i < width; i++){
			g_yuv[indx++] = srcbuffer[i * 2 + j * width*2];
			g_yuv[indx++] = srcbuffer[1 + (i/2)*4 + j * width*2];
			g_yuv[indx++] = srcbuffer[2 + 1 + (i/2)*4 + j*width*2];
		}
		row_pointer[0] = g_yuv;
		jpeg_write_scanlines(&cinfo,row_pointer,1);
		j++;
	}
	return 0;
}

static int nv12_to_jpeg(unsigned char *srcbuffer)
{
	int i = 0,j = 0;
	int width = cinfo.image_width;
	int height = cinfo.image_height;
	unsigned char* ybase = (unsigned char*)srcbuffer;
	unsigned char* uvbase = (unsigned char*)(srcbuffer + width * height);
	int indx = 0;
	JSAMPROW row_pointer[1];
	while(cinfo.next_scanline < cinfo.image_height){
		indx = 0;
		for(i = 0; i < width; i++){
			g_yuv[indx++] = ybase[i + j * width];
			g_yuv[indx++] = uvbase[j/2*width + (i/2)*2];
			g_yuv[indx++] = uvbase[j/2*width + (i/2)*2+1];
		}
		row_pointer[0] = g_yuv;
		jpeg_write_scanlines(&cinfo,row_pointer,1);
		j++;
	}
	return 0;
}

int sw_jpeg_encoder_init(unsigned int uWidth,unsigned int uHeight,int quality)
{
	memset(&cinfo,0,sizeof(struct jpeg_compress_struct));
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	cinfo.image_width = uWidth;
  	cinfo.image_height = uHeight;
  	cinfo.input_components = 3;
  	cinfo.in_color_space = JCS_YCbCr;
  	jpeg_set_defaults(&cinfo);
  	jpeg_set_quality(&cinfo, quality, TRUE);

  	cinfo.jpeg_color_space = JCS_YCbCr;
  	cinfo.comp_info[0].h_samp_factor = 2;
  	cinfo.comp_info[0].v_samp_factor = 2;
  	cinfo.comp_info[1].h_samp_factor = 1;
  	cinfo.comp_info[1].v_samp_factor = 1;
  	cinfo.comp_info[2].h_samp_factor = 1;
  	cinfo.comp_info[2].v_samp_factor = 1;
  	cinfo.dct_method = JDCT_FASTEST;

	g_yuv = (unsigned char*)malloc(uWidth * 3);
	if(g_yuv == NULL){
		printf("malloc g_yuv failed .....\n ");
		return -1;
	}
	return 0;
}


int sw_jpeg_encoder_start(unsigned char* src,unsigned char *out,int pixfmt)
{
	unsigned int outsize = 0;
	unsigned char *output = NULL;
	jpeg_mem_dest (&cinfo,&output, &outsize);
	/* Start compressor */
	jpeg_start_compress(&cinfo, TRUE);
	switch(pixfmt){
		case V4L2_PIX_FMT_YUYV:
			yuyv_to_jpeg(src);
			break;
		case V4L2_PIX_FMT_NV12:
			nv12_to_jpeg(src);
			break;
		default : 
			printf("just support nv12/yuyv to jpeg!!!!!\n");
			return -1;
	}
	jpeg_finish_compress(&cinfo);
	memcpy(out,output,outsize);
	free(output);
	return outsize;
}

int sw_jpeg_encoder_deinit(void)
{
	jpeg_destroy_compress(&cinfo);
	free(g_yuv);
	return 0;
}
