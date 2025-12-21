/**
 * @file    gpio.h
 * @author  MPU系统软件部团队
 * @brief   包含 GPIO HAL 库函数原型的头文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__
#include "ihal_info.h"

/**
 * @defgroup group_GPIO GPIO模块
 * @{
 */

/**
 * @addtogroup group_GPIO_data_type 数据类型定义
 * @{
 */

/**
 * @brief GPIO输入输出
 */
enum IHAL_GPIO_DIRECTION {
    GPIO_IN,        /*!< 输入 */
    GPIO_OUT,       /*!< 输出 */
};

/**
 * @brief GPIO高低电平
 */
enum IHAL_GPIO_VALUE {
    GPIO_LOW,       /*!< 低电平 */
    GPIO_HIGH,      /*!< 高电平 */
};

/**
 * @brief GPIO中断触发方式
 */
enum IHAL_GPIO_INTERRUPT {
    GPIO_NONE,      /*!< 无 */
    GPIO_RISING,    /*!< 上升沿 */
    GPIO_FALLING,   /*!< 下降沿 */
    GPIO_BOTH,      /*!< 双边沿 */
};

/**
* @brief 中断一直探测
*/
#define ALWAYS -1

/**
 * @brief GPIO极性模式
 */
enum IHAL_GPIO_POLARITY {
    GPIO_ACTIVE_LOW,  /*!< 正常极性，输出/高电平即为高电平，输出低电平即为低电平*/
    GPIO_ACTIVE_HIGH, /*!< 反向极性，输出高电平实际输出低电平，输出低电平实际为高电平 */
    /* 对输入模式同样可使用 */
};

/**
 * @brief GPIO端口种类
 */
enum IHAL_GPIO_GROUP {
    GPIOA = 0,
    GPIOB = 32,
    GPIOC = 64,
    GPIOD = 96,
    GPIOE = 128,
};

typedef void IHAL_GPIO_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_GPIO_API API定义
 * @{
 */


/**
 * @brief GPIO初始化
 * @param [in] group  : 端口名称    GPIOA~E
 * @param [in] pin    : 端口编号    0 ~ 31
 * @retval IHAL_GPIO_Handle_t *     成功
 * @retval IHAL_RNULL               失败
 */
IHAL_GPIO_Handle_t *IHal_GPIO_Init(const enum IHAL_GPIO_GROUP group, IHAL_INT16 pin);

/**
 * @brief GPIO反初始化
 * @param [in] handle : GPIO handle
 * @retval 0        成功
 * @retval 非0     失败
 */
IHAL_INT32 IHal_GPIO_DeInit(IHAL_GPIO_Handle_t *handle);

/**
 * @brief GPIO设置输入/输出
 * @param [in] handle    : GPIO handle
 * @param [in] direction : GPIO_IN / GPIO_OUT
 * @retval 0        成功
 * @retval 非0     失败
 */
IHAL_INT32 IHal_GPIO_Set_Direciton(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_DIRECTION direction);

/**
 * @brief GPIO设置电平
 * @param [in] handle  : GPIO handle
 * @param [in] value   :  GPIO_HIGH / GPIO_LOW
 * @retval 0        成功
 * @retval 非0     失败
 */
IHAL_INT32 IHal_GPIO_Set_Value(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_VALUE value);

/**
 * @brief GPIO获取电平
 * @param [in] handle : GPIO handle
 * @retval 0        低电平
 * @retval 1        高电平
 * @retval -1       失败
 */
IHAL_INT32 IHal_GPIO_Get_Value(IHAL_GPIO_Handle_t *handle);

/**
 * @brief GPIO翻转电平
 * @param [in] handle : GPIO handle
 * @retval 0        成功
 * @retval 非0     失败
 */
IHAL_INT32 IHal_GPIO_Toggle_Value(IHAL_GPIO_Handle_t *handle);

/**
 * @brief GPIO设置中断触发方式
 * @param [in] handle     : GPIO handle
 * @param [in] interrupt  : GPIO_NONE / GPIO_RISING / GPIO_FALLING / GPIO_BOTH
 * @retval 0        成功
 * @retval 非0     失败
 * @attention GPIO作为输入的前提, 设置中断触发
 */
IHAL_INT32 IHal_GPIO_Set_Interrupt_mode(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_INTERRUPT interrupt);

/**
 * @brief GPIO中断监测
 * @param [in] handle     : GPIO handle
 * @param [in] callfunc() : 函数指针。中断处理函数
 * @param [in] count      : ALWAYS (一直触发) / count(中断触发次数,如0,1, 2, 3, 4 ...)
 * @attention poll函数阻塞, 如有gpio中断, 立即触发
 */
void IHal_GPIO_Interrupt_Detect(IHAL_GPIO_Handle_t *handle, void (*callfunc)(), const int count);

/**
 * @brief GPIO设置极性
 * @param [in] handle   : GPIO handle
 * @param [in] polarity : GPIO_ACTIVE_LOW / GPIO_ACTIVE_HIGH
 * @retval 0        成功
 * @retval 非0     失败
 * @attention 默认GPIO_ACTIVE_LOW(正向极性) 一般无需改动. GPIO_ACTIVE_HIGH（反向极性）
 */
IHAL_INT32 IHal_GPIO_Set_Polarity(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_POLARITY polarity);

/**
 * @}
 */

/**
 * @}
 */
#endif //__HAL_GPIO_H__