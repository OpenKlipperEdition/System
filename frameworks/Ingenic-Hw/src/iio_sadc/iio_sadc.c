/**
 * @file    iio_sadc.c
 * @author  MPU系统软件部团队
 * @brief   包含 IIO_SADC HAL 库函数原型的源文件
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
#include <dirent.h>
#include "iio_sadc.h"
#include "hw_log.h"

#define TAG                     "IIO_SADC"
#define IIO_SADC_DEV            "/dev/iio:device0"
#define SYS_IIO_SADC_DIR        "/sys/bus/iio/devices/iio:device0"
#define SYS_IIO_TRIGGER_DIR     "/sys/bus/iio/devices/iio_sysfs_trigger"
#define BUF_LEN                 70

typedef struct {// adc方式1
    int channel_fd;  // 通道文件描述符
} iio_sadc_prv_ctx;

IHAL_IIO_SADC_Handle_t *IHal_IIO_SADC_Init_Channel(const enum IHAL_IIO_SADC_CHANNEL channel)
{
    assert(channel >= 0 && channel <= 15);
    char cmd_buf[BUF_LEN] = {0};
    iio_sadc_prv_ctx *ctx = (iio_sadc_prv_ctx *)malloc(sizeof(iio_sadc_prv_ctx));
    if (!ctx) {
        IHAL_LOGE(TAG, "malloc iio_sadc ctx failed!\n");
        return IHAL_RNULL;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/in_voltage%d_raw", SYS_IIO_SADC_DIR, channel);
    ctx->channel_fd = open(cmd_buf, O_RDWR);
    if (ctx->channel_fd < 0) {
        IHAL_LOGE(TAG, "init iio_sadc channle fail, open file(%s) fail\n", cmd_buf);
        free(ctx);
        return IHAL_RNULL;
    }
    return (IHAL_IIO_SADC_Handle_t *)ctx;
}
IHAL_INT32 IHal_IIO_SADC_DeInit_Channel(IHAL_IIO_SADC_Handle_t *handle)
{
    assert(handle);
    iio_sadc_prv_ctx *ctx = (iio_sadc_prv_ctx *)handle;
    close(ctx->channel_fd);
    free(ctx);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_Read_Raw_Data(IHAL_IIO_SADC_Handle_t *handle)
{
    assert(handle);
    int ret = -1;
    unsigned int raw_data = 0;
    char data[10] = {0};    // 采集的raw_data
    iio_sadc_prv_ctx *ctx = (iio_sadc_prv_ctx *)handle;
    ret = read(ctx->channel_fd, data, sizeof(data));
    if (ret < 0) {
        IHAL_LOGE(TAG, "read channle data fail!\n");
        return -IHAL_RERR;
    }
    lseek(ctx->channel_fd, 0, SEEK_SET);
    raw_data = atoi(data);
    raw_data &= 0x0fff;     // 低12位原始数据
    return raw_data;
}

typedef struct {// adc方式2
    char trigger_name[20];      // trigger名称
    char trigger_dir_name[20];  // sys下trigger目录名称
    int  channel_enable_num;    // enable通道数
} iio_sadc_trigger_prv_ctx;

//查看是否存在trigger 如trigger0, 存在则存储trigger信息(trigger名称和sys下trigger目录名称)
static int check_trigger(IHAL_IIO_SADC_Handle_t *handle, const int trigger_num)
{
    assert(handle);
    assert(trigger_num >= 0);
    int fd = -1;
    int ret = -1;
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    char cmd_buf[BUF_LEN] = {0};
    char trigger_name[20] = {0}; // 存放trigger name
    struct dirent *entry;
    DIR *dir = opendir(SYS_IIO_TRIGGER_DIR);
    while ((entry = readdir(dir)) != 0) {
        if (entry->d_type != DT_DIR) {
            continue;
        }
        //对比该路径下的所有文件名, 是否存在以trigger开头的文件夹名称
        if (strstr(entry->d_name, "trigger")) {
            memset(cmd_buf, 0, sizeof(cmd_buf));
            sprintf(cmd_buf, "%s/%s/name", SYS_IIO_TRIGGER_DIR, entry->d_name);
            fd = open(cmd_buf, O_RDONLY);
            if (fd < 0) {
                IHAL_LOGW(TAG, "open trigger file(%s) fail!\n", cmd_buf);
                continue;
            }
            ret = read(fd, trigger_name, sizeof(trigger_name));
            if (ret < 0) {
                IHAL_LOGW(TAG, "read trigger name fail!, read file(%s) fail\n", cmd_buf);
                close(fd);
                continue;
            }
            memset(cmd_buf, 0, sizeof(cmd_buf));
            sprintf(cmd_buf, "sysfstrig%d", trigger_num);
            // 匹配成功
            if (strstr(trigger_name, cmd_buf)) {
                sprintf(ctx->trigger_name, "%s", trigger_name);
                sprintf(ctx->trigger_dir_name, "%s", entry->d_name);
                return IHAL_ROK;
            }
            close(fd);
        }
    }
    return -IHAL_RERR;
}
//创建/摧毁trigger 如trigger0
static int create_and_destroy_trigger(IHAL_IIO_SADC_Handle_t *handle, const int trigger_num, int status)
{
    assert(handle);
    assert(trigger_num >= 0);
    assert(status == 1 || status == 0);
    int ret = -1;
    int fd = -1;
    char cmd_buf[BUF_LEN] = {0};
    memset(cmd_buf, 0, sizeof(cmd_buf));
    if (status == 1) {
        sprintf(cmd_buf, "%s/add_trigger", SYS_IIO_TRIGGER_DIR);
    } else {
        sprintf(cmd_buf, "%s/remove_trigger", SYS_IIO_TRIGGER_DIR);
    }
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "create iio sadc trigger fail, open file(%s) fail\n", cmd_buf);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%d", trigger_num);
    ret = write(fd, cmd_buf, sizeof(cmd_buf));
    if (ret < 0) {
        IHAL_LOGE(TAG, "create iio sadc trigger fail!, write file fail\n");
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    return IHAL_ROK;
}
//绑定/解绑trigger 如trigger0
static int bind_and_unbind_trigger(IHAL_IIO_SADC_Handle_t *handle, int status)
{
    assert(handle);
    assert(status == 1 || status == 0);
    int fd = -1;
    int ret = -1;
    char cmd_buf[BUF_LEN] = {0};
    char trigger_name[20] = {0};
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    sprintf(cmd_buf, "%s/trigger/current_trigger", SYS_IIO_SADC_DIR);
    fd = open(cmd_buf, O_RDWR);
    if (fd < 0) {
        IHAL_LOGE(TAG, "bind and unbind trigger fail, open file(%s) fail\n", cmd_buf);
        return -IHAL_RERR;
    }
    // bind
    if (status == 1) {
        ret = read(fd, trigger_name, sizeof(trigger_name));
        if (ret < 0) {
            IHAL_LOGE(TAG, "read trigger name fail!\n");
            close(fd);
            return -IHAL_RERR;
        }
        memset(cmd_buf, 0, sizeof(cmd_buf));
        sprintf(cmd_buf, "%s", ctx->trigger_name);
        if (strstr(trigger_name, cmd_buf)) {
            //IHAL_LOGW(TAG, "trigger has binded!\n");
            close(fd);
            return IHAL_ROK;
        }
        memset(cmd_buf, 0, sizeof(cmd_buf));
        sprintf(cmd_buf, "%s", ctx->trigger_name);
        // unbind
    } else if (status == 0) {
        sprintf(cmd_buf, "none");

    }
    ret = write(fd, cmd_buf, strlen(cmd_buf));
    if (ret < 0) {
        IHAL_LOGE(TAG, "wirte file fail\n");
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    return IHAL_ROK;
}
//disabe所有的adc通道
static int disable_all_channel(IHAL_IIO_SADC_Handle_t *handle)
{
    assert(handle);
    int fd = -1;
    int ret = -1;
    char cmd_buf[BUF_LEN] = {0};
    for (int i = 0; i < 16 ; i++) {
        memset(cmd_buf, 0, sizeof(cmd_buf));
        sprintf(cmd_buf, "%s/scan_elements/in_voltage%d_en", SYS_IIO_SADC_DIR, i);
        ret = access(cmd_buf, F_OK);
        if (ret == 0) { // 文件存在
            fd = open(cmd_buf, O_RDWR);
            if (fd < 0) {
                IHAL_LOGW(TAG, "open file(%s) fail!\n", cmd_buf);
                return -IHAL_RERR;
            }
            ret = write(fd, "0", 1);
            if (ret < 0) {
                IHAL_LOGW(TAG, "disable channel fail\n");
                close(fd);
                return -IHAL_RERR;
            }
            close(fd);
        }
    }
    return IHAL_ROK;
}
//check create bind
static int check_create_bind_trigger(IHAL_IIO_SADC_Handle_t *handle, const int trigger_num)
{
    assert(handle);
    assert(trigger_num >= 0);
    int ret = -1;
    ret = check_trigger(handle, trigger_num);
    if (ret < 0) { //没有check到
        ret = create_and_destroy_trigger(handle, trigger_num, 1);//创建tigger
        if (ret < 0) {
            IHAL_LOGE(TAG, "create iio sadc trigger fail!\n");
            return -IHAL_RERR;
        }
        ret = check_trigger(handle, trigger_num);//保存trigger信息
        if (ret < 0) {
            IHAL_LOGE(TAG, "check iio sadc trigger fail!\n");
            return -IHAL_RERR;
        }
    }
    ret = bind_and_unbind_trigger(handle, 1);//绑定trigger
    if (ret < 0) {
        IHAL_LOGE(TAG, "bind iio sadc trigger fail!\n");
        return -IHAL_RERR;
    }
    return IHAL_ROK;
}
//check destroy unbind
static int check_destroy_unbind_trigger(IHAL_IIO_SADC_Handle_t *handle, const int trigger_num)
{
    assert(handle);
    assert(trigger_num >= 0);
    int ret = -1;
    ret = check_trigger(handle, trigger_num);
    if (ret >= 0) { //check到
        ret = create_and_destroy_trigger(handle, trigger_num, 0);//摧毁tigger
        if (ret < 0) {
            IHAL_LOGE(TAG, "destroy iio sadc trigger fail!\n");
            return -IHAL_RERR;
        }
    }
    ret = bind_and_unbind_trigger(handle, 0);//解绑trigger
    if (ret < 0) {
        IHAL_LOGE(TAG, "unbind iio sadc trigger fail!\n");
        return -IHAL_RERR;
    }
    return IHAL_ROK;
}
IHAL_IIO_SADC_Handle_t *IHal_IIO_SADC_TRIGGER_Init(void)
{
    int ret = -1;
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)malloc(sizeof(iio_sadc_trigger_prv_ctx));
    if (!ctx) {
        IHAL_LOGE(TAG, "malloc iio_tigger_sadc ctx fail!\n");
        return IHAL_RNULL;
    }
    // trigger0
    ret = check_create_bind_trigger((IHAL_IIO_SADC_Handle_t *)ctx, 0);
    if (ret < 0) {
        IHAL_LOGE(TAG, "check create bind iio_trigger fail!\n");
        free(ctx);
        return IHAL_RNULL;
    }
    // 确保trigger关闭
    IHal_IIO_SADC_TRIGGER_Stop_Sample((IHAL_IIO_SADC_Handle_t *)ctx);
    // 确保disable通道，避免buff数据出现紊乱
    ret = disable_all_channel((IHAL_IIO_SADC_Handle_t *)ctx);
    if (ret < 0) {
        IHAL_LOGE(TAG, "disable all channel fail!\n");
        return IHAL_RNULL;
    }
    ctx->channel_enable_num = 0;
    return (IHAL_IIO_SADC_Handle_t *)ctx;;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_DeInit(IHAL_IIO_SADC_Handle_t *handle)
{
    assert(handle);
    //int ret = -1;
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    /* 注释该代码片段, 不影响整体操作
    //trigger0
    ret = check_destroy_unbind_trigger(handle, 0);// 压力测试 反复创建\摧毁trigger0 100多次后,创建出现Cannot allocate memory
    if (ret < 0)
    {
         IHAL_LOGW(TAG, "deinit fail!\n");
        //return -IHAL_RERR;    //如果撤销失败不影响下次操作
    }*/
    free(ctx);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Enable_Channel(IHAL_IIO_SADC_Handle_t *handle, const enum IHAL_IIO_SADC_CHANNEL channel)
{
    assert(handle);
    assert(channel >= 0 && channel <= 15);
    int fd = -1;
    int ret = -1;
    char cmd_buf[BUF_LEN] = {0};
    char ch = '0';
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/scan_elements/in_voltage%d_en", SYS_IIO_SADC_DIR, channel);
    fd = open(cmd_buf, O_RDWR);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = read(fd, &ch, 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "read file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    // 判断原始是否enable, 无则使能
    if (ch == '1') { // 已经enable
        return IHAL_ROK;
    }
    ret = write(fd, "1", 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "enable sample fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    ctx->channel_enable_num++;
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Disable_Channel(IHAL_IIO_SADC_Handle_t *handle, const enum IHAL_IIO_SADC_CHANNEL channel)
{
    assert(handle);
    assert(channel >= 0 && channel <= 15);
    int fd = -1;
    int ret = -1;
    char cmd_buf[BUF_LEN] = {0};
    char ch = '1';
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/scan_elements/in_voltage%d_en", SYS_IIO_SADC_DIR, channel);
    fd = open(cmd_buf, O_RDWR);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = read(fd, &ch, 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "read file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    if (ch == '0') { // 已经disable
        return IHAL_ROK;
    }
    ret = write(fd, "0", 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "enable sample fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    ctx->channel_enable_num--;
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Set_Sequence_Buff_Length(IHAL_IIO_SADC_Handle_t *handle, const int length)
{
    assert(handle);
    assert(length >= 0);
    int ret = -1;
    int fd = -1;
    char cmd_buf[BUF_LEN] = {0};
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/buffer/length", SYS_IIO_SADC_DIR);
    fd = open(cmd_buf, O_RDWR);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%d", length);
    ret = write(fd, cmd_buf, sizeof(cmd_buf));
    if (ret < 0) {
        IHAL_LOGE(TAG, "set buff length fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Start_Sample(IHAL_IIO_SADC_Handle_t *handle)
{
    assert(handle);
    int ret = -1;
    int fd = -1;
    char cmd_buf[BUF_LEN] = {0};
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/buffer/enable", SYS_IIO_SADC_DIR);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = write(fd, "1", 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "enable buff fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/%s/trigger_now", SYS_IIO_TRIGGER_DIR, ctx->trigger_dir_name);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = write(fd, "1", 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "start sample fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Stop_Sample(IHAL_IIO_SADC_Handle_t *handle)
{
    assert(handle);
    int ret = -1;
    int fd = -1;
    char cmd_buf[BUF_LEN] = {0};
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/%s/trigger_now", SYS_IIO_TRIGGER_DIR, ctx->trigger_dir_name);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = write(fd, "0", 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "start sample fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    /* 该延迟为了确保adc设备能够真的暂停，不能删除该延迟或者减小延迟时间 */
    usleep(10000);
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s/buffer/enable", SYS_IIO_SADC_DIR);
    fd = open(cmd_buf, O_WRONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = write(fd, "0", 1);
    if (ret < 0) {
        IHAL_LOGE(TAG, "enable buff fail, write file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    close(fd);
    return IHAL_ROK;
}
IHAL_INT32 IHal_IIO_SADC_TRIGGER_Read_Sequence_Buff_Data(IHAL_IIO_SADC_Handle_t *handle, IIO_SADC_CHANNEL_DATA *data)
{
    assert(handle);
    assert(data);
    int ret = -1;
    int fd = -1;
    char cmd_buf[BUF_LEN] = {0};
    iio_sadc_trigger_prv_ctx *ctx = (iio_sadc_trigger_prv_ctx *)handle;
    unsigned int raw_data[16] = {0}; // 16个通道的数据
    int ch = -1;// 通道号
    memset(cmd_buf, 0, sizeof(cmd_buf));
    sprintf(cmd_buf, "%s", IIO_SADC_DEV);
    fd = open(cmd_buf, O_RDONLY);
    if (fd < 0) {
        IHAL_LOGE(TAG, "open file(%s) fail!\n", cmd_buf);
        return -IHAL_RERR;
    }
    ret = read(fd, &raw_data[0], sizeof(raw_data[0]) * ctx->channel_enable_num);
    if (ret < 0) {
        IHAL_LOGE(TAG, "read buff data fail, read file(%s) fail!\n", cmd_buf);
        close(fd);
        return -IHAL_RERR;
    }
    for (int i = 0; i < ctx->channel_enable_num; i++) {
        ch = (raw_data[i] & 0x0F000) >> 12;
        //  printf("num:%d\n",ctx->channel_enable_num);
        //  printf("ch:%d data:%d\n",ch,raw_data[i]);
        raw_data[i] = raw_data[i] & 0x0fff;
        (*data).channel_data[ch] = raw_data[i];
    }
    close(fd);
    return IHAL_ROK;
}
