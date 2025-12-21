#ifndef __EVENT_CLIENT_H__
#define __EVENT_CLIENT_H__

#include <linux/input.h>
#include <inttypes.h>

#define EVENT_MESSAGE_MAX_LEN	256

/**
 * @brief : 事件类型
 */
typedef enum event_type {
	KEY_EVENT,                   /*!< 按键事件 */
	TOUCH_SCREEN_EVENT,          /*!< 触屏事件 */
	/* TODO */
} evt_type_t;

/**
 * @brief : 事件消息
 */
typedef struct event_message {
	int event_code;								/*!< 唯一event编码 */
	uint8_t msg_total_len;						/*!< 消息总长度 */
	uint8_t evt_msg[EVENT_MESSAGE_MAX_LEN];		/*!< 事件消息，后续强制类型转换 */
} evt_msg_t;

typedef struct event_client evt_clt_t;

/**
 * @brief : 创建一个事件客户端对象
 * @param clt_name [in] : 事件客户端名字
 * @retval tEventClient *        成功
 * @retval NULL			         失败
 */
evt_clt_t *create_evt_clt(const char *clt_name);

/**
 * @brief : 销毁事件客户端对象
 * @param p_evt_clt [in] : 要销毁的客户端对象的地址
 * @return void
 */
void destroy_evt_clt(evt_clt_t *p_evt_clt);

/**
 * @brief : 注册事件客户端对象到SystemServer中
 * @param p_evt_clt [in] : 事件客户端对象的地址
 * @retval 0       成功
 * @retval 非0     失败
 */
int register_evt_clt(evt_clt_t *p_evt_clt);

/**
 * @brief : 从SystemServer中注销事件客户端对象
 * @param p_evt_clt [in] : 事件客户端对象的地址
 * @retval 0       成功
 * @retval 非0     失败
 */
int unregister_evt_clt(evt_clt_t *p_evt_clt);

/**
 * @brief : 监听事件
 * @param p_evt_clt          [in] : 事件客户端对象的地址
 * @param evt_type           [in] : 要监听的事件类型
 * @param handle_event       [in] : 事件处理函数
 * @retval 0       成功
 * @retval 非0     失败
 */
int listen_event(evt_clt_t *p_evt_clt, evt_type_t evt_type, void (*handle_event)(evt_msg_t evt_msg));

/**
 * @brief : 取消监听事件
 * @param p_evt_clt [in] : 事件客户端对象的地址
 * @param evt_type  [in] : 要取消监听的事件类型
 * @retval 0       成功
 * @retval 非0     失败
 */
int cancel_listen_event(evt_clt_t *p_evt_clt, evt_type_t evt_type);

#endif        /* __EVENT_CLIENT_H__ */

