#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>

#include "sys_common.h"
#include "systemserver.h"


typedef struct power_client
{
    uint32_t tar_handle;
    tIpcThreadInfo *ti;
    char clt_name[50];        /*!< 客户端名字，目前填充为可执行程序的路径*/
    pid_t processPid;  
}power_clt_t;

/**
 * @brief    要进行的看门狗命令控制
 */
struct power_ctrl
{
    unsigned int cmd;       /*!< 命令码  */
    char name[50];        /*!< 客户端名字 */
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

static int connect_power_server(power_clt_t *clt)
{
	if (!clt)
		return -1;
	clt->tar_handle = binder_get_service(POWER_MANAGER_SERVICE_NAME);
	if (clt->tar_handle <= 0) {
		printf("Power clients failed to get binder service[%s].\n", POWER_MANAGER_SERVICE_NAME);
		return -1;
	}
	clt->ti = binder_get_thread_info();

	return 0;
}

power_clt_t *alloc_power_clt(const char *name)
{
    power_clt_t *clt = NULL;

    clt = malloc(sizeof(power_clt_t));
    if (clt == NULL)
        return NULL;
    
    memset(clt, 0, sizeof(power_clt_t));
    strcpy(clt->clt_name, name);
    clt->processPid = getpid();

	if (connect_power_server(clt)) {
		free(clt);
		return NULL;
	}

    return clt;
}

int free_power_clt(power_clt_t *clt)
{
    if (clt == NULL)
        return -2;

    binder_cmd_release(clt->ti, clt->tar_handle);
    flush_commands(clt->ti);
    free(clt);

    return 0;
}

int power_clt_send_cmd(power_clt_t *clt, unsigned int opt, struct power_ctrl *config)
{
    tBinderIo bio, msg;
    sys_msg_t sysmsg;
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
    struct power_ctrl cmd_info;

    if (config != NULL)
    {
        memcpy(&cmd_info, config, sizeof(struct power_ctrl));
    }
    else
    {
        cmd_info.cmd = opt;
    }

    cmd_info.processPid = clt->processPid;
    strcpy(cmd_info.name, clt->clt_name);

    strcpy(sysmsg.target_server_name, POWER_MANAGER_SERVICE_NAME);
	memcpy(sysmsg.msg, &cmd_info, sizeof(struct power_ctrl));
	sysmsg.len = sizeof(struct power_ctrl);

    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&bio,(char *)&sysmsg,sizeof(sysmsg));

    memset(&msg, 0, sizeof(msg));
    if(BINDER_STATUS_OK == binder_cmd_sync_call(clt->ti, &bio, &msg, clt->tar_handle, 0))
    {
        int sz = 0;
		uint8_t* data = 0;

        binder_io_get_data(&msg, &data, (size_t *)&sz);
        binder_cmd_freebuf(clt->ti, msg.data0);

        return *data;
    }
    else
        printf("error\n");

    return -1;
}

int power_suspend(power_clt_t *clt, unsigned int time)
{
    struct power_ctrl cmd_info;

    if (clt == NULL)
    {
        return -2;
    }
    

    if (time == 0)
        return power_clt_send_cmd(clt, SUSPEND, NULL);
    else
    {
        cmd_info.cmd = SUSPENDTIMER;
        cmd_info.suspendTime = time;  // unit: s
        return power_clt_send_cmd(clt, SUSPENDTIMER, &cmd_info);
    }
}


int power_down(power_clt_t *clt, unsigned int time)
{
    struct power_ctrl cmd_info;

    if (time == 0 || clt == NULL)
        return -2;

    cmd_info.cmd = POWERDOWN;
    cmd_info.powerdownTime = time;  // unit: s

    return power_clt_send_cmd(clt, POWERDOWN, &cmd_info);
}

int power_wakeup(power_clt_t *clt, unsigned int time)
{
    struct power_ctrl cmd_info;

    if (time == 0 || clt == NULL)
        return -2;

    cmd_info.cmd = WAKEUP;
    cmd_info.wakeupTime = time;  // unit: s

    return power_clt_send_cmd(clt, WAKEUP, &cmd_info);
}

