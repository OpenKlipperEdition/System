/**
 * @file    i2c.c
 * @author  MPU系统软件部团队
 * @brief   包含 I2C HAL 库函数原型的头文件
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <assert.h>
#include <linux/i2c.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "i2c.h"
#include "hw_log.h"

#define TAG "i2c"

typedef struct {
    IHAL_INT32 fd;          //文件描述符
    pthread_mutex_t i2c_mutex;  //互斥锁
} IHal_I2C_Handle;

IHal_I2C_Handle_t *IHal_I2C_Init(const IHAL_INT8 *devname)
{
    assert(devname);

    IHal_I2C_Handle *I2C_Handle = (IHal_I2C_Handle *)malloc(sizeof(IHal_I2C_Handle));
    if (!I2C_Handle) {
        IHAL_LOGE(TAG, "Malloc error");
        return NULL;
    }
    I2C_Handle->fd = open(devname, O_RDWR);
    if (I2C_Handle->fd == -1) {
        IHAL_LOGE(TAG, "Open dev failed");
        free(I2C_Handle);
        return NULL;
    }
    pthread_mutex_init(&I2C_Handle->i2c_mutex, NULL);
    return (IHal_I2C_Handle_t *)I2C_Handle;
}
IHAL_INT32 IHal_I2C_Deinit(IHal_I2C_Handle_t *I2C_Handle)
{
    assert(I2C_Handle);
    IHal_I2C_Handle *handle = (IHal_I2C_Handle *)I2C_Handle;
    assert(handle->fd > 0);

    if (close(handle->fd) == -1) {
        IHAL_LOGE(TAG, "Close dev failed");
        return -ERROR;
    }
    pthread_mutex_destroy(&handle->i2c_mutex);
    free(I2C_Handle);
    return 0;
}
size_t IHal_I2C_WriteData(IHal_I2C_Handle_t *I2C_Handle, IHal_I2C_Msg_t *I2C_Msg)
{
    assert(I2C_Handle);
    IHal_I2C_Handle *handle = (IHal_I2C_Handle *)I2C_Handle;
    assert(handle->fd > 0);
    assert(I2C_Msg->SlaveAddr > 0);
    assert(I2C_Msg->AddrLength > 0 && I2C_Msg->AddrLength <= 64);
    assert(I2C_Msg->tx_buf != NULL);
    assert(I2C_Msg->tx_len > 0);

    struct i2c_rdwr_ioctl_data rdwr_data;
    IHAL_INT32 offset = I2C_Msg->AddrLength / 8;  //地址长度占多少个字节
    struct i2c_msg messages[1];
    pthread_mutex_lock(&handle->i2c_mutex);
    rdwr_data.msgs = messages;
    rdwr_data.nmsgs = 1;

    messages[0].addr = I2C_Msg->SlaveAddr; // 从设备地址
    messages[0].flags = 0; // 写入模式
    messages[0].buf = I2C_Msg->tx_buf;
    messages[0].len = I2C_Msg->tx_len;

    if (ioctl(handle->fd, I2C_RDWR, &rdwr_data) < 0) {
        IHAL_LOGE(TAG, "Sending I2C write request error");
        pthread_mutex_unlock(&handle->i2c_mutex);
        return -ERROR;
    }
    pthread_mutex_unlock(&handle->i2c_mutex);

    return messages[0].len - offset;
}
size_t IHal_I2C_ReadData(IHal_I2C_Handle_t *I2C_Handle, IHal_I2C_Msg_t *I2C_Msg)
{
    assert(I2C_Handle);
    IHal_I2C_Handle *handle = (IHal_I2C_Handle *)I2C_Handle;
    assert(handle->fd > 0);
    assert(I2C_Msg->SlaveAddr > 0);
    assert(I2C_Msg->AddrLength > 0 && I2C_Msg->AddrLength <= 64);
    assert(I2C_Msg->tx_buf != NULL);
    assert(I2C_Msg->tx_len > 0);
    assert(I2C_Msg->ReadAddr > 0);
    assert(I2C_Msg->rx_buf != NULL);
    assert(I2C_Msg->rx_len > 0);

    IHAL_UINT8 addr_buf[10] = {0};
    struct i2c_rdwr_ioctl_data rdwr_data;
    pthread_mutex_lock(&(handle->i2c_mutex));
    struct i2c_msg messages[2];
    rdwr_data.msgs = messages;
    rdwr_data.nmsgs = 2;

    messages[0].addr = I2C_Msg->SlaveAddr; // 从设备地址
    messages[0].flags = 0; // 写入模式
    for (IHAL_INT32 i = 0; i < (I2C_Msg->AddrLength)  / 8; i++) {
        addr_buf[i] = (I2C_Msg->ReadAddr >> (8 * ((I2C_Msg->AddrLength)  / 8 - i - 1))) & 0xFF;
    }
    messages[0].buf = addr_buf;
    messages[0].len = (I2C_Msg->AddrLength)  / 8 ;
    messages[1].addr = I2C_Msg->SlaveAddr; // 从设备地址
    messages[1].flags = I2C_M_RD; // 读取模式
    messages[1].len = I2C_Msg->rx_len; // 接收数据长度
    messages[1].buf = I2C_Msg->rx_buf; // 接收数据缓冲区

    if (ioctl(handle->fd, I2C_RDWR, &rdwr_data) < 0) {
        IHAL_LOGE(TAG, "Sending I2C read request error");
        pthread_mutex_unlock(&handle->i2c_mutex);
        return -ERROR;
    }
    pthread_mutex_unlock(&(handle->i2c_mutex));

    return messages[1].len;
}
