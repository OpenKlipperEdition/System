/**
 * @file    spi.c
 * @author  MPU系统软件部团队
 * @brief   包含 SPI HAL 库函数原型的头文件
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include "hw_log.h"
#include "spi.h"

#define TAG "SPI"

typedef struct {
	IHAL_INT8 *dev_node; // 设备节点名
	struct spi_ioc_transfer transfer; // SPI_Master传输结构体
} IHal_SPI_Handle;

IHal_SPI_Handle_t *IHal_SPI_Init(IHal_SPI_attr attr)
{
	IHal_SPI_Handle *handle = NULL;
	handle = (IHal_SPI_Handle *)calloc(1, sizeof(IHal_SPI_Handle));
	if(handle == NULL){
		IHAL_LOGE(TAG, "Failed to request memory!");
		return NULL;
	}
	switch(attr.spi_mode){
		case CPOL0_CPHA0:
			attr.spi_mode |= 0;
			break;
		case CPOL0_CPHA1:
			attr.spi_mode |= SPI_CPHA;
			break;
		case CPOL1_CPHA0:
			attr.spi_mode |= SPI_CPOL;
			break;
		case CPOL1_CPHA1:
			attr.spi_mode |= SPI_CPHA | SPI_CPOL;
			break;
	};
	handle->dev_node = attr.dev_node;
	handle->transfer.speed_hz = attr.speed;
	handle->transfer.delay_usecs = attr.delay_usecs;
	handle->transfer.bits_per_word = attr.bits_per_word;
	handle->transfer.cs_change = attr.cs_change;
	handle->transfer.tx_nbits = attr.tx_nbits;
	handle->transfer.rx_nbits = attr.rx_nbits;
	IHAL_INT32 fd = open(handle->dev_node, O_RDWR);
	if(-1 == fd){
		IHAL_LOGE(TAG, "Failed to open spi master device node!");
		free(handle);
		return NULL;
	}
	IHAL_INT32 ret = ioctl(fd, SPI_IOC_WR_MODE32, &attr.spi_mode);
	if(-1 == ret){
		IHAL_LOGE(TAG, "Failed to set spi mode!");
		free(handle);
		return NULL;
	}
	if(-1 == close(fd)){
		IHAL_LOGE(TAG, "Failed to close spi master device node!");
		free(handle);
		return NULL;
	}
	return handle;
}

IHAL_INT32 IHal_SPI_Transfer(IHal_SPI_Handle_t *handle, IHAL_INT8 *send_data, IHAL_INT8 *receiv_data, IHAL_UINT32 len)
{
	assert(handle);
	assert(send_data);
	assert(receiv_data);
	if(len <= 0){
		IHAL_LOGE(TAG, "The len of transfer data error!");
		return -1;
	}
	IHal_SPI_Handle *spi_handle = (IHal_SPI_Handle *)handle;
	spi_handle->transfer.tx_buf = (unsigned long)send_data;
	spi_handle->transfer.rx_buf = (unsigned long)receiv_data;
	spi_handle->transfer.len = len;

	IHAL_INT32 fd = open(spi_handle->dev_node, O_RDWR);
	if(-1 == fd){
		IHAL_LOGE(TAG, "Failed to open spi master devive node!");
		return -1;
	}
	IHAL_UINT32 ret = ioctl(fd, SPI_IOC_MESSAGE(1), &spi_handle->transfer);
	if(ret < 0){
		IHAL_LOGE(TAG, "Failed to transfer data!");
		return -1;
	}
	if(-1 == close(fd)){
		IHAL_LOGE(TAG, "Failed to close spi master device node!");
		return -1;
	}
	return 0;
}

IHAL_INT32 IHal_SPI_SendData(IHal_SPI_Handle_t *handle, IHAL_INT8 *send_data, IHAL_UINT32 len)
{
	assert(handle);
	assert(send_data);
	if(len <= 0){
		IHAL_LOGE(TAG, "The len of transfer data error!");
		return -1;
	}
	IHal_SPI_Handle *spi_handle = (IHal_SPI_Handle *)handle;
	spi_handle->transfer.tx_buf = (unsigned long)send_data;
	spi_handle->transfer.rx_buf = (unsigned long)NULL;
	spi_handle->transfer.len = len;

	IHAL_INT32 fd = open(spi_handle->dev_node, O_RDWR);
	if(-1 == fd){
		IHAL_LOGE(TAG, "Failed to open spi master devive node!");
		return -1;
	}
	IHAL_INT32 ret = ioctl(fd, SPI_IOC_MESSAGE(1), &spi_handle->transfer);
	if(ret < 0){
		IHAL_LOGE(TAG, "Failed to transfer data!");
		return -1;
	}
	if(-1 == close(fd)){
		IHAL_LOGE(TAG, "Failed to close spi master device node!");
		return -1;
	}
	return 0;
}

IHAL_INT32 IHal_SPI_ReceivData(IHal_SPI_Handle_t *handle, IHAL_INT8 *receiv_data, IHAL_UINT32 len)
{
	assert(handle);
	assert(receiv_data);
	if(len <= 0){
		IHAL_LOGE(TAG, "The len of transfer data error!");
		return -1;
	}
	IHal_SPI_Handle *spi_handle = (IHal_SPI_Handle *)handle;
	spi_handle->transfer.rx_buf = (unsigned long)receiv_data;
	spi_handle->transfer.tx_buf = (unsigned long)NULL;
	spi_handle->transfer.len = len;

	IHAL_INT32 fd = open(spi_handle->dev_node, O_RDWR);
	if(-1 == fd){
		IHAL_LOGE(TAG, "Failed to open spi master devive node!");
		return -1;
	}
	IHAL_INT32 ret = ioctl(fd, SPI_IOC_MESSAGE(1), &spi_handle->transfer);
	if(ret < 0){
		IHAL_LOGE(TAG, "Failed to transfer data!");
		return -1;
	}
	if(-1 == close(fd)){
		IHAL_LOGE(TAG, "Failed to close spi master dev node!");
		return -1;
	}
	return 0;
}

IHAL_INT32 IHal_SPI_DeInit(IHal_SPI_Handle_t *handle)
{
	assert(handle);
	free(handle);
	return 0;
}
