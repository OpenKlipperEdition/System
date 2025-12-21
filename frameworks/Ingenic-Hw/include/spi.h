/**
 * @file    spi.h
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

#ifndef __SPI_H__
#define __SPI_H__
#include "ihal_info.h"

/**
 * @defgroup group_SPI SPI模块
 * @{
 */

/**
 * @addtogroup group_SPI_data_type 数据类型定义
 * @{
 */

/**
 * @brief SPI模式
 */
enum SPI_Mode {
    CPOL0_CPHA0, // 模式0
    CPOL0_CPHA1, // 模式1
    CPOL1_CPHA0, // 模式2
    CPOL1_CPHA1, // 模式3
};

/**
 * @brief 是否使用dma
 */
enum CAN_DMA {
    CPU, // cpu模式
    DMA, // dma模式
};

typedef struct {
    IHAL_INT8 *dev_node; // 节点路径名
    enum CAN_DMA can_dma; // 是否使用dma
    enum SPI_Mode spi_mode; // SPI模式
    IHAL_UINT32 speed; // SPI总线的时钟速度,单位(Hz)
    IHAL_UINT16 delay_usecs; // 在每个传输之前插入的延迟时间,以微秒(μs)为单位,可以用于设定传输之间的延迟时间
    IHAL_UINT8 bits_per_word; // 指定每个数据的位数.例如,8位SPI通信中,设置为8.对于不同的SPI设备,可以使用不同的位数进行数据传输
    IHAL_UINT8 cs_change; // 描述片选信号的行为.如果设置为1,表示在每次传输之前自动禁用和重新启用片选信号.如果设置为0,则保持片选信号不变
    IHAL_UINT8 tx_nbits; // 指定发送数据字节中有效位的数量.可以用于设置发送数据的位宽度,只有低位的有效位将被发送,系统默认是8位
    IHAL_UINT8 rx_nbits; // 指定发送数据字节中有效位的数量.可以用于设置发送数据的位宽度,只有低位的有效位将被发送,系统默认是8位
} IHal_SPI_attr;

typedef void IHal_SPI_Handle_t;
/**
 * @}
 */

/**
 * @addtogroup group_SPI_API API定义
 * @{
 */

/**
 * @brief  初始化SPI_Master
 * @param  [in]  attr: 初始化信息结构体
 * @retval IHal_SPI_Handle_t SPI_Master数据传输句柄
 * @retval 失败              NULL
 */
IHal_SPI_Handle_t *IHal_SPI_Init(IHal_SPI_attr attr);

/**
 * @brief  SPI_Master发送或接收数据
 * @param  [in] handle: SPI_Master数据传输句柄
 * @param  [in] send_data: 要发送的数据的首地址
 * @param  [in] receiv_data: 要接收的数据的首地址
 * @param  [in] len: 要发送或接收数据的长度
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_SPI_Transfer(IHal_SPI_Handle_t *handle, IHAL_INT8 *send_data, IHAL_INT8 *receiv_data, IHAL_UINT32 len);

/**
 * @brief  SPI_Master发送数据
 * @param  [in] handle: SPI_Master数据传输句柄
 * @param  [in] send_data: 要发送的数据的首地址
 * @param  [in] len: 要发送的数据的长度
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_SPI_SendData(IHal_SPI_Handle_t *handle, IHAL_INT8 *send_data, IHAL_UINT32 len);

/**
 * @brief  SPI_Master接收数据
 * @param  [in] handle: SPI_Master数据传输句柄
 * @param  [in] receiv_data: 要发送的数据的首地址
 * @param  [in] len: 要发送的数据的长度
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_SPI_ReceivData(IHal_SPI_Handle_t *handle, IHAL_INT8 *receiv_data, IHAL_UINT32 len);

/**
 * @brief  关闭SPI_Master传输
 * @param  [in] handle: SPI_Master数据传输句柄
 * @retval 0        成功
 * @retval 非0     失败
 */
IHAL_INT32 IHal_SPI_DeInit(IHal_SPI_Handle_t *handle);

/**
 * @}
 */
#endif // __INGENIC_SPI_H
