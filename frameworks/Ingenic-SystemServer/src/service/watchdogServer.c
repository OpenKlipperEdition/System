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
#include <stdint.h>

#include "systemserver.h"
#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "fifo.h"
#include "list.h"
#include "sys_common.h"

#define NAME_LEN 30
#define RESTART_LIMIT 3
#define DEAD_TIME_LIMIT 100000
#define MSG_POOL_CAPACITY 20
#define CLIENT_POOL_CAPACITY 20

#define LOG_TAG		WDT_SERVICE_EXECUTE_FILE

/**
 * @brief    客户端信息结构体
 *
 * @note    后续可能会修改成员变量，但processPid成员变量一定不会修改
 */
typedef struct client
{
    char name[NAME_LEN]; /*!< 客户端名字，目前填充为可执行程序的路径*/
    pid_t processPid;    /*!< 程序正常运行后的进程pid */
    int client_handle;
    tIpcThreadInfo *ti;
} client_t;

/**
 * @brief    客户端在看门狗服务中的信息
 */
struct wdtClient
{
    unsigned char is_used : 1;
    long long last_feed;   /*!< 上次喂狗时间 单位us  */
    unsigned int timeouts; /*!< 超时时间  */
    client_t info;
    struct list_head list;
    int registeredCount;
};

/**
 * @brief    看门服务的信息结构体
 */
struct watchdog_service
{
    fifo_t data_fifo;
    struct list_head registeredClients; /*!< 使用看门狗的客户端链表 */
    struct list_head runClients;        /*!< 使用看门狗的客户端链表 */
    struct list_head deadClients;       /*!< 使用看门狗的客户端链表 */
    pthread_mutex_t list_mutex;         /*!< 互斥锁 */
};

/**
 * @brief    要进行的看门狗命令控制
 */
struct wdt_ctrl
{
    unsigned int cmd;         /*!< 命令码  */
    unsigned int wdt_timeout; /*!< 超时时间 */
    char name[50];            /*!< 客户端名字，目前填充为可执行程序的路径*/
    pid_t processPid;
};

struct exec_result
{
    unsigned int cmd;
    unsigned int result;
};

struct mem_pool
{
    int index_len;
    int total_mem_volume;
    int unit_size;
    void *memaddr;
    void *startAddr;
    void *endAddr;
};

/**
 * @brief    命令操作
 */
enum
{
    START_WDT = 1,  /*!< 开始看门狗服务 */
    STOP_WDT,       /*!< 停止看门狗服务 */
    FEED,           /*!< 喂狗 */
    SET_TIMOES,     /*!< 设置超时时间 */
    REGISTER_WDT,   /*!< 注册客户端信息到看门狗链表 */
    UNREGISTER_WDT, /*!< 从看门狗链表注销客户端信息 */
};

enum
{
    ExecuteSuccess = 0,
    ExecuteFailure,
    NoPermission,
};

static struct watchdog_service g_wdt_instance;

static struct wdtClient *g_client_pool;

static struct mem_pool g_msg_pool;

int wdt_manager_init(void);
int wdt_manager_start(void);
int wdt_manager_stop(void);
int wdt_manager_recv_msg(server_msg_t *msg);

service_class_t watchdog_server = {
    .server_name = WDT_SERVICE_NAME,
    .init = wdt_manager_init,
    .start = wdt_manager_start,
    .stop = wdt_manager_stop,
    .recv_msg = wdt_manager_recv_msg,
    .server_status = 0,
};

int send_exec_result(struct wdtClient *clt, int opt, int result)
{
    tBinderIo bio, msg;
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
    struct exec_result data;
    int ret = 0;

    data.cmd = opt;
    data.result = result;

    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&bio, (char *)&data, sizeof(struct exec_result));

    memset(&msg, 0, sizeof(msg));
    ret = binder_cmd_sync_call(clt->info.ti, &bio, &msg, clt->info.client_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        binder_cmd_freebuf(clt->info.ti, msg.data0);
        return 0;
    }
    else
        printf("in %s error  ret = %d\n", __func__, ret);

    return -1;
}

long long get_current_time()
{
    struct timeval current_time;
    long long milliseconds = 0;

    if (gettimeofday(&current_time, NULL) == -1)
    {
        return -1;
    }

    milliseconds = (long long)current_time.tv_sec * 1000 + (long long)current_time.tv_usec / 1000;

    return milliseconds;
}

static int list_lock()
{
    return pthread_mutex_lock(&g_wdt_instance.list_mutex);
}

static int list_unlock()
{
    return pthread_mutex_unlock(&g_wdt_instance.list_mutex);
}

static int init_list_lock()
{
    return pthread_mutex_init(&g_wdt_instance.list_mutex, NULL);
}

static void init_wdt_client_list()
{
    INIT_LIST_HEAD(&g_wdt_instance.registeredClients);
    INIT_LIST_HEAD(&g_wdt_instance.runClients);
    INIT_LIST_HEAD(&g_wdt_instance.deadClients);
}

struct wdtClient *find_wdt_client(const char *name)
{
    struct wdtClient *temp = NULL;

    list_for_each_entry(temp, &g_wdt_instance.registeredClients, list)
    {
        if (strcmp(temp->info.name, name) != 0)
            continue;
        return temp;
    }

    list_for_each_entry(temp, &g_wdt_instance.runClients, list)
    {
        if (strcmp(temp->info.name, name) != 0)
            continue;
        return temp;
    }

    list_for_each_entry(temp, &g_wdt_instance.deadClients, list)
    {
        if (strcmp(temp->info.name, name) != 0)
            continue;
        return temp;
    }

    return NULL;
}

struct wdtClient *alloc_wdt_client()
{
    struct wdtClient *clt = NULL;
    int len = CLIENT_POOL_CAPACITY;
    int i = 0;

    for (i = 0; i < len; i++)
    {
        if (g_client_pool[i].is_used != 0)
            continue;

        g_client_pool[i].is_used = 1;
        break;
    }

    if (i >= len)
    {
        clt = malloc(sizeof(struct wdtClient));
        if (clt != NULL)
            memset(clt, 0, sizeof(struct wdtClient));
        return clt;
    }

    return &g_client_pool[i];
}

void free_wdt_client(struct wdtClient **clt)
{
    unsigned int start_addr = (unsigned int)&g_client_pool[0];
    unsigned int stop_addr = (unsigned int)&g_client_pool[CLIENT_POOL_CAPACITY - 1];
    unsigned int clt_addr = (unsigned int)*clt;

    if (start_addr <= clt_addr && clt_addr <= stop_addr)
    {
        (*clt)->is_used = 0;
        memset(*clt, 0, sizeof(struct wdtClient));
        clt == NULL;
        return;
    }

    free(clt);
    return;
}

struct wdt_ctrl *alloc_wdt_ctrl()
{
    int i = 0;
    int index = 0;
    int pos = 0;
    unsigned char *bit_map = g_msg_pool.memaddr;

    for (i = 0; i < g_msg_pool.index_len; i++)
    {
        unsigned int x = 0xffffff00;
        x += (unsigned int)bit_map[i];
        index = __builtin_ffs(~x);
        if (index != 0)
            break;
        pos += 8;
    }

    pos += (index - 1);

    if (pos > ((g_msg_pool.total_mem_volume / g_msg_pool.unit_size) - 1))
        return malloc(sizeof(struct wdt_ctrl));

    bit_map[i] |= (0x1 << (index - 1));

    struct wdt_ctrl *base = (struct wdt_ctrl *)g_msg_pool.startAddr;

    return &base[pos];
}

void free_wdt_ctrl(struct wdt_ctrl **clt)
{   
    int offset = ((unsigned int)*clt - (unsigned int)g_msg_pool.startAddr) / g_msg_pool.unit_size;
    char *bit_map = g_msg_pool.memaddr;
    int index = 0;
    int bit = 0;

    if ((void *)*clt < g_msg_pool.startAddr || (void *)*clt > g_msg_pool.endAddr)
    {
        free(*clt);
        clt = NULL;
        return;
    }

    index = offset / 8;
    bit = offset % 8;

    bit_map[index] &= (~(0x1 << bit));

    memset(*clt, 0, sizeof(struct wdt_ctrl));
    clt = NULL;

    return;
}

void exec_wdt_cmd(int num)
{
    struct wdt_ctrl *cmd_msg = NULL;

    while (num--)
    {
        cmd_msg = dequeue_fifo(&g_wdt_instance.data_fifo, NO_WAIT);
        switch (cmd_msg->cmd)
        {
        case START_WDT:
        {
            long long milliseconds = 0;
            struct wdtClient *cltInfo = NULL;
            list_lock();

            milliseconds = get_current_time();
            cltInfo = find_wdt_client(cmd_msg->name);
            if (cltInfo == NULL)
            {
                send_exec_result(cltInfo, START_WDT, ExecuteFailure);
                continue;
            }
            list_del(&cltInfo->list);
            cltInfo->last_feed = milliseconds;

            list_add_head(&cltInfo->list, &g_wdt_instance.runClients);
            send_exec_result(cltInfo, START_WDT, ExecuteSuccess);

            list_unlock();
        }
        break;
        case STOP_WDT:
        {
            struct wdtClient *cltInfo = NULL;
            list_lock();

            cltInfo = find_wdt_client(cmd_msg->name);
            if (cltInfo == NULL)
            {
                send_exec_result(cltInfo, START_WDT, ExecuteFailure);
                continue;
            }
            list_del(&cltInfo->list);
            list_add_head(&cltInfo->list, &g_wdt_instance.registeredClients);
            send_exec_result(cltInfo, STOP_WDT, ExecuteSuccess);

            list_unlock();
        }
        break;
        case FEED:
        {
            long long milliseconds = 0;
            struct wdtClient *cltInfo = NULL;
            list_lock();

            cltInfo = find_wdt_client(cmd_msg->name);
            if (cltInfo == NULL)
            {
                send_exec_result(cltInfo, START_WDT, ExecuteFailure);
                continue;
            }
            milliseconds = get_current_time();
            cltInfo->last_feed = milliseconds;
            send_exec_result(cltInfo, FEED, ExecuteSuccess);

            list_unlock();
        }
        break;
        case SET_TIMOES:
        {
            struct wdtClient *cltInfo = NULL;
            list_lock();

            cltInfo = find_wdt_client(cmd_msg->name);
            if (cltInfo == NULL)
            {
                send_exec_result(cltInfo, START_WDT, ExecuteFailure);
                continue;
            }
            cltInfo->timeouts = cmd_msg->wdt_timeout;
            send_exec_result(cltInfo, SET_TIMOES, ExecuteSuccess);

            list_unlock();
        }
        break;
        case REGISTER_WDT:
        {
            struct wdtClient *cltInfo = find_wdt_client(cmd_msg->name);
            list_lock();
            if (cltInfo == NULL)
            {
                cltInfo = alloc_wdt_client();
                if (cltInfo == NULL)
                {
                    send_exec_result(cltInfo, REGISTER_WDT, ExecuteFailure);
                    continue;
                }

                INIT_LIST_HEAD(&cltInfo->list);
                list_add_head(&cltInfo->list, &g_wdt_instance.registeredClients);
                strcpy(cltInfo->info.name, cmd_msg->name);

                cltInfo->info.client_handle = binder_get_service(cltInfo->info.name);
                cltInfo->info.ti = binder_get_thread_info();
            }
            cltInfo->info.processPid = cmd_msg->processPid;
            cltInfo->registeredCount++;
            send_exec_result(cltInfo, REGISTER_WDT, ExecuteSuccess);
            list_unlock();
        }
        break;
        case UNREGISTER_WDT:
        {
            struct wdtClient *cltInfo = NULL;
            list_lock();

            cltInfo = find_wdt_client(cmd_msg->name);
            if (cltInfo == NULL)
            {
                send_exec_result(cltInfo, START_WDT, ExecuteFailure);
                continue;
            }
            list_del(&cltInfo->list);
            send_exec_result(cltInfo, UNREGISTER_WDT, ExecuteSuccess);

            binder_cmd_release(cltInfo->info.ti, cltInfo->info.client_handle);
            flush_commands(cltInfo->info.ti);

            free_wdt_client(&cltInfo);

            list_unlock();
        }
        break;
        }

        free_wdt_ctrl(&cmd_msg);
    }
}

void *watchdog_main_thread(void *arg)
{
    struct wdtClient *temp = NULL;
    long long milliseconds = 0;
    int elemNum = 0;

    while (1)
    {
        usleep(100000);

        elemNum = getFifoElemNum(&g_wdt_instance.data_fifo);
        if (elemNum != 0)
        {
            exec_wdt_cmd(elemNum);
        }

        milliseconds = get_current_time();
        if (milliseconds == -1)
        {
            continue;
        }

        list_lock();
        list_for_each_entry(temp, &g_wdt_instance.runClients, list)
        {
            long long timeout = 0;
            struct wdtClient *prev_clt = container_of(temp->list.prev, struct wdtClient, list);

            timeout = milliseconds - temp->last_feed;
            if (timeout < temp->timeouts)
                continue;
            else
            {
                kill(temp->info.processPid, SIGKILL);
                temp->info.processPid = 0;

                list_del(&temp->list);
                list_add_head(&temp->list, &g_wdt_instance.deadClients);

                temp = prev_clt;
            }
        }

        list_for_each_entry(temp, &g_wdt_instance.deadClients, list)
        {
            long long timeout = 0;
            struct wdtClient *prev_clt = container_of(temp->list.prev, struct wdtClient, list);

            if (temp->registeredCount >= RESTART_LIMIT)
            {
                reboot(RB_AUTOBOOT);
            }

            timeout = milliseconds - temp->last_feed;
            if (timeout > DEAD_TIME_LIMIT)
            {
                list_del(&temp->list);
                free_wdt_client(&temp);
                temp = prev_clt;
            }
        }
        list_unlock();
    }
}

int init_client_pool(int capacity)
{
    g_client_pool = malloc(sizeof(struct wdtClient) * capacity);
    if (g_client_pool == NULL)
        return -1;

    memset(g_client_pool, 0, sizeof(struct wdtClient) * capacity);
    return 0;
}

int init_msg_pool(int capacity)
{
    int index_len = (capacity + 7) / 8;

    g_msg_pool.memaddr = malloc(sizeof(struct wdt_ctrl) * capacity + index_len);
    if (g_msg_pool.memaddr == NULL)
        return -1;

    memset(g_msg_pool.memaddr, 0, (sizeof(struct wdt_ctrl) * capacity + index_len));

    g_msg_pool.unit_size = sizeof(struct wdt_ctrl);
    g_msg_pool.total_mem_volume = g_msg_pool.unit_size * capacity;
    g_msg_pool.index_len = index_len;
    g_msg_pool.startAddr = (char *)g_msg_pool.memaddr + g_msg_pool.index_len;
    g_msg_pool.endAddr = (struct wdt_ctrl *)g_msg_pool.startAddr + capacity;

    return 0;
}

int wdt_manager_init(void)
{
    memset(&g_wdt_instance, 0, sizeof(struct watchdog_service));

    init_fifo(&g_wdt_instance.data_fifo, 30);
    init_list_lock();
    init_wdt_client_list();

    init_client_pool(CLIENT_POOL_CAPACITY);
    init_msg_pool(MSG_POOL_CAPACITY);

    return 0;
}

int wdt_manager_start(void)
{
    int ret = 0;
    int i = 0;
    pthread_t thread_id = 0;

    ret = pthread_create(&thread_id, NULL, watchdog_main_thread, NULL);
    if (ret != 0)
    {
        return -1;
    }
    ret = pthread_detach(thread_id);

    watchdog_server.server_status = 1;
    return ret;
}

int wdt_manager_stop(void)
{
    watchdog_server.server_status = 0;
    deinit_fifo(&g_wdt_instance.data_fifo);
    return 0;
}

int wdt_manager_recv_msg(server_msg_t *msg)
{
    struct wdt_ctrl *cmd_info = NULL;
    int cmd = *(int *)msg->msg;

    cmd_info = alloc_wdt_ctrl();
    if (cmd_info == NULL)
        return -1;

    memset(cmd_info, 0, sizeof(struct wdt_ctrl));
    memcpy(cmd_info, msg->msg, msg->msg_len);

    queue_fifo(&g_wdt_instance.data_fifo, cmd_info, NO_WAIT);

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

    ret = watchdog_server.recv_msg(&ser_msg);
    if(ret){
        //LOGE("call %s recv_msg failed\n");
        binder_io_append_uint32(reply,0xffffffff); // return error code
    }

    return 0;
}

static tBinderService watchdogserver_service = {
    .transact_cb = msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int main()
{
	wdt_manager_init();
	wdt_manager_start();

	binder_add_service(WDT_SERVICE_NAME, &watchdogserver_service);
	binder_thread_enter_loop(1,1);

	while (1) {
		sleep(2);
	}
}
