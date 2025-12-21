#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>


#include "systemserver.h"


int bl_manager_init(void)
{
	printf("%s %d ok !!!\r\n",__func__,__LINE__);
	return 0;
}

int bl_manager_start(void)
{
	printf("%s %d ok !!!\r\n",__func__,__LINE__);
	return 0;
}

int bl_manager_stop(void)
{
	printf("%s %d ok !!!\r\n",__func__,__LINE__);
	return 0;
}

int bl_manager_recv_msg(server_msg_t* msg)
{
	printf("%s %d msg : %s ###\r\n",__func__,__LINE__,(char*)msg->msg);
	//binder_io_append_uint32(reply,0x80);	//for test	
	unsigned char data[] = "hello this is backlightManagerService";
	int ret = msg->reply_callback(data,strlen(data),msg->reply);
	/* printf("%s ret = %d \r\n",__func__,ret); */
	return 0;
}

const service_class_t bl_service = {
	.server_name = "backlightManagerService",
	.init = bl_manager_init,
	.start = bl_manager_start,
	.stop = bl_manager_stop,
	.recv_msg = bl_manager_recv_msg,
	.server_status = 0,
};

