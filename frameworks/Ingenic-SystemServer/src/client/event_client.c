#include <pthread.h>
#include <linux/input.h>
#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "dlog.h"
#include "systemserver.h"
#include "fifo.h"
#include "list.h"


#define EVT_CLIENT_SERVICE_NAME  "EventClientService"
#define EVT_CLIENT_NAME_MAX_LEN  50
#define LOG_TAG                  "EventClient"
#define EVENT_MESSAGE_MAX_LEN	 256


/**
 * @brief : 事件类型
 */
typedef enum event_type {
    KEY_EVENT,                           /*!< 按键事件 */
    TOUCH_SCREEN_EVENT,                  /*!< 触屏事件 */
    /* TODO */
} evt_type_t;

/**
 * @brief : 事件消息
 */
typedef struct event_message {
    int event_code;                             /*!< 唯一event编码 */
    uint8_t msg_total_len;                      /*!< 消息总长度 */
    uint8_t evt_msg[EVENT_MESSAGE_MAX_LEN];     /*!< 事件消息，后续强制类型转换 */
} evt_msg_t;

/**
 * @brief : 事件状态信息
 */
typedef struct event_status {
	evt_type_t evt_type;        /*!< 事件类型 */
	evt_msg_t  evt_msg;         /*!< 事件信息 */
} evt_status_t;

/**
 * @brief : 事件的命令操作
 */
typedef enum evt_cmd {
    REGISTER_EVT = 1,                   /*!< 注册客户端信息到事件链表 */
    UNREGISTER_EVT,                     /*!< 从事件链表注销客户端信息 */
    LISTEN_KEY_EVT,                     /*!< 监听按键事件 */
    CANCEL_LISTEN_KEY_EVT,              /*!< 取消监听按键事件 */
    LISTEN_TOUCH_SCREEN_EVT,            /*!< 监听触屏事件 */
    CANCEL_LISTEN_TOUCH_SCREEN_EVT,     /*!< 取消监听触屏事件 */
} evt_cmd_t;

/**
 * @brief : 事件的命令控制
 */
typedef struct evt_ctrl {
    uint32_t cmd;                                      /*!< 命令码 */
    char clt_name[EVT_CLIENT_NAME_MAX_LEN];            /*!< 客户端的名字 */
    pid_t processID;                                   /*!< 当前事件客户端进程的进程ID */
} evt_ctrl_t;

/**
 * @brief : 事件客户端对象结构体
 */
typedef struct event_client {
    uint32_t tar_handle;                               /*!< 服务的句柄 */
    tIpcThreadInfo *ti;                                /*!< 当前线程的Binder信息 */
    char clt_name[EVT_CLIENT_NAME_MAX_LEN];            /*!< 客户端名字 */
    pid_t processID;                                   /*!< 当前事件客户端进程的进程ID */
    pthread_mutex_t evt_mutex_lock;
} evt_clt_t;

static void (*handle_key_event)(evt_msg_t evt_msg);
static void (*handle_touch_screen_event)(evt_msg_t evt_msg);
static int evt_clt_send_cmd(evt_clt_t *p_evt_clt, uint32_t opt, evt_ctrl_t *p_config);
static int evt_msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag);

static int evt_clt_srv_create_flag = 0;
static pthread_mutex_t g_mutex_lock = PTHREAD_MUTEX_INITIALIZER;
static tBinderService event_client_server = {
    .transact_cb = evt_msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

static int connect_event_server(evt_clt_t *p_evt_clt)
{
	if (!p_evt_clt)
		return -1;
	p_evt_clt->tar_handle = binder_get_service(EVENT_MANAGER_SERVICE_NAME);
    if (p_evt_clt->tar_handle <= 0) {
        printf("Event clients failed to get binder service[%s].\n", EVENT_MANAGER_SERVICE_NAME);
		return -1;
    }
    p_evt_clt->ti = binder_get_thread_info();

	return 0;
}

evt_clt_t *create_evt_clt(const char *clt_name)
{
    evt_clt_t *p_evt_clt = NULL;

    p_evt_clt = (evt_clt_t *)calloc(1, sizeof(evt_clt_t));
    if (!p_evt_clt) {
        LOGE("Failed to allocate event client %s object.\n", clt_name);
        return NULL;
    }

    if (strlen(clt_name) + 1 > EVT_CLIENT_NAME_MAX_LEN) {
        LOGE("The client name is too long[%d], with a maximum length of %d (including \'\0\').\n", \
                strlen(clt_name) + 1, EVT_CLIENT_NAME_MAX_LEN);
        free(p_evt_clt);
        return NULL;
    }
    strcpy(p_evt_clt->clt_name, clt_name);

    p_evt_clt->processID = getpid();
    pthread_mutex_init(&p_evt_clt->evt_mutex_lock, NULL);

	if (connect_event_server(p_evt_clt)) {
		pthread_mutex_destroy(&p_evt_clt->evt_mutex_lock);
		free(p_evt_clt);
		p_evt_clt = NULL;
		return NULL;
	}

    return p_evt_clt;
}

void destroy_evt_clt(evt_clt_t *p_evt_clt)
{
    if (!p_evt_clt) {
        LOGE("parameter error.\n");
        return;
    }

	pthread_mutex_destroy(&p_evt_clt->evt_mutex_lock);
    binder_cmd_release(p_evt_clt->ti, p_evt_clt->tar_handle);
    flush_commands(p_evt_clt->ti);

    free(p_evt_clt);
    p_evt_clt = NULL;

    return;
}

int register_evt_clt(evt_clt_t *p_evt_clt)
{
    if (!p_evt_clt) {
        LOGE("%s[%d] : Check parameter error.\n", __func__, __LINE__);
        return -1;
    }

	pthread_mutex_lock(&g_mutex_lock);
	if (0 == evt_clt_srv_create_flag) {
		binder_add_service(EVT_CLIENT_SERVICE_NAME, &event_client_server);
		binder_thread_enter_loop(1, 0);
		evt_clt_srv_create_flag = 1;
	}
	pthread_mutex_unlock(&g_mutex_lock);

    return evt_clt_send_cmd(p_evt_clt, REGISTER_EVT, NULL);
}

int unregister_evt_clt(evt_clt_t *p_evt_clt)
{
    if (!p_evt_clt) {
        LOGE("%s[%d] : Check parameter error.\n", __func__, __LINE__);
        return -1;
    }

   // binder_threads_shutdown();

    return evt_clt_send_cmd(p_evt_clt, UNREGISTER_EVT, NULL);
}

int listen_event(evt_clt_t *p_evt_clt, evt_type_t evt_type, void (*handle_event)(evt_msg_t evt_msg))
{
	if (!p_evt_clt || !handle_event) {
		LOGE("%s[%d] : Check parameter error.\n", __func__, __LINE__);
		return -1;
	}

	switch (evt_type) {
		case KEY_EVENT:
			pthread_mutex_lock(&g_mutex_lock);
			handle_key_event = handle_event;
			pthread_mutex_unlock(&g_mutex_lock);
			evt_clt_send_cmd(p_evt_clt, LISTEN_KEY_EVT, NULL);
			break;
		case TOUCH_SCREEN_EVENT:
			pthread_mutex_lock(&g_mutex_lock);
			handle_touch_screen_event = handle_event;
			pthread_mutex_unlock(&g_mutex_lock);
			evt_clt_send_cmd(p_evt_clt, LISTEN_TOUCH_SCREEN_EVT, NULL);
			break;
		default:
			LOGE("Unsupported types of listening events[%d].\n", evt_type);
			return -1;
	}

	return 0;
}

int cancel_listen_event(evt_clt_t *p_evt_clt, evt_type_t evt_type)
{
	if (!p_evt_clt)
		return -1;

	switch (evt_type) {
		case KEY_EVENT:
			pthread_mutex_lock(&g_mutex_lock);
			handle_key_event = NULL;
			pthread_mutex_unlock(&g_mutex_lock);
			evt_clt_send_cmd(p_evt_clt, CANCEL_LISTEN_KEY_EVT, NULL);
			break;
		case TOUCH_SCREEN_EVENT:
			pthread_mutex_lock(&g_mutex_lock);
			handle_touch_screen_event = NULL;
			pthread_mutex_unlock(&g_mutex_lock);
			evt_clt_send_cmd(p_evt_clt, CANCEL_LISTEN_TOUCH_SCREEN_EVT, NULL);
		default:
			LOGE("Unsupported types of listening events[%d].\n", evt_type);
			return -1;
	}

	return 0;
}

static int evt_clt_send_cmd(evt_clt_t *p_evt_clt, uint32_t opt, evt_ctrl_t *p_config)
{
	if (!p_evt_clt)
		return -1;

	tBinderIo bio, msg;
	char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
	sys_msg_t sysmsg;
	evt_ctrl_t cmd_info;
	int ret = 0;

	pthread_mutex_lock(&p_evt_clt->evt_mutex_lock);
	if (p_config)
		memcpy(&cmd_info, p_config, sizeof(evt_ctrl_t));
	else
		cmd_info.cmd = opt;

	cmd_info.processID = p_evt_clt->processID;
	strcpy(cmd_info.clt_name, p_evt_clt->clt_name);

	if (strlen(EVENT_MANAGER_SERVICE_NAME) + 1 > sizeof(sysmsg.target_server_name)) {
		LOGE("The target server name is too long[%d], with a maximum length of %d (including \'\0\').\n", \
				strlen(EVENT_MANAGER_SERVICE_NAME) + 1, sizeof(sysmsg.target_server_name));
		pthread_mutex_unlock(&p_evt_clt->evt_mutex_lock);
		return -1;
	}

	strcpy(sysmsg.target_server_name, EVENT_MANAGER_SERVICE_NAME);
	memcpy(sysmsg.msg, &cmd_info, sizeof(evt_ctrl_t));
	sysmsg.len = sizeof(evt_ctrl_t);

	binder_io_init(&bio, (void *)binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);
	binder_io_append_data(&bio, (char *)&sysmsg, sizeof(sys_msg_t));

	memset(&msg, 0, sizeof(tBinderIo));
	if (BINDER_STATUS_OK == binder_cmd_sync_call(p_evt_clt->ti, &bio, &msg, p_evt_clt->tar_handle, 0)) {
		int sz = 0;
		uint8_t* data = 0;

		binder_io_get_data(&msg, &data, (size_t *)&sz);
		binder_cmd_freebuf(p_evt_clt->ti, msg.data0);

		pthread_mutex_unlock(&p_evt_clt->evt_mutex_lock);

		return *data;
	} else {
		LOGE("Binder cmd sync call fail.\n");
		pthread_mutex_unlock(&p_evt_clt->evt_mutex_lock);
		return -1;
	}

	pthread_mutex_unlock(&p_evt_clt->evt_mutex_lock);

	return 0;
}

static int evt_msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	sys_msg_t* sys_msg;
	unsigned char* data;
	int msg_size = 0;
	evt_status_t *evt_status_msg;

	int ret = binder_io_get_data(msg, &data, &msg_size);
	if (ret || msg_size <= 0) {
		LOGE("binder get data failed.\n");
		return -1;
	}

	sys_msg = (sys_msg_t*)data;
	evt_status_msg = (evt_status_t *)sys_msg->msg;

	switch (evt_status_msg->evt_type) {
		case KEY_EVENT:
			pthread_mutex_lock(&g_mutex_lock);
			if (handle_key_event)
				handle_key_event(evt_status_msg->evt_msg);
			pthread_mutex_unlock(&g_mutex_lock);
			break;
		case TOUCH_SCREEN_EVENT:
			pthread_mutex_lock(&g_mutex_lock);
			if (handle_touch_screen_event)
				handle_touch_screen_event(evt_status_msg->evt_msg);
			pthread_mutex_unlock(&g_mutex_lock);
			break;
		default:
			LOGE("Unsupported types of listening events[%d].\n", evt_status_msg->evt_type);
			break;
	}

	return 0;
}
