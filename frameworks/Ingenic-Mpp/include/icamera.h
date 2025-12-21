/**
 * @file icamera.h
 * @author ingenic-team
 * @brief
 * @version 0.1
 * @date 2021-12-06
 *
 * @copyright Copyright (c) ingenic 2021
 *
 */

#ifndef __ICAMERA_H__
#define __ICAMERA_H__

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include "impp.h"
#include "impp_fifo.h"

/**
 * @defgroup group_Camera Camera模块
 * @{
 */

/**
 * @addtogroup group_Camera_data_type 数据类型定义
 * @{
 */

/**
 * @brief ISP Camera参数控制命令
 */
typedef enum {
        ISP_SET_HFLIP                                       = V4L2_CID_HFLIP,
        ISP_SET_VFLIP                                       = V4L2_CID_VFLIP,
        ISP_SET_SHARPNESS                           = V4L2_CID_SHARPNESS,
        ISP_SET_CONTRAST                            = V4L2_CID_CONTRAST,
        ISP_SET_SATURATION                          = V4L2_CID_SATURATION,
        ISP_SET_BRIGHTNESS                          = V4L2_CID_BRIGHTNESS,
        ISP_SET_POWER_LINE_FREQUENCY    = V4L2_CID_POWER_LINE_FREQUENCY,
        ISP_SET_WDR                                             = V4L2_CID_WIDE_DYNAMIC_RANGE,
        ISP_SET_AE                                          = V4L2_CID_EXPOSURE_AUTO,
        ISP_SET_AUTOGAIN                                = V4L2_CID_AUTOGAIN,

        ISP_SET_MODULE_CONTROL                  = V4L2_CID_PRIVATE_BASE,
        ISP_SET_DAY_OR_NIGHT,
        ISP_SET_AE_LUMA,
        ISP_SET_FACE_AE_CONTROL,
        ISP_SET_AWB_CONTROL,
        ISP_SET_BIN_PATH,
} IHAL_CAMERA_CONTROL_CMD;

typedef enum {
		ANTIFLICKER_DISABLE_MODE,
		ANTIFLICKER_NORMAL_MODE,
		ANTIFLICKER_AUTO_MODE,// can reach min it
		ANTIFLICKER_BUTT,
}IHAL_CameraAntiflickerMode_t;

/**
 * @brief Camera的基本参数
 * @detail: 裁剪属性，是基于sensor原始图像大小进行裁剪；
 * 裁剪后可再进行缩放
 */
typedef struct {
        IHAL_INT32      imageWidth;                                         /*!< 图像宽度      */
        IHAL_INT32      imageHeight;                                        /*!< 图像高度      */
        IMPP_PIX_FMT    imageFmt;                                           /*!< 图像像素格式  */
        IHAL_INT8       *imageFmtStr;                                       /*!< 图像像素格式字符串.str可取. IMPP_PIX_FMT_['fmt'], 当imageFmtStr不为NULL时，以imageFmtStr为主. 否则使用imageFmt.*/
        IHAL_INT16      wdr_enable;                                             /*!< 是否使能WDR   */
        IHAL_INT16      bin_select;                                             /*!< 是否需要ISP-BIN文件选择  */
        IHAL_INT16      crop_posx;                                              /*!< ISP-CORE输出裁剪起始坐标X     */
        IHAL_INT16      crop_posy;                                              /*!< ISP-CORE输出裁剪起始坐标Y     */
        IHAL_INT16      crop_width;                                             /*!< ISP-CORE输出裁剪宽度　　　　　*/
        IHAL_INT16      crop_height;                                    /*!< ISP-CORE输出裁剪高度　　　　　*/
        IHAL_INT16      scale_out_crop_posx;                    /*!< 缩放输出裁剪起始坐标X*/
        IHAL_INT16      scale_out_crop_posy;                    /*!< 缩放输出裁剪起始坐标Y */
        IHAL_INT16      scale_out_crop_width;                   /*!< 缩放输出裁剪宽度 */
        IHAL_INT16      scale_out_crop_height;                  /*!< 缩放输出裁剪高度 */
        IHAL_INT8       binpath[64];                                    /*!< ISP-BIN文件路径 */
} IHAL_CAMERA_PARAMS;

/**
 * @brief Camera handle
 */
typedef struct {
        IHAL_INT32 vfd;
        void *ctx;
} IHAL_CAMERA_HANDLE;

typedef IHAL_CAMERA_HANDLE IHAL_CameraHandle_t;

/**
 * @}
 */

/**
 * @addtogroup group_Camera_API API定义
 * @{
 */

/**
 * @brief 打开一个视频输入节点
 * @param [in] videoname : 视频节点路径，如：/dev/video4
 * @retval IHAL_CameraHandle_t 成功
 * @retval IHAL_RNULL              失败
 */
IHAL_CameraHandle_t *IHal_CameraOpen(IHAL_INT8 *videoname);

/**
 * @brief 关闭一个已打开的视频输入节点
 * @param [in] handle : Camera handle
 * @retval 0    成功
 * @retval 非0 失败
 */
IHAL_INT32 IHal_CameraClose(IHAL_CameraHandle_t *handle);

/**
 * @brief 设置Camera的基本参数，主要是图像宽高和像素格式设置
 * @param [in] handle : Camera handle
 * @param [in] params : Camera的基本参数
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraSetParams(IHAL_CameraHandle_t *handle, IHAL_CAMERA_PARAMS *params);

/**
 * @brief 设置ISP控制参数，如AE、AWB等参数
 * @param [in] handle : Camera handle
 * @param [in] cmd    : 要设置的参数对应的控制命令
 * @param [in] value  : 参数值
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraSetControlParams(IHAL_CameraHandle_t *handle, IHAL_CAMERA_CONTROL_CMD cmd, IHAL_UINT32 value);

/**
 * @brief 获取ISP控制参数，如AE、AWB等参数
 * @param [in]  handle : Camera handle
 * @param [in]  cmd    : 要获取的参数对应的控制命令
 * @param [out] value  : 参数值
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraGetControlParams(IHAL_CameraHandle_t *handle, IHAL_CAMERA_CONTROL_CMD cmd, IHAL_UINT32 *value);

/**
 * @brief 创建Camera的数据缓冲区
 * @param [in] handle      : Camera handle
 * @param [in] type        : 缓冲区类型
 * @param [in] num_buffers : 需要创建的缓冲区个数
 * @retval 实际创建的缓冲区个数 成功
 * @retval 错误码                            失败
 * @attention 创建缓冲区的个数内部最大限制是3个
 */
IHAL_INT32 IHal_CameraCreateBuffers(IHAL_CameraHandle_t *handle, IMPP_BUFFER_TYPE type, IHAL_INT32 num_buffers);

/**
 * @brief 设置Camera的数据缓冲区，用于Camera共享缓冲区
 * @param [in] handle   : Camera handle
 * @param [in] index    : 缓冲区序号
 * @param [in] sharebuf : 缓冲区信息
 * @retval 0    成功
 * @retval 非0  失败
 * @attention 该API需要在Camera开始前调用，且只需要调用一次
 */
IHAL_INT32 IHal_CameraSetBuffers(IHAL_CameraHandle_t *handle, IHAL_INT32 index, IMPP_BufferInfo_t *sharebuf);

/**
 * @brief 获取Camera数据缓冲区，用于Camera共享缓冲区
 * @param [in]  handle   : Camera handle
 * @param [in]  index    : 缓冲区序号
 * @param [out] sharebuf : 缓冲区信息
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_GetCameraBuffers(IHAL_CameraHandle_t *handle, IHAL_INT32 index, IMPP_BufferInfo_t *sharebuf);

/**
 * @brief 启动Camera开始视频流数据的采集
 * @param [in] handle : Camera handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraStart(IHAL_CameraHandle_t *handle);

/**
 * @brief 停止Camera采集数据
 * @param [in] handle : Camera handle
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraStop(IHAL_CameraHandle_t *handle);

/**
 * @brief 等待Camera数据缓冲区可用
 * @param [in] handle    : Camera handle
 * @param [in] impp_wait : 等待类型（ #IMPP_NO_WAIT 或 #IMPP_WAIT_FOREVER ）
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_Camera_WaitBufferAvailable(IHAL_CameraHandle_t *handle, IHAL_INT32 impp_wait);

/**
 * @brief 从视频缓存队列中取出一个已填充数据的Camera数据缓冲区
 * @param [in]  handle : Camera handle
 * @param [out] buf    : 缓冲区信息
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraDeQueueBuffer(IHAL_CameraHandle_t *handle, IMPP_BufferInfo_t *buf);

/**
 * @brief 将Camera数据缓冲区放回到视频缓存队列中
 * @param [in] handle : Camera handle
 * @param [in] buf    : 缓冲区信息
 * @retval 0    成功
 * @retval 非0  失败
 */
IHAL_INT32 IHal_CameraQueuebuffer(IHAL_CameraHandle_t *handle, IMPP_BufferInfo_t *buf);

IHAL_INT32 IHal_CameraAntiflickerSet(IHAL_CameraHandle_t *handle,IHAL_INT32 freq,IHAL_CameraAntiflickerMode_t mode,IHAL_INT32 enable);

/**
 * @}
 */

/**
 * @}
 */

#endif  // __ICAMERA_H__
