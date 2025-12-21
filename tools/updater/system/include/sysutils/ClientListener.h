#ifndef __CLIENTLISTENER_H
#define __CLIENTLISTENER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>
#include <stddef.h>
#include <icutils/sockets.h>
#include <thread>
#include <list>
#include <functional>
#include <sys/uio.h>
class ClientCommand{
private:
  char mCommand[20];
public:

  ClientCommand(int code){
    memset(mCommand,0,sizeof(mCommand));
    if(code != -1)
      snprintf(mCommand,19,"%d",code);
  }
  virtual ~ClientCommand() {

  }

  virtual int runCommand(int argc, char **argv){
    printf("Command %s has no run handler!", getCommand());
    errno = ENOSYS;
    return -1;
  }
  const char *getCommand() { return mCommand; }
};
class ClientListener {
private:
  std::list<ClientCommand*> mCommands;
  std::string sockName;
  int sock;
  bool isData;
  std::thread mThread;
  volatile bool stop;
  void dispatchCommand(char *data);
  void dispatchCommandData(struct iovec *iov);
public:
  static const int CMD_ARGS_MAX = 26;
  ClientListener(const char *sockName_);
  ~ClientListener();
  void registerCmd(ClientCommand *cmd);

  int startListener();
  int startListenerData();
  int stopListener();
  void RunLoop();
};

#endif /* __CLIENTLISTENER_H */
