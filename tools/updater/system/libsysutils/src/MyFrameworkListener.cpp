/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "utils.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sysutils/FrameworkCommand.h>
#include <sysutils/MyFrameworkListener.h>
#include <sysutils/SocketClient.h>

static const int CMD_BUF_SIZE = 4096;

#define UNUSED __attribute__((unused))

MyFrameworkListener::MyFrameworkListener(const char *socketName, bool withSeq) : SocketListener(socketName, true, withSeq) {
    init(socketName, withSeq);
}

MyFrameworkListener::MyFrameworkListener(const char *socketName) : SocketListener(socketName, true, false) {
    init(socketName, false);
}

MyFrameworkListener::MyFrameworkListener(int sock) : SocketListener(sock, true) {
    init(NULL, false);
}

void MyFrameworkListener::init(const char *socketName UNUSED, bool withSeq) {
    mCommands = new FrameworkCommandCollection();
    errorRate = 0;
    mCommandCount = 0;
    mWithSeq = withSeq;
}
#if 1
static int printPacketInfo(const char *title, const char *buf, int size) {
    int j, i, k;
    char prtinfo[512], pr2[256];

    printf(" -%s-size=%d--- -start-------\n", title, size);
    for (k = i = 0; i < size; k++) {
        prtinfo[0] = 0;
        pr2[0] = 0;
        for (j = 0; (i < size && j < 16); j++, i++) {
            sprintf(pr2 + j, "%c", isgraph(buf[i]) ? buf[i] : ' ');
            sprintf(prtinfo + strlen(prtinfo), "%02X, ", ((unsigned int)buf[i]) & 0xff);
        }
        printf("%3x| %s -- %s\n", k, prtinfo, pr2);
    }
    printf("str:--%s--\n", buf);
    printf(" -%s-size=%d--end----------\n", title, size);
}
#endif
bool MyFrameworkListener::onDataAvailable(SocketClient *c) {
    char buffer[CMD_BUF_SIZE];
    int len;
    bool ret = true;

    len = TEMP_FAILURE_RETRY(read(c->getSocket(), buffer, sizeof(buffer)));
    if (len < 0) {
        printf("read() failed (%s)\n", strerror(errno));
        ret = false;
        sprintf(buffer, "%s %s", "Register", "false");
        len = strlen(buffer) + 1;
        return ret;
    } else if (!len) {
        sprintf(buffer, "%s %s", "Register", "false");
        len = strlen(buffer) + 1;
        ret = false;
        return ret;
    }
    //clivia printPacketInfo("MyFrameworkListener", buffer, (len < 64) ? 64 : len + 8);
#if 0
    c->sendBinaryMsg(0x1234, buffer, len);
#else
    if (buffer[len - 1] != '\0')
        printf("String is not zero-terminated\n");

    int offset = 0;
    int i;

    if (!strncmp(buffer, "Register", 8)) {
        for (i = 0; i < len; i++) {
            if (buffer[i] == '\0') {
                /* IMPORTANT: dispatchCommand() expects a zero-terminated string */
                dispatchCommand(c, buffer + offset);
                offset = i + 1;
            }
        }
    } else {
        dispatchCommand(c, buffer, len);
    }
#endif

    return ret;
}

void MyFrameworkListener::registerCmd(FrameworkCommand *cmd) {
    mCommands->push_back(cmd);
}

void MyFrameworkListener::dispatchCommand(SocketClient *cli, char *data, int len) {
    FrameworkCommandCollection::iterator i;
    int argc = 0;
    char *argv[MyFrameworkListener::CMD_ARGS_MAX];
    char tmp[CMD_BUF_SIZE];
    char *ptr = data;
    int *a1, *a2, buflen;

    printf("MyFrameworkListener::dispatchCommand\n");
    while (len > 8) {
        memset(argv, 0, sizeof(argv));
        memcpy(&buflen, (ptr + sizeof(int)), sizeof(int));
        if (buflen+8 <= len) {
            memcpy(tmp, ptr, buflen+8);
            argv[0] = tmp;
            argv[1] = tmp + sizeof(int) * 2;
            argc = 2;

            for (i = mCommands->begin(); i != mCommands->end(); ++i) {
                FrameworkCommand *c = *i;
                if (c->runCommand(cli, argc, argv)) {
                    printf("Handler '%s' error (%s)\n", c->getCommand(), strerror(errno));
                }
            }
            ptr += buflen+8;
            len -= buflen+8;
        }else {
            printf("Wrong ,server buflen:%d,len:%d,drop it\n", buflen, len);
            printPacketInfo("MyFrameworkListener drop buffer", ptr, len);
            break;
        }
    }
    return;
}

void MyFrameworkListener::dispatchCommand(SocketClient *cli, char *data) {
    FrameworkCommandCollection::iterator i;
    int argc = 0;
    char *argv[MyFrameworkListener::CMD_ARGS_MAX];
    char tmp[CMD_BUF_SIZE];
    char *p = data;
    char *q = tmp;
    char *qlimit = tmp + sizeof(tmp) - 1;
    bool esc = false;
    bool quote = false;
    bool haveCmdNum = !mWithSeq;

    memset(argv, 0, sizeof(argv));
    memset(tmp, 0, sizeof(tmp));
    while (*p) {
        if (*p == '\\') {

        } else if (esc) {
            if (*p == '"') {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '"';
            } else if (*p == '\\') {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '\\';
            } else {
                cli->sendMsg(500, "Unsupported escape sequence", false);
                goto out;
            }
            p++;
            esc = false;
            continue;
        }

        if (*p == '"') {
            if (quote)
                quote = false;
            else
                quote = true;
            p++;
            continue;
        }

        if (q >= qlimit)
            goto overflow;
        *q = *p++;
        if (!quote && *q == ' ') {
            *q = '\0';
            if (!haveCmdNum) {
                char *endptr;
                int cmdNum = (int)strtol(tmp, &endptr, 0);
                if (endptr == NULL || *endptr != '\0') {
                    cli->sendMsg(500, "Invalid sequence number", false);
                    goto out;
                }
                cli->setCmdNum(cmdNum);
                haveCmdNum = true;
            } else {
                if (argc >= CMD_ARGS_MAX)
                    goto overflow;
                argv[argc++] = strdup(tmp);
            }
            memset(tmp, 0, sizeof(tmp));
            q = tmp;
            continue;
        }
        q++;
    }

    *q = '\0';
    if (argc >= CMD_ARGS_MAX)
        goto overflow;
    argv[argc++] = strdup(tmp);
#if 0
    for (int k = 0; k < argc; k++) {
        printf("arg[%d] = '%s'\n", k, argv[k]);
    }
#endif

    if (quote) {
        cli->sendMsg(500, "Unclosed quotes error", false);
        goto out;
    }

    if (errorRate && (++mCommandCount % errorRate == 0)) {
        /* ignore this command - let the timeout handler handle it */
        printf("Faking a timeout\n");
        goto out;
    }
    for (i = mCommands->begin(); i != mCommands->end(); ++i) {
        FrameworkCommand *c = *i;

        if (!strcmp(argv[0], c->getCommand())) {

            if (c->runCommand(cli, argc, argv)) {
                printf("Handler '%s' error (%s)\n", c->getCommand(), strerror(errno));
            }
            goto out;
        }
    }
    cli->sendMsg(500, "Command not recognized", false);
out:
    int j;
    for (j = 0; j < argc; j++)
        free(argv[j]);
    return;

overflow:
    //LOG_EVENT_INT(78001, cli->getUid());
    cli->sendMsg(500, "Command too long", false);
    goto out;
}
