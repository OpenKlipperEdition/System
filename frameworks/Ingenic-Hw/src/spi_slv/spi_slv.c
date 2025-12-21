/**
 * @file    spi_slv.c
 * @author  MPU系统软件部团队
 * @brief   包含 SPI_SLV HAL 库函数原型的头文件
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include  <linux/spi/spidev.h>
#include <assert.h>
#include "spi_slv.h"
#include "hw_log.h"
#define TAG "SPI_SLV"

typedef struct {
    IHAL_INT8 *dev_node; // spi_slv设备节点路径名
    enum SLV_CAN_DMA can_dma; // 是否使用dma
    enum SPI_SLV_MODE spi_slv_mode; // spi_slv相位和极性模式
    struct spi_ioc_transfer transfer; // 传输消息结构体
} IHal_SPI_SLV_Handle;

IHal_SPI_SLV_Handle_t *IHal_SPI_SLV_Init(IHal_SPI_SLV_attr *attr)
{
    IHal_SPI_SLV_Handle *handle = NULL;
    handle = (IHal_SPI_SLV_Handle *)calloc(1, sizeof(IHal_SPI_SLV_Handle));
    if (handle == NULL) {
        IHAL_LOGE(TAG, "Failed to request memory!");
        return NULL;
    }
    handle->dev_node = attr->dev_node;
    handle->can_dma = attr->can_dma;
    handle->spi_slv_mode = attr->spi_slv_mode;
    handle->transfer.bits_per_word = attr->bits_per_word;
    return handle;
}
IHAL_INT32 IHal_SPI_SLV_Transfer(IHal_SPI_SLV_Handle_t *handle, IHAL_INT8 *send_data, IHAL_INT8 *receiv_data, IHAL_UINT32 len)
{
    assert(handle);
    assert(send_data);
    assert(receiv_data);
    if (len <= 0) {
        IHAL_LOGE(TAG, "The len of transfer data error!");
        return -1;
    }
    IHAL_INT32 fd = 0, ret = 0;
    IHal_SPI_SLV_Handle *spi_slv_handle = (IHal_SPI_SLV_Handle *)handle;
    spi_slv_handle->transfer.tx_buf = send_data;
    spi_slv_handle->transfer.rx_buf = receiv_data;
    spi_slv_handle->transfer.len = len;
    fd = open(spi_slv_handle->dev_node, O_RDWR);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Failed to open spi_slv device node!");
        return -1;
    }
    ret = ioctl(fd, spi_slv_handle->spi_slv_mode, NULL);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to set spi_slv mode!");
        return -1;
    }
    ret = ioctl(fd, spi_slv_handle->can_dma, (unsigned long)&spi_slv_handle->transfer);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to transfer!");
        return -1;
    }
    if (-1 == close(fd)) {
        IHAL_LOGE(TAG, "Failed to close spi_slv device node!");
        return -1;
    }
    return 0;
}
IHAL_INT32 IHal_SPI_SLV_SendData(IHal_SPI_SLV_Handle_t *handle, IHAL_INT8 *send_data, IHAL_UINT32 len)
{
    assert(handle);
    assert(send_data);
    if (len <= 0) {
        IHAL_LOGE(TAG, "The len of send data error!");
        return -1;
    }
    IHAL_INT32 fd = 0, ret = 0;
    IHal_SPI_SLV_Handle *spi_slv_handle = (IHal_SPI_SLV_Handle *)handle;
    spi_slv_handle->transfer.tx_buf = send_data;
    spi_slv_handle->transfer.rx_buf = NULL;
    spi_slv_handle->transfer.len = len;
    fd = open(spi_slv_handle->dev_node, O_RDWR);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Failed to open spi_slv device node!");
        return -1;
    }
    ret = ioctl(fd, spi_slv_handle->spi_slv_mode, NULL);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to set spi_slv mode!");
        return -1;
    }
    ret = ioctl(fd, spi_slv_handle->can_dma, (unsigned long)&spi_slv_handle->transfer);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to send data!");
        return -1;
    }
    if (-1 == close(fd)) {
        IHAL_LOGE(TAG, "Failed to close spi_slv device node!");
        return -1;
    }
    return 0;
}
IHAL_INT32 IHal_SPI_SLV_ReceivData(IHal_SPI_SLV_Handle_t *handle, IHAL_INT8 *receiv_data, IHAL_UINT32 len)
{
    assert(handle);
    assert(receiv_data);
    if (len <= 0) {
        IHAL_LOGE(TAG, "The len of receiv data error!");
        return -1;
    }
    IHAL_INT32 fd = 0, ret = 0;
    IHal_SPI_SLV_Handle *spi_slv_handle = (IHal_SPI_SLV_Handle *)handle;
    spi_slv_handle->transfer.tx_buf = NULL;
    spi_slv_handle->transfer.rx_buf = receiv_data;
    spi_slv_handle->transfer.len = len;
    fd = open(spi_slv_handle->dev_node, O_RDWR);
    if (-1 == fd) {
        IHAL_LOGE(TAG, "Failed to open spi_slv device node!");
        return -1;
    }
    ret = ioctl(fd, spi_slv_handle->spi_slv_mode, NULL);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to set spi_slv mode!");
        return -1;
    }
    ret = ioctl(fd, spi_slv_handle->can_dma, (unsigned long)&spi_slv_handle->transfer);
    if (-1 == ret) {
        IHAL_LOGE(TAG, "Failed to receiv data!");
        return -1;
    }
    if (-1 == close(fd)) {
        IHAL_LOGE(TAG, "Failed to close spi_slv device node!");
        return -1;
    }
    return 0;
}
IHAL_INT32 IHal_SPI_SLV_DeInit(IHal_SPI_SLV_Handle_t *handle)
{
    assert(handle);
    free(handle);
    return 0;
}
