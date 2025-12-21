#ifndef __MY_CLIENTLISTENER_H
#define __MY_CLIENTLISTENER_H
#include <functional>
#include <icutils/sockets.h>
#include <list>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <thread>
#include <unistd.h>
#include "ClientListener.h"
class MyClientListener {
private:
    std::list<ClientCommand *> mCommands;
    std::string sockName;
    int sock;
    bool isData;
    std::thread mThread;
    volatile bool stop;
    void dispatchCommand(char *data);
    void dispatchCommandData(struct iovec *iov);

public:
    static const int CMD_ARGS_MAX = 26;
    MyClientListener(const char *sockName_);
    ~MyClientListener();
    void registerCmd(ClientCommand *cmd);

    int sendDataLockedv(struct iovec *iov, int iovcnt);
    int SendBinaryMsg(int code, void *data, int len);

    int startListener();
    int startListenerData();
    int stopListener();
    void RunLoop();
};

#endif /* __MY_CLIENTLISTENER_H */
