#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ihal_config.h"
#include "ihal.h"

int main() {
	int ret = 0;

	IHal_WatchDog_Handle_t * wtd_handle = IHal_WatchDog_Init(WATCHDOG_DEV_NODE);
	if (!wtd_handle) {
		goto init_err;
	}

	// 配置和启动watchdog定时器（设置超时时间为30秒）
	ret = IHal_WatchDog_SetTimeout(wtd_handle, TIMEOUT);
	if (ret) {
		goto set_timeout_err;
	}

	// 启动watchdog定时器
	ret = IHal_WatchDog_Start(wtd_handle);
	if (ret) {
		goto start_err;
	}

	sleep(FEED_TIME);

	// 喂狗
	ret = IHal_WatchDog_FeedDog(wtd_handle);
	if (ret) {
		printf("feed dog error\n");
	}
	printf("feed dog\n");

	sleep(2);

start_err:
	// 关闭watchdog定时器和设备
	ret = IHal_WatchDog_Stop(wtd_handle);
	if (ret) {
		printf("stop watchdog failed\n");
	}
set_timeout_err:
init_err:
	ret = IHal_WatchDog_DeInit(wtd_handle);
	assert(ret == 0);

	return ret;
}

