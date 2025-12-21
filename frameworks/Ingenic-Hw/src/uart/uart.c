/**
 * @file    uart.c
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
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include "uart.h"
#include "hw_log.h"

#define TAG "UART"
typedef struct {
    int uart_fd;                         // 串口文件描述符
    pthread_mutex_t uart_mutex;          // 串口互斥锁
} IHal_Uart_Handle;


IHal_Uart_Handle_t *IHal_Uart_Init(const IHAL_INT8 *uart_name)
{
    IHAL_INT32 ret = -1;
    struct termios option = {0}; // 串口属性结构体
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)malloc(sizeof(IHal_Uart_Handle));
    if (!handle) {
        IHAL_LOGE(TAG, "malloc IHal_Uart_Handle error");
        return NULL;
    }
    handle->uart_fd = open(uart_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (handle->uart_fd < 0) {
        free(handle);
        return NULL;
    }
    // 获取终端属性
    tcgetattr(handle->uart_fd, &option);
    // 设置输入波特率
    cfsetspeed(&option, B115200);
    // 设置输出波特率
    cfsetospeed(&option, B115200);
    option.c_cflag &= ~CSIZE;
    option.c_cflag |= CS8;
    option.c_cflag &= ~PARENB;
    option.c_iflag &= ~INPCK;
    option.c_cflag &= ~CSTOPB;

    // 一般必设置的标志
    option.c_cflag |= (CLOCAL | CREAD);
    option.c_oflag &= ~(OPOST);
    option.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    option.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | IXOFF | IXANY);

    // 清空输入输出缓冲区
    tcflush(handle->uart_fd, TCIOFLUSH);

    // 设置最小接收字符数和超时时间
    // 当MIN=0, TIME=0时, 如果有数据可用, 则read最多返回所要求的字节数,
    // 如果无数据可用, 则read立即返回0
    option.c_cc[VMIN] = 0;
    option.c_cc[VTIME] = 0;

    // 设置终端属性
    ret = tcsetattr(handle->uart_fd, TCSANOW, &option);
    if (ret < 0) {
        free(handle);
        close(handle->uart_fd);
        return NULL;
    }

    // 初始化互斥锁
    pthread_mutex_init(&handle->uart_mutex, NULL);

    return (IHal_Uart_Handle_t *)handle;
}

IHAL_INT32 IHal_Uart_Deinit(IHal_Uart_Handle_t *Uart_Handle)
{

    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);

    if (close(handle->uart_fd) == -1) {
        IHAL_LOGE(TAG, "close uart device node error");
        return -ERROR;
    }
    free(Uart_Handle);
    memset(handle, 0, sizeof(IHal_Uart_Handle));

    // 销毁互斥锁
    pthread_mutex_destroy(&handle->uart_mutex);

    return 0;
}

IHAL_INT32 IHal_UartClearInputData(IHal_Uart_Handle_t *Uart_Handle)
{
    IHAL_INT32 ret = -1;
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    ret = tcflush(handle->uart_fd, TCIFLUSH);
    return ret;
}

IHAL_INT32 IHal_UartClearOututData(IHal_Uart_Handle_t *Uart_Handle)
{
    IHAL_INT32 ret = -1;
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    ret = tcflush(handle->uart_fd, TCOFLUSH);
    return ret;
}
IHAL_INT32 IHal_UartClearData(IHal_Uart_Handle_t *Uart_Handle)
{
    IHAL_INT32 ret = -1;
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    ret = tcflush(handle->uart_fd, TCIOFLUSH);
    return ret;
}

IHAL_INT32 IHal_UartSetBaudRate(IHal_Uart_Handle_t *Uart_Handle, enum BaudRate baudrate)
{
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    assert(baudrate > 0);

    struct termios uart_options;
    if (tcgetattr(handle->uart_fd, &uart_options) != 0) {
        IHAL_LOGE(TAG, "Uart get attributes error");
        return -ERROR;
    }

    cfsetispeed(&uart_options, baudrate);    // 设置波特率为115200
    cfsetospeed(&uart_options, baudrate);
    if (tcsetattr(handle->uart_fd, TCSANOW, &uart_options) == -1) {
        IHAL_LOGE(TAG, "Uart set baud rate error");
        return -ERROR;
    }
    return 0;
}

IHAL_INT32 IHal_UartSetDataBit(IHal_Uart_Handle_t *Uart_Handle, enum DataBits databits)
{
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    assert(databits > 0);

    struct termios uart_options;
    if (tcgetattr(handle->uart_fd, &uart_options) != 0) {
        IHAL_LOGE(TAG, "Uart get attributes error");
        return -ERROR;
    }

    uart_options.c_cflag &= ~CSIZE;    // 清除数据位 方便后续设置
    uart_options.c_cflag |= databits;
    if (tcsetattr(handle->uart_fd, TCSANOW, &uart_options) == -1) {
        IHAL_LOGE(TAG, "Uart set data bits error");
        return -ERROR;
    }

    return 0;
}
IHAL_INT32 IHal_UartSetParityBit(IHal_Uart_Handle_t *Uart_Handle, enum ParityBits paritybits)
{
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    assert(paritybits >= 0 && paritybits <= 1);

    struct termios uart_options;
    if (tcgetattr(handle->uart_fd, &uart_options) != 0) {
        IHAL_LOGE(TAG, "Uart get attributes error");
        return -ERROR;
    }

    uart_options.c_cflag |= PARENB;
    if (paritybits == EVEN) {
        uart_options.c_cflag &= ~PARODD;  // 设置为偶校验
    } else {
        uart_options.c_cflag |= PARODD;   //设置为奇校验
    }
    if (tcsetattr(handle->uart_fd, TCSANOW, &uart_options) == -1) {
        IHAL_LOGE(TAG, "Uart set parity bits error");
        return -ERROR;
    }

    return 0;

}

IHAL_INT32 IHal_UartSetStopBit(IHal_Uart_Handle_t *Uart_Handle, enum StopBits stopbits)
{
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);
    assert(stopbits >= 0 && stopbits <= 1);

    struct termios uart_options;
    if (tcgetattr(handle->uart_fd, &uart_options) != 0) {
        IHAL_LOGE(TAG, "Uart get attributes error");
        return -ERROR;
    }

    if (stopbits == ONE) {
        uart_options.c_cflag &= ~CSTOPB;  //1位停止位
    } else {
        uart_options.c_cflag |= CSTOPB; //2位停止位
    }
    if (tcsetattr(handle->uart_fd, TCSANOW, &uart_options) == -1) {
        IHAL_LOGE(TAG, "Uart set parity bits error");
        return -ERROR;
    }

    return 0;
}

IHAL_INT32 IHal_UartSendData(IHal_Uart_Handle_t *Uart_Handle, IHAL_UINT8 *send_data,
                             IHAL_INT32 send_data_size)
{
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);

    pthread_mutex_lock(&handle->uart_mutex);
    IHAL_INT32 ret = write(handle->uart_fd, send_data, send_data_size);
    if (send_data_size != ret) {
        printf("write 函数出错\n");
        pthread_mutex_unlock(&handle->uart_mutex);
        return -ERROR;
    }

    // 解锁
    pthread_mutex_unlock(&handle->uart_mutex);
    return ret;
}

IHAL_INT32 IHal_UartReceiveData(IHal_Uart_Handle_t *Uart_Handle, IHAL_UINT8 *recv_data, IHAL_INT32 recv_data_size, IHAL_INT32 timeout)
{
    assert(Uart_Handle);
    IHal_Uart_Handle *handle = (IHal_Uart_Handle *)Uart_Handle;
    assert(handle->uart_fd > 0);

    IHAL_INT32 ret = -1;
    fd_set readfds;
    struct timeval tv;

    IHAL_INT32 total_data_len = 0;  // 已读取数据长度
    IHAL_INT32 remain_data_len = recv_data_size; // 未读取数据长度
    memset(recv_data, 0, recv_data_size);

    // 加锁
    pthread_mutex_lock(&handle->uart_mutex);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(handle->uart_fd, &readfds);

        // 设置超时时间
        tv.tv_sec = timeout;  // 设置超时秒数
        tv.tv_usec = 0;  // 设置微秒数

        ret = select(handle->uart_fd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            IHAL_LOGE(TAG, "---select error\n");
            pthread_mutex_unlock(&handle->uart_mutex);
            return -ERROR;
        } else if (ret == 0) { // 超时
            IHAL_LOGE(TAG, "---select timeout");
            if (total_data_len > 0) {
                pthread_mutex_unlock(&handle->uart_mutex);
                return total_data_len;
            }
            pthread_mutex_unlock(&handle->uart_mutex);
            return -ERROR;
        } else { // 可读
            if (FD_ISSET(handle->uart_fd, &readfds)) {
                lseek(handle->uart_fd, 0, SEEK_SET);
                ret = read(handle->uart_fd, &recv_data[total_data_len], remain_data_len);
                if (ret < 0) {
                    pthread_mutex_unlock(&handle->uart_mutex);
                    return -ERROR;
                }
                total_data_len += ret;
                remain_data_len = recv_data_size - total_data_len;
                if (total_data_len == recv_data_size)
                    break;
            }
        }
    }
    pthread_mutex_unlock(&handle->uart_mutex);

    return total_data_len;
}
