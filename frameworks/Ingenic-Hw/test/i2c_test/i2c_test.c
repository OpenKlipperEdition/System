/**
 * @file    i2c_test.c
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

#include <string.h>
#include <stdio.h>
#include <linux/i2c.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <pthread.h>
#include "ihal_config.h"
#include "ihal.h"

#define I2C_DEV_ADDR    0x51
#define I2C_READ_ADDR   0x02
#define TAG  "i2c"

IHal_I2C_Handle_t *I2C_Handle;
pthread_mutex_t mutex;
pthread_cond_t cond;
IHAL_INT32 flag = 0;
// 定义线程函数，用于读数据
//我将结构体变量的地址传给了每个线程，初始化一次让所有线程使用同样的数据。
//a线写数据，b进程读数据。
void *i2c_read(void *arg)
{
	IHAL_INT32 ret = 0;
	pthread_mutex_lock(&mutex);
	while (!flag) {
		pthread_cond_wait(&cond, &mutex);
	}
	pthread_mutex_unlock(&mutex);
	if ((ret = IHal_I2C_ReadData(I2C_Handle, arg)) < 0) {
		IHAL_LOGD(TAG, "I2C Read data error");
		IHal_I2C_Deinit(I2C_Handle);
		return NULL;
	}
	printf("read byte=%d\n", ret);
	return NULL;
}
//定义线程函数，用以写数据
void *i2c_write(void *arg)
{
	IHAL_INT32 ret = 0;
	if ((ret = IHal_I2C_WriteData(I2C_Handle, arg)) < 0) {
		IHAL_LOGD(TAG, "I2C Write data error");
		IHal_I2C_Deinit(I2C_Handle);
		return NULL;
	}
	// 发送信号给等待的线程
	pthread_mutex_lock(&mutex);
	flag = 1;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
	printf("write byte=%d\n", ret);
	return NULL;
}
int main()
{
	//初始化I2C_Msg结构体
	IHal_I2C_Msg_t I2C_Msg;
	I2C_Msg.SlaveAddr = I2C_DEV_ADDR;
	I2C_Msg.ReadAddr = I2C_READ_ADDR;
	I2C_Msg.AddrLength = ADDRLENGTH_8BIT;
	IHAL_UINT8 tx_buf[] = {0x02,0x01,0x02,0x03};
	I2C_Msg.tx_len = sizeof(tx_buf);
	IHAL_UINT8 rx_buf[3] = {0};
	I2C_Msg.rx_len = sizeof(rx_buf);
	I2C_Msg.tx_buf = tx_buf;
	I2C_Msg.rx_buf = rx_buf;

	//初始化I2C句柄   返回值为全局变量所有线程都可使用
	I2C_Handle = IHal_I2C_Init(I2C2_DEV_NODE);
	if (I2C_Handle == NULL) {
		IHAL_LOGE(TAG, "I2C Init error");
		return -ERROR;
	}
	pthread_t i2c_read_pid;
	pthread_t i2c_write_pid;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	//创建一个写线程
	pthread_create(&i2c_write_pid, NULL, i2c_write, (void *)&I2C_Msg);
	//创建一个读线程
	pthread_create(&i2c_read_pid, NULL, i2c_read, (void *)&I2C_Msg);
	//等待线程执行完成
	pthread_join(i2c_write_pid, NULL);
	pthread_join(i2c_read_pid, NULL);
	printf("buf[0]=%02x,buf[1]=%02x\n", rx_buf[0], rx_buf[1]);
	printf("buf[2]=%02x\n", rx_buf[2]);

	IHal_I2C_Deinit(I2C_Handle);
	return 0;
}


