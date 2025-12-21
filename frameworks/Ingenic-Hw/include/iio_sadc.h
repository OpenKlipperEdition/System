/**
 * @file    iio_sadc.h
 * @author  MPU系统软件部团队
 * @brief   包含 IIO_SADC HAL 库函数原型的头文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __HAL_IIO_SADC_H__
#define __HAL_IIO_SADC_H__
#include "ihal_info.h"

/**
 * @defgroup group_IIO_SADC IIO_SADC模块
 * @{
 */

/**
 * @addtogroup group_IIO_SADC_data_type 数据类型定义
 * @{
 */

/**
 * @brief IIO_SADC 通道
 */
enum IHAL_IIO_SADC_CHANNEL {
    IIO_SADC_AUX0,
    IIO_SADC_AUX1,
    IIO_SADC_AUX2,
    IIO_SADC_AUX3,
    IIO_SADC_AUX4,
    IIO_SADC_AUX5,
    IIO_SADC_AUX6,
    IIO_SADC_AUX7,
    IIO_SADC_AUX8,
    IIO_SADC_AUX9,
    IIO_SADC_AUX10,
    IIO_SADC_AUX11,
    IIO_SADC_AUX12,
    IIO_SADC_AUX13,
    IIO_SADC_AUX14,
    IIO_SADC_AUX15,
};

typedef struct {
    int channel_data[16];    /*!< 16个通道采样数据 */
} IIO_SADC_CHANNEL_DATA;

typedef void IHAL_IIO_SADC_Handle_t;
typedef void IHAL_IIO_SADC_TRIGGER_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_IIO_SADC_API API定义
 * @{
 */
/*********************** adc方式1 单通道采集 *****************************/
/**
 * @brief  IIO_SADC 初始化通道
 * @param  [in] channel : IIO_SADC_AUX0 ~ 15
 * @retval IHAL_IIO_SADC_Handle_t *  成功
 * @retval IHAL_RNULL                失败
 */
IHAL_IIO_SADC_Handle_t *IHal_IIO_SADC_Init_Channel(const enum IHAL_IIO_SADC_CHANNEL channel);

/**
 * @brief  IIO_SADC 反初始化通道
 * @param  [in] handle : IIO SADC handle
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_DeInit_Channel(IHAL_IIO_SADC_Handle_t *handle);

/**
 * @brief  IIO_SADC 获取原始数据
 * @param  [in] handle : IIO SADC handle
 * @retval 0        成功
 * @retval 非0      失败
 * @attention 原始数据：ADC寄存器的数据, 12位adc的得到原始数据 0 ~ 4095. 2^12（4096）个离散的数值
 */
IHAL_INT32 IHal_IIO_SADC_Read_Raw_Data(IHAL_IIO_SADC_Handle_t *handle);

/*********************** adc方式2 多通道扫描采集 *****************************/
/**
 * @brief  IIO_SADC TRIGGER 初始化
 * @retval IHAL_IIO_SADC_Handle_t *  成功
 * @retval IHAL_RNULL                失败
 * @attention adc方式2 默认使用trigger0
 */
IHAL_IIO_SADC_Handle_t *IHal_IIO_SADC_TRIGGER_Init(void);

/**
 * @brief  IIO SADC TRIGGER 反初始化
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_DeInit(IHAL_IIO_SADC_TRIGGER_Handle_t *handle);

/**
 * @brief  IIO SADC TRIGGER enable通道
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @param  [in] channel : IIO_SADC_AUX0 ~ 15
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Enable_Channel(IHAL_IIO_SADC_TRIGGER_Handle_t *handle, const enum IHAL_IIO_SADC_CHANNEL channel);

/**
 * @brief  IIO SADC TRIGGER disable通道
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @param  [in] channel : IIO_SADC_AUX0 ~ 15
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Disable_Channel(IHAL_IIO_SADC_TRIGGER_Handle_t *handle, const enum IHAL_IIO_SADC_CHANNEL channel);

/**
 * @brief  IIO SADC TRIGGER 设置序列buff长度
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @param  [in] length : 推荐 0-1024
 * @retval 0        成功
 * @retval 非0      失败
 * @attention 实际的buff大小 == 可存放 length 个转换序列通道数据。比如length==3。通道序列为 0、1。即buff最大存放0 1 0 1 0 1的通道数据
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Set_Sequence_Buff_Length(IHAL_IIO_SADC_TRIGGER_Handle_t *handle, const int length);

/**
 * @brief  IIO SADC TRIGGER 开始
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Start_Sample(IHAL_IIO_SADC_TRIGGER_Handle_t *handle);

/**
 * @brief  IIO SADC TRIGGER 停止
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Stop_Sample(IHAL_IIO_SADC_TRIGGER_Handle_t *handle);
/**
 * @brief  IIO SADC TRIGGER 读取一次序列数据
 * @param  [in] handle : IIO SADC TRIGGER handle
 * @param  [in] data   : 存放通道数据的 结构体变量, 类型为IIO_SADC_CHANNEL_DATA
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Read_Sequence_Buff_Data(IHAL_IIO_SADC_TRIGGER_Handle_t *handle, IIO_SADC_CHANNEL_DATA *data);
/**
 * @}
 */

/**
 * @}
 */
#endif //__HAL_IIO_SADC_H__
