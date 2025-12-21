/**
 * @file    uart.h
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

#ifndef __UART_H__
#define __UART_H__

#include "ihal_info.h"
#include <termios.h>
typedef void  IHal_Uart_Handle_t;
//波特率
enum BaudRate {
    BAUD_50 = B50,
    BAUD_75 = B75,
    BAUD_110 = B110,
    BAUD_134 = B134,
    BAUD_150 = B150,
    BAUD_200 = B200,
    BAUD_300 = B300,
    BAUD_600 = B600,
    BAUD_1200 = B1200,
    BAUD_1800 = B1800,
    BAUD_2400 = B2400,
    BAUD_4800 = B4800,
    BAUD_9600 = B9600,
    BAUD_19200 = B19200,
    BAUD_38400 = B38400,
    BAUD_57600 = B57600,
    BAUD_115200 = B115200,
    BAUD_230400 = B230400,
    BAUD_460800 = B460800,
    BAUD_500000 = B500000,
    BAUD_576000 = B576000,
    BAUD_921600 = B921600,
    BAUD_1000000 = B1000000,
    BAUD_1152000 = B1152000,
    BAUD_1500000 = B1500000,
    BAUD_2000000 = B2000000,
    BAUD_2500000 = B2500000,
    BAUD_3000000 = B3000000,
    BAUD_3500000 = B3500000,
    BAUD_4000000 = B4000000,
};
//数据位
enum DataBits {
    CS_5 = CS5,
    CS_6 = CS6,
    CS_7 = CS7,
    CS_8 = CS8,
};
//奇偶校验位
enum ParityBits {
    ODD,             //奇校验
    EVEN,            //偶校验
};
//停止位
enum StopBits {
    ONE,             //1位停止位
    TWO,             //2位停止位
};

/**
 * @brief  初始化UART
 * @param  [in] uart_name : 需要初始化的UART设备节点
 * @retval 所申请的通道句柄结构体指针 成功
 * @retval NULL             失败
 */

IHal_Uart_Handle_t *IHal_Uart_Init(const IHAL_INT8 *uart_name);

/**
 * @brief  反初始化UART
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @retval 0 成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_Uart_Deinit(IHal_Uart_Handle_t *Uart_Handle);

/**
 * @brief  清空输入缓冲区
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @retval 0 成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartClearInputData(IHal_Uart_Handle_t *Uart_Handle);

/**
 * @brief  清空输出缓冲区
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @retval 0 成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartClearOutputData(IHal_Uart_Handle_t *Uart_Handle);

/**
 * @brief  清空输入输出缓冲区
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @retval 0 成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartClearData(IHal_Uart_Handle_t *Uart_Handle);

/**
 * @brief  设置波特率
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @param  [in] baudrate    ：波特率（枚举变量）
 * @retval 0              成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartSetBaudRate(IHal_Uart_Handle_t *Uart_Handle, enum BaudRate baudrate);

/**
 * @brief  设置数据位
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @param  [in] databis     ：数据位（枚举变量）
 * @retval 0              成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartSetDataBit(IHal_Uart_Handle_t *Uart_Handle, enum DataBits databits);

/**
 * @brief  设置奇偶校验位
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @param  [in] baudrate    ：奇偶校验位（枚举变量）
 * @retval 0              成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartSetParityBit(IHal_Uart_Handle_t *Uart_Handle, enum ParityBits paritybits);

/**
 * @brief  设置停止位
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @param  [in] stopbits    ：停止位（枚举变量）
 * @retval 0              成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartSetStopBit(IHal_Uart_Handle_t *Uart_Handle, enum StopBits stopbits);

/**
 * @brief  UART 发送数据
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @param  [in] send_data   ：发送数据缓冲区
 * @param  [in] send_data_size:  想要发送的数据的长度
 * @retval 成功发送的字节数              成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartSendData(IHal_Uart_Handle_t *Uart_Handle, IHAL_UINT8 *send_data, IHAL_INT32 send_data_size);

/**
 * @brief  UART 接收数据
 * @param  [in] Uart_Handle : Uart 句柄结构体指针
 * @param  [in] recv_data   ：接收数据缓冲区
 * @param  [in] recv_data_size:  想要接收的数据的长度
 * @retval 成功接收的字节数              成功
 * @retval -1             失败
 */

IHAL_INT32 IHal_UartReceiveData(IHal_Uart_Handle_t *Uart_Handle, IHAL_UINT8 *recv_data, IHAL_INT32 recv_data_size, IHAL_INT32 timeout);

#endif
