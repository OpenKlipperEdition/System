#ifndef __ROTATER_H__
#define __ROTATER_H__

#include "impp.h"

/**
 * @defgroup group_Rotater Rotater模块
 * @{
 */

/**
 * @addtogroup group_Rotater_data_type 数据类型定义
 * @{
 */

/**
 * @brief Rotater handle
 */
typedef void IHal_Rot_Handle_t;

/**
 * @brief 图像旋转通道的属性
 */
typedef struct {
        IHAL_INT32 sWidth;                                              /*!< 源图像宽度                                    */
        IHAL_INT32 sHeight;                                             /*!< 源图像高度                                    */
        IMPP_BUFFER_TYPE        srcBufType;                     /*!< 源数据缓冲区类型                              */
        IMPP_BUFFER_TYPE        dstBufType;                     /*!< 输出缓冲区类型                              　*/
        IHAL_INT32 numSrcBuf;                                   /*!< 源数据缓冲区个数                              */
        IHAL_INT32 numDstBuf;                                   /*!< 输出数据缓冲区个数                            */
        IMPP_PIX_FMT srcFmt;                                    /*!< 源图像像素格式                                */
        IMPP_PIX_FMT dstFmt;                                    /*!< 源图像像素格式   (X2500忽略)                  */
} IHal_Rot_ChanAttr_t;

/**
 * @brief 图像处理方式
 */
typedef enum {
        ROT_PROCESS_ROTATE,                                             /*!< 旋转                                */
        ROT_PROCESS_FLIP,                                               /*!< 翻转                                */
        ROT_PROCESS_MIRROR,                                             /*!< 镜像                                */
} IHal_Rot_ProcessType_t;

/**
 * @brief 图像旋转角度
 */
typedef enum {
        ROT_ANGLE_0 = 0,                                                /*!< 旋转0度                             */
        ROT_ANGLE_90 = 90,                                              /*!< 旋转90度                            */
        ROT_ANGLE_180 = 180,                                    /*!< 旋转180度                           */
        ROT_ANGLE_270 = 270,                                    /*!< 旋转270度                           */
} IHal_Rot_Angle_t;

/**
 * @}
 */

/**
 * @addtogroup group_Rotater_API API定义
 * @{
 */

/**
 * @brief 创建一个图像旋转通道
 * @param [in] attr : 图像旋转通道的属性
 * @retval IHal_Rot_Handle_t 成功
 * @retval IHAL_RNULL        失败
 */
IHal_Rot_Handle_t *IHal_Rot_CreateChan(IHal_Rot_ChanAttr_t *attr);

/**
 * @brief 销毁一个图像旋转通道
 * @param [in] handle : Rotater handle
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_DestroyChan(IHal_Rot_Handle_t *handle);

/**
 * @brief 获取图像旋转的源dma-buf，用于共享缓冲区
 * @param [in]  handle : Rotater handle
 * @param [out] share  : 缓冲区信息
 * @param [in]  index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_SrcDmaBufGet(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index);

/**
 * @brief 获取图像旋转的输出dma-buf，用于共享缓冲区
 * @param [in]  handle : Rotater handle
 * @param [out] share  : 缓冲区信息
 * @param [in]  index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_DstDmaBufGet(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index);

/**
 * @brief 设置图像旋转的源数据外部缓冲区，用于共享缓冲区
 * @param [in] handle : Rotater handle
 * @param [in] share  : 缓冲区信息
 * @param [in] index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_SetExtSrcBuffer(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index);

/**
 * @brief 设置图像旋转的输出数据外部缓冲区，用于共享缓冲区
 * @param [in] handle : Rotater handle
 * @param [in] share  : 缓冲区信息
 * @param [in] index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_SetExtDstBuffer(IHal_Rot_Handle_t *handle, IMPP_BufferInfo_t *share, IHAL_INT32 index);

/**
 * @brief 对一帧图像数据进行图像旋转处理
 * @param [in] handle       : Rotater handle
 * @param [in] frame        : 帧信息
 * @param [in] process_type : 图像处理方式
 * @param [in] angle        : 旋转角度，参考 #IHal_Rot_Angle_t
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_ProcessFrame(IHal_Rot_Handle_t *handle, IMPP_FrameInfo_t *frame, IHal_Rot_ProcessType_t process_type, IHAL_INT32 angle);

/**
 * @brief 获取一帧图像旋转处理后的图像数据
 * @param [in]  handle : Rotater handle
 * @param [out] dst    : 帧信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_GetFrame(IHal_Rot_Handle_t *handle, IMPP_FrameInfo_t *dst);

/**
 * @brief 释放一帧图像旋转处理后的图像数据
 * @param [in] handle : Rotater handle
 * @param [in] dst    : 帧信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_Rot_ReleaseFrame(IHal_Rot_Handle_t *handle, IMPP_FrameInfo_t *dst);

/**
 * @}
 */

/**
 * @}
 */

#endif // __ROTATER_H__
