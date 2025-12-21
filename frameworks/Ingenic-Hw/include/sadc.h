/**
 * @file    sadc.h
 * @author  MPU系统软件部团队
 * @brief   包含 SADC HAL 库函数原型的头文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __HAL_SADC_H__
#define __HAL_SADC_H__
#include "ihal_info.h"

/**
 * @defgroup group_SADC SADC模块
 * @{
 */

/**
 * @addtogroup group_SADC_data_type 数据类型定义
 * @{
 */

/**
 * @brief SADC 通道
 */
enum IHAL_SADC_CHANNEL {
    SADC_AUX0,
    SADC_AUX1,
    SADC_AUX2,
    SADC_AUX3,
    SADC_AUX4,
    SADC_AUX5,
};

/**
 * @brief SADC 参考电压
 * @attention 驱动默认设置VREF, 用户不需要配置VREF
 */
enum IHAL_SADC_VREF {
    SADC_AUX_VREF_1V8,
    SADC_AUX_VREF_3V3,
};

typedef void IHAL_SADC_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_SADC_API API定义
 * @{
 */

/**
 * @brief  SADC 通道初始化
 * @param  [in] channel : SADC_AUX0 ~ 5
 * @retval IHAL_SADC_Handle_t *  成功
 * @retval IHAL_RNULL            失败
 */
IHAL_SADC_Handle_t *IHal_SADC_Init_Channel(const enum IHAL_SADC_CHANNEL channel);

/**
 * @brief SADC 通道反初始化
 * @param [in] handle : SADC handle
 * @retval 0             成功
 * @retval 非0           失败
 */
IHAL_INT32 IHal_SADC_DeInit_Channel(IHAL_SADC_Handle_t *handle);

/**
 * @brief SADC 读取通道电压 (ADC采集转换后的电压)
 * @param [in] handle : SADC handle
 * @retval data     电压 (mv)
 * @retval -1       失败
 * @attention 每调用一次该函数。驱动内部：ADC立即采集、转换, 等待ADC转化完成, ADC利用装换完成中断返回数据。驱动内部采集已是最大效率
 */
IHAL_INT32 IHal_SADC_Read_Voltage_Data(IHAL_SADC_Handle_t *handle);

/**
 * @brief SADC 设置参考电压
 * @param [in] handle : SADC handle
 * @param [in] verf   : SADC_AUX_VREF_1V8 / SADC_AUX_VREF_3V3
 * @retval 0        成功
 * @retval 非0      失败
 * @attention 驱动默认设置, 用户不需要配置VREF. 此接口留给用户, 在某些情况下, 实现可设置VREF. 其中设置的VREF, ADC的所有通道共享
 */
IHAL_INT32 IHal_SADC_Set_Vref(IHAL_SADC_Handle_t *handle, const enum IHAL_SADC_VREF vref);
/**
 * @}
 */

/**
 * @}
 */
#endif //__HAL_SADC_H__
