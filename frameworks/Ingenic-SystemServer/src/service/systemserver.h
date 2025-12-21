#ifndef __SYSTEMSERVER_H__
#define __SYSTEMSERVER_H__

#include "systemserver_config.h"
#include "binder_io.h"
typedef struct _server_msg {
	void *msg;			// 不同的服务，可定义不同的消息结构，自行解析
	unsigned int msg_len;
	tBinderIo *reply;	//用于返回执行结果的，binder需要
	int (*reply_callback)(char* priv,int datalen,tBinderIo* rpt);	//服务线程执行该命令后，调用此回调函数，将结果返回给client  (此函数在server.c实现)
}server_msg_t;

typedef struct _server_class{
	char server_name[64];		// watch_dog....event.....pwr
	int (*init)(void);		// 服务的初始化(一些资源初始化)
	int (*start)(void);		// 服务线程创建....
	int (*stop)(void);
	int (*recv_msg)(server_msg_t* msg);	// 将msg填到rx_fifo
	int server_status;		// run or stop
}service_class_t;







#endif	// __SYSTEMSERVER_H__
