/**
 * @file    TCU_test.c
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
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "ihal_config.h"
#include "ihal.h"

#define TAG  "tcu"
#define PAGE_SIZE 4096

#define DEBUG_GENERAL_MODE 0
#define DEBUG_GATE_MODE  1
#define DEBUG_DIRECTION_MODE  0
#define DEBUG_QUADRATURE_MODE  0
#define DEBUG_POS_MODE   0
/**
 * @brief  物理地址映射
 * @param [in] physical_address ：想要进行映射的物理地址
 * @retval 映射后的虚拟地址        成功
 * @retval NULL     失败
 */

void *mmap_physical_address(off_t physical_address)
{

	IHAL_INT32 mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem_fd == -1) {
		perror("无法打开 /dev/mem 设备文件");
		return IHAL_RNULL;;
	}
	void *virtual_addr = NULL;
	virtual_addr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, physical_address);
	if (virtual_addr == MAP_FAILED) {
		perror("无法映射物理地址到用户空间");
		close(mem_fd);
		return IHAL_RNULL;
	}
	return virtual_addr;
}

/**
 * @brief  取消物理地址映射
 * @param [in] mapped_base ：进行mmap映射后的地址
 */
void unmap_physical_address(void *mapped_base)
{
	if (munmap(mapped_base, PAGE_SIZE) == -1) {
		perror("无法解除映射");
	}
}
/**
 * @brief  测试TCU的GENERAL_MODE、GATE_MODE、DIRECTION_MODE中的一种
 * @param [in] handle ：TCU句柄结构体指针
 * @param [in] tcu_vir_addr ：TCU控制器的虚拟地址
 * @param [in] tcumode      ：想要测试的TCU的模式
 * @retval 0        成功
 * @retval -1       失败
 */
IHAL_INT32 tcu_general_gate_direction_mode(IHal_TCU_Handle_t* handle,void *tcu_vir_addr,enum tcu_mode tcumode){
	// GENERAL_MODE
	if (IHal_TcuEnable(handle, chan0, tcumode) < 0) {
		IHAL_LOGE(TAG, "TCU chan0 %d enable error",tcumode);
		IHal_TCU_Deinit(handle);
		unmap_physical_address(tcu_vir_addr);
		return -1;
	}
	IHAL_INT32 count = -1;
	count = IHal_TcuGetCount(tcu_vir_addr,chan0);
	printf("count=%08x\n", count);
	return 0;
}
/**
 * @brief  测试QUADRTURE_MODE的函数
 * @param [in] handle ：TCU句柄结构体指针
 * @param [in] tcu_vir_addr ：TCU控制器的虚拟地址
 * @retval 0        成功
 * @retval -1       失败
 */
IHAL_INT32 tcu_quadrature_mode(IHal_TCU_Handle_t* handle,void* tcu_vir_addr){
	//QUADRATURE_MODE
	void *gpio_handle1 = IHal_GPIO_Init(GPIOC, 2); //初始化gpio引脚
	if (!gpio_handle1) {
		printf("Init gpio fail!\n");
	}
	void *gpio_handle2 = IHal_GPIO_Init(GPIOC, 3); //初始化gpio引脚
	if (!gpio_handle2) {
		printf("Init gpio fail!\n");
	}
	IHAL_INT32 ret1 = IHal_GPIO_Set_Direciton(gpio_handle1, GPIO_OUT); //设置引脚为输出模式
	if (ret1 < 0) {
		printf("set gpio_handle1 out direction fail!\n");
		IHal_GPIO_DeInit(gpio_handle1);
		return -1;
	}
	IHAL_INT32 ret2 = IHal_GPIO_Set_Direciton(gpio_handle2, GPIO_OUT); //设置引脚为输出模式
	if (ret2 < 0) {
		printf("set gpio_handle2 out direction fail!\n");
		IHal_GPIO_DeInit(gpio_handle2);
		return -1;
	}
	ret1 = IHal_GPIO_Set_Value(gpio_handle1, GPIO_HIGH);
	if (ret1 < 0) {
		printf("set high value fail!\n");
		IHal_GPIO_DeInit(gpio_handle1);
		return -1;
	}
	if (IHal_TcuEnable(handle, chan0, QUADRATURE_MODE) < 0) {
		IHAL_LOGE(TAG, "TCU chan0 QUADRATURE_MODE enable error");
		IHal_TCU_Deinit(handle);
		unmap_physical_address(tcu_vir_addr);
		return -1;
	}
	usleep(50000);
	ret2 = IHal_GPIO_Set_Value(gpio_handle2, GPIO_HIGH);
	if (ret2 < 0) {
		printf("set high value fail!\n");
		IHal_GPIO_DeInit(gpio_handle2);
		return -1;
	}
	usleep(50000);
	ret1 = IHal_GPIO_Set_Value(gpio_handle1, GPIO_LOW);
	if (ret1 < 0) {
		printf("set high value fail!\n");
		IHal_GPIO_DeInit(gpio_handle1);
		return -1;
	}
	usleep(50000);
	ret2 = IHal_GPIO_Set_Value(gpio_handle2, GPIO_LOW);
	if (ret2 < 0) {
		printf("set high value fail!\n");
		IHal_GPIO_DeInit(gpio_handle2);
		return -1;
	}
	IHAL_INT32 count = -1;
	count = IHal_TcuGetCount(tcu_vir_addr,chan0);
	printf("count=%08x\n", count);
	return 0;
}

/**
 * @brief  测试POS_MODE的函数
 * @param [in] handle ：TCU句柄结构体指针
 * @param [in] tcu_vir_addr ：TCU控制器的虚拟地址
 * @retval 0        成功
 * @retval -1       失败
 */
IHAL_INT32 tcu_pos_mode(IHal_TCU_Handle_t* handle,void* tcu_vir_addr){
	//POS MODE
	IHAL_INT32 n = 5;
	void *gpio_handle1 = IHal_GPIO_Init(GPIOC, 2); //初始化gpio引脚
	if (!gpio_handle1) {
		printf("Init gpio fail!\n");
	}
	void *gpio_handle2 = IHal_GPIO_Init(GPIOC, 3); //初始化gpio引脚
	if (!gpio_handle2) {
		printf("Init gpio fail!\n");
	}
	IHAL_INT32 ret1 = IHal_GPIO_Set_Direciton(gpio_handle1, GPIO_OUT); //设置引脚为输出模式
	if (ret1 < 0) {
		printf("set gpio_handle1 out direction fail!\n");
		IHal_GPIO_DeInit(gpio_handle1);
		return -1;
	}
	IHAL_INT32 ret2 = IHal_GPIO_Set_Direciton(gpio_handle2, GPIO_OUT); //设置引脚为输出模式
	if (ret2 < 0) {
		printf("set gpio_handle2 out direction fail!\n");
		IHal_GPIO_DeInit(gpio_handle2);
		return -1;
	}
	if (IHal_TcuEnable(handle, chan0, POS_MODE) < 0) {
		IHAL_LOGE(TAG, "TCU chan0 POS_MODE enable error");
		IHal_TCU_Deinit(handle);
		unmap_physical_address(tcu_vir_addr);
		return -1;
	}
	while (n--) {
		ret1 = IHal_GPIO_Set_Value(gpio_handle1, GPIO_LOW);
		if (ret1 < 0) {
			printf("set high value fail!\n");
			IHal_GPIO_DeInit(gpio_handle1);
			return -1;
		}
		printf("------------<tcu down cnt>---------------\n");
		IHAL_INT32 count = -1;
		count = IHal_TcuGetCount(tcu_vir_addr,chan0);
		printf("count=%08x\n", count);
		sleep(1);
		printf("--------------<tcu cnt>------------------\n");
		count = IHal_TcuGetCount(tcu_vir_addr,chan0);
		printf("count=%08x\n", count);
		ret1 = IHal_GPIO_Set_Value(gpio_handle1, GPIO_HIGH);
		if (ret1 < 0) {
			printf("set high value fail!\n");
			IHal_GPIO_DeInit(gpio_handle1);
			return -1;
		}
		printf("------------<tcu up cnt>----------------\n");
		count = IHal_TcuGetCount(tcu_vir_addr,chan0);
		printf("count=%08x\n", count);
		sleep(1);
		printf("-------------<tcu cnt>------------------\n");
		count = IHal_TcuGetCount(tcu_vir_addr,chan0);
		printf("count=%08x\n", count);
	}
	return 0;
}

int main()
{
	IHal_TCU_Handle_t *handle = NULL;
	void *tcu_vir_addr = NULL;
	//初始化TCU，打开TCU的enable节点
	handle = IHal_TCU_Init(DEVNAME);
	if (handle == NULL) {
		IHAL_LOGE(TAG, "TCU INIT ERROR");
		return -ERROR;
	}
	//将TCU的物理地址映射成虚拟地址，以便在应用层对TCU的地址进行操作
	tcu_vir_addr = mmap_physical_address(CONTROLLER_ADDR);
	if (tcu_vir_addr == NULL) {
		IHAL_LOGE(TAG, "TCU address mapping failed");
		IHal_TCU_Deinit(handle);
		return -ERROR;
	}


#if DEBUG_GENERAL_MODE | DEBUG_GATE_MODE | DEBUG_DIRECTION_MODE

	tcu_general_gate_direction_mode(handle,tcu_vir_addr,GENERAL_MODE);

#endif

#if DEBUG_QUADRATURE_MODE

	tcu_quadrature_mode(handle,tcu_vir_addr);

#endif
#if DEBUG_POS_MODE

	tcu_pos_mode(handle,tcu_vir_addr);

#endif

	IHal_TcuDisable(x2000_TCU, chan0);
	unmap_physical_address(tcu_vir_addr);
	IHal_TCU_Deinit(handle);
	return 0;
}
