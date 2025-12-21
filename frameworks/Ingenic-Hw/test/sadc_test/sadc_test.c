/**
 * @file    sadc_test.c
 * @author  MPU系统软件部团队
 * @brief   包含 SADC HAL 库函数sadc_test测试的源文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include "ihal_config.h"
#include "ihal.h"

int main()
{
    int ret = -1, data = 0, sum = 0;
    float average_data = 0;
    /* 初始化SADC通道aux0 */
    IHAL_SADC_Handle_t *sadc_handle = IHal_SADC_Init_Channel(SADC_AUX0);
    if (!sadc_handle) {
        printf("Init gpio fail!\n");
    }
    /* 读取通道电压 */
    data = IHal_SADC_Read_Voltage_Data(sadc_handle);
    if (data < 0) {
        printf("set out direction fail!\n");
        goto deinit;
    }
    printf("aux0 read voltage data:%d mv\n", data);
    /* 连续读取10次取平均值 */
    for (int i = 0; i < 10; i++) {
        data = IHal_SADC_Read_Voltage_Data(sadc_handle);
        if (data < 0) {
            printf("set out direction fail!\n");
            goto deinit;
        }
        printf("aux0 read voltage data:%d mv\n", data);
        sum += data;
    }
    average_data = sum / 10;
    printf("aux0 read 10 times, average data:%.2f mv\n", average_data);
    /* 反初始化通道, 回收资源 */
    ret = IHal_SADC_DeInit_Channel(sadc_handle);
    if (ret < 0) {
        printf("DeInit sadc fail\n");
    }
    return 0;

deinit:
    /* 反初始化通道, 回收资源 */
    ret = IHal_SADC_DeInit_Channel(sadc_handle);
    if (ret < 0) {
        printf("DeInit sadc fail\n");
    }
    return 0;
}