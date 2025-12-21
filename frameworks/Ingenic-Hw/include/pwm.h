/**
 * @file    pwm.h
 * @author  MPU系统软件部团队
 * @brief   包含 PWM HAL 库函数原型的头文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __PWM_H__
#define __PWM_H__
#include "ihal_info.h"
#include "ihal.h"

/**
 * @defgroup group_PWM PWM模块
 * @{
 */

/**
 * @addtogroup group_PWM_data_type 数据类型定义
 * @{
 */

typedef void IHal_PWM_Handle_t;

/**
 * @brief PWM初始电平
 */
enum Pwm_Polarity {
    LOW,
    HIGH,
};

/**
 * @brief PWM通道使能状态
 */
enum Pwm_Enable {
    DISABLE,
    ENABLE,
};

/**
 * @}
 */

/**
 * @addtogroup group_PWM_API API定义
 * @{
 */

/**
 * @brief  申请pwm通道
 * @param  [in] pwm_chan : PWM 通道号
 * @retval 所申请的通道句柄 成功
 * @retval NULL             失败
 */
IHal_PWM_Handle_t *IHal_PWM_RequestChan(IHAL_UINT32 pwm_chan);

/**
 * @brief  设置pwm初始电平
 * @param  [in] pwm_chan : 所申请的通道句柄指针
 * @param  [in] polarity : 初始电平值(LOW/HIGH)
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_PWM_SetPolarity(IHal_PWM_Handle_t *handle, enum Pwm_Polarity polarity);

/**
 * @brief  设置PWM周期
 * @param  [in] pwm_chan : 所申请的通道句柄指针
 * @param  [in] period   : 周期(单位ns)
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_PWM_SetPeriod(IHal_PWM_Handle_t *handle, IHAL_UINT32 period);

/**
 * @brief  设置PWM占空比
 * @param  [in] pwm_chan   : 所申请的通道句柄指针
 * @param  [in] duty_cycle : 占空比(单位ns)
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_PWM_SetDutyCycle(IHal_PWM_Handle_t *handle, IHAL_UINT32 duty_cycle);

/**
 * @brief  使能或关闭已申请通道
 * @param  [in] pwm_chan   : 所申请的通道句柄指针
 * @param  [in] pwm_enable : ENABLE(使能)/DISABLE(关闭)
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_PWM_EnableChan(IHal_PWM_Handle_t *handle, enum Pwm_Enable pwm_enable);

/**
 * @brief  释放所选通道
 * @param  [in] pwm_chan   : 将要释放的通道句柄指针
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_PWM_FreeChan(IHal_PWM_Handle_t *handle);

/**
 * @}
 */

/**
 * @}
 */
#endif // __INGENIC_PWM_H
