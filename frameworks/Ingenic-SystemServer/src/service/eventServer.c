#include <sys/un.h>
#include <pthread.h>
#include <sys/reboot.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "systemserver.h"
#include "fifo.h"
#include "list.h"
#include "dlog.h"

#define EVENT_MESSAGE_MAX_LEN 256


#define EVENT_CLIENT_SERVICE_NAME   "EventClientService"
#define EVT_CLIENT_NAME_MAX_LEN     50
#define GPIO_KEYS_INPUT_DEVICE_PATH "/dev/input/event0"
#define TS_INPUT_DEVICE_PATH        "/dev/input/event1"
#define LOG_TAG						EVENT_MANAGER_SERVICE_EXECUTE_FILE


/**
 * @brief : 事件类型
 */
typedef enum _event_type {
    KEY_EVENT,                           /*!< 按键事件 */
    TOUCH_SCREEN_EVENT,                  /*!< 触屏事件 */
    /* TODO */
} _evt_type_t;

/**
 * @brief : 事件消息
 */
typedef struct _event_message {
    int event_code;                             /*!< 唯一event编码 */
    uint8_t msg_total_len;                      /*!< 消息总长度 */
    uint8_t evt_msg[EVENT_MESSAGE_MAX_LEN];     /*!< 事件消息，后续强制类型转换 */
} _evt_msg_t;

/**
 * @brief : 事件状态消息
 */
typedef struct _event_status {
	_evt_type_t evt_type;		 /*!< 事件类型 */
	_evt_msg_t	evt_msg;         /*!< 事件状态信息 */
} _evt_status_t;

/**
 * @brief : 事件的命令操作
 */
typedef enum _event_cmd {
    REGISTER_EVT = 1,                   /*!< 注册客户端信息到事件链表 */
    UNREGISTER_EVT,                     /*!< 从事件链表注销客户端信息 */
    LISTEN_KEY_EVT,                     /*!< 监听按键事件 */
    CANCEL_LISTEN_KEY_EVT,              /*!< 取消监听按键事件 */
    LISTEN_TOUCH_SCREEN_EVT,            /*!< 监听触屏事件 */
    CANCEL_LISTEN_TOUCH_SCREEN_EVT,     /*!< 取消监听触屏事件 */
} _evt_cmd_t;

/**
 * @brief : 事件的命令控制
 */
typedef struct _event_ctrl {
    uint32_t cmd;                                      /*!< 命令码 */
    char clt_name[EVT_CLIENT_NAME_MAX_LEN];            /*!< 客户端的名字 */
    pid_t processID;                                   /*!< 当前事件客户端进程的进程ID */
    uint32_t input_device_index;                       /*!< 输入事件设备的索引号 */
} _evt_ctrl_t;

/**
 * @brief : 事件客户端信息
 */
typedef struct _event_client {
	char name[EVT_CLIENT_NAME_MAX_LEN];                /*!< 客户端名字 */
	pid_t processID;                                   /*!< 程序正常运行后的进程ID */
	struct list_head registered_list;
	struct list_head listen_key_evt_list;
	struct list_head listen_ts_evt_list;
	tIpcThreadInfo *ti;
	uint32_t tar_handle;
} _evt_clt_t;

/**
 * @brief : 事件服务的信息
 */
typedef struct _event_service {
	struct list_head registered_clients_list;     /*!< 注册的事件客户端列表 */
	uint32_t registered_clt_count;                /*!< 已经注册的事件客户端数目 */
	struct list_head listen_key_evt_clt_list;     /*!< 监听按键事件的客户端列表 */
	uint32_t listen_key_evt_clt_count;            /*!< 监听按键事件的客户端数目 */
	struct list_head listen_ts_evt_clt_list;      /*!< 监听触屏事件的客户端列表 */
	uint32_t listen_ts_evt_clt_count;             /*!< 监听触屏事件的客户端数目 */
	pthread_mutex_t evtSrv_lock;
	pthread_t evt_detection_tid;                  /*!< 事件检测的线程ID */
	int gpio_keys_fd;                             /*!< GPIO KEY对应的输入事件文件描述符 */
	int touch_screen_fd;                          /*!< 触摸屏对应的输入事件文件描述符 */
} _evt_srv_t;

static _evt_srv_t g_evt_instance;

static int event_manager_init(void);
static int event_manager_start(void);
static int event_manager_stop(void);
static int event_manager_recv_msg(server_msg_t* msg);
static _evt_clt_t *find_registered_clt(const char *const clt_name);
static _evt_clt_t *find_listen_key_evt_clt(const char *const clt_name);
static _evt_clt_t *find_listen_ts_evt_clt(const char *const clt_name);
static int evt_srv_send_status_msg(const _evt_clt_t *client, _evt_status_t evt_status_msg);
static int evtmgr_process_register_evtcmd(const char *const clt_name, pid_t processID);
static int evtmgr_process_unregister_evtcmd(const char *const clt_name);
static int evtmgr_process_listen_key_evtcmd(const char *const clt_name, uint32_t input_device_index);
static int evtmgr_process_cancel_listen_key_evtcmd(const char *const clt_name);
static int evtmgr_process_listen_ts_evtcmd(const char *const clt_name, uint32_t input_device_index);
static int evtmgr_process_cancel_listen_ts_evtcmd(const char *const clt_name);
static int event_manager_process_cmd(_evt_ctrl_t *msg);
static void *evtmgr_evt_detection_thread(void *arg);

service_class_t event_server = {
    .server_name = {EVENT_MANAGER_SERVICE_NAME},
    .init = event_manager_init,
    .start = event_manager_start,
    .stop = event_manager_stop,
    .recv_msg = event_manager_recv_msg,
    .server_status = 0,
};

pid_t gettid()
{
	return syscall(SYS_gettid);
}


static int event_manager_init(void)
{
	int ret = 0;

	INIT_LIST_HEAD(&g_evt_instance.registered_clients_list);
	INIT_LIST_HEAD(&g_evt_instance.listen_key_evt_clt_list);
	INIT_LIST_HEAD(&g_evt_instance.listen_ts_evt_clt_list);

	pthread_mutex_init(&g_evt_instance.evtSrv_lock, NULL);

	g_evt_instance.gpio_keys_fd = open(GPIO_KEYS_INPUT_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
	if (-1 == g_evt_instance.gpio_keys_fd) {
		LOGE("open %s fail.\n", GPIO_KEYS_INPUT_DEVICE_PATH);

		return -1;
	}

	g_evt_instance.touch_screen_fd = open(TS_INPUT_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
	if (-1 == g_evt_instance.touch_screen_fd) {
		LOGE("open %s fail.\n", TS_INPUT_DEVICE_PATH);
		close(g_evt_instance.gpio_keys_fd);

		return -1;
	}

	ret = pthread_create(&g_evt_instance.evt_detection_tid, NULL, evtmgr_evt_detection_thread, NULL);
	if (ret) {
		LOGE("Failed to create the event detection thread.\n");
		close(g_evt_instance.gpio_keys_fd);
		close(g_evt_instance.touch_screen_fd);

		return ret;
	}

	return 0;
}

static int event_manager_start(void)
{
	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	event_server.server_status = 1;
	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int event_manager_stop(void)
{
	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	event_server.server_status = 0;
	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int event_manager_recv_msg(server_msg_t* msg)
{
	if (!msg) {
		LOGE("%s[%d] : Check parameter error.\n", __func__, __LINE__);
		return -1;
	}

	int ret = event_manager_process_cmd((_evt_ctrl_t *)msg->msg);

	msg->reply_callback(&ret,sizeof(int),msg->reply);
	return 0;
}

static _evt_clt_t *find_registered_clt(const char *const clt_name)
{
	_evt_clt_t *temp = NULL;

	list_for_each_entry(temp, &g_evt_instance.registered_clients_list, registered_list) {
		if (0 == strcmp(temp->name, clt_name))
			return temp;
	}

	return NULL;
}

static _evt_clt_t *find_listen_key_evt_clt(const char *const clt_name)
{
	_evt_clt_t *temp = NULL;

	list_for_each_entry(temp, &g_evt_instance.listen_key_evt_clt_list, listen_key_evt_list) {
		if (0 == strcmp(temp->name, clt_name))
			return temp;
	}

	return NULL;
}

static _evt_clt_t *find_listen_ts_evt_clt(const char *const clt_name)
{
	_evt_clt_t *temp = NULL;

	list_for_each_entry(temp, &g_evt_instance.listen_ts_evt_clt_list, listen_ts_evt_list) {
		if (0 == strcmp(temp->name, clt_name))
			return temp;
	}

	return NULL;
}

static int evt_srv_send_status_msg(const _evt_clt_t *client, _evt_status_t evt_status_msg)
{
	if (!client) {
		LOGE("%s[%d] : Check parameter error.\n", __func__, __LINE__);
		return -1;
	}

	tBinderIo bio, msg;
	sys_msg_t sysmsg;
	char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};

	strcpy(sysmsg.target_server_name, client->name);
	memcpy(sysmsg.msg, &evt_status_msg, sizeof(_evt_status_t));
	sysmsg.len = sizeof(_evt_status_t);

	binder_io_init(&bio, (void *)binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);
	binder_io_append_data(&bio, (char *)&sysmsg, sizeof(sys_msg_t));

	memset(&msg, 0, sizeof(msg));
	if(BINDER_STATUS_OK == binder_cmd_async_call(client->ti, &bio, &msg, client->tar_handle, 0)) {
		//char* data = NULL;
		//int sz = 0;
		//binder_io_get_data(&msg,&data,&sz);
		//binder_cmd_freebuf(client->ti, msg.data0);
		//return 0;
	} else {
		LOGE("Binder cmd sync call fail.\n");
		return -1;
	}

	return 0;
}

#define FDS_NUM 2

static void *evtmgr_evt_detection_thread(void *arg)
{
	_evt_status_t evt_status_msg;
	_evt_clt_t *evt_client = NULL;
	struct input_event iev;
	ssize_t bytes_read = 0;
	int ret = 0;
	fd_set fds;
	printf("%s %d current tid = %d \r\n",__func__,__LINE__,gettid());
	while (1) {
		FD_ZERO(&fds);
		if(g_evt_instance.gpio_keys_fd)
			FD_SET(g_evt_instance.gpio_keys_fd,&fds);
		if(g_evt_instance.touch_screen_fd)
			FD_SET(g_evt_instance.touch_screen_fd,&fds);
		select(g_evt_instance.touch_screen_fd+1,&fds,NULL,NULL,NULL);
		if(g_evt_instance.gpio_keys_fd){
			bytes_read = read(g_evt_instance.gpio_keys_fd, &iev, sizeof(struct input_event));
			if (sizeof(struct input_event) == bytes_read) {
				list_for_each_entry(evt_client, &g_evt_instance.listen_key_evt_clt_list, listen_key_evt_list) {
					evt_status_msg.evt_type = KEY_EVENT;
					// evt_status_msg.evt_msg.event_code = 0;
					evt_status_msg.evt_msg.msg_total_len = bytes_read;
					memcpy(evt_status_msg.evt_msg.evt_msg, &iev, evt_status_msg.evt_msg.msg_total_len);
					evt_srv_send_status_msg(evt_client, evt_status_msg);
				}
			}
		}
		if(g_evt_instance.touch_screen_fd){
			bytes_read = read(g_evt_instance.touch_screen_fd, &iev, sizeof(struct input_event));
			if (sizeof(struct input_event) == bytes_read) {
				list_for_each_entry(evt_client, &g_evt_instance.listen_ts_evt_clt_list, listen_ts_evt_list) {
					evt_status_msg.evt_type = TOUCH_SCREEN_EVENT;
					// evt_status_msg.evt_msg.event_code = 0;
					evt_status_msg.evt_msg.msg_total_len = bytes_read;
					memcpy(evt_status_msg.evt_msg.evt_msg, &iev, evt_status_msg.evt_msg.msg_total_len);
					evt_srv_send_status_msg(evt_client, evt_status_msg);
				}
			}
		}
	}
}

static int evtmgr_process_register_evtcmd(const char *const clt_name, pid_t processID)
{
	_evt_clt_t *evt_client = NULL;
	int ret = 0;

	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	evt_client = find_registered_clt(clt_name);
	if (!evt_client) {
		evt_client = (_evt_clt_t *)calloc(1, sizeof(_evt_clt_t));
		if (!evt_client) {
			pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
			return -1;
		}

		strcpy(evt_client->name, clt_name);
		evt_client->processID = processID;
		evt_client->tar_handle = binder_get_service(EVENT_CLIENT_SERVICE_NAME);
		evt_client->ti = binder_get_thread_info();
		if (evt_client->tar_handle <= 0) {
			LOGE("get %s target handle failed.\n", clt_name);
			free(evt_client);
			pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
			return -1;
		}

		INIT_LIST_HEAD(&evt_client->registered_list);
		list_add_head(&evt_client->registered_list, &g_evt_instance.registered_clients_list);
		g_evt_instance.registered_clt_count++;
	} else {
		LOGI("The %s event client has been registered.\n", clt_name);
	}

	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
	return 0;
}

static int evtmgr_process_unregister_evtcmd(const char *const clt_name)
{
	_evt_clt_t *evt_client = NULL;

	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	evt_client = find_registered_clt(clt_name);
	if (!evt_client) {
		LOGE("The %s event client is not registered.\n");
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	if (g_evt_instance.listen_key_evt_clt_count == g_evt_instance.registered_clt_count || \
			g_evt_instance.listen_ts_evt_clt_count == g_evt_instance.registered_clt_count) {
		LOGE("Please cancel the listen of the event before unregister.\n");
		return -1;
	}

	list_del(&evt_client->registered_list);

	binder_cmd_release(evt_client->ti, evt_client->tar_handle);
	flush_commands(evt_client->ti);

	free(evt_client);
	g_evt_instance.registered_clt_count--;

	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int evtmgr_process_listen_key_evtcmd(const char *const clt_name, uint32_t input_device_index)
{
	if (!clt_name)
		return -1;

	_evt_clt_t *evt_client = NULL;
	_evt_clt_t *listen_temp = NULL;

	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	evt_client = find_registered_clt(clt_name);
	if (!evt_client) {
		LOGE("The %s event client is not registered.\n");
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	listen_temp = find_listen_key_evt_clt(clt_name);
	if (listen_temp) {
		LOGI("%s event client has listened for key events.\n", clt_name);
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return 0;
	}

	INIT_LIST_HEAD(&evt_client->listen_key_evt_list);
	list_add_head(&evt_client->listen_key_evt_list, &g_evt_instance.listen_key_evt_clt_list);
	g_evt_instance.listen_key_evt_clt_count++;

	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int evtmgr_process_cancel_listen_key_evtcmd(const char *const clt_name)
{
	if (!clt_name)
		return -1;

	_evt_clt_t *evt_client = NULL;
	_evt_clt_t *listen_temp = NULL;

	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	evt_client = find_registered_clt(clt_name);
	if (!evt_client) {
		LOGE("%s event client unregistered.\n");
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	listen_temp = find_listen_key_evt_clt(clt_name);
	if (!listen_temp) {
		LOGE("%s event client is not monitored for key events.\n", clt_name);
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	list_del(&evt_client->listen_key_evt_list);
	g_evt_instance.listen_key_evt_clt_count--;

	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int evtmgr_process_listen_ts_evtcmd(const char *const clt_name, uint32_t input_device_index)
{
	if (!clt_name)
		return -1;

	_evt_clt_t *evt_client = NULL;
	_evt_clt_t *listen_temp = NULL;

	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	evt_client = find_registered_clt(clt_name);
	if (!evt_client) {
		LOGE("%s event client unregistered.\n");
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	listen_temp = find_listen_ts_evt_clt(clt_name);
	if (listen_temp) {
		LOGI("%s event client has listened for Touch Screen events.\n", clt_name);
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return 0;
	}

	INIT_LIST_HEAD(&evt_client->listen_ts_evt_list);
	list_add_head(&evt_client->listen_ts_evt_list, &g_evt_instance.listen_ts_evt_clt_list);
	g_evt_instance.listen_ts_evt_clt_count++;

	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int evtmgr_process_cancel_listen_ts_evtcmd(const char *const clt_name)
{
	if (!clt_name)
		return -1;

	_evt_clt_t *evt_client = NULL;
	_evt_clt_t *listen_temp = NULL;

	pthread_mutex_lock(&g_evt_instance.evtSrv_lock);
	evt_client = find_registered_clt(clt_name);
	if (!evt_client) {
		LOGE("%s event client unregistered.\n", clt_name);
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	listen_temp = find_listen_ts_evt_clt(clt_name);
	if (!listen_temp) {
		LOGE("%s event client is not monitored for Touch Screen events.\n", clt_name);
		pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);
		return -1;
	}

	list_del(&evt_client->listen_ts_evt_list);
	g_evt_instance.listen_ts_evt_clt_count--;

	pthread_mutex_unlock(&g_evt_instance.evtSrv_lock);

	return 0;
}

static int event_manager_process_cmd(_evt_ctrl_t *msg)
{
	if (!msg)
		return -1;

	int ret = 0;

	switch (msg->cmd) {
		case REGISTER_EVT:
			ret = evtmgr_process_register_evtcmd(msg->clt_name, msg->processID);
			break;
		case UNREGISTER_EVT:
			ret = evtmgr_process_unregister_evtcmd(msg->clt_name);
			break;
		case LISTEN_KEY_EVT:
			ret = evtmgr_process_listen_key_evtcmd(msg->clt_name, msg->input_device_index);
			break;
		case CANCEL_LISTEN_KEY_EVT:
			ret = evtmgr_process_cancel_listen_key_evtcmd(msg->clt_name);
			break;
		case LISTEN_TOUCH_SCREEN_EVT:
			ret = evtmgr_process_listen_ts_evtcmd(msg->clt_name, msg->input_device_index);
			break;
		case CANCEL_LISTEN_TOUCH_SCREEN_EVT:
			ret = evtmgr_process_cancel_listen_ts_evtcmd(msg->clt_name);
			break;
		default:
			ret = -1;
	}

	return ret;
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

    ret = event_server.recv_msg(&ser_msg);
    if(ret){
        LOGE("call %s recv_msg failed\n");
        binder_io_append_uint32(reply,0xffffffff); // return error code
    }

    return 0;
}

static tBinderService eventserver_service = {
    .transact_cb = msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int main()
{
    event_manager_init();
    event_manager_start();

    binder_add_service(EVENT_MANAGER_SERVICE_NAME,&eventserver_service);
    binder_thread_enter_loop(1,1);

    while (1) {
        sleep(2);
    }
}
