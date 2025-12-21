#ifndef __IPU_H__
#define __IPU_H__

#include "impp.h"

/**
 * @defgroup group_IPU IPU模块
 * @{
 */

/**
 * @addtogroup group_IPU_data_type 数据类型定义
 * @{
 */


/**
 * @brief 色彩格式转换通道的属性
 */
typedef struct {
        IHAL_INT32 sWidth;                                              /*!< 源图像宽度                                    */
        IHAL_INT32 sHeight;                                             /*!< 源图像高度                                    */
        IHAL_INT32 isSrcUseExtDmaBuf;                   /*!< 源数据缓冲区是否使用外部dma-buf               */
        IHAL_INT32 isDstUseExtDmaBuf;                   /*!< 输出数据缓冲区是否使用外部dma-buf             */
        IHAL_INT32 numSrcBuf;                                   /*!< 源数据缓冲区个数                              */
        IHAL_INT32 numDstBuf;                                   /*!< 输出数据缓冲区个数                            */
        IMPP_PIX_FMT srcFmt;                                    /*!< 源图像像素格式                                */
        IMPP_PIX_FMT dstFmt;                                    /*!< 输出图像像素格式                              */
} IHal_CSC_ChanAttr_t;


/**
 * @brief 图像叠加通道的属性
 */
typedef struct {
        IHAL_INT32 maxFifoNum;                                  /*!< 帧FIFO个数的最大值                             */
} IHal_OSD_ChanAttr_t;


/**
 * @brief 图像叠加的背景图属性
 */
typedef struct {
        IHAL_INT32 width;                                               /*!< 背景图宽度                                     */
        IHAL_INT32 height;                                              /*!< 背景图高度                                     */
        IHAL_UINT32 paddr;                                              /*!< 物理地址                                       */
        IHAL_UINT32 vaddr;                                              /*!< 虚拟地址                                       */
        IMPP_PIX_FMT fmt;                                               /*!< 像素格式                                       */
        IHAL_INT32 fd;                                                  /*!< 用于dma-buf                                    */
} IHal_OSD_BG_Attr_t;

/**
 * @brief 图像叠加的通道属性
 */
typedef struct {
        IHAL_INT32 width;                                               /*!< 宽度                                           */
        IHAL_INT32 height;                                              /*!< 高度                                           */
        IHAL_INT32 alpha;                                               /*!< 透明度                                                                      */
        IHAL_INT32 posX;                                                /*!< X坐标值                                        */
        IHAL_INT32 posY;                                                /*!< Y坐标值                                        */
        IHAL_UINT32 paddr;                                              /*!< 物理地址                                       */
        IHAL_UINT32 vaddr;                                              /*!< 虚拟地址                                       */
        IHAL_INT32 fd;                                                  /*!< 用于dma-buf                                    */
        IMPP_PIX_FMT fmt;                                               /*!< 像素格式                                       */
} IHal_OSD_CH_Attr_t;

/**
 * @brief 图像叠加的通道使能标志
 */
typedef enum {
        OSD_CH0_EN = 0x1,                                               /*!< 使能通道0                                                                          */
        OSD_CH1_EN = 0x2,                                               /*!< 使能通道1                                                                              */
        OSD_CH2_EN = 0x4,                                               /*!< 使能通道2                                                                              */
        OSD_CH3_EN = 0x8,                                               /*!< 使能通道3                                                                              */
} IHal_OSD_Flags_t;

/**
 * @brief 图像叠加的帧描述信息
 */
typedef struct {
        IHal_OSD_BG_Attr_t  bgAttr;                             /*!< 背景图属性                                     */
        IHal_OSD_CH_Attr_t  chAttr[4];                  /*!< 通道属性                                       */
        IHAL_INT32          osd_flags;                  /*!< 通道使能标志，参考 #IHal_OSD_Flags_t           */
} IHal_OSD_FrameDesc_t;

typedef enum {
        DMA_SYNC_FOR_READ,
        DMA_SYNC_FOR_WRITE,
} Ipu_DMA_SyncType_t;

typedef struct {
        int fd;
        Ipu_DMA_SyncType_t sync_type;
} IPU_DmaBuf_SyncInfo_t;
/**
 * @brief CSC handle
 */
typedef void IHal_CSC_Handle_t;

/**
 * @brief OSD handle
 */
typedef void IHal_OSD_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_IPU_API API定义
 * @{
 */


/*********************** CSC API **********************/

/**
 * @brief 创建一个色彩格式转换通道
 * @param [in] attr : 色彩格式转换通道的属性
 * @retval IHal_CSC_Handle_t 成功
 * @retval IHAL_RNULL            失败
 */
IHal_CSC_Handle_t *IHal_CSC_ChanCreate(IHal_CSC_ChanAttr_t *attr);

/**
 * @brief 销毁一个色彩格式转换通道
 * @param [in] handle : CSC handle
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_DestroyChan(IHal_CSC_Handle_t *handle);

/**
 * @brief 获取色彩格式转换源数据缓冲区，用于共享缓冲区
 * @param [in]  handle : CSC handle
 * @param [out] buf    : 缓冲区信息
 * @param [in]  index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_GetSrcBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index);

/**
 * @brief 获取色彩格式转换输出数据缓冲区，用于共享缓冲区
 * @param [in]  handle : CSC handle
 * @param [out] buf    : 缓冲区信息
 * @param [in]  index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_GetDstBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index);

/**
 * @brief 设置色彩格式转换源数据外部缓冲区，用于共享缓冲区,支持物理地址BUF/DMA_BUF-FD
 * @param [in] handle : CSC handle
 * @param [in] buf    : 缓冲区信息
 * @param [in] index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_SetSrcExtBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index);

/**
 * @brief 设置色彩格式转换输出数据外部缓冲区，用于共享缓冲区,支持物理地址BUF/DMA_BUF-FD
 * @param [in] handle : CSC handle
 * @param [in] buf    : 缓冲区信息
 * @param [in] index  : 缓冲区序号
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_SetDstExtBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buf, IHAL_INT32 index);

/**
 * @brief 导入dma-buf，可以获取其物理地址
 * @param [in]  handle : CSC handle
 * @param [out] buffer : 缓冲区信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_ImportDmaBuf(IHal_CSC_Handle_t *handle, IMPP_BufferInfo_t *buffer);

/**
 * @brief 对一帧图像数据进行色彩格式转换处理
 * @param [in] handle : CSC handle
 * @param [in] frame  : 帧信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_FrameProcess(IHal_CSC_Handle_t *handle, IMPP_FrameInfo_t *frame);

/**
 * @brief 获取一帧色彩格式转换后输出的图像数据
 * @param [in]  handle : CSC handle
 * @param [out] frame  : 帧信息
 * @param [in]  wait   : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_GetDstFrame(IHal_CSC_Handle_t *handle, IMPP_FrameInfo_t *frame, IHAL_INT32 wait);

/**
 * @brief 释放一帧色彩格式转换后输出的图像数据
 * @param [in] handle : CSC handle
 * @param [in] frame  : 帧信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_ReleaseDstFrame(IHal_CSC_Handle_t *handle, IMPP_FrameInfo_t *frame);

/**
 * @brief CSC DMA-BUF Cache刷新
 * @param [in] handle : CSC handle
 * @param [in] info   : Dmabuf 信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CSC_FlushCache(IHal_CSC_Handle_t *handle, IPU_DmaBuf_SyncInfo_t *info);


/*********************** OSD API **********************/

/**
 * @brief 创建一个图像叠加通道
 * @param [in] attr : 图像叠加通道的属性
 * @retval IHal_OSD_Handle_t 成功
 * @retval IHAL_RNULL        失败
 */
IHal_OSD_Handle_t *IHal_OSD_ChanCreate(IHal_OSD_ChanAttr_t *attr);

/**
 * @brief 销毁一个图像叠加通道
 * @param [in] handle : OSD handle
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_OSD_DestroyChan(IHal_OSD_Handle_t *handle);

/**
 * @brief 创建一个图像叠加通道缓冲区
 * @param [in]  handle : OSD handle
 * @param [out] buffer : 缓冲区信息
 * @param [in]  size   : 缓冲区大小
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_OSD_CHx_BufCreate(IHal_OSD_Handle_t *handle, IMPP_BufferInfo_t *buffer, IHAL_UINT32 size);

/**
 * @brief 释放一个图像叠加通道缓冲区
 * @param [in] handle : OSD handle
 * @param [in] buffer : 缓冲区信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_OSD_CHx_BufFree(IHal_OSD_Handle_t *handle, IMPP_BufferInfo_t *buffer);

/**
 * @brief 导入dma-buf，可以获取其物理地址
 * @param [in]  handle : OSD handle
 * @param [out] buffer : 缓冲区信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_OSD_ImportDmaBuf(IHal_OSD_Handle_t *handle, IMPP_BufferInfo_t *buffer);

/**
 * @brief 对一帧图像数据进行图像叠加处理
 * @param [in]  handle    : OSD handle
 * @param [in]  framedesc : 图像叠加的帧描述信息
 * @param [out] out       : 图像叠加后的帧信息
 * @retval 0   成功
 * @retval 非0 失败
 * @warning 背景输入只支持NV12，且输出应和背景保持一致！
 */
IHAL_INT32 IHal_OSD_ProcessFrame(IHal_OSD_Handle_t *handle, IHal_OSD_FrameDesc_t *framedesc, IMPP_FrameInfo_t *out);

/**
 * @brief OSD DMA-BUF Cache刷新
 * @param [in]  handle    : OSD handle
 * @param [in]  info      : Dmabuf 信息
 * @retval 0   成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_OSD_FlushCache(IHal_OSD_Handle_t *handle, IPU_DmaBuf_SyncInfo_t *info);
/**
 * @}
 */

/**
 * @}
 */

#endif // __IPU_H__
