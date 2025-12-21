/*
 *  Copyright (C) 2017 Ingenic Semiconductor
 *
 *  MaWeiBin <weibin.ma@ingenic.com>
 *
 *  Elf/IDWS Project
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __KEY_H__
#define __KEY_H__

#ifdef  __cplusplus
extern "C" {
#endif

struct key_event {
    int key_type;
    int is_press;
};

/**
 * @brief 查找并打开多个按键的设备
 * @param fds 用于保存打开按键设备的数组
 * @param len 数组长度
 * @return 按键设备的个数
 */
int keys_open(int fds[], int len);

/**
 * @brief 查找并打开一个按键的设备
 * @return 小于0： 打开设备失败
 *         其他： 按键设备的描述符
 */
int key_open(void);

/**
 * @brief 关闭按键设备
 * @param fds 按键设备数组
 * @param len 数组长度
 */
void keys_close(int fds[], int len);
void key_close(int fd);

/**
 * @brief 读取按键事件
 * @param fd 按键设备文件描述符
 * @param key_event 用于接收按键事件
 * @return 小于0：读取按键事件出错
 *         0：读取到非按键事件
 *         1：读取到按键事件
 */
int read_key_event(int fd, struct key_event *key_event);

/**
 * @brief 读取多个按键设备事件
 * @param fds 按键设备文件描述符数组
 * @param len fds数组长度
 * @param key_event 用于接收按键事件
 * @return 小于0：读取按键事件出错
 *         0：读取到非按键事件
 *         1：读取到按键事件
 */
int read_keys_event(int fds[], int len, struct key_event *key_event);

/**
 * @brief 读取按键事件
 * @param fd 按键设备文件描述符
 * @param key_event 用于接收按键事件
 * @param timeout_ms 用于指定超时时间，单位ms
 * @return 小于0：读取按键事件出错
 *         0：读取到非按键事件
 *         1：读取到按键事件
 */
int read_key_event_timeout(int fd, struct key_event *key_event, int timeout_ms);

/**
 * @brief 读取多个按键设备事件
 * @param fds 按键设备文件描述符数组
 * @param len fds数组长度
 * @param key_event 用于接收按键事件
 * @param timeout_ms 用于指定超时时间，单位ms
 * @return 小于0：读取按键事件出错
 *         0：读取到非按键事件
 *         1：读取到按键事件
 */
int read_keys_event_timeout(int fds[], int len, struct key_event *key_event, int timeout_ms);

#ifdef  __cplusplus
}
#endif

#endif

