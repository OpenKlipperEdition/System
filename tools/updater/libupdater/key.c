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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <linux/input.h>
#include <dirent.h>
#include <math.h>
#include <assert.h>
#include <Utils/log.h>
#include <hardware/key.h>

#define INPUT_DEV_PATH  "/dev/input"

int read_key_event(int fd, struct key_event *key_event) {
    int ret = 0;
    struct input_event event;

    ret = read(fd, &event, sizeof(event));
    if (ret < 0) {
        log_e("%s: read key_event error, %s(%d)\n",
                __func__, strerror(errno), -errno);

        return ret;
    }

    if ((event.type == EV_KEY) && (event.code < BTN_MISC)) {
        key_event->key_type = event.code;
        key_event->is_press = event.value;

        return 1;
    }

    return 0;
}

int read_keys_event(int fds[], int len, struct key_event *key_event) {
    int i, ret = 0;
    int fd_max = 0;
    struct input_event event;

    fd_set read_fs;
    FD_ZERO(&read_fs);

    for (i = 0; i < len; i++) {
        fd_max = fd_max > fds[i] ? fd_max : fds[i];
        FD_SET(fds[i], &read_fs);
    }

    ret = select(FD_SETSIZE, &read_fs, NULL, NULL, NULL);
    if (ret < 0) {
        log_e("%s: select error: %s(%d)\n",
                __func__, strerror(errno), -errno);
        return 0;
    }

    for (i = 0; i < len; i++) {
        if (FD_ISSET(fds[i], &read_fs)) {
            ret = read(fds[i], &event, sizeof(event));
            if (ret < 0) {
                log_e("%s: read key_event error, %s(%d)\n",
                        __func__, strerror(errno), -errno);

                return ret;
            }

            if ((event.type == EV_KEY) && (event.code < BTN_MISC)) {
                key_event->key_type = event.code;
                key_event->is_press = event.value;

                return 1;
            }
        }
    }

    return 0;
}

int read_key_event_timeout(int fd, struct key_event *key_event, int timeout_ms) {
    return read_keys_event_timeout(&fd, 1, key_event, timeout_ms);
}

int read_keys_event_timeout(int fds[], int len, struct key_event *key_event, int timeout_ms) {
    int i, ret = 0;
    int fd_max = 0;
    struct input_event event;
    struct timeval tv;

    fd_set read_fs;
    FD_ZERO(&read_fs);

    for (i = 0; i < len; i++) {
        fd_max = fd_max > fds[i] ? fd_max : fds[i];
        FD_SET(fds[i], &read_fs);
    }

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    ret = select(FD_SETSIZE, &read_fs, NULL, NULL, &tv);
    if (ret < 0) {
        log_e("%s: select error: %s(%d)\n",
                __func__, strerror(errno), -errno);
        return 0;
    }

    for (i = 0; i < len; i++) {
        if (FD_ISSET(fds[i], &read_fs)) {
            ret = read(fds[i], &event, sizeof(event));
            if (ret < 0) {
                log_e("%s: read key_event error, %s(%d)\n",
                        __func__, strerror(errno), -errno);

                return ret;
            }

            if ((event.type == EV_KEY) && (event.code < BTN_MISC)) {
                key_event->key_type = event.code;
                key_event->is_press = event.value;

                return 1;
            }
        }
    }

    return 0;
}


static int is_key_device(const struct dirent *dir) {
    return strncmp("event", dir->d_name, 5) == 0;
}

int keys_open(int fds[], int len) {
    struct dirent **namelist;
    int i, ndev, fd;
    int cnt = 0;

    assert(len > 0);

    ndev = scandir(INPUT_DEV_PATH, &namelist, is_key_device, NULL);
    if (ndev < 0) {
        log_e("failed to find key event deivce.\n");
        return -1;
    }

    for (i = 0; i < ndev; i++) {
        char fname[64];
        char name[256];
        unsigned char mask[EV_MAX / 8 + 1];

        sprintf(fname, "%s/%s", INPUT_DEV_PATH, namelist[i]->d_name);

        fd = open(fname, O_RDONLY);
        if (fd < 0) {
            log_e("failed to open %s, %s\n", fname, strerror(errno));
            continue;
        }

        free(namelist[i]);

        ioctl(fd, EVIOCGBIT(0, sizeof(mask)), mask);
        if (mask[EV_KEY / 8] & (1 << (EV_KEY % 8))) {

            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            fds[cnt++] = fd;

            if (cnt >= len) {
                for (i++; i < ndev; i++) {
                    free(namelist[i]);
                }

                break;
            }
        }
        else {
            close(fd);
        }
    }

    return cnt;
}

int key_open(void) {
    int fd = -1;

    keys_open(&fd, 1);

    return fd;
}

void keys_close(int fds[], int len) {
    int i;
    for (i = 0; i < len; i++) {
        close(fds[i]);
    }
}

void key_close(int fd) {
    keys_close(&fd, 1);
}

