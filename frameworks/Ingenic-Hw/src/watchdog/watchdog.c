#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>
#include <assert.h>
#include <pthread.h>

#include "watchdog.h"
#include "hw_log.h"

#define TAG "WatchDog"

IHal_WatchDog_Handle_t *IHal_WatchDog_Init(IHAL_INT8 *devnode)
{
	int fd = 0;

	assert(devnode);

        IHal_WatchDog_Attr *attr = (IHal_WatchDog_Attr *)malloc(sizeof(IHal_WatchDog_Attr));
        if (!attr) {
                IHAL_LOGE(TAG, "malloc watchdog attr failed");
                return IHAL_RNULL;
        }

	strncpy(attr->node, devnode, sizeof(attr->node));
        pthread_mutex_init(&attr->lock, NULL);

	fd = open(attr->node, O_WRONLY);
	if (fd < 0) {
		IHAL_LOGE(TAG, "Open dev node (%s) failed", attr->node);
		return -IHAL_RFAILED;
	}
	attr->fd = fd;
	attr->status = 0;

	return (IHal_WatchDog_Handle_t *)attr;
}

IHAL_INT32 IHal_WatchDog_DeInit(IHal_WatchDog_Handle_t *handle)
{
	assert(handle);
	IHal_WatchDog_Attr *attr = (IHal_WatchDog_Attr *)handle;

	close(attr->fd);
	pthread_mutex_destroy(&attr->lock);
	free(attr);
	return IHAL_ROK;
}

IHAL_INT32 IHal_WatchDog_Start(IHal_WatchDog_Handle_t *handle)
{
	assert(handle);
	int ret = 0;
	IHal_WatchDog_Attr *attr = (IHal_WatchDog_Attr *)handle;
	attr->status = WATCHDOG_START;

        pthread_mutex_lock(&attr->lock);

	ret = ioctl(attr->fd, WDIOC_SETOPTIONS, &attr->status);
	if (ret) {
                IHAL_LOGE(TAG, "Watchdog start failed");
                pthread_mutex_unlock(&attr->lock);
                return -IHAL_RERR;
	}

        pthread_mutex_unlock(&attr->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_WatchDog_Stop(IHal_WatchDog_Handle_t *handle)
{
	assert(handle);
	int ret = 0;
	IHal_WatchDog_Attr *attr = (IHal_WatchDog_Attr *)handle;
	attr->status = WATCHDOG_STOP;

        pthread_mutex_lock(&attr->lock);

	ret = ioctl(attr->fd, WDIOC_SETOPTIONS, &attr->status);
	if (ret) {
                IHAL_LOGE(TAG, "Watchdog stop failed");
                pthread_mutex_unlock(&attr->lock);
                return -IHAL_RERR;
	}

        pthread_mutex_unlock(&attr->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_WatchDog_SetTimeout(IHal_WatchDog_Handle_t *handle, int timeout)
{
	assert(handle);
	int ret = 0;
	IHal_WatchDog_Attr *attr = (IHal_WatchDog_Attr *)handle;
	attr->set_timeout = timeout;
        pthread_mutex_lock(&attr->lock);

	ret = ioctl(attr->fd, WDIOC_SETTIMEOUT, &attr->set_timeout);
	if (ret) {
                IHAL_LOGE(TAG, "Watchdog set timeout failed");
                pthread_mutex_unlock(&attr->lock);
                return -IHAL_RERR;
	}

        pthread_mutex_unlock(&attr->lock);
        return IHAL_ROK;
}

IHAL_INT32 IHal_WatchDog_FeedDog(IHal_WatchDog_Handle_t *handle)
{
	assert(handle);
	int ret = 0;
	IHal_WatchDog_Attr *attr = (IHal_WatchDog_Attr *)handle;

        pthread_mutex_lock(&attr->lock);

	//以下两种方式均可进行喂狗操作
#if 1
	ret = ioctl(attr->fd, WDIOC_KEEPALIVE, NULL);
	if (ret) {
                IHAL_LOGE(TAG, "Watchdog feed dog failed");
                pthread_mutex_unlock(&attr->lock);
                return -IHAL_RERR;
	}
#else
	ret = write(attr->fd, "1", 1);
	if (ret != 1) {
                IHAL_LOGE(TAG, "Watchdog stop failed");
                pthread_mutex_unlock(&attr->lock);
                return -IHAL_RERR;
	}
#endif

        pthread_mutex_unlock(&attr->lock);
        return IHAL_ROK;
}


