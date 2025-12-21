#ifndef __NETWORK_SERVICE_H__
#define __NETWORK_SERVICE_H__

#include <stdbool.h>

typedef enum {
	WLAN_CLI_REGISTER,
	WLAN_CLI_UNREGISTER,
	WLAN_ENABLE,
	WLAN_DISABLE,
	WLAN_NETWORK_ENABLE,
	WLAN_NETWORK_DISABLE,
	WLAN_START_SCAN,
	WLAN_GET_SCAN_RESULTS,
	WLAN_ADD_CONFIG,
	WLAN_DEL_CONFIG,
	WLAN_CONNECT_NETWORK,
	WLAN_DISCONNECT_NETWORK,
	WLAN_DISPATCH_EVENT,
	WLAN_GET_CONNECT_STA,
	WLAN_SAVE_CONFIG,
	NETWORK_CMD_MAX,
}Network_Cmd_t;


typedef struct {
	pid_t client_pid;
	int16_t client_level;
	bool need_listen_wlan_sta;
	bool need_listen_eth_sta;
}network_register_info_t;

typedef struct {
	Network_Cmd_t cmd;
	char msg[128];
	int32_t client_pid;
}iss_network_msg_t;

typedef struct {
	char ssid[32];
	char passwd[32];
}iss_wlan_config_info_t;

typedef enum {
	WPA_EVT_CONNECTED,
	WPA_EVT_DISCONNECTED,
	WPA_EVT_PASSWORD_CHANGED,
	WPA_EVT_SCAN_RESULTS,
	WPA_EVT_STATE_CHANGE,
	WPA_EVT_NETWORK_NOT_FOUND,
	WPA_EVT_SIGNAL_CHANGE,
	WPA_EVT_TERMINATING,
	WLAN_EVT_INTERNET_ERROR,
}NetWorkEvent_t;


#endif // __NETWORK_SERVICE_H__

