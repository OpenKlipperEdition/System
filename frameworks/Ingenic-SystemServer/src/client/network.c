#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
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
#include "fifo.h"
#include "list.h"
#include "systemserver_config.h"
#include "network_service.h"
#include "network.h"

typedef struct {
	int32_t ipc_handle;
	pid_t client_pid;
	pthread_mutex_t lock;
	pthread_cond_t connect_cond;
	NetState_Cb_t *err_cb;
}net_client_info_t;


static net_client_info_t* g_client_ptr;

/* static int network_event_process(NetWorkEvent_t evt) */
/* { */
/* 	switch(evt){ */
/* 		case WPA_E */
/* 	} */
/* } */

static int received_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	uint8_t* data;
	int msg_size = 0;
	sys_msg_t* sys_msg;
	int32_t* rawmsg = NULL;
	int32_t ret  =  binder_io_get_data(msg,&data,&msg_size);
	if(!ret){
		iss_network_msg_t *net_msg = (iss_network_msg_t*)data;
		switch(net_msg->cmd){
			case WLAN_DISPATCH_EVENT:
				rawmsg = (int32_t*)&net_msg->msg[0];
				/* printf("client >>> get from master event: %d ##\r\n",rawmsg[0]); */
				if(rawmsg[0] ==  WPA_EVT_CONNECTED){
					pthread_mutex_lock(&g_client_ptr->lock);
					pthread_cond_signal(&g_client_ptr->connect_cond);
					pthread_mutex_unlock(&g_client_ptr->lock);
				} else if(rawmsg[0] ==  WLAN_EVT_INTERNET_ERROR || rawmsg[0] == WPA_EVT_DISCONNECTED){
					if(g_client_ptr->err_cb){
						g_client_ptr->err_cb->net_sta_callback(g_client_ptr->err_cb->priv);
					}
				}
				/* printf("%s %d evt:%d ###\r\n",__func__,__LINE__,rawmsg[0]); */
				break;
			default:
				break;
		}
	}
	return 0;
}

static tBinderService func_cb = {
    .transact_cb = received_cb,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};


static int32_t network_client_register(int32_t applevel)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_CLI_REGISTER;
	net_msg->client_pid = g_client_ptr->client_pid;

	network_register_info_t* reg_info = &net_msg->msg[0];
	reg_info->client_pid = g_client_ptr->client_pid;
	reg_info->client_level = applevel;
	reg_info->need_listen_wlan_sta = true;
	reg_info->need_listen_eth_sta = false;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	binder_io_append_obj(&data, &func_cb);

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t rep = binder_io_get_uint32(&reply);
		if(rep == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			binder_thread_enter_loop(0,0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t network_client_send_cmd(Network_Cmd_t cmd)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();


	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd = cmd;
	net_msg->client_pid = g_client_ptr->client_pid;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t rep = binder_io_get_uint32(&reply);
		if(rep == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t client_wlan_enable(void)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();


	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_ENABLE;
	net_msg->client_pid = g_client_ptr->client_pid;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t rep = binder_io_get_uint32(&reply);
		if(rep == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t wlan_get_scan_results(WifiScanResult_t** results)
{
	sys_msg_t sys_msg;
    char binder_buf[1024] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd = WLAN_GET_SCAN_RESULTS;
	net_msg->client_pid = g_client_ptr->client_pid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t* data;
		int32_t sz = 0;
		uint32_t rep = binder_io_get_data(&reply,(uint8_t**)&data,&sz);
		if(rep == 0){
			int32_t results_num = data[0];
			if(results_num > 0){
				*results = malloc(results_num * sizeof(WifiScanResult_t));
				if(*results == NULL){
					binder_cmd_freebuf(ti, reply.data0);
					return -1;
				}
				memcpy(*results,&data[1],results_num * sizeof(WifiScanResult_t));
			}
			binder_cmd_freebuf(ti, reply.data0);
			return results_num;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);

	return -1;
}

static int32_t client_wlan_add_network(char* ssid,char* passwd)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();


	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_ADD_CONFIG;
	net_msg->client_pid = g_client_ptr->client_pid;

	iss_wlan_config_info_t* config_info = &net_msg->msg[0];
	strncpy(config_info->ssid,ssid,strlen(ssid));
	strncpy(config_info->passwd,passwd,strlen(passwd));

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		int sz = 0;
		char* data;
		binder_io_get_data(&reply,&data,&sz);
		iss_remote_reply_t* rep = (iss_remote_reply_t*)data;
		uint32_t nid = binder_io_get_uint32(&reply);
		if(rep->exec_result == ISS_SUCCESS && rep->cmd == WLAN_ADD_CONFIG){
			binder_cmd_freebuf(ti, reply.data0);
			/* printf("%s: nid = %d ##\r\n",__func__,rep->retval); */
			return rep->retval;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t client_wlan_enable_network(int32_t nid,bool enable)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);

	if(true == enable)
		net_msg->cmd = WLAN_NETWORK_ENABLE;
	else
		net_msg->cmd = WLAN_NETWORK_DISABLE;

	net_msg->client_pid = g_client_ptr->client_pid;
	int32_t* dstnid = (int32_t*)&net_msg->msg[0];
	*dstnid = nid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t rep = binder_io_get_uint32(&reply);
		if(rep == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t client_wlan_remove_network(int32_t nid)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_DEL_CONFIG;
	net_msg->client_pid = g_client_ptr->client_pid;
	int32_t* dstnid = (int32_t*)&net_msg->msg[0];
	*dstnid = nid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t rep = binder_io_get_uint32(&reply);
		if(rep == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t client_wlan_connect_network(int32_t nid,bool connect)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	if(connect)
		net_msg->cmd =  WLAN_CONNECT_NETWORK;
	else
		net_msg->cmd =  WLAN_DISCONNECT_NETWORK;
	net_msg->client_pid = g_client_ptr->client_pid;
	int32_t* dstnid = (int32_t*)&net_msg->msg[0];
	*dstnid = nid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t rep = binder_io_get_uint32(&reply);
		if(rep == ISS_SUCCESS){
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

static int32_t client_wlan_get_link_sta(wlan_connect_sta_t* sta)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_GET_CONNECT_STA;
	net_msg->client_pid = g_client_ptr->client_pid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_ptr->ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t* data;
		int32_t sz = 0;
		uint32_t rep = binder_io_get_data(&reply,(uint8_t**)&data,&sz);
		if(rep == 0){
			memcpy(sta,data,sz);
			binder_cmd_freebuf(ti, reply.data0);
			return 0;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);
	return -1;
}

int32_t NetWorkManager_Init(NET_APP_LEVEL_t level)
{
	if(g_client_ptr){
		printf("network manager has been inited ##\r\n");
		return -1;
	}
	g_client_ptr = (net_client_info_t*)malloc(sizeof(net_client_info_t));
	if(!g_client_ptr){
		return -1;
	}
	g_client_ptr->ipc_handle = binder_get_service(NETWORK_SERVICE_NAME);
	pthread_mutex_init(&g_client_ptr->lock,NULL);
	pthread_cond_init(&g_client_ptr->connect_cond,NULL);
	g_client_ptr->client_pid = getpid();
	if(g_client_ptr->client_pid <= 0){
		return -1;
	}
	int32_t ret = network_client_register(level);
	if(ret){
		free(g_client_ptr);
		g_client_ptr = NULL;
		return -1;
	}
	return 0;
}

int32_t NetWorkManager_DeInit(void)
{
	if(!g_client_ptr)
		return -1;
	network_client_send_cmd(WLAN_CLI_UNREGISTER);
	pthread_mutex_destroy(&g_client_ptr->lock);
	pthread_cond_destroy(&g_client_ptr->connect_cond);
	free(g_client_ptr->err_cb);
	free(g_client_ptr);
	g_client_ptr = NULL;
	return 0;
}

// Wifi
int32_t WifiEnable(void)
{
	return network_client_send_cmd(WLAN_ENABLE);
}

int32_t WifiDisable(void)
{
	return network_client_send_cmd(WLAN_DISABLE);
}

int32_t WifiScan(void)
{
	return network_client_send_cmd(WLAN_START_SCAN);
}

int32_t WifiGetScanResults(WifiScanResult_t** results,int32_t wait_ms)
{
	int32_t retry = wait_ms / 100;
	int32_t ret = 0;
	do {
		ret = wlan_get_scan_results(results);
		if(ret > 0){
			/* printf("%s %d retry = %d ##\r\n",__func__,__LINE__,retry); */
			return ret;
		} else {
			usleep(100*1000);
		}
	}while(retry-- > 0);

	return -1;
}

int32_t WifiAddConfig(WifiConfigInfo_t* config)
{
	if(strlen(config->ssid) <= 0 || strlen(config->passwd) <= 0){
		return -1;
	}
	return client_wlan_add_network(config->ssid,config->passwd);
}

int32_t WifiDeleteConfig(int32_t nid)
{
	return client_wlan_remove_network(nid);
}

int32_t WifiSaveCurrentConfig(void)
{
	return network_client_send_cmd(WLAN_SAVE_CONFIG);
}

int32_t WifiGetConfig(WifiConfigInfo_t* configs)
{
	// TODO
	//
	return 0;
}

int32_t WifiEnableNetwork(int32_t nid)
{
	return client_wlan_enable_network(nid,true);
}

int32_t WifiDisableNetwork(int32_t nid)
{
	return client_wlan_enable_network(nid,false);
}

int32_t WifiConnectNetwork(int32_t nid,int16_t wait_s)
{
	/* return client_wlan_connect_network(nid,true); */
	int ret = client_wlan_connect_network(nid,true);
	if(!ret){
		pthread_mutex_lock(&g_client_ptr->lock);
		struct timespec ts;
    	clock_gettime(CLOCK_REALTIME, &ts);
    	ts.tv_sec += wait_s;
		ret = pthread_cond_timedwait(&g_client_ptr->connect_cond, &g_client_ptr->lock, &ts);
		if(ret == ETIMEDOUT){
			printf("wait connect error ##\r\n");
			return -1;
		}

		return 0;
	} else {
		return -1;
	}
}

int32_t WifiDisconnectNetwork(int32_t nid)
{
	return client_wlan_connect_network(nid,false);
}

int32_t WifiGetLinkedSta(wlan_connect_sta_t* sta)
{
	return client_wlan_get_link_sta(sta);
}

int32_t WifiSetListenCallback(NetState_Cb_t* cb)
{
	if(!cb){
		return -1;
	}
	g_client_ptr->err_cb = malloc(sizeof(NetState_Cb_t));
	if(!g_client_ptr->err_cb){
		return -1;
	}
	g_client_ptr->err_cb->net_sta_callback = cb->net_sta_callback;
	g_client_ptr->err_cb->priv = cb->priv;
	return 0;
}

int32_t WifiGetMac(char* mac)
{
	// TODO
}

int32_t WifiSetNetCheckIp(char* ip)
{
	// TODO
}




#if 0
// ETH
int32_t EthGetMac(char* mac);
int32_t EthSetMac(char* mac);
int32_t EthGetStatus();
int32_t EthSetListenCallback(NetError_Cb_t* cb);
int32_t EthSetNetCheckIp(char* ip); // default baidu

#endif
