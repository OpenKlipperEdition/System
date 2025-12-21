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
#ifndef _MY_FRAMEWORKSOCKETLISTENER_H
#define _MY_FRAMEWORKSOCKETLISTENER_H

#include "SocketListener.h"
#include "FrameworkCommand.h"

class SocketClient;

class MyFrameworkListener : public SocketListener {
public:
    static const int CMD_ARGS_MAX = 26;

    /* 1 out of errorRate will be dropped */
    int errorRate;

private:
    int mCommandCount;
    bool mWithSeq;
    FrameworkCommandCollection *mCommands;

public:
    MyFrameworkListener(const char *socketName);
    MyFrameworkListener(const char *socketName, bool withSeq);
    MyFrameworkListener(int sock);
    virtual ~MyFrameworkListener() {}

protected:
    void registerCmd(FrameworkCommand *cmd);
    virtual bool onDataAvailable(SocketClient *c);

private:
    void dispatchCommand(SocketClient *cli, char *data, int len);
    void dispatchCommand(SocketClient *c, char *data);
    void init(const char *socketName, bool withSeq);
};
#endif
