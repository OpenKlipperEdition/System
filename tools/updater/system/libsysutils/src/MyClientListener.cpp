#include <arpa/inet.h>
#include <fcntl.h>
#include <functional>
#include <list>
#include <signal.h>
#include <sysutils/MyClientListener.h>
#include <thread>

static const int CMD_BUF_SIZE = 1024;
MyClientListener::MyClientListener(const char *sockName_) {
    sockName = sockName_;
    sock = -1;
}
MyClientListener::~MyClientListener() {
    for (auto i = mCommands.begin(); i != mCommands.end(); ++i) {
        ClientCommand *c = *i;
        delete c;
    }
}
int MyClientListener::startListener() {
    stop = false;
    isData = false;
    mThread = std::thread(std::bind(&MyClientListener::RunLoop, this));
    return 0;
}
int MyClientListener::stopListener() {
    stop = true;
    mThread.join();
    return 0;
}
int MyClientListener::startListenerData() {
    stop = false;
    isData = true;
    mThread = std::thread(std::bind(&MyClientListener::RunLoop, this));
    return 0;
}
int MyClientListener::sendDataLockedv(struct iovec *iov, int iovcnt) {

    if (sock < 0) {
        errno = EHOSTUNREACH;
        return -1;
    }

    if (iovcnt <= 0) {
        return 0;
    }

    int ret = 0;
    int e = 0; // SLOGW and sigaction are not inert regarding errno
    int current = 0;

    struct sigaction new_action, old_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &new_action, &old_action);

    for (;;) {
        ssize_t rc = TEMP_FAILURE_RETRY(
            writev(sock, iov + current, iovcnt - current));

        if (rc > 0) {
            size_t written = rc;
            while ((current < iovcnt) && (written >= iov[current].iov_len)) {
                written -= iov[current].iov_len;
                current++;
            }
            if (current == iovcnt) {
                break;
            }
            iov[current].iov_base = (char *)iov[current].iov_base + written;
            iov[current].iov_len -= written;
            continue;
        }

        if (rc == 0) {
            e = EIO;
            printf("0 length write :(\n");
        } else {
            e = errno;
            printf("write error (%s)\n", strerror(e));
        }
        ret = -1;
        break;
    }

    sigaction(SIGPIPE, &old_action, &new_action);

    errno = e;
    return ret;
}

int MyClientListener::SendBinaryMsg(int code, void *data, int len) {
    if (!stop) {
        if (sock >= 0) {
            int buf[2] = {0};

            buf[0] = code;
            buf[1] = len;

            struct iovec vec[2];
            vec[0].iov_base = (void *)buf;
            vec[0].iov_len = sizeof(int) * 2;
            vec[1].iov_base = (void *)data;
            vec[1].iov_len = len;

            //pthread_mutex_lock(&mWriteMutex);
            int result = sendDataLockedv(vec, (len > 0) ? 2 : 1);
            //pthread_mutex_unlock(&mWriteMutex);

            return result;
        }
    }
}

void MyClientListener::RunLoop() {
#define PAGE_SIZE 4096
    char pageBuffer[PAGE_SIZE];
    char *cp;
    int len;
    int ret = -1;
    int offset;
    int retry = 0;
    while (!stop) {
        if (sock == -1) {
            char reg[] = "Register";
            char buff[100];
            sock = socket_local_client(sockName.c_str(), ANDROID_SOCKET_NAMESPACE_RESERVED,
                                       SOCK_STREAM);
            if (sock < 0) {
                sleep(1);
                sock = -1;
                continue;
            }
        retrysend:
            ret = TEMP_FAILURE_RETRY(write(sock, reg, strlen(reg) + 1));
            if (ret <= 0) {
                close(sock);
                sock = -1;
                continue;
            }
            cp = buff;
            len = sizeof(buff);

            while ((ret = TEMP_FAILURE_RETRY(read(sock, cp, len))) > 0) {
                if (cp - buff + ret >= 7 && strncmp("success", buff, 7) == 0) {
                    printf("%s client register ok.", sockName.c_str());
                    retry = 0;
                    break;
                } else {
                    retry = 1;
                }
                len -= ret;
                cp += ret;
            }
            if (ret < 0) {
                close(sock);
                sock = -1;
                printf("retry register %s\n", sockName.c_str());
                sleep(1);
                continue;
            }

            if (retry)
                goto retrysend;
        }
        cp = pageBuffer;
        len = sizeof(pageBuffer);
        offset = 0;

        int code[2] = {0};
        char data[PAGE_SIZE] = {0};

        struct iovec iov[2];
        iov[0].iov_base = code;
        iov[0].iov_len = sizeof(code);
        iov[1].iov_base = data;
        iov[1].iov_len = 0;
        struct iovec *iov_tmp = iov;

        while (!stop) {
            if (isData) {

                int i = 0;
                while ((ret = TEMP_FAILURE_RETRY(readv(sock, iov, 1))) > 0) {
                    if (ret < iov[0].iov_len)
                        continue;
                    iov[1].iov_len = code[1];
                    break;
                }
                while ((ret = TEMP_FAILURE_RETRY(readv(sock, &iov[1], 1))) > 0) {
                    if (ret < iov[1].iov_len)
                        continue;

                    dispatchCommandData(iov);
                    break;
                }
            } else {
                while ((ret = TEMP_FAILURE_RETRY(read(sock, cp, len))) > 0) {
                    if (cp[len - 1] == 0) {
                        dispatchCommand(pageBuffer + offset);
                        cp = pageBuffer;
                        len = sizeof(pageBuffer);
                        offset = 0;
                    } else {
                        len -= ret;
                        cp += ret;
                        if (len == 0) {
                            printf("String is not zero-terminated\n");
                        }
                    }
                }
            }
            if (!(ret > 0))
                break;
        }
        if (stop) {
            struct pollfd p;
            memset(&p, 0, sizeof(p));
            p.fd = sock;
            p.events = POLLIN;
            TEMP_FAILURE_RETRY(poll(&p, 1, 20));
        }

        close(sock);
        sock = -1;
    }
}
void MyClientListener::dispatchCommand(char *data) {
    int argc = 0;
    char *argv[MyClientListener::CMD_ARGS_MAX];
    char tmp[CMD_BUF_SIZE];
    char *p = data;
    char *q = tmp;
    char *qlimit = tmp + sizeof(tmp) - 1;
    bool esc = false;
    bool quote = false;

    memset(argv, 0, sizeof(argv));
    memset(tmp, 0, sizeof(tmp));
    while (*p) {
        if (*p == '\\') {
            if (esc) {
                if (q >= qlimit)
                    goto overflow;
                *q++ = '\\';
                esc = false;
            } else
                esc = true;
            p++;
            continue;
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
                printf("Unsupported escape sequence\n");
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
            if (argc >= CMD_ARGS_MAX)
                goto overflow;
            argv[argc++] = strdup(tmp);
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
        printf("Unclosed quotes error\n");
        goto out;
    }

    for (auto i = mCommands.begin(); i != mCommands.end(); ++i) {
        ClientCommand *c = *i;
        if (c->getCommand()[0] == 0) {
            if (c->runCommand(argc, argv)) {
                printf("Handler '%s' error (%s)\n", c->getCommand(), strerror(errno));
            }
            goto out;
        }
        printf("argv[0]:%s,getCommand:%s\n", argv[0], c->getCommand());
        if (!strcmp(argv[0], c->getCommand())) {
            if (c->runCommand(argc, argv)) {
                printf("Handler '%s' error (%s)\n", c->getCommand(), strerror(errno));
            }
            goto out;
        }
    }
    printf("Command not recognized\n");
out:
    int j;
    for (j = 0; j < argc; j++)
        free(argv[j]);
    return;

overflow:
    //LOG_EVENT_INT(78001, cli->getUid());
    printf("Command too long\n");
    goto out;
}

void MyClientListener::dispatchCommandData(struct iovec *iov) {
    int argc = 0;
    char *argv[MyClientListener::CMD_ARGS_MAX];
    int buf[2];
    char tmp[PAGE_SIZE];

    memset(argv, 0, sizeof(argv));
    buf[0] = *(int *)iov[0].iov_base;
    buf[1] = iov[1].iov_len;
    argv[0] = (char *)buf;

    memcpy(tmp, iov[1].iov_base, iov[1].iov_len);
    argv[1] = tmp;
    argc = 2;

    for (auto i = mCommands.begin(); i != mCommands.end(); ++i) {
        ClientCommand *c = *i;
        if (c->getCommand()[0] == 0) {
            printf("c->getCommand()[0] == 0\n");
            if (c->runCommand(argc, argv)) {
                printf("Handler '%s' error (%s)\n", c->getCommand(), strerror(errno));
            }
            goto out;
        }
        if (!strcmp(argv[0], c->getCommand())) {
            printf("!strcmp(argv[0], c->getCommand())\n");
            if (c->runCommand(argc, argv)) {
                printf("Handler '%s' error (%s)\n", c->getCommand(), strerror(errno));
            }
            goto out;
        }
    }
    printf("Command not recognized\n");
out:
    return;
}

void MyClientListener::registerCmd(ClientCommand *cmd) {
    mCommands.push_back(cmd);
}
