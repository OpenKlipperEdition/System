#ifndef __SW_JPEG_H__
#define __SW_JPEG_H__



int sw_jpeg_encoder_init(unsigned int uWidth,unsigned int uHeight,int quality);
int sw_jpeg_encoder_start(unsigned char* src,unsigned char **out,int pixfmt);
int sw_jpeg_encoder_deinit(void);


#endif /* __SW_JPEG_H__  */


