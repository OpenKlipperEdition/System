/**
 * @file    sadc.c
 * @author  MPU系统软件部团队
 * @brief   包含 SADC HAL 库函数原型的源文件
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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>
#include "sadc.h"
#include "hw_log.h"

#define TAG                 "SADC"
#define DEV_SADC_DIR        "/dev/ingenic_adc_aux_"
#define BUF_LEN             40
#define ADC_MAGIC_NUMBER    'A'
// #define ADC_ENABLE      _IO(ADC_MAGIC_NUMBER, 11)
// #define ADC_DISABLE     _IO(ADC_MAGIC_NUMBER, 22)
#define ADC_SET_VREF       _IOW(ADC_MAGIC_NUMBER, 33, unsigned int)

typedef struct {
    int sadc_channel_fd;
    pthread_mutex_t lock;
} sadc_prv_ctx;

IHAL_SADC_Handle_t *IHal_SADC_Init_Channel(const enum IHAL_SADC_CHANNEL channel)
{
    assert(channel == SADC_AUX0 || channel == SADC_AUX1 || channel == SADC_AUX2 || channel == SADC_AUX3 || channel == SADC_AUX4 || channel == SADC_AUX5);
    char cmd_buf[BUF_LEN] = {0};
    sadc_prv_ctx *ctx = (sadc_prv_ctx *)malloc(sizeof(sadc_prv_ctx));
    if (!ctx) {
        IHAL_LOGE(TAG, "malloc sadc ctx failed\n");
        return IHAL_RNULL;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s%d", DEV_SADC_DIR, channel);
    ctx->sadc_channel_fd = open(cmd_buf, O_RDWR);
    if (ctx->sadc_channel_fd < 0) {
        IHAL_LOGE(TAG, "init sadc channle fail, open file(%s) fail\n", cmd_buf);
        free(ctx);
        return IHAL_RNULL;
    }
    pthread_mutex_init(&ctx->lock, NULL);
    return (IHAL_SADC_Handle_t *)ctx;
}
IHAL_INT32 IHal_SADC_DeInit_Channel(IHAL_SADC_Handle_t *handle)
{
    assert(handle);
    sadc_prv_ctx *ctx = (sadc_prv_ctx *)handle;
    close(ctx->sadc_channel_fd);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    return IHAL_ROK;
}
IHAL_INT32 IHal_SADC_Read_Voltage_Data(IHAL_SADC_Handle_t *handle)
{
    assert(handle);
    int ret = -1;
    int data = 0;
    sadc_prv_ctx *ctx = (sadc_prv_ctx *)handle;
    ret = read(ctx->sadc_channel_fd, (char *)&data, sizeof(data));
    if (ret < 0) {
        IHAL_LOGE(TAG, "read channle data fail!\n");
        return -IHAL_RERR;
    }
    return data;
}
IHAL_INT32 IHal_SADC_Set_Vref(IHAL_SADC_Handle_t *handle, const enum IHAL_SADC_VREF vref)
{
    assert(handle);
    assert(vref == SADC_AUX_VREF_1V8 || vref == SADC_AUX_VREF_3V3);
    unsigned int data;
    int ret = -1;
    sadc_prv_ctx *ctx = (sadc_prv_ctx *)handle;
    if (vref == SADC_AUX_VREF_1V8) {
        data = 1800;
    } else if (vref == SADC_AUX_VREF_3V3) {
        data = 3300;
    }
    pthread_mutex_lock(&ctx->lock);
    ret = ioctl(ctx->sadc_channel_fd, ADC_SET_VREF, &data);
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "set sadc vref fail!\n");
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    return IHAL_ROK;
}
