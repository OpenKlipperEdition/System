/**
 * @file    iio_sadc_test.c
 * @author  MPU系统软件部团队
 * @brief   包含 IIO_SADC HAL 库函数iio_sadc_test测试的源文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ihal_config.h"
#include "ihal.h"

//adc方式1 单通道采集
void sample_way1()
{
    int raw_data = 0;
    int ret = -1;
    /* 初始化IIO_SADC通道aux0 */
    IHAL_IIO_SADC_Handle_t *iio_sadc_handle = IHal_IIO_SADC_Init_Channel(IIO_SADC_AUX0);
    if (!iio_sadc_handle) {
        printf("Init iio_sadc aux0 fail!\n");
    }
    /* 读取通道原始数据、获取转化电压 */
    raw_data = IHal_IIO_SADC_Read_Raw_Data(iio_sadc_handle);
    if (raw_data < 0) {
        printf("read raw data fail!\n");
        goto deinit;
    }
    printf("aux0 raw_data:%d,voltage %.2f mv\n", raw_data, raw_data * 1800.0 / 4096); /*1800为参考电压, 4096为ADC12位*/

deinit:
    /* 反初始化通道, 回收资源 */
    ret = IHal_IIO_SADC_DeInit_Channel(iio_sadc_handle);
    if (ret < 0) {
        printf("DeInit iio_sadc fail\n");
    }
}

//adc方式2 多通道扫描采集
void sample_way2()
{
    int ret = -1;
    /* 初始化IIO_SADC trigger */
    IHAL_IIO_SADC_TRIGGER_Handle_t *iio_sadc_trigger_handle = IHal_IIO_SADC_TRIGGER_Init();
    if (!iio_sadc_trigger_handle) {
        printf("Init iio sadc tigger fail!\n");
    }
    /* enable 通道 aux0-1 */
    ret = IHal_IIO_SADC_TRIGGER_Enable_Channel(iio_sadc_trigger_handle, IIO_SADC_AUX0);
    if (ret < 0) {
        printf("enable aux0 channel fail/n");
        goto deinit;
    }
    ret = IHal_IIO_SADC_TRIGGER_Enable_Channel(iio_sadc_trigger_handle, IIO_SADC_AUX1);
    if (ret < 0) {
        printf("enable aux1 channel fail/n");
        goto deinit;
    }
    /* 设置序列（enable channel）buff的长度 2*/
    /* buff 满的时候可以read 2次, 每次都有使能通道 aux0-1 的数据 */
    IHal_IIO_SADC_TRIGGER_Set_Sequence_Buff_Length(iio_sadc_trigger_handle, 2);
    if (ret < 0) {
        printf("set buff length fail/n");
        goto deinit;
    }
    /* 开始扫描 */
    ret = IHal_IIO_SADC_TRIGGER_Start_Sample(iio_sadc_trigger_handle);
    if (ret < 0) {
        printf("start adc fail/n");
        goto deinit;
    }
    /* adc 采集大概2s后关闭, 扫描的通道数据放在会放在buff, buff会填满*/
    sleep(2);
    /* 停止扫描 */
    IHal_IIO_SADC_TRIGGER_Stop_Sample(iio_sadc_trigger_handle);
    if (ret < 0) {
        printf("stop adc fail/n");
        goto deinit;
    }
    /* 获取buff通道数据 */
    /* 设置序列（enable channel）buff的长度 2*/
    /* buff 满的时候可以read 2次, 每次都有使能通道 aux0-1 的数据 */
    IIO_SADC_CHANNEL_DATA data1, data2; // iio_sadc.h 定义结构体

    /*data1有2个通道的原始数据 aux0-1. 打印通道原始数据*/
    IHal_IIO_SADC_TRIGGER_Read_Sequence_Buff_Data(iio_sadc_trigger_handle, &data1);
    printf("aux0 rawdata:%d\n", data1.channel_data[IIO_SADC_AUX0]);
    printf("aux1 rawdata:%d\n", data1.channel_data[IIO_SADC_AUX1]);

    /*data2有2个通道的原始数据 aux0-1. 打印通道原始数据*/
    IHal_IIO_SADC_TRIGGER_Read_Sequence_Buff_Data(iio_sadc_trigger_handle, &data2);
    printf("aux0 rawdata:%d\n", data2.channel_data[IIO_SADC_AUX0]);
    printf("aux1 rawdata:%d\n", data2.channel_data[IIO_SADC_AUX1]);

deinit:
    /* 反初始化 trigger, 回收资源 */
    ret = IHal_IIO_SADC_TRIGGER_DeInit(iio_sadc_trigger_handle);
    if (ret < 0) {
        printf("DeInit iio_sadc trigger fail\n");
    }
}

int main()
{
    sample_way1();  //adc方式1 单通道采集
    printf("-----------------------------\n");
    sample_way2();  //adc方式2 多通道扫描采集
    return 0;
}