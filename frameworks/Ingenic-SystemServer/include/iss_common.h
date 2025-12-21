#ifndef __ISS_COMMOM_H__
#define __ISS_COMMOM_H__


#include <stdint.h>

/**
 * @brief 图像格式
 */
typedef enum {
        PIX_FMT_NV12,
        PIX_FMT_NV21,
        PIX_FMT_NV16,
        PIX_FMT_YUV422,
        PIX_FMT_YUV422P,
        PIX_FMT_I420,
        PIX_FMT_I422,
        PIX_FMT_YUV420p,
        PIX_FMT_RAW8,
        PIX_FMT_RAW10,
        PIX_FMT_RGBA_8888,
        PIX_FMT_RGBX_8888,
        PIX_FMT_BGRA_8888,
        PIX_FMT_BGRX_8888,
        PIX_FMT_ABGR_8888,
        PIX_FMT_ARGB_8888,
        PIX_FMT_RGB_888,
        PIX_FMT_BGR_888,
        PIX_FMT_RGB_565,
        PIX_FMT_RGB_555,
        PIX_FMT_RGBA_5551,
        PIX_FMT_BGRA_5551,
        PIX_FMT_ARGB_1555,
        PIX_FMT_HSV,

        /* Camera Raw8 */
        PIX_FMT_SBGGR8,
        PIX_FMT_SGBRG8,
        PIX_FMT_SGRBG8,
        PIX_FMT_SRGGB8,

        /* Camera Raw10 */
        PIX_FMT_SBGGR10,
        PIX_FMT_SGBRG10,
        PIX_FMT_SGRBG10,
        PIX_FMT_SRGGB10,

        /* Camera Raw12 */
        PIX_FMT_SBGGR12,
        PIX_FMT_SGBRG12,
        PIX_FMT_SGRBG12,
        PIX_FMT_SRGGB12,

        /* Y only.*/
        PIX_FMT_GREY,
        PIX_FMT_Y4,
        PIX_FMT_Y6,
        PIX_FMT_Y12,
        PIX_FMT_Y16,

        /* YUV422. */
        PIX_FMT_YUYV,
        PIX_FMT_YYUV,
        PIX_FMT_YVYU,
        PIX_FMT_VYUY,

        /* YUV444. */
        PIX_FMT_YUV444,

		PIX_FMT_MJPEG,

		/* TILE420 */
		PIX_FMT_JZ420B,

}PixFmt_t;



typedef struct {
	int32_t fd;
	uint32_t size;
	uint32_t vaddr;
	uint32_t paddr;
	int32_t index;
}DataBuffer_t;


#endif //__ISS_COMMOM_H__




