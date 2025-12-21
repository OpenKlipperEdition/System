#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>
#include <pthread.h>

#include "sys_common.h"
#include "systemserver.h"

typedef int (*recv_cb)(uint32_t, tBinderIo *, tBinderIo *, uint32_t);

typedef struct wdt_client
{
    uint32_t tar_handle;
    tIpcThreadInfo *ti;
    char clt_name[50];        /*!< 客户端名字，目前填充为可执行程序的路径*/
    pid_t processPid;  
}wdt_clt_t;

/**
 * @brief    要进行的看门狗命令控制
 */
struct wdt_ctrl
{
    unsigned int cmd;       /*!< 命令码  */
    unsigned int wdt_timeout;       /*!< 超时时间 */
    char name[50];        /*!< 客户端名字，目前填充为可执行程序的路径*/
    pid_t processPid;   
};

struct exec_result
{
    unsigned int cmd;
    unsigned int result;
};

/**
 * @brief    命令操作
 */
enum
{
    START_WDT = 1,      /*!< 开始看门狗服务 */
    STOP_WDT,        /*!< 停止看门狗服务 */
    FEED,               /*!< 喂狗 */
    SET_TIMOES,         /*!< 设置超时时间 */
    REGISTER_WDT,               /*!< 注册客户端信息到看门狗链表 */
    UNREGISTER_WDT,         /*!< 从看门狗链表注销客户端信息 */
};

enum
{
    ExecuteSuccess = 0,
    ExecuteFailure,
    NoPermission,
};

int execute_result_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag);

static pthread_mutex_t mutex;
static pthread_cond_t regisetResultCond;
static pthread_cond_t unregisetResultCond;
static pthread_cond_t feedResultCond;
static pthread_cond_t timeResultCond;
static pthread_cond_t startResultCond;
static pthread_cond_t stopResultCond;

static int execResult;


tBinderService wdt_execute_recv = {
    .transact_cb = execute_result_cb,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int execute_result_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	unsigned char* data;
	int msg_size = 0;
    struct exec_result *results;
    int ret = binder_io_get_data(msg, (uint8_t **)&results, &msg_size);
    
	if(ret || msg_size <= 0){
		printf("binder get data failed \n");
		return -1;
	}

    pthread_mutex_lock(&mutex);
    execResult = (int)results->result;

    switch(results->cmd)
    {
        case START_WDT:      
            pthread_cond_signal(&startResultCond);
            break; 
        case STOP_WDT:       
            pthread_cond_signal(&stopResultCond);
            break;
        case FEED:           
            pthread_cond_signal(&feedResultCond);
            break;
        case SET_TIMOES:     
            pthread_cond_signal(&timeResultCond);
            break;
        case REGISTER_WDT:   
            pthread_cond_signal(&regisetResultCond);
            break;
        case UNREGISTER_WDT: 
            pthread_cond_signal(&unregisetResultCond);
            break;
    }
    pthread_mutex_unlock(&mutex);

	return 0;

}

static int connect_watchdog_server(wdt_clt_t *clt)
{
	if (!clt)
		return -1;

	clt->tar_handle = binder_get_service(WDT_SERVICE_NAME);
	if(clt->tar_handle <= 0){
		printf("The target server is unavailable, creating watchdog client failed.\r\n");
		return -1;
	}
	clt->ti = binder_get_thread_info();

	return 0;
}

wdt_clt_t *alloc_wdt_clt(const char *name, recv_cb func)
{
    wdt_clt_t *clt = NULL;

    clt = malloc(sizeof(wdt_clt_t));
    if (clt == NULL)
        return NULL;

    memset(clt, 0, sizeof(wdt_clt_t));
    strcpy(clt->clt_name, name);
    clt->processPid = getpid();

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&regisetResultCond, NULL);
    pthread_cond_init(&unregisetResultCond, NULL);
    pthread_cond_init(&feedResultCond, NULL);
    pthread_cond_init(&timeResultCond, NULL);
    pthread_cond_init(&startResultCond, NULL);
    pthread_cond_init(&stopResultCond, NULL);

	if (connect_watchdog_server(clt)) {
		pthread_mutex_destroy(&mutex);
		pthread_cond_destroy(&regisetResultCond);
		pthread_cond_destroy(&unregisetResultCond);
		pthread_cond_destroy(&feedResultCond);
		pthread_cond_destroy(&timeResultCond);
		pthread_cond_destroy(&startResultCond);
		pthread_cond_destroy(&stopResultCond);
		free(clt);
		return NULL;
	}

    binder_add_service(name, &wdt_execute_recv);
    binder_thread_enter_loop(1, 0);

    return clt;
}

int free_wdt_clt(wdt_clt_t *clt)
{
    if (clt == NULL)
        return -2;

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&regisetResultCond);
    pthread_cond_destroy(&unregisetResultCond);
    pthread_cond_destroy(&feedResultCond);
    pthread_cond_destroy(&timeResultCond);
    pthread_cond_destroy(&startResultCond);
    pthread_cond_destroy(&stopResultCond);

    binder_cmd_release(clt->ti, clt->tar_handle);
    flush_commands(clt->ti);
    binder_threads_shutdown();
    free(clt);

    return 0;
}

int wdt_clt_send_cmd(wdt_clt_t *clt, unsigned int opt, struct wdt_ctrl *config, int sync)
{
    tBinderIo bio, msg;
    sys_msg_t sysmsg;
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
    struct wdt_ctrl cmd_info;
	int ret = 0;

    if (config != NULL)
    {
        memcpy(&cmd_info, config, sizeof(struct wdt_ctrl));
    }
    else
    {
        cmd_info.cmd = opt;
    }

    cmd_info.processPid = clt->processPid;
    strcpy(cmd_info.name, clt->clt_name);

    strcpy(sysmsg.target_server_name, WDT_SERVICE_NAME);
	memcpy(sysmsg.msg, &cmd_info, sizeof(struct wdt_ctrl));
	sysmsg.len = sizeof(struct wdt_ctrl);

    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&bio,(char *)&sysmsg,sizeof(sysmsg));

    memset(&msg, 0, sizeof(msg));
	if (sync)
		ret = binder_cmd_sync_call(clt->ti, &bio, &msg, clt->tar_handle, 0);
	else
		ret = binder_cmd_async_call(clt->ti, &bio, &msg, clt->tar_handle, 0);

	if (BINDER_STATUS_OK == ret) {

	} else {
		printf("in %s error\n", __func__);
	}

    if(BINDER_STATUS_OK == binder_cmd_async_call(clt->ti, &bio, &msg, clt->tar_handle, 0))
    {
		if (sync) {
			int sz = 0;
			uint8_t *data = 0;

			binder_io_get_data(&msg,&data,&sz);
			binder_cmd_freebuf(clt->ti, msg.data0);

			return *data;
		}

        return 0;
    }
    else
        printf("in %s error\n", __func__);

    return -1;
}

int regiset_wdt_clt(wdt_clt_t *clt)
{
    int ret = 0;

    if (clt == NULL)
        return -2;

    ret = wdt_clt_send_cmd(clt, REGISTER_WDT, NULL, 0);
    if (ret != 0)
        return -1;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&regisetResultCond, &mutex);

    ret = execResult;
    if (ret != ExecuteSuccess)
        ret = -1;

    pthread_mutex_unlock(&mutex);

    return ret;
}

int unregiset_wdt_clt(wdt_clt_t *clt)
{
    int ret = 0;

    if (clt == NULL)
        return -2;

    ret = wdt_clt_send_cmd(clt, UNREGISTER_WDT, NULL, 0);
    if (ret != 0)
        return -1;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&unregisetResultCond, &mutex);

    ret = execResult;
    if (ret != ExecuteSuccess)
        ret = -1;

    pthread_mutex_unlock(&mutex);
    return ret;
}

int start_wdt(wdt_clt_t *clt)
{
    int ret = 0;

    if (clt == NULL)
        return -2;

    ret = wdt_clt_send_cmd(clt, START_WDT, NULL, 0);
    if (ret != 0)
        return -1;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&startResultCond, &mutex);

    ret = execResult;
    if (ret != ExecuteSuccess)
        ret = -1;

    pthread_mutex_unlock(&mutex);
    return ret;
}

int stop_wdt(wdt_clt_t *clt)
{
    int ret = 0;

    if (clt == NULL)
        return -2;

    ret = wdt_clt_send_cmd(clt, STOP_WDT, NULL, 0);
    if (ret != 0)
        return -1;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&stopResultCond, &mutex);

    ret = execResult;
    if (ret != ExecuteSuccess)
        ret = -1;

    pthread_mutex_unlock(&mutex);
    return ret;
}

int feed_wdt(wdt_clt_t *clt)
{
    int ret = 0;

    if (clt == NULL)
        return -2;

    ret = wdt_clt_send_cmd(clt, FEED, NULL, 0);
    if (ret != 0)
        return -1;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&feedResultCond, &mutex);

    ret = execResult;
    if (ret != ExecuteSuccess)
        ret = -1;

    pthread_mutex_unlock(&mutex);
    return ret;
}

int set_wdt_timeout(wdt_clt_t *clt, int time)
{
    int ret = 0;
    struct wdt_ctrl cmd_info;

    if (clt == NULL || time <= 0)
        return -2;

    cmd_info.cmd = SET_TIMOES;
    cmd_info.wdt_timeout = time;

    ret = wdt_clt_send_cmd(clt, SET_TIMOES, &cmd_info, 0);
    if (ret != 0)
        return -1;

    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&timeResultCond, &mutex);

    ret = execResult;
    if (ret != ExecuteSuccess)
        ret = -1;

    pthread_mutex_unlock(&mutex);
    return ret;
}
