#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include "sys_common.h"

#include "network_service.h"
#include "fifo.h"
#include "systemserver_config.h"
#include "network.h"


#undef CU_TRUE
#undef CU_FALSE
#define CU_TRUE 0
#define CU_FALSE 1
#include <assert.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

typedef struct {
	int32_t ipc_handle;
	pid_t client_pid;
	pthread_mutex_t lock;
}net_client_info_t;

net_client_info_t g_client_info;

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
				printf("client >>> get from master event: %d ##\r\n",rawmsg[0]);
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
	net_msg->client_pid = g_client_info.client_pid;

	network_register_info_t* reg_info = &net_msg->msg[0];
	reg_info->client_pid = g_client_info.client_pid;
	reg_info->client_level = applevel;
	reg_info->need_listen_wlan_sta = true;
	reg_info->need_listen_eth_sta = false;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	binder_io_append_obj(&data, &func_cb);

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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


static int32_t network_client_unregister(void)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();


	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_CLI_UNREGISTER;
	net_msg->client_pid = g_client_info.client_pid;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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
	net_msg->client_pid = g_client_info.client_pid;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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

static int32_t client_wlan_disable(void)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();


	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_DISABLE;
	net_msg->client_pid = g_client_info.client_pid;


    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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

static int32_t client_wlan_add_network(char* ssid,char* passwd)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();


	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_ADD_CONFIG;
	net_msg->client_pid = g_client_info.client_pid;

	iss_wlan_config_info_t* config_info = &net_msg->msg[0];
	strncpy(config_info->ssid,ssid,strlen(ssid));
	strncpy(config_info->passwd,passwd,strlen(passwd));

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		int sz = 0;
		char* data;
		binder_io_get_data(&reply,&data,&sz);
		iss_remote_reply_t* rep = (iss_remote_reply_t*)data;
		uint32_t nid = binder_io_get_uint32(&reply);
		if(rep->exec_result == ISS_SUCCESS && rep->cmd == WLAN_ADD_CONFIG){
			binder_cmd_freebuf(ti, reply.data0);
			printf("%s: nid = %d ##\r\n",__func__,rep->retval);
			return rep->retval;
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
	net_msg->client_pid = g_client_info.client_pid;
	int32_t* dstnid = (int32_t*)&net_msg->msg[0];
	*dstnid = nid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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

static int32_t client_wlan_save_config(void)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd =  WLAN_SAVE_CONFIG;
	net_msg->client_pid = g_client_info.client_pid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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
	net_msg->client_pid = g_client_info.client_pid;
	int32_t* dstnid = (int32_t*)&net_msg->msg[0];
	*dstnid = nid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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

static int32_t wlan_start_scan(void)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd = WLAN_START_SCAN;
	net_msg->client_pid = g_client_info.client_pid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
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

static int32_t wlan_get_scan_results(WifiScanResult_t* results)
{
	sys_msg_t sys_msg;
    char binder_buf[1024] = {0};
	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_network_msg_t* net_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_network_msg_t);
	net_msg->cmd = WLAN_GET_SCAN_RESULTS;
	net_msg->client_pid = g_client_info.client_pid;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));

	int ret = binder_cmd_sync_call(ti, &data, &reply,g_client_info.ipc_handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
		uint32_t* data;
		int32_t sz = 0;
		uint32_t rep = binder_io_get_data(&reply,(uint8_t**)&data,&sz);
		if(rep == ISS_SUCCESS){
			int32_t results_num = data[0];
			if(results_num > 0)
				memcpy(results,&data[1],results_num * sizeof(WifiScanResult_t));
			binder_cmd_freebuf(ti, reply.data0);
			return results_num;
		}
	}
    binder_cmd_freebuf(ti, reply.data0);

	return -1;
}

int main(int argc,char** argv)
{
	memset(&g_client_info,0,sizeof(net_client_info_t));
	pthread_mutex_init(&g_client_info.lock,NULL);
	g_client_info.ipc_handle = binder_get_service(NETWORK_SERVICE_NAME);
	if(g_client_info.ipc_handle <= 0){
		printf("can not find service:%s ##\r\n",NETWORK_SERVICE_NAME);
	}
	g_client_info.client_pid = getpid();
	WifiScanResult_t* results = malloc(15 * sizeof(WifiScanResult_t));
	int ret = network_client_register(0);
	if(!ret){
		printf("client register success !!!\r\n");
		client_wlan_enable();
		wlan_start_scan();
		int retry = 1000;
		ret = wlan_get_scan_results(results);
		while(ret < 0 && retry > 0){
			usleep(10*1000);
			retry--;
			ret = wlan_get_scan_results(results);
		}
		printf("get scan results num = %d ##\r\n",ret);
		for(int i = 0; i < ret; i++){
			printf(" client >>> ssid: %s freq:%d  siglevel: %d\r\n",results[i].ssid,results[i].freq,results[i].signal_level);
		}
#if 0
		int nid = client_wlan_add_network("S20Plus","23456789");
		sleep(2);
		if(nid >= 0){
			printf("client >>> add net work success\r\n");
			ret = client_wlan_connect_network(nid,true);
			if(!ret){
				printf("client >>> connect network id: %d ##\r\n",nid);
			}
			client_wlan_save_config();
			sleep(3);
			/* system("cat /etc/wpa_supplicant.conf"); */
			client_wlan_connect_network(nid,false);
			sleep(6);
			client_wlan_remove_network(nid);
			/* system("cat /etc/wpa_supplicant.conf"); */
		}
#endif
		client_wlan_disable();
		sleep(1);
		ret = network_client_unregister();
		if(!ret){
			printf("net client unregister success !!\r\n");
		}
	} else {
		printf("client register failed !!!\r\n");
	}
	return 0;
}




