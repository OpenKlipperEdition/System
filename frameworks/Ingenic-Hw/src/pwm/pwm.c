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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "pwm.h"

#define NODE_PATH "/sys/class/pwm/pwmchip0"
#define CMD_BUFFER_MAX_LEN 60
#define TAG "PWM"

typedef struct {
    IHAL_UINT32 pwm_chan;
} IHal_PWM_Info;

IHal_PWM_Handle_t *IHal_PWM_RequestChan(IHAL_UINT32 pwm_chan)
{
    int fd = 0;
    int ret = 0;
    int len = 0;
    char cmd_buf[CMD_BUFFER_MAX_LEN] = {0};
    /* 获取通道数 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/npwm", NODE_PATH);
    fd = open(cmd_buf, O_RDONLY);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Open npwm node error!");
        return NULL;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    ret = read(fd, cmd_buf, sizeof(cmd_buf));
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to obtain number of channels!");
        close(fd);
        return NULL;
    }
    int channel_sum = atoi(cmd_buf);
    close(fd);

    /* 判断所申请通道是否存在 */
    if (pwm_chan < 0 || pwm_chan >= channel_sum) {
        IHAL_LOGE(TAG, "The requested channel does not exist!");
        return NULL;
    }

    /* 申请pwm通道 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/export", NODE_PATH);
    fd = open(cmd_buf, O_WRONLY);
    if (fd == -1) {
        IHAL_LOGE(TAG, "Open export node error!");
        return NULL;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    len = snprintf(cmd_buf, sizeof(cmd_buf), "%u", pwm_chan);
    ret = write(fd, cmd_buf, len);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to apply for channel!");
        close(fd);
        return NULL;
    }
    close(fd);
    IHal_PWM_Info *info = (IHal_PWM_Info *)malloc(sizeof(IHal_PWM_Info));
    info->pwm_chan = pwm_chan;
    return (IHal_PWM_Handle_t *)info;
}

IHAL_INT32 IHal_PWM_SetPolarity(IHal_PWM_Handle_t *handle, enum Pwm_Polarity polarity)
{
    assert(handle);
    IHal_PWM_Info hd = *(IHal_PWM_Info *)handle;

    int fd = 0;
    int ret = 0;
    int len = 0;
    char cmd_buf[CMD_BUFFER_MAX_LEN] = {0};
    /* 打开所申请的通道文件 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/pwm%u/polarity", NODE_PATH, hd.pwm_chan);
    fd = open(cmd_buf, O_WRONLY);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Open polarity node error!");
        return -1;
    }

    /* 设置初始电平 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    if (HIGH == polarity) {
        len = snprintf(cmd_buf, sizeof(cmd_buf), "normal");
    } else {
        len = snprintf(cmd_buf, sizeof(cmd_buf), "inversed");
    }
    ret = write(fd, cmd_buf, len);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to set polarity!");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

IHAL_INT32 IHal_PWM_SetPeriod(IHal_PWM_Handle_t *handle, IHAL_UINT32 period)
{
    assert(handle);
    IHal_PWM_Info hd = *(IHal_PWM_Info *)handle;

    int fd = 0;
    int ret = 0;
    int len = 0;
    char cmd_buf[CMD_BUFFER_MAX_LEN] = {0};
    /* 打开所申请通道文件 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/pwm%u/period", NODE_PATH, hd.pwm_chan);
    fd = open(cmd_buf, O_WRONLY);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Open period node error!");
        return -1;
    }

    /* 设置周期 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    len = snprintf(cmd_buf, sizeof(cmd_buf), "%u", period);
    ret = write(fd, cmd_buf, len);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to set period!");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

IHAL_INT32 IHal_PWM_SetDutyCycle(IHal_PWM_Handle_t *handle, IHAL_UINT32 duty_cycle)
{
    assert(handle);
    IHal_PWM_Info hd = *(IHal_PWM_Info *)handle;

    int fd = 0;
    int ret = 0;
    int len = 0;
    char cmd_buf[CMD_BUFFER_MAX_LEN] = {0};
    /* 打开所申请的通道文件 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/pwm%u/duty_cycle", NODE_PATH, hd.pwm_chan);
    fd = open(cmd_buf, O_WRONLY);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Open duty_cycle node error!");
        return -1;
    }

    /* 设置占空比 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    len = snprintf(cmd_buf, sizeof(cmd_buf), "%u", duty_cycle);
    ret = write(fd, cmd_buf, len);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to set duty_cycle!");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

IHAL_INT32 IHal_PWM_EnableChan(IHal_PWM_Handle_t *handle, enum Pwm_Enable pwm_enable)
{
    assert(handle);
    IHal_PWM_Info hd = *(IHal_PWM_Info *)handle;

    int fd = 0;
    int ret = 0;
    int len = 0;
    char cmd_buf[CMD_BUFFER_MAX_LEN] = {0};
    /* 打开所申请的通道文件 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/pwm%u/enable", NODE_PATH, hd.pwm_chan);
    fd = open(cmd_buf, O_WRONLY);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Open enable node error!");
        return -1;
    }

    /* 开关pwm通道 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    len = snprintf(cmd_buf, sizeof(cmd_buf), "%u", pwm_enable);
    ret = write(fd, cmd_buf, len);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to enable or disable %u channel!", hd.pwm_chan);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

IHAL_INT32 IHal_PWM_FreeChan(IHal_PWM_Handle_t *handle)
{
    assert(handle);
    IHal_PWM_Info hd = *(IHal_PWM_Info *)handle;

    int fd = 0;
    int ret = 0;
    int len = 0;
    char cmd_buf[CMD_BUFFER_MAX_LEN] = {0};
    /* 打开unexport节点 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    snprintf(cmd_buf, sizeof(cmd_buf), "%s/unexport", NODE_PATH);
    fd = open(cmd_buf, O_WRONLY);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Open enable node error!");
        return -1;
    }

    /* 释放通道 */
    memset(cmd_buf, 0, sizeof(cmd_buf));
    len = snprintf(cmd_buf, sizeof(cmd_buf), "%u", hd.pwm_chan);
    ret = write(fd, cmd_buf, len);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to free %u channel!\n", hd.pwm_chan);
        close(fd);
        return -1;
    }
    close(fd);
    free(handle);
    return 0;
}
