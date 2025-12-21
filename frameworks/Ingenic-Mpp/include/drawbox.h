#ifndef __DRAW_BOX_H__
#define __DRAW_BOX_H__

#include <impp.h>

/**
 * @defgroup group_DrawBox DrawBox模块
 * @{
 */

/**
 * @addtogroup group_DrawBox_data_type 数据类型定义
 * @{
 */

/**
 * @brief Box信息
 */
typedef struct {
        IHAL_INT32 x0;              /*!< Box在图像上的横向位置数据x  */
        IHAL_INT32 y0;              /*!< Box在图像上的纵向位置数据y */
        IHAL_INT32 x1;              /*!< x1 - x0为所画Box的宽度 */
        IHAL_INT32 y1;              /*!< y1 - y0为所画Box的高度 */
} IHal_BoxInfo_t;

/**
 * @brief Box的颜色模式
 */
typedef enum {
        BoxColorMode_0,            /*!< 模式0：红色 */
        BoxColorMode_1,            /*!< 模式1：黑色 */
        BoxColorMode_2,            /*!< 模式2：绿色 */
        BoxColorMode_3,            /*!< 模式3：黄色 */
} Box_ColorMode_t;

/**
 * @brief box的画框模式
 */
typedef enum {
        FULL_BOX_MODE,             /*!< 全边框模式 */
        HALF_BOX_MODE,             /*!< 半边框模式 */
} Box_Type_t;
/**
 * full box：
 *    --------
 *    |      |
 *    |      |d
 *    |      |
 *    --------
 *
 * half box：
 *    -----   -----  <-------
 *    |           |          \
 *    |           |            line_l
 *    |           |  <-------/
 *
 *    |           |
 *    |           |
 *    |           |
 *    -----   -----
 */

/**
 * @brief DrawBox信息
 */
typedef struct {
        IHAL_UINT32 img_width;             /*!< 图像宽 */
        IHAL_UINT32 img_height;            /*!< 图像高 */
        IHAL_UINT32 img_paddr;             /*!< 图像数据的物理地址 */
        IMPP_PIX_FMT img_fmt;              /*!< 图像像素格式（目前仅支持NV12格式） */
        IHAL_INT32 box_num;                /*!< Box个数，最大值20 */
        IHal_BoxInfo_t box[20];            /*!< Box信息 */
        IHAL_INT32 line_w;                 /*!< 画线的宽度，范围：0 ~ 7，像素值为(line_w + 1) * 2 */
        IHAL_INT32 line_l;                 /*!< 画线的长度（0 - 511pt），只有半边框模式使用 */
        Box_Type_t boxtype;                /*!< Box的模式 */
        Box_ColorMode_t color;             /*!< Box的颜色（边框颜色） */
} IHal_DrawBoxInfo_t;

/**
 * @brief DrawBox handle
 */
typedef void IHal_DrawBoxHandle_t;

/**
 * @}
 */

/**
 * @addtogroup group_DrawBox_API API定义
 * @{
 */

/**
 * @brief 初始化DrawBox模块
 * @param [in] ch : 通道号（0 - 1）
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_DrawBox_Init(IHAL_INT32 ch);

/**
 * @brief 去初始化DrawBox模块
 * @param [in] ch : 通道号（0 - 1）
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_DrawBox_DeInit(IHAL_INT32 ch);

/**
 * @brief 导入dma-buf，调用这个API可以获得dma-buf的paddr
 * @param [in] ch  : 通道号（0 - 1）
 * @param [in] buf : Buffer信息
 * @retval paddr 成功
 * @retval 0     失败
 */
IHAL_UINT32 IHal_DrawBox_ImportDmaBuf(IHAL_INT32 ch, IMPP_BufferInfo_t *buf);

/**
 * @brief DrawBox模块进行画框处理
 * @param [in] ch      : 通道号（0 - 1）
 * @param [in] boxinfo : 画框信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_DrawBox_Process(IHAL_INT32 ch, IHal_DrawBoxInfo_t *boxinfo);

/**
 * @}
 */

/**
 * @}
 */

#endif  // __DRAW_BOX_H__
