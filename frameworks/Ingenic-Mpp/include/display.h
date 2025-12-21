#ifndef __DISPLAY_H__
#define __DISPLAY_H__
#include "impp.h"

/**
 * @defgroup group_Display Display模块
 * @{
 */

/**
 * @addtogroup group_Display_data_type 数据类型定义
 * @{
 */


/**
 * @brief 显示模式
 */
typedef enum {
        RDma_Mode     = 0x10,                                   /*!< rmda模式                    */
        Composer_Mode = 0x11,                                   /*!< 叠加模式                    */
} IHal_DisplayMode_t;

/**
 * @brief 叠加顺序
 */
typedef enum {
        Order_0 = 0,                                                    /*!< 底部                        */
        Order_1 = 1,
        Order_2 = 2,
        Order_3 = 3,                                                    /*!< 顶部                        */
        Order_End = 0xff
} FB_Composer_Zorder_t;

/**
 * @brief FrameBuffer的属性
 */
typedef struct {
        char node[12];                                                  /*!< 设备节点，如/dev/fb0        */
        IHal_DisplayMode_t mode;                                /*!< 显示模式                    */
        IHAL_INT32 frame_width;                                 /*!< 宽度                        */
        IHAL_INT32 frame_height;                                /*!< 高度                        */
        IMPP_PIX_FMT frame_fmt;                                 /*!< 像素格式                    */
        IHAL_INT32 crop_x;                                      /*!< 源buffer 裁剪信息<x,y,w,h>     */
        IHAL_INT32 crop_y;
        IHAL_INT32 crop_w;
        IHAL_INT32 crop_h;
        IHAL_INT32 alpha;                                       /*!< 当前图层的透明度alhpa 0~255. */
} IHal_SampleFB_Attr;

/**
 * @brief DPU OSD各层的属性参数
 */
typedef struct {
        char node[12];                  /*!< 设备节点，如/dev/fb0		*/
        int lay_en;			/*!< 使能layer				*/
	IMPP_PIX_FMT srcFmt;            /*!< 源数据格式				*/
        IHAL_INT32 srcWidth;            /*!< 源数据宽度				*/
        IHAL_INT32 srcHeight;           /*!< 源数据高度				*/
        IHAL_INT32 srcCropx;            /*!< crop start x			*/
        IHAL_INT32 srcCropy;            /*!< crop start y			*/
        IHAL_INT32 srcCropw;            /*!< crop size width			*/
        IHAL_INT32 srcCroph;            /*!< crop size height			*/
        int scale_enable;               /*!< 使能缩放(最多支持两层)		*/
        IHAL_INT32 scaleHeight;         /*!< 缩放后的图像高度                   */
        IHAL_INT32 scaleWidth;          /*!< 缩放后的图像宽度                   */
        IHAL_INT32 osd_posX;            /*!< 叠加起始坐标X			*/
        IHAL_INT32 osd_posY;            /*!< 叠加起始坐标Y			*/
        IHAL_INT32 osd_order;           /*!< 叠加顺序				*/
        IHAL_INT32 alpha;               /*!< 透明度				*/
        IHAL_UINT32 color;		/*!< 颜色				*/
        IHAL_UINT32 version;		/*!< 版本				*/
	IHAL_UINT32 update_fbinfo;	/*!< 是否fb信息                         */
} IHal_SampleFB_LayerAttr_t;


/**
 * @brief FrameBuffer handle
 */
typedef void IHal_SampleFB_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_Display_API API定义
 * @{
 */


/**
 * @brief FrameBuffer的初始化
 * @param [in] attr : FrameBuffer的属性
 * @retval IHal_SampleFB_Handle_t 成功
 * @retval IHAL_RNULL             失败
 */
IHal_SampleFB_Handle_t *IHal_SampleFB_Init(IHal_SampleFB_Attr *attr);

/**
 * @brief FrameBuffer的初始化
 * @param [in] attr : FrameBuffer的属性
 * @retval IHal_SampleFB_Handle_t 成功
 * @retval IHAL_RNULL             失败
 */
IHal_SampleFB_Handle_t *IHal_SimpleFB_Init(IHal_SampleFB_LayerAttr_t *attr);

/**
 * @brief FrameBuffer的反初始化
 * @param [in] handle : FrameBuffer handle
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_DeInit(IHal_SampleFB_Handle_t *handle);

/**
 * @brief FrameBuffer的反初始化
 * @param [in] handle : FrameBuffer handle
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SimpleFB_DeInit(IHal_SampleFB_Handle_t *handle);

/**
 * @brief 刷新FrameBuffer显示数据
 * @param [in] handle : FrameBuffer handle
 * @param [in] buffer : 缓冲区信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_Update(IHal_SampleFB_Handle_t *handle, IMPP_BufferInfo_t *buffer);

/**
 * @brief 刷新Framebuffer数据的同时更改属性.
 * @param [in] handle : FrameBuffer handle
 * @param [in] buffer : 缓冲区信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SimpleFB_UpdateWithAttr(IHal_SampleFB_Handle_t *handle, IMPP_BufferInfo_t *buffer, IHal_SampleFB_LayerAttr_t *attr);

/**
 * @brief 等待Vsync同步信号
 * @desc  为了防止软件和硬件使用相同的buffer，应用程序需要WaitForVsync成功返回之后，
 * 	  再调用Update 进行显示更新。
 *        例如:
 * 		While(1) {
 *			_updateFrameBuffer(i + 1);
 *			_WaitForVsync(); //实际是上一帧刷新完成.
 *			_Update(i + 1);  // PanDisplay
 *		}
 * @param [in] handle : FrameBuffer handle
 * @param [in] vsync : 填充的sync类型，是否需要等vsync，默认0.
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_WaitForVsync(IHal_SampleFB_Handle_t *handle, int *vsync);

/**
 * @brief 获取FrameBuffer 有多少个帧Buffer.
 * @param [in] handle : FrameBuffer handle
 * @retval >0  返回Buffer的个数.
 */
IHAL_INT32 IHal_SampleFB_GetBufferNumbers(IHal_SampleFB_Handle_t *handle);

/**
 * @brief 获取FrameBuffer的虚拟地址和物理地址
 * @param [in]  handle : FrameBuffer handle
 * @param [out] buf    : 缓冲区信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_GetMem(IHal_SampleFB_Handle_t *handle, IMPP_BufferInfo_t *buf);

/**
 * @brief 设置FrameBuffer源图像大小
 * @param [in] handle : FrameBuffer handle
 * @param [in] width  : 宽度
 * @param [in] height : 高度
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetSrcFrameSize(IHal_SampleFB_Handle_t *handle, IHAL_INT32 width, IHAL_INT32 height);

/**
 * @brief 设置FrameBuffer源图像Stride大小
 * @param [in] handle   : FrameBuffer handle
 * @param [in] stride   : 宽度
 * @param [in] uv_stride: 宽度
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetSrcStrideSize(IHal_SampleFB_Handle_t *handle, IHAL_INT32 stride, IHAL_INT32 uv_stride);

/**
 * @brief 设置源FrameBuffer裁剪信息.
 * @param [in] handle : FrameBuffer handle
 * @param [in] crop_x : 起始坐标x
 * @param [in] crop_y : 起始坐标y
 * @param [in] crop_w : 裁剪的宽度
 * @param [in] crop_h : 裁剪的高度
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetSrcCrop(IHal_SampleFB_Handle_t *handle, IHAL_INT32 crop_x, IHAL_INT32 crop_y, IHAL_INT32 crop_w, IHAL_INT32 crop_h);

/**
 * @brief 设置FrameBuffer目标图像大小
 * @param [in] handle : FrameBuffer handle
 * @param [in] width  : 宽度
 * @param [in] height : 高度
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetTargetFrameSize(IHal_SampleFB_Handle_t *handle, IHAL_INT32 width, IHAL_INT32 height);

/**
 * @brief 设置FrameBuffer目标图像位置
 * @param [in] handle : FrameBuffer handle
 * @param [in] posx   : X坐标值
 * @param [in] posy   : Y坐标值
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetTargetPos(IHal_SampleFB_Handle_t *handle, IHAL_INT32 posx, IHAL_INT32 posy);

/**
 * @brief 设置FrameBuffer的叠加顺序
 * @param [in] handle : FrameBuffer handle
 * @param [in] order  : 叠加顺序
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetZorder(IHal_SampleFB_Handle_t *handle, FB_Composer_Zorder_t order);

/**
 * @brief 设置FrameBuffer的全局alpha
 * @param [in] handle : FrameBuffer handle
 * @param [in] alpha  : 全局alpha值，0 ~ 255，全透明 ~ 不透明.
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_SetAlpha(IHal_SampleFB_Handle_t *handle, IHAL_INT32 alpha);

/**
 * @brief Composer重启，更新参数设置
 * @param [in] handle : FrameBuffer handle
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_CompRestart(IHal_SampleFB_Handle_t *handle);

/**
 * @brief 使能或禁用对应的Layer
 * @param [in] handle : FrameBuffer handle
 * @param [in] enable : 使能位（0或1）
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_SampleFB_Layer_Enable(IHal_SampleFB_Handle_t *handle, int enable);

/**
 * @}
 */

/**
 * @}
 */

#endif  // __DISPLAY_H__
