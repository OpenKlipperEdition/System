/**
 * @file    uart_test.c
 * @author  MPU系统软件部团队
 * @brief   包含 UART HAL 库函数原型的头文件
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
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include "ihal_config.h"
#include "ihal.h"

#define TAG "UART"
#define TIMEOUT 10
#define UART_NUM 2
#define TRANSFER_LEN 32

int main(int argc, char *argv[])
{
	IHAL_UINT8 j = 0x11;
    IHAL_INT32 ret = -1, i = 0;

    const IHAL_INT8 *uart[] = {"/dev/ttyS4", "/dev/ttyS5"};

    //定义两个结构体指针数组 分别保存两个串口的信息
    IHal_Uart_Handle_t *Uart_Handle[UART_NUM];

    //初始化串口
    for (IHAL_INT32 i = 0; i < UART_NUM; i++) {
        Uart_Handle[i] = IHal_Uart_Init(uart[i]);
        if (Uart_Handle[i] == NULL) {
            IHAL_LOGE(TAG, "%s init error", uart[i]);
            return -ERROR;
        }
    }

    //UART4 向 UART5 发送消息
    IHAL_UINT8 uart_send[TRANSFER_LEN] = {0};
	for(i = 0; i < TRANSFER_LEN; i++){
		uart_send[i] = j;
		j++;
		if(j == 0x1A){
			j = 0x11;
		}
	}
	printf("%s send buffer:\n", uart[0]);
	for(i = 0; i < TRANSFER_LEN; i++){
		printf("0x%02X ", uart_send[i]);
		if((i+1) % 10 == 0)
			printf("\n");
	}
	printf("\n");
    IHAL_UINT32 uart_send_size = TRANSFER_LEN;
    IHAL_UINT8 uart_recv[TRANSFER_LEN] = {0};

    ret = IHal_UartSendData(Uart_Handle[0], uart_send, uart_send_size);
	printf("send num = [%d]\n", ret);
    if (ret != uart_send_size) {
        IHal_Uart_Deinit(Uart_Handle[0]);
        return -ERROR;
    }

    ret = IHal_UartClearData(Uart_Handle[0]);
    if (0 != ret) {
        IHal_Uart_Deinit(Uart_Handle[0]);
        return -ERROR;
    }

    ret = -1;
    //UART5 从 UART4 接收消息
    // 串口接收数据
    ret = IHal_UartReceiveData(Uart_Handle[1], uart_recv, uart_send_size, TIMEOUT);
	printf("%s recv buffer:\n", uart[1]);
    if (ret > 0) {
		for(int i = 0; i < TRANSFER_LEN; i++){
			printf("0x%02X ", uart_recv[i]);
			if((i+1) % 10 == 0)
				printf("\n");
		}
		printf("\n");
    } else {
        IHal_Uart_Deinit(Uart_Handle[1]);
        return -ERROR;
    }
    printf("recv num = [%d]\n", ret);

    //释放两个串口的资源
    IHal_Uart_Deinit(Uart_Handle[0]);
    IHal_Uart_Deinit(Uart_Handle[1]);

    return 0;
}
