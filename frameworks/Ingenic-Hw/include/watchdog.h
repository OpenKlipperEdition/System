#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__
#include "ihal_info.h"

/**
 *      模块说明：
 *      内核配置条件:
 */

/**
 * @defgroup group_Watchdog Watchdog模块
 * @{
 */

/**
 * @addtogroup group_Watchdog_data_type 数据类型定义
 * @{
 */

#define WATCHDOG_START	0x2
#define WATCHDOG_STOP	0x1

/**
 * @brief WatchDog的属性
 */
typedef struct {
	int fd;				/*!< 设备节点文件描述符*/
	char node[16];			/*!< 设备节点，如/dev/watchdog0 */
	int set_timeout;		/*!< 设置超时时间 */
	int status;			/*!< watchdog工作状态（start/stop）*/
	pthread_mutex_t lock;		/*!< WatchDog互斥锁 */
} IHal_WatchDog_Attr;

typedef void IHal_WatchDog_Handle_t;

/**
 * @}
 */

/**
 * @addtogroup group_Watchdog_API API定义
 * @{
 */

/**
 * @brief WatchDog的初始化
 * @param [in] devnode : 设备节点路径
 * @retval IHal_WatchDog_Handle_t* 成功
 * @retval IHAL_RNULL              失败
 */
IHal_WatchDog_Handle_t *IHal_WatchDog_Init(IHAL_INT8 *devnode);

/**
 * @brief  WatchDog的反初始化
 * @param [in] handle  : WatchDog handle
 * @retval 0		成功
 * @retval 非0		失败
 */
IHAL_INT32 IHal_WatchDog_DeInit(IHal_WatchDog_Handle_t *handle);

/**
 * @brief  WatchDog设置超时时间
 * @param [in] handle  : WatchDog handle
 * @param [in] timeout : 超时时间
 * @retval 0		成功
 * @retval 非0		失败
 */
IHAL_INT32 IHal_WatchDog_SetTimeout(IHal_WatchDog_Handle_t *handle, int timeout);

/**
 * @brief  WatchDog的启动
 * @param [in] handle  : WatchDog handle
 * @retval 0		成功
 * @retval 非0		失败
 */
IHAL_INT32 IHal_WatchDog_Start(IHal_WatchDog_Handle_t *handle);

/**
 * @brief  WatchDog的停止
 * @param [in] handle  : WatchDog handle
 * @retval 0		成功
 * @retval 非0		失败
 */
IHAL_INT32 IHal_WatchDog_Stop(IHal_WatchDog_Handle_t *handle);

/**
 * @brief  WatchDog的喂狗操作
 * @param [in] handle  : WatchDog handle
 * @retval 0		成功
 * @retval 非0		失败
 */
IHAL_INT32 IHal_WatchDog_FeedDog(IHal_WatchDog_Handle_t *handle);

/**
 * @}
 */

/**
 * @}
 */
#endif  // __DPU_H__
