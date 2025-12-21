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
#include <sys/reboot.h>
#include <time.h>
#include <unistd.h>
#include <linux/rtc.h>

#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "systemserver.h"
#include "fifo.h"
#include "list.h"
#include "dlog.h"

#include "systemserver.h"
#include "mt_timer.h"

#define LOG_TAG		POWER_MANAGER_SERVICE_EXECUTE_FILE

#define SYSTEM_CLIENT_NAME "SystemClt025"

int power_manager_init(void);
int power_manager_start(void);
int power_manager_stop(void);
int power_manager_recv_msg(server_msg_t* msg);

service_class_t powerManager_server = {
    .server_name = POWER_MANAGER_SERVICE_NAME,
    .init = power_manager_init,
    .start = power_manager_start,
    .stop = power_manager_stop,
    .recv_msg = power_manager_recv_msg,
    .server_status = 0,
};

/**
 * @brief    要进行的电源管理消息结构体
 */
struct power_ctrl
{
    unsigned int cmd;       /*!< 命令码  */
    char name[50];          /*!< 客户端名字 */
    pid_t processPid;   
    unsigned int suspendTime;
    unsigned int powerdownTime;
    unsigned int wakeupTime;
};

/**
 * @brief    命令操作
 */
enum
{
    SUSPEND = 1,            /*!< 休眠 */
    SUSPENDTIMER,          /*!< 定时休眠 */
    POWERDOWN,             /*!< 定时关机 */
    WAKEUP,                /*!< 定时唤醒 */
};

enum
{
    ExecuteSuccess = 0,
    ExecuteFailure,
    NoPermission,
};

enum
{
    SYS_PERMISSION = 1,
    USER_PERMISSION,
};

TIMER_CREATE(timer);

int power_manager_suspend(void *arg)
{
    int fd = 0;
    int ret = 0;

    fd = open("/sys/power/state", O_RDWR);
    if (fd < 0)
        return -1;
    
    ret = write(fd, "mem", 3);
    if (ret == -1)
        return -2;

    close(fd);

    return 0;
}

int power_down(void *arg)
{
    int ret = 0;
    ret = reboot(RB_POWER_OFF);

    return ret;
}

int power_manager_wakeup(unsigned int timeout)
{
    int rtc_fd = 0;
    int ret = 0;
    struct rtc_wkalrm alrm;
    struct rtc_time curr_time;
    memset(&alrm, 0, sizeof(struct rtc_wkalrm));

    rtc_fd = open("/dev/rtc", O_RDWR);
    if (rtc_fd < 0)
        return -1;

    time_t now = time(NULL);

    ioctl(rtc_fd, RTC_RD_TIME, &alrm.time);

    time_t alarm_sec = now + timeout;
    struct tm *alarm_tm = localtime(&alarm_sec);

    alrm.time.tm_sec = alarm_tm->tm_sec;
    alrm.time.tm_min = alarm_tm->tm_min;
    alrm.time.tm_hour = alarm_tm->tm_hour;
    alrm.time.tm_mday = alarm_tm->tm_mday;
    alrm.time.tm_mon = alarm_tm->tm_mon;
    alrm.time.tm_year = alarm_tm->tm_year;
    alrm.time.tm_wday = alarm_tm->tm_wday;
    alrm.time.tm_yday = alarm_tm->tm_yday;

    alrm.enabled = 1;
    alrm.pending = 0;
    ret = ioctl(rtc_fd, RTC_WKALM_SET, &alrm);

    close(rtc_fd);

    return 0;
}

int check_permission(const char *clt_name)
{
    if (strcmp(clt_name, SYSTEM_CLIENT_NAME) == 0)
        return SYS_PERMISSION;

    return USER_PERMISSION;
}

int power_manager_init(void)
{
    TIMER_INIT(timer, 10);
    return 0;
}

int power_manager_start(void)
{  
    powerManager_server.server_status = 1;
    return 0;
}

int power_manager_stop(void)
{
    powerManager_server.server_status = 0;
    return 0;
}

int power_manager_recv_msg(server_msg_t* msg)
{
    struct power_ctrl cmd_info;
    int cmd = *(int *)msg->msg;
    int ret = 0;
    struct itimerspec itimespec;
    int permission = 0;
    int reply_data = ExecuteSuccess;

    memset(&cmd_info, 0, sizeof(struct power_ctrl));
    memcpy(&cmd_info, msg->msg, msg->msg_len);

    permission = check_permission(cmd_info.name);
    if (permission == USER_PERMISSION && (cmd == POWERDOWN))
    {
        reply_data = NoPermission;
        msg->reply_callback((char *)&reply_data, sizeof(int), msg->reply);
        return 0;
    }

    switch (cmd)
    {
        case SUSPEND:
            ret = power_manager_suspend(NULL);
            break;
        case SUSPENDTIMER:
            itimespec.it_value.tv_sec = cmd_info.suspendTime;
            itimespec.it_value.tv_nsec = 0;
            itimespec.it_interval.tv_sec = 0;
            itimespec.it_interval.tv_nsec = 0;
   
            ret = TIMER_ADD(timer, &itimespec, 1, (timer_callback_t)power_manager_suspend, NULL);

            break;
        case POWERDOWN:
            itimespec.it_value.tv_sec = cmd_info.powerdownTime;
            itimespec.it_value.tv_nsec = 0;
            itimespec.it_interval.tv_sec = 0;
            itimespec.it_interval.tv_nsec = 0;

            ret = TIMER_ADD(timer, &itimespec, 1, (timer_callback_t)power_down, NULL);
            
            break;
        case WAKEUP:
            ret = power_manager_wakeup(cmd_info.wakeupTime);
            break;
    }

    if (ret < 0)
        reply_data = ExecuteFailure;
	msg->reply_callback((char *)&reply_data, sizeof(int), msg->reply);

    return 0;
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

    ser_msg.msg = sys_msg->msg;
    ser_msg.msg_len = sys_msg->len;
    ser_msg.reply = reply;
    ser_msg.reply_callback = cmd_exec_end_cb;

    ret = powerManager_server.recv_msg(&ser_msg);
    if(ret){
        LOGE("call %s recv_msg failed\n");
        binder_io_append_uint32(reply,0xffffffff); // return error code
    }

    return 0;
}

static tBinderService powerserver_service = {
       .transact_cb = msg_arrive_transact,
       .link_to_death_cb = NULL,
       .unlink_to_death_cb = NULL,
       .death_notify_cb = NULL,
};

int main()
{
	power_manager_init();
	power_manager_start();

	binder_add_service(POWER_MANAGER_SERVICE_NAME, &powerserver_service);
	binder_thread_enter_loop(1,1);

	while (1) {
        sleep(2);
	}
}
