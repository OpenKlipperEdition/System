#ifndef __SERVERLISTENER_H
#define __SERVERLISTENER_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <stdint.h>
#include <stddef.h>
#include <icutils/sockets.h>
#include <sysutils/FrameworkListener.h>

class ServerListener: public FrameworkListener
{
private:
  class RegisterCmd:public FrameworkCommand {
  public:
     RegisterCmd():FrameworkCommand("Register"){}
     virtual ~RegisterCmd(){}
     int runCommand(SocketClient *cli, int argc, char ** argv){
       if(strncmp(argv[0],"Register",8) == 0){
         cli->sendMsg("success");
       }else{
         cli->sendMsg("failed");
       }
       return 0;
     }
   };

public:
 ServerListener(const char* serverName):FrameworkListener(serverName){
  registerCmd(new RegisterCmd());
 }
};

#endif /* __SERVERLISTENER_H */
