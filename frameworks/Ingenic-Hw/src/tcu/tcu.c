/**
 * @file    tcu.c
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

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "tcu.h"
#include "hw_log.h"

#define TAG "TCU"
#define PAGE_SIZE 4096

typedef struct {
	IHAL_INT32 fd;          //文件描述符
	pthread_mutex_t tcu_mutex;  //互斥锁
} IHal_TCU_Handle;


IHal_TCU_Handle_t *IHal_TCU_Init(const IHAL_INT8 *devname)
{
	IHal_TCU_Handle *handle = (IHal_TCU_Handle *)malloc(sizeof(IHal_TCU_Handle));
	if (handle == NULL) {
		IHAL_LOGE(TAG, "malloc IHal_TCU_Handle error");
		return IHAL_RNULL;
	}
	handle->fd = open(devname, O_RDWR);
	if (handle->fd == -1) {
		IHAL_LOGE(TAG, "open %s error", devname);
		free(handle);
		return IHAL_RNULL;
	}
	pthread_mutex_init(&handle->tcu_mutex, NULL);
	return (IHal_TCU_Handle_t *)handle;
}

IHAL_INT32 IHal_TCU_Deinit(IHal_TCU_Handle_t *handle)
{
	IHal_TCU_Handle *tcu_handle = (IHal_TCU_Handle *)handle;
	if (close(tcu_handle->fd) == -1) {
		IHAL_LOGE(TAG, "Close dev failed");
		return -IHAL_RERR;
	}
	pthread_mutex_destroy(&tcu_handle->tcu_mutex);
	free(handle);
	return 0;
}

IHAL_INT32 IHal_TcuEnable(IHal_TCU_Handle_t *handle, enum tcu_chan channum, enum tcu_mode mode)
{
	assert(handle != NULL);
	assert(channum >= 0);
	assert(mode >= 0);


	IHal_TCU_Handle *tcu_handle = (IHal_TCU_Handle *)handle;
	IHAL_INT32 ret = -1;
	IHAL_INT8 buff[4] = {0};
	snprintf(buff, sizeof(buff), "%d %d\n", channum, mode);

	pthread_mutex_lock(&tcu_handle->tcu_mutex);
	ret = write(tcu_handle->fd, buff, sizeof(buff));
	if (ret < 0) {
		IHAL_LOGE(TAG, "set %dchannel %dmode error", channum, mode);
		pthread_mutex_unlock(&tcu_handle->tcu_mutex);
		return -IHAL_RERR;
	}
	pthread_mutex_unlock(&tcu_handle->tcu_mutex);
	return 0;
}


IHAL_INT32 IHal_TcuDisable(enum tcu tcu_name, enum tcu_chan channum)
{
	assert(channum >= 0);

	IHAL_INT32 ret = -1;
	IHAL_INT8 command[128] = {0};
	switch (tcu_name) {
		case x26xx_TCU0:
			sprintf(command, "echo %d > /sys/devices/platform/apb/13630000.tcu0/disable", channum);
			break;
		case x26xx_TCU1:
			sprintf(command, "echo %d > /sys/devices/platform/apb/1360000.tcu1/disable", channum);
			break;
		case x1600_TCU:
			sprintf(command, "echo %d > /sys/devices/platform/apb/10002000.tcu/disable", channum);
			break;
		case x2000_TCU:
			sprintf(command, "echo %d > /sys/devices/platform/apb/10002000.tcu/disable", channum);
			break;
		case x2500_TCU:
			sprintf(command, "echo %d > /sys/devices/platform/apb/10002000.tcu/disable", channum);
			break;
		default :
			IHAL_LOGE(TAG,"invalid parameter");
			return -IHAL_RERR;
			break;

	}

	ret = system(command);
	if (ret == -1) {
		IHAL_LOGE(TAG, "%s error", command);
		return -IHAL_RERR;
	}

	return 0;
}

IHAL_INT32 IHal_TcuGetCount(void *virtual_addr,enum tcu_chan channum)
{

	return  *(volatile IHAL_UINT32 *)(virtual_addr + (0x48+channum*0x10));

}

