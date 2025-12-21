#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdbool.h>
#include <pthread.h>

#include <arpa/inet.h>     
#include <sys/socket.h>
#include <errno.h>

#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include "sys_common.h"
#include "list.h"
#include "systemserver_config.h"
#include "network_service.h"
#include "network.h"

#include "wpa_ctrl.h"

#define LOG_TAG "ISS_NETS"
#include "dlog.h"


#define MAX_CONFIGUERD_NUM 32

#define CONFIG_CTRL_IFACE_DIR "/var/run/wpa_supplicant"
#define CONFIG_CTRL_PATH CONFIG_CTRL_IFACE_DIR"/wlan0"


#define WPA_CMD_BUFFER_SIZE		512
#define WPA_MIN_REPBUFFER_SIZE 64
#define WPA_MAX_REPBUFFER_SIZE 2048
#define REP_RESULTS_NUM 15

typedef struct {
	pid_t client_pid;
	int32_t client_ipc_handle;
}client_death_priv_t;

typedef struct {
	int32_t client_ipc_handle;
	pid_t client_pid;
	int32_t client_level;
	struct list_head list;
	struct binder_death* client_death;
	bool listen_wlan_evt;
	bool listen_eth_evt;
}network_client_ctx_t;


typedef struct {
	char ssid[32];
	int16_t nid;
}network_info_t;

struct {
	int16_t client_cnt;
	int16_t level0_cnt;
	int16_t level1_cnt;
	int16_t level2_cnt;
	int16_t current_connect_nid;	// 当前连接网络的nid
	bool connect_sta;
	struct wpa_ctrl* wpa_cmd_ctrl;
	struct wpa_ctrl* wpa_mon_ctrl;		// attach for recv event

	struct list_head client_manager_list;
	struct list_head configured_net_list;

	pthread_t wlan_ping_tid;
	pthread_t eth_ping_tid;
	pthread_t wlan_evt_mon_tid;

	pthread_mutex_t slock;
	pthread_mutex_t mg_lock;

	WifiScanResult_t* wlan_results_save;
	int16_t max_results_num;
	int16_t current_results_num;

	pid_t wpas_pid;
	pid_t udhcpc_pid;
	char wlan_ifname[8];
	char eth_ifname[8];
	network_info_t* configured_net;
	int16_t configured_cnt;
}g_network_service;

struct wpa_results {
    char bssid[32];
    char freq[8];
    char riss[4];
    char flags[64];
    char ssid[64];
};



static int32_t wpa_ctrl_send_cmd(char* cmd,int32_t cmd_len,uint8_t* reply,int32_t* reply_len);

static int32_t wlan_connect_network(int16_t nid,char* ifname);
static int32_t wlan_start_scan(char* ifname);
static int32_t wpa_cmd_flush(char* ifname);
static int32_t wpa_reload_conf(void);
static int32_t wlan_disconnect_network(char* ifname);
static int32_t wlan_disable(void);
static int32_t wpa_enable_autoscan(bool enable);

static int tcp_connect_test(const char *ip, int port) 
{
    int sockfd;
    struct sockaddr_in servaddr;
    // 创建套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

        perror("socket creation failed");

        return -1;

    }
    memset(&servaddr, 0, sizeof(servaddr));
    // 设置服务器地址结构
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        close(sockfd);
        return -1;
    }
    // 尝试连接
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("Net_Check: connect to internet failed!! \r\n");
        close(sockfd);
        return -1;

    }
    /* printf("Successfully connected to %s:%d\n", ip, port); */
    close(sockfd);
    return 0;
}


static int wpa_supplicant_start(char* ifname)
{
	pid_t pid;
	pid = fork();
	if(pid < 0){
		printf("%s fork error !!\r\n",__func__);
		return -1;
	} else if(pid == 0){
		char* argv[] = {"wpa_supplicant","-Dnl80211","-i",ifname,"-c/etc/wpa_supplicant.conf",NULL};
		if(execve("/usr/sbin/wpa_supplicant",argv,NULL) == -1){
			perror("execv :");
			return -1;
		}
	} else {
		g_network_service.wpas_pid = pid;
	}
	printf("%s wpa_pid: %d ##\r\n",__func__,g_network_service.wpas_pid);
	return 0;
}

static int udhcpc_start(char* ifname)
{
	pid_t pid;
	pid = fork();
	if(pid < 0){
		printf("%s fork error !!\r\n",__func__);
		return -1;
	} else if(pid == 0){
		char* argv[] = {"udhcpc","-i",ifname,"-f",NULL};
		if(execve("/sbin/udhcpc",argv,NULL) == -1){
			perror("execv :");
			return -1;
		}
	} else {
		g_network_service.udhcpc_pid = pid;
		printf("%s udhchc_pid: %d ##\r\n",__func__,g_network_service.udhcpc_pid);
	}
	return 0;
}

static void wpa_supplicant_stop(void)
{
	int retry = 100;
	if(g_network_service.wpas_pid){
		if(kill(g_network_service.wpas_pid,SIGTERM) == -1){
			kill(g_network_service.wpas_pid,SIGKILL);
		}
		int status = -1;
	printf("%s wpa_pid: %d ##\r\n",__func__,g_network_service.wpas_pid);

		do{
			waitpid(g_network_service.wpas_pid,&status,0);
			if(WIFSIGNALED(status)){
				break;
			} else {
				usleep(5000);
				retry -= 1;
			}
		}while(retry > 0);

	}
}

static void udhcpc_stop(void)
{
	int retry = 1000;
	if(g_network_service.udhcpc_pid){
		if(kill(g_network_service.udhcpc_pid,SIGKILL) == -1){
			kill(g_network_service.udhcpc_pid,SIGKILL);
		}
		int status = -1;

		printf("%s udhchc_pid: %d ##\r\n",__func__,g_network_service.udhcpc_pid);
		do{
			waitpid(g_network_service.udhcpc_pid,&status,0);
			if(WIFSIGNALED(status)){
				break;
			} else {
				usleep(5000);
				retry -= 1;
			}
		}while(retry > 0);

	}
}


static void client_death_cb(tIpcThreadInfo *info, void* priv)
{
	client_death_priv_t* clipriv = (client_death_priv_t*)priv;

	network_client_ctx_t* ctx = NULL;
	network_client_ctx_t* tmp = NULL;
	list_for_each_entry(tmp,&g_network_service.client_manager_list,list){
     if(tmp->client_pid == clipriv->client_pid){
         ctx = tmp;
         break;
     }
	}
	if(!ctx){
 	    free(priv);
 	    return;
 	}
	pthread_mutex_lock(&g_network_service.mg_lock);
	if(ctx->client_level == 0){
		wlan_disable();
		g_network_service.level0_cnt -= 1;
	} else if(ctx->client_level == 1){
		g_network_service.level1_cnt -= 1;
	} else {
		g_network_service.level2_cnt -= 1;
	}
	pthread_mutex_unlock(&g_network_service.mg_lock);

	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_release(ti,ctx->client_ipc_handle);
	free(ctx->client_death);
	free(priv);
	free(ctx);
	printf("%s %d ###\r\n",__func__,__LINE__);
}

static int32_t wpa_results_line(struct wpa_results* results,char* str)
{
    const char *delim = "\t";
    char temp[256];
	memset(temp,0,sizeof(temp));
	char* saveptr;
	int len = strlen(str);
    memcpy(temp, str,len);
	temp[len] = '\0';
	/* printf("temp: %s \n",temp); */
	/* printf("print hex start ###\r\n"); */

	/* for(int i = 0; i < 48; i++){ */
	/* 	printf("temp[%d] = 0x%x \n",i,temp[i]); */
	/* } */
	/* printf("print hex stop ###\r\n"); */
    char *token = strtok_r(temp, delim,&saveptr);
    int index = 0;
    char* dstptr = NULL;
    while (token != NULL) {
		if(token[0] != ' '){
            switch(index){
                case 0:
                    dstptr = &results->bssid[0];break;
                case 1:
                    dstptr = &results->freq[0];break;
                case 2:
                    dstptr = &results->riss[0];break;
                case 3:
                    dstptr = &results->flags[0];break;
                case 4:
                    dstptr = &results->ssid[0];break;
                default:
                    dstptr = NULL;
            }
            if(dstptr && strlen(token))
                strncpy(dstptr,token,strlen(token));
            index += 1;
		}
        token = strtok_r(NULL, delim,&saveptr);
    }
    return 0;
}


static network_client_ctx_t* find_target_client(iss_network_msg_t* msg)
{
	network_client_ctx_t* ctx = NULL;
	list_for_each_entry(ctx,&g_network_service.client_manager_list,list){
		if(ctx->client_pid == msg->client_pid){
			return ctx;
		}
	}
	LOGE("can not find client_ctx");
	return NULL;
}

static int32_t dispatch_event_to_all_client(NetWorkEvent_t evt)
{
#if 1
	tBinderIo data;
	tBinderIo reply;
	char binder_buf[384];
	iss_network_msg_t net_msg;
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_io_init(&data,binder_buf,384,DEFAULT_OFFSET_LIST_SIZE);
	net_msg.cmd = WLAN_DISPATCH_EVENT;
	int32_t* event = (int32_t*)&net_msg.msg[0];

	network_client_ctx_t* ctx = NULL;
	list_for_each_entry(ctx,&g_network_service.client_manager_list,list){
		if(true == ctx->listen_wlan_evt){
			event[0] = evt;
			net_msg.client_pid = ctx->client_pid;
			binder_io_append_data(&data, (char *)&net_msg, sizeof(iss_network_msg_t));
			int32_t ret = binder_cmd_async_call(ti, &data,NULL,ctx->client_ipc_handle, 0);
			if(ret){
				return -1;
			}
		}
	}
#endif
	//printf("%s : %d ##\r\n",__func__,evt);
	return 0;
}


static bool __is_need_save(char* ssid,int32_t freq,WifiScanResult_t* result,int32_t count)
{
	int freq_2400M_cnt = 0;
	int freq_5000M_cnt = 0;
	if(count == 0)
		return true;
	for(int i = 0; i < count; i++){
		if(!strncmp(result[i].ssid,ssid,strlen(ssid))){
			if((result[i].freq / 1000) == 2){
				freq_2400M_cnt += 1;
			} else if((result[i].freq / 1000) == 5){
				freq_5000M_cnt += 1;
			}
		}
	}
	// 同名网络2.4G和5G，分别只存储一个
	int tmp = freq / 1000;
	if((tmp == 2 && freq_2400M_cnt == 0) || (tmp == 5 && freq_5000M_cnt == 0))
		return true;
	else
		return false;
}

static int32_t process_list_networks_line(network_info_t* info,char* str)
{
    const char *delim = "\t";
    char temp[128];
	memset(temp,0,sizeof(temp));
	char* saveptr;
	int len = strlen(str);
    memcpy(temp, str,len);
	temp[len] = '\0';
    char *token = strtok_r(temp, delim,&saveptr);
    int index = 0;
    char* dstptr = NULL;
    while (token != NULL) {
		if(token[0] != ' '){
            switch(index){
                case 0:
                    info->nid = atoi(token);break;
                case 1:
					strncpy(info->ssid,token,strlen(token));
                default:
                    dstptr = NULL;
            }
            index += 1;
		}
        token = strtok_r(NULL, delim,&saveptr);
    }
    return 0;
}

static int32_t wpa_load_configered_network(void)
{
	char repbuf[WPA_MAX_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];


	memset(g_network_service.configured_net,0,sizeof(network_info_t) * MAX_CONFIGUERD_NUM);

	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);

	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"LIST_NETWORKS");
	int32_t ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	int32_t count = 0;
	char *delim = "\n";
	char* saveptr = NULL;
	network_info_t* configured_info = g_network_service.configured_net;
	char* token = strtok_r(repbuf,delim,&saveptr);
	token = strtok_r(NULL,delim,&saveptr);
	while(token != NULL){
		process_list_networks_line(&configured_info[count],token);
		count++;
		token = strtok_r(NULL,delim,&saveptr);
	}
	if(count > 0){
		for(int i = 0; i < count; i++){
			printf("list configured >>> ssid:%s nid:%d ##\r\n",configured_info[i].ssid,configured_info[i].nid);
		}
	}


	return count;
}

static int32_t wlan_update_scan_results(void)
{
	char repbuf[WPA_MAX_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);

	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"SCAN_RESULTS");
	int32_t ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	int32_t count = 0;
	char *delim = "\n";
	char* saveptr = NULL;
	struct wpa_results wpa_result;
	uint32_t freq = 0;
	WifiScanResult_t* result = g_network_service.wlan_results_save;
	pthread_mutex_lock(&g_network_service.mg_lock);
	char* token = strtok_r(repbuf,delim,&saveptr);
	token = strtok_r(NULL,delim,&saveptr);
	/* printf("##### token process ###### \r\n"); */
	while(token != NULL){
		memset(&wpa_result,0,sizeof(struct wpa_results));
		wpa_results_line(&wpa_result,token);
		if(strlen(wpa_result.ssid)) {
			freq = atoi(wpa_result.freq);
			if(__is_need_save(wpa_result.ssid,freq,result,count)){
				strcpy(result[count].ssid,wpa_result.ssid);
				result[count].signal_level = atoi(wpa_result.riss);
				result[count].freq = atoi(wpa_result.freq);
				count += 1;
				if(count >= g_network_service.max_results_num){
					break;
				}
			}
		}
        token = strtok_r(NULL, delim,&saveptr);
	}
	g_network_service.current_results_num = count;

	pthread_mutex_unlock(&g_network_service.mg_lock);
	/* if(g_network_service.current_results_num > 0){ */
	/* 	for(int i = 0; i < g_network_service.current_results_num; i++){ */
	/* 		printf("ssid: %s freq:%d  siglevel: %d\r\n",result[i].ssid,result[i].freq,result[i].signal_level); */
	/* 	} */
	/* } */
	return 0;
}

void* auto_connect_configured_net(void* arg)
{
	WifiScanResult_t* result = NULL;
	for(;;){
		pthread_mutex_lock(&g_network_service.mg_lock);
		if(g_network_service.current_results_num > 0){
			result = g_network_service.wlan_results_save;
			for(int i = 0; i < g_network_service.current_results_num; i++){
				if(abs(result[i].signal_level) <= 62){	// 信号强度限制 (调试使用)
					for(int j = 0; j < g_network_service.configured_cnt; j++){
						if(!strcmp(result[i].ssid,g_network_service.configured_net[j].ssid)){
							printf("%s : connect ssid: %s nid:%d ##\r\n",__func__,result[i].ssid,g_network_service.configured_net[j].nid);
							wlan_connect_network(g_network_service.configured_net[j].nid,g_network_service.wlan_ifname);
						}
					}
				}
				if(g_network_service.connect_sta)
					break;
			}
			pthread_mutex_unlock(&g_network_service.mg_lock);
			break;
		} else {
			pthread_mutex_unlock(&g_network_service.mg_lock);
			usleep(5*1000);
		}
	}
}

void* wpa_evt_mon_thread(void* arg)
{
	char buf[WPA_MAX_REPBUFFER_SIZE];
	int len = WPA_MAX_REPBUFFER_SIZE - 1;
	for(;;){
		if (wpa_ctrl_pending(g_network_service.wpa_mon_ctrl) > 0) {
			len = WPA_MAX_REPBUFFER_SIZE - 1;
			memset(buf,0,len);
			if(wpa_ctrl_recv(g_network_service.wpa_mon_ctrl, buf, &len) == 0){
				buf[len]= '\0';
				if(strstr(buf,"CTRL-EVENT-CONNECTED")){
					pthread_mutex_lock(&g_network_service.slock);
					dispatch_event_to_all_client(WPA_EVT_CONNECTED);
					g_network_service.connect_sta = true;
					pthread_mutex_unlock(&g_network_service.slock);
				} else if(strstr(buf,"CTRL-EVENT-DISCONNECTED")){
					pthread_mutex_lock(&g_network_service.slock);
					dispatch_event_to_all_client(WPA_EVT_DISCONNECTED);
					pthread_mutex_unlock(&g_network_service.slock);
				} else if(strstr(buf,"CTRL-EVENT-STATE-CHANGE")){
					pthread_mutex_lock(&g_network_service.slock);
					dispatch_event_to_all_client(WPA_EVT_STATE_CHANGE);
					pthread_mutex_unlock(&g_network_service.slock);
				} else if(strstr(buf,"CTRL-EVENT-PASSWORD-CHANGED")){
					//dispatch_event_to_all_client(WPA_EVT_PASSWORD_CHANGED);
				} else if(strstr(buf,"CTRL-EVENT-SCAN-RESULTS")){
					// TODO
					wlan_update_scan_results();
				} else if(strstr(buf,"CTRL-EVENT-NETWORK-NOT-FOUND")){
					// TODO
					pthread_mutex_lock(&g_network_service.slock);
					dispatch_event_to_all_client(WPA_EVT_NETWORK_NOT_FOUND);
					pthread_mutex_unlock(&g_network_service.slock);
				} else if(strstr(buf,"CTRL-EVENT-SIGNAL-CHANGE")){
					// service TODO
					pthread_mutex_lock(&g_network_service.slock);
					dispatch_event_to_all_client(WPA_EVT_SIGNAL_CHANGE);
					pthread_mutex_unlock(&g_network_service.slock);
				} else if(strstr(buf,"CTRL-EVENT-TERMINATING")){
					//dispatch_event_to_all_client(WPA_EVT_TERMINATING);
				}
			} else {
				usleep(200*1000);
			}
		} else {
			// TODO reconnect wpa_supplicant
		}
	}
}

void* wlan_internet_monitor_thread(void* arg)
{
	for(;;){
		if(g_network_service.connect_sta == true){
			if(tcp_connect_test("36.152.44.95",80)){
				// net error
				pthread_mutex_lock(&g_network_service.slock);
				dispatch_event_to_all_client(WLAN_EVT_INTERNET_ERROR);
				pthread_mutex_unlock(&g_network_service.slock);
			} else {
				usleep(500*1000);
			}
			usleep(1500*1000);
		} else {
			usleep(100*1000);
		}
	}
}

static int32_t wlan_enable(char* ifname)
{
	int retry = 10;
	int ret = -1;

	// wpa_supplicant damon start
	if(g_network_service.wpa_mon_ctrl != NULL && g_network_service.wpa_cmd_ctrl != NULL){
		return -1;
	}
	wpa_supplicant_start(ifname);
	udhcpc_start(ifname);
	while(retry--){
		ret = access(CONFIG_CTRL_PATH,F_OK);
		if(ret){
			usleep(30*1000);	//wait 30ms
		}
	}
	if(retry == 0 || ret != 0){
		// error
		return -1;
	}

	g_network_service.wpa_cmd_ctrl = wpa_ctrl_open(CONFIG_CTRL_PATH);
	if(g_network_service.wpa_cmd_ctrl == NULL){
		return -1;
	}

	g_network_service.wpa_mon_ctrl = wpa_ctrl_open(CONFIG_CTRL_PATH);
	if(g_network_service.wpa_mon_ctrl == NULL){
		return -1;
	}
	if(g_network_service.wpa_mon_ctrl){
		if(wpa_ctrl_attach(g_network_service.wpa_mon_ctrl) == 0){
			// create event monitor thread
		} else {
			wpa_ctrl_close(g_network_service.wpa_cmd_ctrl);
			wpa_ctrl_close(g_network_service.wpa_mon_ctrl);
			return -1;
		}
	}
	g_network_service.configured_cnt = wpa_load_configered_network();
	// create event monitor thread TODO
	pthread_create(&g_network_service.wlan_evt_mon_tid,NULL,wpa_evt_mon_thread,NULL);

	// 此线程用于自动连接一个扫描到的已配置网络热点(在打开wifi的时候)
	pthread_t tid;
	pthread_create(&tid,NULL,auto_connect_configured_net,NULL);
	pthread_detach(tid);
	// TODO create ping test thread
	pthread_create(&g_network_service.wlan_ping_tid,NULL,wlan_internet_monitor_thread,NULL);
	return 0;
}

static int32_t wlan_disable(void)
{
	/* wpa_cmd_flush(NULL); */
	wlan_disconnect_network(g_network_service.wlan_ifname);
	if(g_network_service.wlan_evt_mon_tid){
		pthread_cancel(g_network_service.wlan_evt_mon_tid);
		pthread_join(g_network_service.wlan_evt_mon_tid,NULL);
	}
	if(g_network_service.wlan_ping_tid){
		pthread_cancel(g_network_service.wlan_ping_tid);
		pthread_join(g_network_service.wlan_ping_tid,NULL);
	}
	wpa_ctrl_close(g_network_service.wpa_cmd_ctrl);
	wpa_ctrl_detach(g_network_service.wpa_mon_ctrl);
	wpa_ctrl_close(g_network_service.wpa_mon_ctrl);
	g_network_service.wpa_cmd_ctrl = NULL;
	g_network_service.wpa_mon_ctrl = NULL;
	wpa_supplicant_stop();
	udhcpc_stop();
	return 0;
}

static void msg_callback(char *msg, size_t len)
{
	printf("hello cmd\r\n");
}

static int32_t wpa_ctrl_send_cmd(char* cmd,int32_t cmd_len,uint8_t* reply,int32_t* reply_len)
{
	pthread_mutex_lock(&g_network_service.slock);
	int32_t ret = wpa_ctrl_request(g_network_service.wpa_cmd_ctrl, cmd,cmd_len,reply,reply_len,msg_callback);
	if (ret == -2) {
	    printf("'%s' command timed out.\n", cmd);
		pthread_mutex_unlock(&g_network_service.slock);
	    return -2;
	} else if (ret < 0) {
		pthread_mutex_unlock(&g_network_service.slock);
	    printf("'%s' command failed.\n", cmd);
	    return -1;
	}
	reply[*reply_len - 1] = '\0';
	if(strstr(reply,"FAIL") || strstr(reply,"UNKNOWN COMMAND")){
		printf("send cmd failed !@!!\r\n");
		return -1;
	}
	pthread_mutex_unlock(&g_network_service.slock);
	return 0;
}


static int32_t wlan_add_network(int8_t* ssid,int8_t* psk,int8_t* key_mgmt,char* ifname)
{
	char repbuf[WPA_MAX_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	int32_t nid = -1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	// 0-- add network
	memset(repbuf,'\0',WPA_MAX_REPBUFFER_SIZE);
	int ret = wpa_ctrl_send_cmd("ADD_NETWORK",strlen("ADD_NETWORK"),repbuf,&replylen);
	if(ret == 0){
		repbuf[replylen] = '\0';
		nid = atoi(repbuf);
		printf("%s nid: %d ##\r\n",__func__,nid);
	} else {
		return -1;
	}

	// 1 -- set ssid
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"SET_NETWORK %d ssid \"%s\"",nid,ssid);
	memset(repbuf,'\0',WPA_MAX_REPBUFFER_SIZE);
	replylen = sizeof(repbuf) - 1;
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}

	// 2 -- set psk
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"SET_NETWORK %d psk \"%s\"",nid,psk);
	memset(repbuf,'\0',WPA_MAX_REPBUFFER_SIZE);
	replylen = sizeof(repbuf) - 1;
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}

	// 3 -- set scan_ssid=1
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"SET_NETWORK %d scan_ssid 1",nid);
	memset(repbuf,'\0',WPA_MAX_REPBUFFER_SIZE);
	replylen = sizeof(repbuf) - 1;
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}

	return nid;
}

static int32_t wpa_reload_conf(void)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	int ret = wpa_ctrl_send_cmd("RECONFIGURE",strlen("RECONFIGURE"),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	usleep(5*1000);
	return 0;
}

static int32_t wpa_cmd_flush(char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	int ret = wpa_ctrl_send_cmd("FLUSH",strlen("FLUSH"),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wlan_save_config(char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	int ret = wpa_ctrl_send_cmd("SAVE_CONFIG",strlen("SAVE_CONFIG"),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wpa_enable_autoscan(bool enable)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	char cmd[64];
	if(enable)
		snprintf(cmd,32 - 1,"AUTOSCAN 1");
	else
		snprintf(cmd,32 - 1,"AUTOSCAN 0");
	int ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wlan_remove_network(int32_t nid,char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	char cmd[64];
	snprintf(cmd,32 - 1,"REMOVE_NETWORK %d",nid);
	int ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

// select && connect
//
static int32_t wlan_network_select(int32_t nid,char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"SELECT_NETWORK %d",nid);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wlan_network_enable(int32_t nid,char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);

	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"ENABLE_NETWORK %d",nid);
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wlan_network_disable(int32_t nid,char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);

	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"DISABLE_NETWORK %d",nid);
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}


static int32_t wlan_connect_network(int16_t nid,char* ifname)
{
	int32_t ret = 0;
	ret = wlan_network_enable(nid,ifname);
	ret = wlan_network_select(nid,ifname);
	if(!ret){
		pthread_mutex_lock(&g_network_service.slock);
		g_network_service.current_connect_nid = nid;
		pthread_mutex_unlock(&g_network_service.slock);
		return 0;
	}

	return -1;
}

static int32_t wlan_disconnect_network(char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);

	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"DISCONNECT");
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	pthread_mutex_lock(&g_network_service.slock);
	g_network_service.connect_sta = false;
	pthread_mutex_unlock(&g_network_service.slock);
	return 0;
}

static int32_t wlan_reconnect_network(char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"RECONNECT");
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wlan_start_scan(char* ifname)
{
	char repbuf[WPA_MIN_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);

	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"SCAN");
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		return -1;
	}
	return 0;
}

static int32_t wlan_get_status(char* ifname,wlan_connect_sta_t* sta)
{
	char repbuf[WPA_MAX_REPBUFFER_SIZE];
	int32_t replylen = sizeof(repbuf) - 1;
	char cmd[WPA_CMD_BUFFER_SIZE];
	int32_t ret = 0;
	memset(cmd,0,WPA_CMD_BUFFER_SIZE);
	memset(repbuf,'\0',WPA_MIN_REPBUFFER_SIZE);
	snprintf(cmd,WPA_CMD_BUFFER_SIZE - 1,"STATUS");
	ret = wpa_ctrl_send_cmd(cmd,strlen(cmd),repbuf,&replylen);
	if(ret != 0){
		printf("%s %d ##\r\n",__func__,__LINE__);
		return -1;
	}
	//printf("%s: \n %s ##\r\n",__func__,repbuf);
	char *delim = "\n";
	char* saveptr = NULL;
	char tmp[64];
	char* ssid = NULL;
	char* freq = NULL;
	char* ipstring = NULL;

	char* token = strtok_r(repbuf,delim,&saveptr);
	while(token != NULL){
		memset(tmp,0,sizeof(tmp));
		memcpy(tmp,token,strlen(token));
		if(strstr(tmp,"ssid=") && !strstr(tmp,"bssid=")){
			ssid = (char*)&tmp[5];
			int len = strlen(tmp) - 5;
			memcpy(sta->connected_ssid,ssid,len);
		} else if(strstr(tmp,"freq=")){
			freq = (char*)&tmp[5];
			int len = strlen(tmp) - 5;
			sta->freq = atoi(freq);
		} else if(strstr(tmp,"ip_address=")) {
			ipstring = (char*)&tmp[11];
			int len = strlen(tmp) - 11;
			memcpy(sta->ipstring,ipstring,len);
		}
		token = strtok_r(NULL,delim,&saveptr);
	}
	if((strlen(sta->connected_ssid) == 0) || sta->freq == 0 || (strlen(sta->ipstring) == 0)){
		/* printf("%s: wlan device  not connect ##\r\n",__func__); */
		memset(sta,0,sizeof(wlan_connect_sta_t));
		sta->connected_sta = false;
		return -1;
	} else {
		/* printf("%s: wlan device  connect to %s ##\r\n",__func__,sta->connected_ssid); */
		sta->connected_sta = true;
	}
	return 0;
}


static int32_t iss_network_client_register(iss_network_msg_t* netmsg,tBinderIo* msg,tBinderIo* reply)
{
	network_register_info_t* register_msg = (network_register_info_t*)&netmsg->msg[0];
	if(register_msg->client_level == 0 && g_network_service.level0_cnt == 1){
		binder_io_append_uint32(reply,ISS_FAILED);
		return -1;
	}
	network_client_ctx_t* ctx = (network_client_ctx_t*)malloc(sizeof(network_client_ctx_t));
	if(!ctx){
		binder_io_append_uint32(reply,ISS_FAILED);
		return -1;
	}
	network_register_info_t *register_info = (network_register_info_t*)register_msg;
	memset(ctx,0,sizeof(network_client_ctx_t));
	ctx->client_pid = register_msg->client_pid;
	ctx->client_level = register_msg->client_level;
	if(register_msg->client_level == 0 && (g_network_service.level0_cnt == 0)){
		g_network_service.level0_cnt += 1;
	} else if(register_msg->client_level == 1 && (g_network_service.level1_cnt < 2)){
		g_network_service.level1_cnt += 1;
	} else if(register_msg->client_level == 2){
		g_network_service.level2_cnt += 1;
	} else {
		binder_io_append_uint32(reply,ISS_FAILED);
		free(ctx);
		return -1;
	}
	uint32_t hdl = binder_io_get_ref(msg, 0);
	tIpcThreadInfo* ti = binder_get_thread_info();
	binder_cmd_acquire(ti, hdl);
	flush_commands(ti);
	ctx->client_ipc_handle = hdl;

	ctx->listen_wlan_evt = register_msg->need_listen_wlan_sta;
	ctx->listen_eth_evt = register_msg->need_listen_eth_sta;

	ctx->client_death = (struct binder_death*)malloc(sizeof(struct binder_death));
	if(!ctx->client_death){
		binder_io_append_uint32(reply,ISS_FAILED);
		binder_cmd_release(ti,ctx->client_ipc_handle);
		free(ctx->client_death);
		free(ctx);
		return -1;
	}

	//TODO death link
	client_death_priv_t* death_priv = (client_death_priv_t*)malloc(sizeof(client_death_priv_t));
	if(!death_priv){
		binder_io_append_uint32(reply,ISS_FAILED);
		binder_cmd_release(ti,ctx->client_ipc_handle);
		free(ctx);
		return -1;
	}
	death_priv->client_pid = ctx->client_pid;
	death_priv->client_ipc_handle = ctx->client_ipc_handle;
	ctx->client_death->death_cb = client_death_cb;
	ctx->client_death->ptr = death_priv;
	binder_cmd_link_to_death(ti,ctx->client_ipc_handle,ctx->client_death);

	list_add_head(&ctx->list,&g_network_service.client_manager_list);
	binder_io_append_uint32(reply,ISS_SUCCESS);
	return 0;
}

static int32_t iss_network_client_unregister(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(ctx != NULL){

		tIpcThreadInfo* ti = binder_get_thread_info();
		binder_cmd_release(ti,ctx->client_ipc_handle);
		pthread_mutex_lock(&g_network_service.mg_lock);
		list_del(&ctx->list);
		if(ctx->client_level == 0)
			g_network_service.level0_cnt -= 1;
		else if(ctx->client_level == 1)
			g_network_service.level1_cnt -= 1;
		else
			g_network_service.level2_cnt -= 1;
		pthread_mutex_unlock(&g_network_service.mg_lock);
		// free(ctx->client_death);
		free(ctx->client_death);
		free(ctx);
		binder_io_append_uint32(reply,ISS_SUCCESS);
	} else {
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}
	return 0;
}

static int32_t iss_add_wlan_config(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	iss_wlan_config_info_t* config = &net_msg->msg[0];
	iss_remote_reply_t rep;
	rep.cmd = WLAN_ADD_CONFIG;
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		rep.exec_result = ISS_CLIENT_NOT_EXIST;
		rep.retval = -1;
		binder_io_append_data(reply,&rep,sizeof(iss_remote_reply_t));
		return -1;
	}
	if(strlen(config->ssid) <= 0 || strlen(config->passwd) <= 0){
		printf("ssid error or passwd len error ##\r\n");
		rep.exec_result = ISS_FAILED;
		rep.retval = -1;
		binder_io_append_data(reply,&rep,sizeof(iss_remote_reply_t));
		return -1;
	}
	int nid = wlan_add_network(config->ssid,config->passwd,NULL,g_network_service.wlan_ifname);
	if(nid < 0){
		printf("wlan_add_network failed ##\r\n");
		rep.exec_result = ISS_FAILED;
		rep.retval = -1;
		binder_io_append_data(reply,&rep,sizeof(iss_remote_reply_t));
		return -1;
	}
	rep.exec_result = ISS_SUCCESS;
	rep.retval = nid;
	binder_io_append_data(reply,&rep,sizeof(iss_remote_reply_t));
	return 0;
}

static int32_t iss_wlan_enable(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply,bool enable)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}
	if(ctx->client_level == 0){
		if(true == enable){
			wlan_enable(g_network_service.wlan_ifname);
		} else {
			wlan_disable();
		}
		binder_io_append_uint32(reply,ISS_SUCCESS);
		return 0;
	}

	binder_io_append_uint32(reply,ISS_FAILED);
	return -1;
}


static int32_t iss_wlan_netowrk_enable(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply,bool enable)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}

	int32_t* nid = (int32_t*)&net_msg->msg[0];
	if(ctx->client_level == 0 || ctx->client_level == 1){
		if(true == enable)
			wlan_network_enable(*nid,g_network_service.wlan_ifname);
		else
			wlan_network_disable(*nid,g_network_service.wlan_ifname);
		binder_io_append_uint32(reply,ISS_SUCCESS);
		return 0;
	}

	binder_io_append_uint32(reply,ISS_FAILED);
	return -1;
}

static int32_t iss_wlan_netowrk_scan(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}

	g_network_service.current_results_num = 0;
	if(ctx->client_level == 0 || ctx->client_level == 1){
		wlan_start_scan(g_network_service.wlan_ifname);
		binder_io_append_uint32(reply,ISS_SUCCESS);
		return 0;
	}

	binder_io_append_uint32(reply,ISS_FAILED);
	return -1;
}

static int32_t iss_wlan_netowrk_connect(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply,bool connect)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}

	int32_t* nid = (int32_t*)&net_msg->msg[0];

	if(ctx->client_level == 0 || ctx->client_level == 1){
		if(true == connect)
			wlan_connect_network(*nid,g_network_service.wlan_ifname);
		else
			wlan_disconnect_network(g_network_service.wlan_ifname);
		binder_io_append_uint32(reply,ISS_SUCCESS);
		return 0;
	}

	binder_io_append_uint32(reply,ISS_FAILED);
	return -1;
}

static int32_t iss_wlan_remove_netowrk(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}

	int32_t* nid = (int32_t*)&net_msg->msg[0];

	if(ctx->client_level == 0 || ctx->client_level == 1){
		wlan_remove_network(*nid,g_network_service.wlan_ifname);
		wlan_save_config(g_network_service.wlan_ifname);
		binder_io_append_uint32(reply,ISS_SUCCESS);
		return 0;
	}

	binder_io_append_uint32(reply,ISS_FAILED);
	return -1;
}

static int32_t iss_wlan_save_config(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	network_client_ctx_t* ctx = find_target_client(net_msg);
	if(!ctx){
		binder_io_append_uint32(reply,ISS_CLIENT_NOT_EXIST);
		return -1;
	}


	if(ctx->client_level == 0 || ctx->client_level == 1){
		wlan_save_config(g_network_service.wlan_ifname);
		binder_io_append_uint32(reply,ISS_SUCCESS);
		return 0;
	}

	binder_io_append_uint32(reply,ISS_FAILED);
	return -1;
}

static int32_t iss_wlan_get_scan_results(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	int size = 0;
	int results_num = REP_RESULTS_NUM;
	int32_t ret = 0;
	pthread_mutex_lock(&g_network_service.mg_lock);
	if(g_network_service.current_results_num > 0){
		if(results_num < g_network_service.current_results_num){
			size = results_num * sizeof(WifiScanResult_t);
		} else {
			results_num = g_network_service.current_results_num;
			size = g_network_service.current_results_num * sizeof(WifiScanResult_t);
		}
		uint32_t* tmp = (uint32_t*)malloc(size + sizeof(uint32_t));
		tmp[0] = results_num;
		memcpy(&tmp[1],g_network_service.wlan_results_save,size);
		binder_io_append_data(reply,tmp,size + sizeof(uint32_t));
	} else {
		ret = -1;
		size = sizeof(int);
		binder_io_append_data(reply,&ret,size);
		pthread_mutex_unlock(&g_network_service.mg_lock);
		return -1;
	}

	// TODO 将信号强度前10或15的热点发送到客户端
	pthread_mutex_unlock(&g_network_service.mg_lock);
	return 0;
}


static int32_t iss_wlan_get_connect_sta(iss_network_msg_t* net_msg,tBinderIo* msg,tBinderIo* reply)
{
	wlan_connect_sta_t connect_sta;
	memset(&connect_sta,0,sizeof(wlan_connect_sta_t));
	wlan_get_status(g_network_service.wlan_ifname,&connect_sta);
	binder_io_append_data(reply,&connect_sta,sizeof(wlan_connect_sta_t));
	return 0;
}


static int client_msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int32_t msg_size = 0;
	sys_msg_t* sys_msg;
	iss_network_msg_t* net_msg;
	int32_t* nid = NULL;
	int32_t ret = binder_io_get_data(msg,&data, &msg_size);
	sys_msg = (sys_msg_t*)data;
	net_msg = (iss_network_msg_t*)sys_msg->msg;
	switch(net_msg->cmd){
		case WLAN_CLI_REGISTER:
			iss_network_client_register(net_msg,msg,reply);
			break;
		case WLAN_CLI_UNREGISTER:
			iss_network_client_unregister(net_msg,msg,reply);
			break;
		case WLAN_ENABLE:
			iss_wlan_enable(net_msg,msg,reply,true);
			break;
		case WLAN_DISABLE:
			iss_wlan_enable(net_msg,msg,reply,false);
			break;
		case WLAN_NETWORK_ENABLE:
			iss_wlan_netowrk_enable(net_msg,msg,reply,true);
			break;
		case WLAN_NETWORK_DISABLE:
			iss_wlan_netowrk_enable(net_msg,msg,reply,false);
			break;
		case WLAN_START_SCAN:
			iss_wlan_netowrk_scan(net_msg,msg,reply);
			break;
		case WLAN_GET_SCAN_RESULTS:
			iss_wlan_get_scan_results(net_msg,msg,reply);
			break;
		case WLAN_ADD_CONFIG:
			iss_add_wlan_config(net_msg,msg,reply);
			break;
		case WLAN_DEL_CONFIG:
			iss_wlan_remove_netowrk(net_msg,msg,reply);
			break;
		case WLAN_CONNECT_NETWORK:
			iss_wlan_netowrk_connect(net_msg,msg,reply,true);
			break;
		case WLAN_DISCONNECT_NETWORK:
			iss_wlan_netowrk_connect(net_msg,msg,reply,false);
			break;
		case WLAN_GET_CONNECT_STA:
			iss_wlan_get_connect_sta(net_msg,msg,reply);
			break;
		case WLAN_SAVE_CONFIG:
			iss_wlan_save_config(net_msg,msg,reply);
			break;
		default:
			printf("network cmd error !!!!!");
			return -1;
	}
	return 0;
}

static tBinderService network_service = {
    .transact_cb = client_msg_arrive_transact,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

int main(int argc,char** argv)
{

	if(argc < 2){
		printf("param error : networkService wlan0/wlan1 \r\n");
		return -1;
	}
	char* wlan_name = (char*)argv[1];
	if(strlen(wlan_name) < 4){
		printf("wlan ifname error !!\r\n");
		return -1;
	}
	memset(&g_network_service,0,sizeof(g_network_service));
	memcpy(&g_network_service.wlan_ifname,wlan_name,strlen(wlan_name));
	INIT_LIST_HEAD(&g_network_service.client_manager_list);
	pthread_mutex_init(&g_network_service.slock,NULL);
	pthread_mutex_init(&g_network_service.mg_lock,NULL);
	g_network_service.max_results_num = 40;
	g_network_service.current_results_num = 0;
	g_network_service.current_connect_nid = -1;
	g_network_service.wlan_results_save = (WifiScanResult_t*)malloc(g_network_service.max_results_num * sizeof(WifiScanResult_t));
	if(!g_network_service.wlan_results_save){
		printf("alloc results save buffer failed !!\r\n");
		return -1;
	}
	g_network_service.configured_net = (network_info_t*)malloc(MAX_CONFIGUERD_NUM * sizeof(network_info_t));

	int ret = binder_add_service(NETWORK_SERVICE_NAME,&network_service);
	binder_thread_enter_loop(0,0);

	while(1){
		sleep(1);
	}
	return 0;
}


/******* TODO ETH ********/

#ifdef ETH_SUPPORT
static int32_t eth_get_link_state(char* ifname)
{
	int sockfd;
	struct ifrq ifr;

	sockfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sockfd < 0){
		perror("soket: ")
		LOGE("%s init socket error",__func__);
		return -1;
	}
	strncpy(ifr.ifr_name,ifname,IFNAMESIZ - 1);

	// get interface flags
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
        perror("ioctl:");
		LOGE("%s: get interface flags failed !!");
        close(sockfd);
        return -1;
	}

	if (ifr.ifr_flags & IFF_UP)
		return 1;
	else
		return 0;
}

static int32_t eth_get_trans_speed(char* ifname)
{
}

static int32_t eth_get_mac_addr(char* ifname,char* mac)
{
	struct nl_sock *sk;
    struct nl_cache *cache;
    struct rtnl_link *link;

    sk = nl_socket_alloc();
    if (!sk) {
        perror("nl_socket_alloc");
		return -1;
    }

    if (nl_connect(sk, NETLINK_ROUTE) < 0) {
        perror("nl_connect");
        nl_socket_free(sk);
		return -1;
    }

    if (rtnl_link_alloc_cache(sk, AF_UNSPEC, &cache) < 0) {
        perror("rtnl_link_alloc_cache");
        nl_close(sk);
        nl_socket_free(sk);
		return -1;
    }

    link = rtnl_link_get_by_name(cache, ifname);
    if (!link) {
        nl_cache_free(cache);
        nl_close(sk);
        nl_socket_free(sk);
		return -1;
    }

    ret = rtnl_link_get_addr(link, mac);
    nl_cache_free(cache);
    nl_close(sk);
    nl_socket_free(sk);

	if(ret)
		return -1;
	return 0;
}

static int32_t eth_get_ip_addr(char* ifname,char* ip_address,bool ipv6)
{
    int sockfd;
    struct ifreq ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
    }

    strncpy(ifr.ifr_name, ifname, IFNAMSIZ-1);

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl");
        close(sockfd);
    }
    struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
    inet_ntop(AF_INET, &(ipaddr->sin_addr), ip_address, INET_ADDRSTRLEN);
    //printf("Interface %s IPv4 address: %s\n", ifname, addr);

    close(sockfd);
}
#endif
