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
#include "fifo.h"
#include "list.h"

#include "dlog.h"

#define MAX_BL_LEVEL_NODE_PATH	"/sys/class/backlight/backlight/max_brightness"
#define CUR_BL_LEVEL_NODE_PATH	"/sys/class/backlight/backlight/actual_brightness"
#define SET_BL_LEVEL_NODE_PATH	"/sys/class/backlight/backlight/brightness"
#define MAX_MSG_NUM 12

#define LOG_TAG		BACKLIGHT_MANAGER_SERVICE_EXECUTE_FILE


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


typedef struct _bl_server_msg {
	tBinderIo *reply;
	int (*reply_callback)(char* data,int datalen,tBinderIo* rpt);
	uint8_t msg[128];
}bl_server_msg_t;

typedef struct _bl_manager {
	int set_fd;
	int cur_fd;
	int max_bl_level;
	pthread_mutex_t bl_lock;
	fifo_t msg_fifo;
	fifo_t msg_buf_fifo;
	bl_server_msg_t *bl_msg_buf;
	pthread_t tid;
	int last_brightness;
	int max_brightness;
	struct list_head cli_list_head;
}bl_manager_t;

typedef struct _cli_request_info {
	pid_t cli_pid;
	char cli_name[32];
	struct list_head list;
}client_request_into_t;



bl_manager_t manager_info;

static void set_backlight_brightness(bl_manager_t* hdl,int value)
{
	char data[8] = {0};
	float tmp = (float)(value / 100.0);
	/* int setval = (int)(tmp * hdl->max_brightness); */
	/* printf("%s set value [%d] #####\r\n",__func__,setval); */
	sprintf(data,"%d",(int)(tmp * hdl->max_brightness));
	lseek(hdl->set_fd,0,SEEK_SET);
	write(hdl->set_fd,data,strlen(data));
}

static int get_backlight_brightness(bl_manager_t* hdl)
{
	char data[8] = {0};
	memset(data,0,sizeof(data));
	lseek(hdl->cur_fd,0,SEEK_SET);
	read(hdl->cur_fd,data,8);
	float tmp = atoi(data) / (float)hdl->max_brightness;
	/* printf("current backlight brightness = %s \n",data); */
	return (int)(tmp*100);
}

static void backlight_on_off(bl_manager_t *hdl,bl_ops_cmd_t onoff)
{
	if(onoff == BL_OPS_OFF){
		hdl->last_brightness = get_backlight_brightness(hdl);
		set_backlight_brightness(hdl,0);
	} else {
		if(hdl->last_brightness <= 0){
			set_backlight_brightness(hdl,50);
			hdl->last_brightness = 50;
		} else {
			set_backlight_brightness(hdl,hdl->last_brightness);
		}
	}
}

static int client_request(bl_manager_t *hdl,bl_ctrl_msg_t *msg)
{
	client_request_into_t *reqinfo = (client_request_into_t*)calloc(1,sizeof(client_request_into_t));
	if(!reqinfo)
		return -1;
	reqinfo->cli_pid = msg->pid;
	memcpy(reqinfo->cli_name,msg->cli_name,32);
	list_add_head(&reqinfo->list, &hdl->cli_list_head);
	return 0;
}

static int client_release(bl_manager_t *hdl,bl_ctrl_msg_t *msg)
{
	client_request_into_t *info = NULL;
	list_for_each_entry(info,&hdl->cli_list_head,list){
		if(info->cli_pid == msg->pid
				&& !strncmp(info->cli_name,msg->cli_name,strlen(msg->cli_name))){
			list_del(&info->list);
			return 0;
		}
	}

	return -1;
}

static int check_client_request(bl_manager_t *hdl,bl_ctrl_msg_t *msg)
{
	client_request_into_t *info = NULL;
	list_for_each_entry(info,&hdl->cli_list_head,list){
		if(info->cli_pid == msg->pid
				&& !strncmp(info->cli_name,msg->cli_name,strlen(msg->cli_name))){
			return 1;
		}
	}
	return 0;
}

void exec_cmd(server_msg_t *ser_msg)
{
	int ret = 0;
	bl_ctrl_msg_t *msg = (bl_ctrl_msg_t*)ser_msg->msg;
	pthread_mutex_lock(&manager_info.bl_lock);
	switch(msg->cmd){
		case BL_OPS_SET_LEVEL:
			if(check_client_request(&manager_info,msg))
				set_backlight_brightness(&manager_info,msg->setvalue);
			break;
		case BL_OPS_GET_LEVEL:
			if(check_client_request(&manager_info,msg)){
				ret = get_backlight_brightness(&manager_info);
				if(ser_msg->reply_callback && ser_msg->reply){
					ser_msg->reply_callback(&ret,4,ser_msg->reply);
				}
			} else {
				ret = 0xffff;
				ser_msg->reply_callback(&ret,4,ser_msg->reply);
			}
			break;
		case BL_OPS_ON:
			if(check_client_request(&manager_info,msg))
				backlight_on_off(&manager_info,BL_OPS_ON);
			break;
		case BL_OPS_OFF:
			if(check_client_request(&manager_info,msg))
				backlight_on_off(&manager_info,BL_OPS_OFF);
			break;
		case BL_OPS_REQUEST_SERVER:
			if(!check_client_request(&manager_info,msg)){
				client_request(&manager_info,msg);
				ret = manager_info.max_brightness;
				ser_msg->reply_callback((char*)&ret,4,ser_msg->reply);
			}
			break;
		case BL_OPS_RELEASE_SERVER:
				client_release(&manager_info,msg);break;
		default:
			LOGE("invalid cmd type !!!\n");
			break;
	}
	pthread_mutex_unlock(&manager_info.bl_lock);
}

int bl_manager_init(void)
{
	int ret = 0;
	char r_data[8];
	memset(&manager_info,0,sizeof(bl_manager_t));
	manager_info.set_fd = open(SET_BL_LEVEL_NODE_PATH,O_RDWR);
	if(manager_info.set_fd < 0){
		LOGE("open brightness set node failed \n");
		return -1;
	}
	manager_info.cur_fd = open(CUR_BL_LEVEL_NODE_PATH,O_RDONLY);
	if(manager_info.cur_fd < 0){
		close(manager_info.set_fd);
		LOGE("open brightness get node failed \n");
		return -1;
	}
	int fd = open(MAX_BL_LEVEL_NODE_PATH,O_RDONLY);
	if(fd < 0){
		close(manager_info.set_fd);
		close(manager_info.cur_fd);
		LOGE("get max brightness node failed \n");
		return -1;
	} else {
		memset(r_data,0,sizeof(r_data));
		ret = read(fd,r_data,8);
		if(ret < 1){
			LOGE("get max brightness failed\n");
			goto err;
		}
		manager_info.max_brightness = atoi(r_data);
	}
	manager_info.last_brightness = get_backlight_brightness(&manager_info);


	pthread_mutex_init(&manager_info.bl_lock,NULL);
	INIT_LIST_HEAD(&manager_info.cli_list_head);
	return 0;
err:
	close(manager_info.set_fd);
	close(manager_info.cur_fd);
	close(fd);
	return -1;
}

int bl_manager_start(void)
{
	return 0;
}

int bl_manager_stop(void)
{
	return 0;
}

int bl_manager_recv_msg(server_msg_t* msg)
{
	exec_cmd(msg);
	return 0;
}

service_class_t backlight_service = {
	.server_name = BACKLIGHT_MANAGER_SERVICE_NAME,
	.init = bl_manager_init,
	.start = bl_manager_start,
	.stop = bl_manager_stop,
	.recv_msg = bl_manager_recv_msg,
	.server_status = 0,
};


static int cmd_exec_end_cb(char* data,int len,tBinderIo* reply)
{
        return binder_io_append_data(reply,data,len);
}

static int msg_arrive_transact(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
    server_msg_t ser_msg;
    sys_msg_t* sys_msg;
    char* data;
    int msg_size = 0;
    int ret = binder_io_get_data(msg,&data,&msg_size);

    if(ret || msg_size <= 0){
        printf("binder get data failed \n");
        return -1;
    }
    sys_msg = (sys_msg_t*)data;

    ser_msg.msg = sys_msg->msg;
    ser_msg.msg_len = sys_msg->len;
    ser_msg.reply = reply;
    ser_msg.reply_callback = cmd_exec_end_cb;

    ret = backlight_service.recv_msg(&ser_msg);
    if(ret){
        LOGE("call %s recv_msg failed\n");
        binder_io_append_uint32(reply,0xffffffff); // return error code
    }

    return 0;
}

static tBinderService backlightserver_service = {
       .transact_cb = msg_arrive_transact,
       .link_to_death_cb = NULL,
       .unlink_to_death_cb = NULL,
       .death_notify_cb = NULL,
};

int main()
{
	bl_manager_init();
	bl_manager_start();

	binder_add_service(BACKLIGHT_MANAGER_SERVICE_NAME, &backlightserver_service);
	binder_thread_enter_loop(1,1);

	while (1) {
		sleep(2);
	}
}


