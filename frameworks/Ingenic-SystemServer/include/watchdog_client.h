#ifndef __WATCHDOG_CLIENT_H__
#define __WATCHDOG_CLIENT_H__



typedef struct wdt_client wdt_clt_t;

/**
 * @brief    申请看门狗客户端句柄
 * @param[in] name  客户端名称
 * @return wdt_clt_t* 成功返回看门狗客户端句柄,失败返回NULL
 */
wdt_clt_t *alloc_wdt_clt(const char *name);
/**
 * @brief    释放看门狗客户端句柄
 * @param[in] clt   看门狗客户端句柄
 * @return int         参数非法返回-2,成功返回0
 */
int free_wdt_clt(wdt_clt_t *clt);
/**
 * @brief    向服务端注册看门狗信息
 * @param[in] clt   看门狗客户端句柄
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int regiset_wdt_clt(wdt_clt_t *clt);
/**
 * @brief    向服务端注销看门狗信息
 * @param[in] clt   看门狗客户端句柄
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int unregiset_wdt_clt(wdt_clt_t *clt);
/**
 * @brief    开始看门狗服务
 * @param[in] clt   看门狗客户端句柄
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int start_wdt(wdt_clt_t *clt);
/**
 * @brief    暂停看门狗服务
 * @param[in] clt   看门狗客户端句柄
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int stop_wdt(wdt_clt_t *clt);
/**
 * @brief    喂狗
 * @param[in] clt   看门狗客户端句柄
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int feed_wdt(wdt_clt_t *clt);
/**
 * @brief    设置超时时间
 * @param[in] clt   看门狗客户端句柄
 * @param[in] time  超时时间,单位毫秒
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int set_wdt_timeout(wdt_clt_t *clt, int time);


#endif