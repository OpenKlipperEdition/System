#ifndef __BACKLIGHT_CLIENT_H__
#define __BACKLIGHT_CLIENT_H__


typedef struct _bl_handle {
	void *ti;
	uint32_t target_handle;
	char cli_name[32];
	int max_bl_level;
}backlight_handle_t;

/**
 * @brief 背光控制服务申请
 * @param [in] client_name : CLIENT名称
 * @retval backlight_handle_t   成功
 * @retval NULL                   失败
 */
backlight_handle_t* backlight_server_request(char* client_name);

/**
 * @brief 设置背光亮度
 * @param [in] hdl : 申请的服务句柄
 * @param [in] level : 背光亮度值
 * @retval 0   成功
 * @retval 非0 失败
 */
int backlight_set_level(backlight_handle_t* hdl,int level);

/**
 * @brief 获取背光亮度
 * @param [in] hdl : 申请的服务句柄
 * @param [in] level : 背光亮度值
 * @retval 0   成功
 * @retval 非0 失败
 */
int backlight_get_level(backlight_handle_t* hdl,int *level);

/**
 * @brief 背光开关控制
 * @param [in] hdl : 申请的服务句柄
 * @param [in] onoff : 0--关闭 1--开启
 * @retval 0   成功
 * @retval 非0 失败
 */
int backlight_on_off(backlight_handle_t* hdl,int onoff);

/**
 * @brief 服务释放
 * @param [in] hdl : 申请的服务句柄
 * @retval 0   成功
 * @retval 非0 失败
 */
int backlight_server_release(backlight_handle_t *hdl);

#endif	// __BACKLIGHT_CLIENT_H__


