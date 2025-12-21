#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stdbool.h>

typedef enum {
    LEVEL_0,
    LEVEL_1,
    LEVEL_2,
}NET_APP_LEVEL_t;


typedef struct {
  char ssid[32];
  int16_t signal_level;
  int16_t freq;
}WifiScanResult_t;

typedef struct {
  char ssid[32];
  char passwd[32];
  char key_mgmt[8];
  int32_t nid;
}WifiConfigInfo_t;

typedef struct {
	bool connected_sta;
	char connected_ssid[32];
	char ipstring[16];
	int32_t freq;
}wlan_connect_sta_t;

 typedef struct {
   void (*net_sta_callback)(void*);
   void* priv;
 }NetState_Cb_t;

int32_t NetWorkManager_Init(NET_APP_LEVEL_t level);
int32_t NetWorkManager_DeInit(void);
int32_t WifiEnable(void);
int32_t WifiDisable(void);
int32_t WifiScan(void);
int32_t WifiGetScanResults(WifiScanResult_t** results,int32_t wait_ms);
int32_t WifiAddConfig(WifiConfigInfo_t* config);
int32_t WifiDeleteConfig(int32_t nid);
int32_t WifiSaveCurrentConfig(void);
int32_t WifiGetConfig(WifiConfigInfo_t* configs);
int32_t WifiEnableNetwork(int32_t nid);
int32_t WifiDisableNetwork(int32_t nid);
int32_t WifiConnectNetwork(int32_t nid,int16_t wait_s);
int32_t WifiDisconnectNetwork(int32_t nid);
int32_t WifiGetLinkedSta(wlan_connect_sta_t* sta);
int32_t WifiSetListenCallback(NetState_Cb_t* cb);


#endif // __NETWORK_H__


