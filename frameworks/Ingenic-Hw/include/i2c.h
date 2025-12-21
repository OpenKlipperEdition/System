/**
 * @file    i2c.h
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

#ifndef __I2C_H__
#define __I2C_H__
#include "ihal_info.h"

typedef struct {
    IHAL_UINT32 SlaveAddr;  //I2C从设备地址
    IHAL_UINT32 ReadAddr;   //想要进行读操作的地址
    IHAL_UINT8  AddrLength; //地址长度
    IHAL_UINT8 *tx_buf;     //发送数据缓冲区
    IHAL_INT32  tx_len;     //想要发送数据的长度
    IHAL_UINT8 *rx_buf;     //接收数据缓冲区
    IHAL_INT32  rx_len;     //想要接收数据的长度
} IHal_I2C_Msg_t;


typedef void IHal_I2C_Handle_t;

/**
 * @brief  I2C的初始化
 * @param [in] devname  : I2C device节点路径及名字
 * @retval 0        成功
 * @retval 非0     失败
 */
IHal_I2C_Handle_t *IHal_I2C_Init(const IHAL_INT8 *devname);
/**
 * @brief  I2C的反初始化
 * @param [in] I2C_Handle  : I2C句柄
 * @retval 0         成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_I2C_Deinit(IHal_I2C_Handle_t *I2C_Handle);
/**
 * @brief  I2C读数据
 * @param [in] I2C_Handle    : I2C句柄
 * @param [in] I2C_Msg       : I2C消息结构体
 * @param [in] AddrLength    : I2C外设存储地址长度
 * @retval  读取到的字节数     成功
 * @retval  -1              失败
 */
size_t IHal_I2C_ReadData(IHal_I2C_Handle_t *I2C_Handle, IHal_I2C_Msg_t *I2C_Msg);
/**
 * @brief  I2C写数据
 * @param [in] I2C_Handle    : I2C句柄
 * @param [in] I2C_Msg       : I2C消息结构体
 * @param [in] AddrLength    : I2C外设存储地址长度
 * @retval  写入的字节数    成功
 * @retval  -1              失败
 */
size_t IHal_I2C_WriteData(IHal_I2C_Handle_t *I2C_Handle, IHal_I2C_Msg_t *I2C_Msg);

#endif



