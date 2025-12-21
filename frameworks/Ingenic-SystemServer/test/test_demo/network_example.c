#include <sys/time.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sys_common.h"
#include <assert.h>

#include "network.h"

// for client API test
int main(int argc,char** argv)
{
	if(argc < 2){
		printf("noralm exp: ./netcliTest run \r\n");
		printf("add exp: ./netcliTest add ssid passwd \r\n");
	}
	WifiScanResult_t* results = NULL;
	int32_t ret = NetWorkManager_Init(LEVEL_0);
	if(!ret){
		WifiEnable();
		WifiScan();
		ret = WifiGetScanResults(&results,3000);
		if(ret > 0){
			for(int i = 0; i < ret; i++){
				printf(" client >>> ssid: %s freq:%d  siglevel: %d\r\n",\
						results[i].ssid,results[i].freq,results[i].signal_level);
			}
			free(results);
		}
		WifiConfigInfo_t config;
		memset(&config,0,sizeof(WifiConfigInfo_t));
		if(argc >= 4 && !strcmp(argv[1],"add")){
			strncpy(&config.ssid[0],argv[2],strlen(argv[2]));
			strncpy(&config.passwd[0],argv[3],strlen(argv[3]));
			printf("add ssid : %s psk: %s ###\r\n",config.ssid,config.passwd);
			int nid = WifiAddConfig(&config);
			ret = WifiConnectNetwork(nid,3);
			if(ret){
				printf("connect error ####\r\n");
			} else {
				WifiSaveCurrentConfig();
				printf("connect sueccess ####\r\n");
				sleep(20);
			}
		} else {
			int wait_s = 8;
			wlan_connect_sta_t sta;
			WifiGetLinkedSta(&sta);
			do{
				if(sta.connected_sta == true)
					break;
				sleep(1);
				WifiGetLinkedSta(&sta);

			}while(wait_s-- > 0);
			if(sta.connected_sta == true){
				printf("device connect wifi_point: %s ip:%s ##\r\n",sta.connected_ssid,sta.ipstring);
				sleep(20);
			} else {
				printf("device not connected any hp ##\r\n");
			}
		}
		WifiDisable();
		NetWorkManager_DeInit();
	} else {
		printf("net work manager init error \r\n");
		return -1;
	}
	return 0;
}



