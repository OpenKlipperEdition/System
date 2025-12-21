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
#include <stdbool.h>

#include "systemserver.h"
#include "dlog.h"
#include "binder_io.h"
#include "binder_ipc.h"
#include "sys_common.h"


/**
 * @brief : 用于描述服务信息
 */
typedef struct service_info {
    char *service_name;                 /*!< 服务器名称 */
    char *execute_file;                 /*!< 服务对应的可执行程序的文件名，用于执行启动子服务进程 */
    pid_t processID;                    /*!< 子服务进程的进程ID */
} service_info_t;


static service_info_t server_list[] = {
#if ENABLE_EVENTMANAGER_SERVER
    { EVENT_MANAGER_SERVICE_NAME, EVENT_MANAGER_SERVICE_EXECUTE_FILE },
#endif
#if ENABLE_WATCHDOG_SERVER
    { WDT_SERVICE_NAME, WDT_SERVICE_EXECUTE_FILE },
#endif
#if ENABLE_POWERMANAGER_SERVER
    { POWER_MANAGER_SERVICE_NAME, POWER_MANAGER_SERVICE_EXECUTE_FILE },
#endif
#if ENABLE_BACKLIGHT_SERVER
    { BACKLIGHT_MANAGER_SERVICE_NAME, BACKLIGHT_MANAGER_SERVICE_EXECUTE_FILE },
#endif
};

static bool check_service_availability(char * service_name)
{
	int namelen = strlen(service_name);
	int sernum = sizeof(server_list) / sizeof(server_list[0]);

	for(int i = 0; i < sernum; i++){
		if(!strncmp(service_name, server_list[i].service_name, namelen)){
			if (server_list[i].processID > 0)
				return true;
		}
	}

	return false;
}

static int cmd_exec_end_cb(char* data,int len,tBinderIo* reply)
{
		return binder_io_append_data(reply,data,len);
}


static int msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	server_msg_t ser_msg;
	sys_msg_t* sys_msg;
	char* data;
	int msg_size = 0;
	int ret = binder_io_get_data(msg,&data,&msg_size);

	if(ret || msg_size <= 0){
		printf("binder get data failed \n");
		return -1;
	}
	sys_msg = (sys_msg_t*)data;

	ret = check_service_availability(sys_msg->target_server_name);
	if (ret) {
		cmd_exec_end_cb((char *)&ret, sizeof(int), reply);
	} else {
		printf("%s not found or not started.\r\n", sys_msg->target_server_name);
		cmd_exec_end_cb((char *)&ret, sizeof(int), reply);
	}

	return 0;
}

static int check_service_list(void)
{
    int num = 0;
    num = sizeof(server_list) / sizeof(server_list[0]);
    for(int i = 0; i < num; i++){
        LOGI("%s\n", server_list[i].service_name);
    }
    return num;
}

static tBinderService systemserver_service = {
    .transact_cb = msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int main(int argc,char** argv)
{
    int ser_num = 0;
    int ret = 0;
    pid_t pid;
    ser_num = check_service_list();

    for (int i = 0; i < ser_num; i++) {
        pid = vfork();
        if (0 == pid) {
            ret = execlp(server_list[i].execute_file, server_list[i].execute_file, (char *)NULL);
            if (ret) {
                server_list[i].processID = -1;
                _exit(1);
            }
        } else if (0 < pid) {
            if (0 == server_list[i].processID)
                server_list[i].processID = pid;
        } else {
            return -1;
        }
    }

    /* char name[64] = {0}; */
    binder_add_service(SYSTEM_SERVICE_NAME,&systemserver_service);
    /* binder_list_service(0, name, sizeof(name)); */
    /* printf("binder service service 0 : %s\n", name); */
    binder_thread_enter_loop(1,1);
    while(1){
        sleep(10);
    }
    binder_threads_shutdown();

	return 0;
}


