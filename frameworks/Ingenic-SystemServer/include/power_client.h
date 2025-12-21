#ifndef __POWER_CLIENT_H__
#define __POWER_CLIENT_H__

typedef struct power_client power_clt_t;

/**
 * @brief    申请电源管理的客户端句柄
 * @param[in] name  客户端名称
 * @return power_clt_t* 电源管理的客户端句柄
 */
power_clt_t *alloc_power_clt(const char *name);
/**
 * @brief    释放电源管理的客户端句柄
 * @param[in] clt   电源管理的客户端句柄
 * @return int         参数非法返回-2,成功返回0
 */
int free_power_clt(power_clt_t *clt);
/**
 * @brief    定时休眠
 * @param[in] clt   电源管理的客户端句柄
 * @param[in] time  定时休眠的时间,传入0立即休眠,单位秒
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int power_suspend(power_clt_t *clt, unsigned int time);
/**
 * @brief    定时关机
 * @param[in] clt   电源管理的客户端句柄
 * @param[in] time  定时关机的时间,单位秒
 * @return int         参数非法返回-2,失败返回-1,成功返回0,权限不足返回2
 */
int power_down(power_clt_t *clt, unsigned int time);
/**
 * @brief    定时唤醒
 * @param[in] clt   电源管理的客户端句柄
 * @param[in] time  定时唤醒的时间,单位秒
 * @return int         参数非法返回-2,失败返回-1,成功返回0
 */
int power_wakeup(power_clt_t *clt, unsigned int time);


#endif