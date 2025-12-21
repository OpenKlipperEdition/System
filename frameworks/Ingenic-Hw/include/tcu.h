/**
 * @file    tcu.h
 * @author  MPU系统软件部团队
 * @brief   包含 TCU HAL 库函数原型的头文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __TCU_H__
#define __TCU_H__

#include "ihal_info.h"


typedef void  IHal_TCU_Handle_t;

//TCU模式选择
enum tcu_mode {
	GENERAL_MODE,
	GATE_MODE,
	DIRECTION_MODE,
	QUADRATURE_MODE,
	POS_MODE,
	CAPTURE_MODE,
	FILTER_MODE,
};
//TCU通道选择
enum tcu_chan {
	chan0,
	chan1,
	chan2,
	chan3,
	chan4,
	chan5,
	chan6,
	chan7,
};
//TCU控制器选择
enum  tcu {
	x26xx_TCU0,
	x26xx_TCU1,
	x2000_TCU,
	x1600_TCU,
	x2500_TCU,
};

/**
 * @brief  TCU的初始化
 * @param  [in] devname  : TCU device节点路径及名字
 * @retval 指向TCU句柄结构体的指针        成功
 * @retval NULL     失败
 */

IHal_TCU_Handle_t *IHal_TCU_Init(const IHAL_INT8 *devname);

/**
 * @brief  TCU的反初始化
 * @param [in] handle  : 指向TCU句柄结构体的指针
 * @retval  0        成功
 * @retval -1        失败
 */

IHAL_INT32 IHal_TCU_Deinit(IHal_TCU_Handle_t *handle);

/**
 * @brief  Enable TCU
 * @param [in] handle  ：指向TCU句柄结构体的指针
 * @param [in] channum ：想要使能的tcu通道 (范围0-7)
 * @param [in] mode    ：想要使能的tcu模式
 * @retval 指向TCU句柄结构体的指针        成功
 * @retval NULL     失败
 */

IHAL_INT32 IHal_TcuEnable(IHal_TCU_Handle_t *handle, enum tcu_chan channum, enum tcu_mode mode);

/**
 * @brief  Disable TCU
 * @param [in] tcu_num ：TCU控制器
 * @param [in] channum ：想要使能的tcu通道 (范围0-7)
 * @retval 指向TCU句柄结构体的指针        成功
 * @retval NULL     失败
 */

IHAL_INT32 IHal_TcuDisable(enum tcu tcu_num, enum tcu_chan channum);

/**
 * @brief  获取TCU的数值
 * @param [in] virtual_addr ：映射后的虚拟地址
 * @retval TCU当前的数值        成功
 * @retval -1     失败
 */

IHAL_INT32 IHal_TcuGetCount(void *virtual_addr,enum tcu_chan channum);

#endif
