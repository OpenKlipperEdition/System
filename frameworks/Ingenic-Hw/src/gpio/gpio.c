/**
 * @file    gpio.c
 * @author  MPU系统软件部团队
 * @brief   包含 GPIO HAL 库函数原型的源文件
 *
 * @copyright 版权所有 (北京君正集成电路股份有限公司) {2023}
 * @copyright Copyright© 2023 Ingenic Semiconductor Co.,Ltd
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <poll.h>
#include "gpio.h"
#include "hw_log.h"

#define TAG "GPIO"
#define SYS_GPIO_DIR "/sys/class/gpio"
#define BUF_LEN 40

static IHAL_INT32 gpio_export(IHAL_GPIO_Handle_t *handle);
static IHAL_INT32 gpio_unexport(IHAL_GPIO_Handle_t *handle);
typedef struct {
    enum IHAL_GPIO_GROUP group;
    short pin;
    pthread_mutex_t lock;
} gpio_prv_ctx;

static IHAL_INT32 gpio_export(IHAL_GPIO_Handle_t *handle)
{
    assert(handle);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret == 0) {
        ret = gpio_unexport(ctx);  //for interrupt
        if (ret < 0) {
            IHAL_LOGW(TAG, "gpio%d unexport failed\n", ctx->group + ctx->pin);
            return -IHAL_RERR;
        }
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/export", SYS_GPIO_DIR);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%d", ctx->group + ctx->pin);
    pthread_mutex_lock(&ctx->lock);
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "export gpio fail, write file(%s) failed\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    close(fd);
    return IHAL_ROK;
}
static IHAL_INT32 gpio_unexport(IHAL_GPIO_Handle_t *handle)
{
    assert(handle);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret < 0) {
        IHAL_LOGW(TAG, "gpio%d has been unexported\n", ctx->group + ctx->pin);
        return IHAL_ROK;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/unexport", SYS_GPIO_DIR);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%d", ctx->group + ctx->pin);
    pthread_mutex_lock(&ctx->lock);
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "unexport gpio fail, write file(%s) failed\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    close(fd);
    return IHAL_ROK;
}
IHAL_GPIO_Handle_t *IHal_GPIO_Init(const enum IHAL_GPIO_GROUP group, const IHAL_INT16 pin)
{
    assert(group == GPIOA || group == GPIOB || group == GPIOC || group == GPIOD || group == GPIOE);
    assert(pin >= 0 && pin <= 31);
    int ret = -1;

    gpio_prv_ctx *ctx = (gpio_prv_ctx *)malloc(sizeof(gpio_prv_ctx));
    if (!ctx) {
        IHAL_LOGE(TAG, "malloc gpio ctx failed\n");
        return IHAL_RNULL;
    }
    ctx->group = group;
    ctx->pin = pin;
    pthread_mutex_init(&ctx->lock, NULL);

    ret = gpio_export(ctx);
    if (ret < 0) {
        IHAL_LOGE(TAG, "export gpio failed\n");
        pthread_mutex_destroy(&ctx->lock);
        free(ctx);
        return IHAL_RNULL;
    }
    return (IHAL_GPIO_Handle_t *)ctx;
}
IHAL_INT32 IHal_GPIO_DeInit(IHAL_GPIO_Handle_t *handle)
{
    assert(handle);
    int ret = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    ret = gpio_unexport(ctx);   // 先Unexport后free 避免内存泄露
    if (ret < 0) {
        IHAL_LOGE(TAG, "unexport gpio failed\n");
    }
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    return ret;
}
IHAL_INT32 IHal_GPIO_Set_Direciton(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_DIRECTION direction)
{
    assert(handle);
    assert(direction == GPIO_IN || direction == GPIO_OUT);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret < 0) {
        IHAL_LOGE(TAG, "gpio%d has no exported\n", ctx->group + ctx->pin);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d/direction", SYS_GPIO_DIR, ctx->group + ctx->pin);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return IHAL_RERR;
    }

    memset(cmd_buf, 0, sizeof(cmd_buf));
    if (direction == GPIO_IN) {
        sprintf(cmd_buf, "in");
    } else if (direction == GPIO_OUT) {
        sprintf(cmd_buf, "out");
    }

    pthread_mutex_lock(&ctx->lock);
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "set direction failed\n");
        close(fd);
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_GPIO_Set_Value(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_VALUE value)
{
    assert(handle);
    assert(value == GPIO_LOW || value == GPIO_HIGH);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret < 0) {
        IHAL_LOGE(TAG, "gpio%d has no exported\n", ctx->group + ctx->pin);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d/value", SYS_GPIO_DIR, ctx->group + ctx->pin);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return IHAL_RERR;
    }

    memset(cmd_buf, 0, sizeof(cmd_buf));
    if (value == GPIO_LOW) {
        sprintf(cmd_buf, "0");
    } else if (value == GPIO_HIGH) {
        sprintf(cmd_buf, "1");
    }

    pthread_mutex_lock(&ctx->lock);
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "set value failed ");
        close(fd);
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_GPIO_Get_Value(IHAL_GPIO_Handle_t *handle)
{
    assert(handle);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    char vals = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret < 0) {
        IHAL_LOGE(TAG, "gpio%d has no exported\n", ctx->group + ctx->pin);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d/value", SYS_GPIO_DIR, ctx->group + ctx->pin);
    fd = open(cmd_buf, O_RDONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = read(fd, &vals, 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "read file(%s) failed\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    return vals - '0';
}
IHAL_INT32 IHal_GPIO_Toggle_Value(IHAL_GPIO_Handle_t *handle)
{
    assert(handle);
    int ret = -1;

    ret = IHal_GPIO_Get_Value(handle);
    if (ret < 0) {
        IHAL_LOGE(TAG, "GPIO_Get_Value failed\n");
        return -IHAL_RERR;
    } else if (ret == 0) {
        ret = IHal_GPIO_Set_Value(handle, GPIO_HIGH);
        if (ret < 0) {
            IHAL_LOGE(TAG, "GPIO_Toggle failed\n");
            return -IHAL_RERR;
        }
    } else if (ret == 1) {
        ret = IHal_GPIO_Set_Value(handle, GPIO_LOW);
        if (ret < 0) {
            IHAL_LOGE(TAG, "GPIO_Toggle failed\n");
            return -IHAL_RERR;
        }
    }
    return IHAL_ROK;
}
IHAL_INT32 IHal_GPIO_Set_Interrupt_mode(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_INTERRUPT interrupt)
{
    assert(handle);
    assert(interrupt == GPIO_NONE || interrupt == GPIO_RISING || interrupt == GPIO_FALLING || interrupt == GPIO_BOTH);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret < 0) {
        IHAL_LOGE(TAG, "gpio%d has no exported\n", ctx->group + ctx->pin);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d/edge", SYS_GPIO_DIR, ctx->group + ctx->pin);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    if (interrupt == GPIO_NONE) {
        sprintf(cmd_buf, "none");
    } else if (interrupt == GPIO_RISING) {
        sprintf(cmd_buf, "rising");
    } else if (interrupt == GPIO_FALLING) {
        sprintf(cmd_buf, "falling");
    } else if (interrupt == GPIO_BOTH) {
        sprintf(cmd_buf, "both");
    }
    pthread_mutex_lock(&ctx->lock);
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "set interrupt mode failed\n");
        close(fd);
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    close(fd);
    return IHAL_ROK;
}
void IHal_GPIO_Interrupt_Detect(IHAL_GPIO_Handle_t *handle, void (*callfunc)(), const IHAL_INT32 count)
{
    assert(handle);
    assert(callfunc);
    assert(count >= -1);
    int ret = -1;
    int gpio_fd = -1;
    struct pollfd pfd;
    char gpio_value;
    char cmd_buf[BUF_LEN] = {0};
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d/value", SYS_GPIO_DIR, ctx->group + ctx->pin);
    gpio_fd = open(cmd_buf, O_RDONLY);
    if (gpio_fd < 0) {
        IHAL_LOGE(TAG, "read file(%s) failed\n", cmd_buf);
    }
    ret = read(gpio_fd, &gpio_value, 1);// 清除中断标志 避免poll()触发
    if (ret < 0) {
        IHAL_LOGE(TAG, "read file(%s) failed\n", cmd_buf);
    }
    //poll() 监测 GPIO 中断事件
    pfd.fd = gpio_fd;
    pfd.events = POLLPRI;
    //监听可读事件（包括中断事件）
    //设置标志为POLLPRI，关心高优先级数据可读，也就是中断，中断就是一种高优先级事件，当中断触发时表示有高优先级数据可被读取

    if (count == ALWAYS) {
        while (1) {
            //poll() 函数等待事件发生，阻塞等待
            ret = poll(&pfd, 1, -1);
            if (ret < 0) {
                IHAL_LOGE(TAG, "poll() failed\n");
                break;
            } else if (ret == 0) {
                IHAL_LOGE(TAG, "timeout occurred\n");
            } else {
                if (pfd.revents & POLLPRI) {
                    //清除中断标志
                    lseek(gpio_fd, 0, SEEK_SET);
                    read(gpio_fd, &gpio_value, 1);
                    //调用中断处理函数
                    callfunc();
                }
            }
        }
    } else {
        for (int i = 0; i < count; i++) {
            //使用 poll() 函数等待事件发生
            ret = poll(&pfd, 1, -1);
            if (ret < 0) {
                IHAL_LOGE(TAG, "poll() failed\n");
                break;
            } else if (ret == 0) {
                IHAL_LOGE(TAG, "timeout occurred\n");
            } else {
                if (pfd.revents & POLLPRI) {
                    //清除中断标志
                    lseek(gpio_fd, 0, SEEK_SET);
                    read(gpio_fd, &gpio_value, 1);
                    //调用中断处理函数
                    callfunc();
                }
            }
        }
    }
    close(gpio_fd);
}
IHAL_INT32 IHal_GPIO_Set_Polarity(IHAL_GPIO_Handle_t *handle, const enum IHAL_GPIO_POLARITY polarity)
{
    assert(handle);
    assert(polarity == GPIO_ACTIVE_LOW || polarity == GPIO_ACTIVE_HIGH);
    char cmd_buf[BUF_LEN] = {0};
    int ret = -1;
    int fd = -1;
    gpio_prv_ctx *ctx = (gpio_prv_ctx *)handle;

    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d", SYS_GPIO_DIR, ctx->group + ctx->pin);
    ret = access(cmd_buf, F_OK);
    if (ret < 0) {
        IHAL_LOGE(TAG, "gpio%d has no exported\n", ctx->group + ctx->pin);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/gpio%d/active_low", SYS_GPIO_DIR, ctx->group + ctx->pin);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) failed\n", cmd_buf);
        return IHAL_RERR;
    }
    if (polarity == GPIO_ACTIVE_LOW) {
        sprintf(cmd_buf, "0");
    } else if (polarity == GPIO_ACTIVE_HIGH) {
        sprintf(cmd_buf, "1");
    }
    pthread_mutex_lock(&ctx->lock);
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        pthread_mutex_unlock(&ctx->lock);
        IHAL_LOGE(TAG, "set ACTIVE_LOW failed\n");
        close(fd);
        return -IHAL_RERR;
    }
    pthread_mutex_unlock(&ctx->lock);
    close(fd);
    return IHAL_ROK;
}
