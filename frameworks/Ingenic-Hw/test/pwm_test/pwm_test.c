/**
 * @file    pwm.c
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

#include <stdio.h>
#include <stdlib.h>
#include "ihal_config.h"
#include "ihal.h"

int main(int argc, char *argv[])
{
    int ret = 0;
    IHal_PWM_Handle_t *handle1 = NULL;
    // 申请通道
    handle1 = IHal_PWM_RequestChan(1);
    if (NULL == handle1) {
        printf("Failed to apply for channel!\n");
        return -1;
    }
    // 设置周期
    ret = IHal_PWM_SetPeriod(handle1, 100000);
    if (0 != ret) {
        printf("Failed to set period!\n");
        return -1;
    }
    // 设置占空比
    ret = IHal_PWM_SetDutyCycle(handle1, 50000);
    if (0 != ret) {
        printf("Failed to set duty_cycle!\n");
        return -1;
    }
    // 设置pwm初始电平
    ret = IHal_PWM_SetPolarity(handle1, HIGH);
    if (0 != ret) {
        printf("Failed to set polarity!\n");
        return -1;
    }
    // 使能pwm通道
    ret = IHal_PWM_EnableChan(handle1, ENABLE);
    if (0 != ret) {
        printf("Failed to enabled that channel!\n");
        return -1;
    }

#if 1
    // 关闭pwm通道
    ret = IHal_PWM_EnableChan(handle1, DISABLE);
    if (0 != ret) {
        printf("Failed to disabled that channel!\n");
        return -1;
    }
    // 释放通道
    IHal_PWM_FreeChan(handle1);
    if (0 != ret) {
        printf("Failed to free that channel!\n");
        return -1;
    }
#endif

#if 1
    IHal_PWM_Handle_t *handle2 = NULL;
    // 申请通道
    handle2 = IHal_PWM_RequestChan(2);
    if (NULL == handle2) {
        printf("Failed to apply for channel!\n");
        return -1;
    }
    // 设置周期
    ret = IHal_PWM_SetPeriod(handle2, 100000);
    if (0 != ret) {
        printf("Failed to set period!\n");
        return -1;
    }
    // 设置占空比
    ret = IHal_PWM_SetDutyCycle(handle2, 50000);
    if (0 != ret) {
        printf("Failed to set duty_cycle!\n");
        return -1;
    }
    // 设置pwm初始电平
    ret = IHal_PWM_SetPolarity(handle2, LOW);
    if (0 != ret) {
        printf("Failed to set polarity!\n");
        return -1;
    }
    // 使能pwm通道
    ret = IHal_PWM_EnableChan(handle2, ENABLE);
    if (0 != ret) {
        printf("Failed to enabled that channel!\n");
        return -1;
    }

#if 1
    // 关闭pwm通道
    ret = IHal_PWM_EnableChan(handle2, DISABLE);
    if (0 != ret) {
        printf("Failed to disabled that channel!\n");
        return -1;
    }
    // 释放通道
    IHal_PWM_FreeChan(handle2);
    if (0 != ret) {
        printf("Failed to free that channel!\n");
        return -1;
    }
#endif
#endif
    return 0;
}
