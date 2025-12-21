#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "systemserver.h"
#include "binder_io.h"
#include "binder_ipc.h"
#include "sys_common.h"
#include "backlight_client.h"

#define LOG_TAG	"BACKLIGHT_CLI"
#include "dlog.h"

typedef enum _cmd{
	BL_OPS_SET_LEVEL,
	BL_OPS_GET_LEVEL,
	BL_OPS_ON,
	BL_OPS_OFF,
	BL_OPS_REQUEST_SERVER,
	BL_OPS_RELEASE_SERVER,
	BL_OPS_CMD_MAX,
}bl_ops_cmd_t;

typedef struct _bl_ctrl_msg {
	bl_ops_cmd_t cmd;
	pid_t pid;
	char cli_name[32];
	int setvalue;
}bl_ctrl_msg_t;


char bl_binder_buf_g[DEFAULT_BINDER_IOBUF_SIZE];
pthread_mutex_t bl_lock;
// client api
static int bl_client_send_cmd(backlight_handle_t *hdl,int invalue,int *outvalue,bl_ops_cmd_t ops,int sync)
{
	int ret = 0;
	bl_ctrl_msg_t *bl_msg;
	sys_msg_t sys_msg;
	memset(&sys_msg,0,sizeof(sys_msg));
    tBinderIo bio, msg;
	bl_msg = (bl_ctrl_msg_t*)sys_msg.msg;
	sys_msg.len = sizeof(bl_ctrl_msg_t);
	strcpy(sys_msg.target_server_name, BACKLIGHT_MANAGER_SERVICE_NAME);

	pthread_mutex_lock(&bl_lock);
	binder_io_init(&bio, bl_binder_buf_g, sizeof(bl_binder_buf_g), DEFAULT_OFFSET_LIST_SIZE);
	bl_msg->cmd = ops;
	bl_msg->pid = getpid();
	strncpy(bl_msg->cli_name,hdl->cli_name,strlen(hdl->cli_name));
	bl_msg->setvalue = invalue;
    binder_io_append_data(&bio,&sys_msg,sizeof(sys_msg));
    memset(&msg, 0, sizeof(msg));
	//LOGE("binder send data entry %d \n",__LINE__);
	if(sync){
		ret =  binder_cmd_sync_call((tIpcThreadInfo*)hdl->ti,&bio,&msg,hdl->target_handle,0);
	} else {
		ret =  binder_cmd_async_call((tIpcThreadInfo*)hdl->ti,&bio,&msg,hdl->target_handle,0);
	}
	if(BINDER_STATUS_OK == ret){
		if(sync){
			char* data = NULL;
			int sz = 0;
			binder_io_get_data(&msg,&data,&sz);
			/* LOGE("%s %d msg-sz = %d data = %d \n",__func__,__LINE__,sz,*((int*)data)); */
			if((BL_OPS_REQUEST_SERVER || BL_OPS_GET_LEVEL) && outvalue != NULL){
				*outvalue = *((int*)data);
			}
        	binder_cmd_freebuf(hdl->ti, msg.data0);
		}
	} else {
		LOGE("binder status error %d \n",__LINE__);
		pthread_mutex_unlock(&bl_lock);
		return -1;
	}

	pthread_mutex_unlock(&bl_lock);
	return 0;
}

static int connect_backlight_server(backlight_handle_t *hdl)
{
	if (!hdl)
		return -1;
	hdl->target_handle = binder_get_service(BACKLIGHT_MANAGER_SERVICE_NAME);
	if (hdl->target_handle <= 0) {
		printf("Backlight clients failed to get binder service[%s].\n", BACKLIGHT_MANAGER_SERVICE_NAME);
		return -1;
	}
	hdl->ti = binder_get_thread_info();

	return 0;
}

backlight_handle_t* backlight_server_request(char* client_name)
{
	int ret = 0;
	if(!client_name)
		return NULL;
	if((ret = strlen(client_name)) > 32){
		LOGE("request_name is too len\n");
		return NULL;
	}
	backlight_handle_t *hdl = (backlight_handle_t*)malloc(sizeof(backlight_handle_t));
	pthread_mutex_init(&bl_lock,NULL);
	strncpy(hdl->cli_name,client_name,strlen(client_name));

	if (connect_backlight_server(hdl)) {
		pthread_mutex_destroy(&bl_lock);
		free(hdl);
		hdl = NULL;
		return NULL;
	}

	int max_level = 0;
	ret = bl_client_send_cmd(hdl,0,&max_level,BL_OPS_REQUEST_SERVER,1);
	if(ret == 0){
		if(max_level > 0 && max_level != 0xffff)
			hdl->max_bl_level = 100;//max_level;
	} else {
		LOGE("connect server failed \n");
		free(hdl);
		return NULL;
	}

	return hdl;
}



int backlight_set_level(backlight_handle_t* hdl,int level)
{
	int ret = 0;
	if(!hdl)
		return -1;
	if(level < 0 || level > hdl->max_bl_level){
		LOGE("level error range[0-%d] \n",hdl->max_bl_level);
		return -1;
	}
	ret = bl_client_send_cmd(hdl,level,NULL,BL_OPS_SET_LEVEL,0);
	if(ret){
		LOGE("set backlight failed \n");
		return -1;
	}
	return 0;
}

int backlight_get_level(backlight_handle_t* hdl,int *level)
{
	int ret = 0;
	if(!hdl || !level)
		return -1;
	ret = bl_client_send_cmd(hdl,0,level,BL_OPS_GET_LEVEL,1);
	if(ret){
		LOGE("get backlight failed \n");
		return -1;
	}
	/* printf("%s level = %d ####\r\n",__func__,*level); */
	return 0;

}

int backlight_on_off(backlight_handle_t* hdl,int onoff)
{

	int ret = 0;
	if(!hdl)
		return -1;
	if(onoff)
		ret = bl_client_send_cmd(hdl,0,NULL,BL_OPS_ON,0);
	else
		ret = bl_client_send_cmd(hdl,0,NULL,BL_OPS_OFF,0);
	if(ret){
		if(onoff)
			LOGE("set backlight ON failed \n");
		else
			LOGE("set backlight OFF failed \n");
		return -1;
	}
	return 0;
}

int backlight_server_release(backlight_handle_t *hdl)
{
	int ret = bl_client_send_cmd(hdl,0,NULL,BL_OPS_RELEASE_SERVER,0);
	free(hdl);
	return 0;
}

