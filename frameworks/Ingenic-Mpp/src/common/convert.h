#ifndef __CONVERT_H__
#define __CONVERT_H__
#include <stdio.h>
#include <stdint.h>
#include "impp.h"

struct tile420_fmt
{
	uint8_t *y;
    uint8_t *uv;
    int width;
    int height;
    int stride;
};

struct nv12_fmt
{
    uint8_t *d;
    int width;
    int stride;
    int height;
};

/**
 * @brief : 将TILE420格式转换为NV12格式
 *
 * @param src_buf [in] : 源Buffer信息
 * @param dst_buf [in] : 输出Buffer信息
 *
 * @retval IHAL_ROK		成功
 */
IHAL_INT32 IHal_Convert_Tile420ToNV12(struct tile420_fmt src_buf, struct nv12_fmt *dst_buf);

#endif	/* __CONVERT_H__ */
