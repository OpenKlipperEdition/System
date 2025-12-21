/**
 * @file    spi_slv.h
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
#ifndef __SPI_SLV_H__
#define __SPI_SLV_H__
#include "ihal_info.h"

#define DEV_NODE_PATH_LEN 32

/**
 * @defgroup group_SPI_SLV SPI_SLV模块
 * @{
 */

/**
 * @addtogroup group_SPI_SLV_data_type 数据类型定义
 * @{
 */

/**
 * @brief SPI_SLV模式
 */
enum SPI_SLV_MODE {
    SPI_MODE0 = 4, // 模式0
    SPI_MODE1,   // 模式1
    SPI_MODE2,   // 模式2
    SPI_MODE3,   // 模式3
};

/**
 * @brief SPI_SLV是否使用dma
 */
enum SLV_CAN_DMA {
    CPU_MODE = 2, // cpu模式
    DMA_MODE, // dma模式
};

typedef struct {
    IHAL_INT8 *dev_node; // spi_slv设备节点路径名
    enum SPI_SLV_MODE spi_slv_mode; // spi_slv相位极性模式
    enum SLV_CAN_DMA can_dma; // spi_slv是否使用dma 
    unsigned char bits_per_word; // 指定每个数据的位数.例如,8位SPI通信中,设置为8.对于不同的SPI设备,可以使用不同的位数进行数据传输
} IHal_SPI_SLV_attr; 

typedef void IHal_SPI_SLV_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_SPI_SLV_API API定义
 * @{
 */

/**
 * @brief  初始化SPI_SLV
 * @param  [in]  attr: 初始化信息结构体指针
 * @retval IHal_SPI_SLV_Handle_t SPI_SLV数据传输句柄
 * @retval 失败              NULL
 */
IHal_SPI_SLV_Handle_t *IHal_SPI_SLV_Init(IHal_SPI_SLV_attr *attr);

/**
 * @brief  SPI_SLV发送或接收数据
 * @param  [in] handle: SPI_SLV数据传输句柄
 * @param  [in] send_data: 要发送的数据的首地址
 * @param  [in] receiv_data: 保存接收数据的首地址
 * @param  [in] len: 要发送或接收数据的长度
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_SPI_SLV_Transfer(IHal_SPI_SLV_Handle_t *handle, IHAL_INT8 *send_data, IHAL_INT8 *receiv_data, IHAL_UINT32 len);

/**
 * @brief  SPI_SLV发送数据
 * @param  [in] handle: SPI_SLV数据传输句柄
 * @param  [in] send_data: 要发送的数据的首地址
 * @param  [in] len: 要发送的数据的长度
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_SPI_SLV_SendData(IHal_SPI_SLV_Handle_t *handle, IHAL_INT8 *send_data, IHAL_UINT32 len);

/**
 * @brief  SPI_SLV接收数据
 * @param  [in] handle: SPI_SLV数据传输句柄
 * @param  [in] receiv_data: 保存接收数据的首地址
 * @param  [in] len: 要接收的数据的长度
 * @retval 0        成功
 * @retval 非0      失败
 */
IHAL_INT32 IHal_SPI_SLV_ReceivData(IHal_SPI_SLV_Handle_t *handle, IHAL_INT8 *receiv_data, IHAL_UINT32 len);

/**
 * @brief  关闭SPI_SLV传输
 * @param  [in] handle: SPI_SLV数据传输句柄
 * @retval 0        成功
 * @retval 非0     失败
 */
IHAL_INT32 IHal_SPI_SLV_DeInit(IHal_SPI_SLV_Handle_t *handle);

/**
 * @}
 */
#endif // __INGENIC_SPI_SLV_H
