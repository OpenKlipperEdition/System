/**
 * @file    spi_test.h
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
#include <stdlib.h>
#include <string.h>
#include "ihal_config.h"
#include "ihal.h"

#define DEV_NODE_LEN 32
#define TRANSFER_LEN 512

int t_test(IHal_SPI_Handle_t *handle, char *data_buf)
{
	int ret = 0;
	char send_buf[TRANSFER_LEN] = {0};
	memcpy(send_buf, data_buf, TRANSFER_LEN);
	ret = IHal_SPI_SendData(handle, send_buf, TRANSFER_LEN);
	if (ret != 0) {
		printf("Failed to send data!\n");
		return -1;
	}
	printf("Send ok!\n");
	return 0;
}
int r_test(IHal_SPI_Handle_t *handle, char *data_buf)
{
	int i = 0;
	int ret = 0;
	char receiv_buf[TRANSFER_LEN] = {0};
	ret = IHal_SPI_ReceivData(handle, receiv_buf, TRANSFER_LEN);
	if (ret != 0) {
		printf("Failed to recv data!\n");
		return -1;
	}
	ret = 0;
	for (i = 0; i < TRANSFER_LEN; i++) {
		if(receiv_buf[i] != data_buf[i]){
			ret = -1;
		}
	}
	if(ret == 0){
		printf("Recv ok!\n");
	}else{
		printf("Recv data error!\n");
	}
	return 0;
}

int tr_test(IHal_SPI_Handle_t *handle, char *data_buf)
{
	int i = 0;
	int ret = 0;
	char send_buf[TRANSFER_LEN] = {0};
	char receiv_buf[TRANSFER_LEN] = {0};
	memcpy(send_buf, data_buf, TRANSFER_LEN);
	ret = IHal_SPI_Transfer(handle, send_buf, receiv_buf, TRANSFER_LEN);
	if (ret != 0) {
		printf("Failed to tr data!\n");
		return -1;
	}
	ret = 0;
	for (i = 0; i < TRANSFER_LEN; i++) {
		if(receiv_buf[i] != data_buf[i]){
			ret = -1;
		}
	}
	if(ret == 0){
		printf("TR ok!\n");
	}else{
		printf("Recv data error!\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc != 2){
		printf("Please select transfer dirction(t | r | tr)!\n");
		return -1;
	}
	IHal_SPI_attr  attr;
	char dev_node[DEV_NODE_LEN] = "/dev/spidev0.0";
	attr.dev_node = dev_node;
	attr.can_dma = DMA;
	attr.spi_mode = CPOL1_CPHA1;
	attr.speed = 1000000;
	attr.delay_usecs = 0;
	attr.bits_per_word = 8;
	attr.cs_change = 0;
	attr.tx_nbits = 8;
	attr.rx_nbits = 8;

	IHal_SPI_Handle_t *handle = NULL;
	handle = IHal_SPI_Init(attr);
	if (handle == NULL) {
		printf("Failed to SPI init!\n");
		return -1;
	}

	int i = 0;
	int ret = 0;
	char j = '0';
	char data_buf[TRANSFER_LEN] = {0};
	for (i = 0; i < TRANSFER_LEN; i++) {
		data_buf[i] = j;
		j++;
		if ((i + 1) % 10 == 0) {
			j = '0';
		}
	}
	if(!strcmp(argv[1], "r")){
		r_test(handle, data_buf);
	}else if(!strcmp(argv[1], "t")){
		t_test(handle, data_buf);
	}else if(!strcmp(argv[1], "tr")){
		tr_test(handle, data_buf);
	}else{
		printf("The argv[1] is error, please input (t | r | tr)\n");
		return -1;
	}
	ret = IHal_SPI_DeInit(handle);
	if(ret != 0){
		printf("Failed to DeInit spi!\n");
		return -1;
	}
	return 0;
}
