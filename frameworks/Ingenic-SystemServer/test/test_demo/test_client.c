#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>

#include "sys_common.h"


#define SYSTEM_SERVICE_NAME "ingenic.systemServer.service"


int main(int argc, char * argv[]){
    struct timeval tpstart,tpend;
    long int v_sec = 0;
    char binder_buf[DEFAULT_BINDER_IOBUF_SIZE] = {0};
    tBinderIo bio, msg;


	tIpcThreadInfo *ti = binder_get_thread_info();
	if(ti)
		printf("get thread info ok !!\r\n");

    uint32_t tar_handle = binder_get_service(SYSTEM_SERVICE_NAME);
	if(tar_handle)
		printf("get target handle success %d \r\n",tar_handle);

	sys_msg_t sysmsg;
	strcpy(sysmsg.target_server_name,"backlightManagerService");
	memset(sysmsg.msg,'A',128);
	sysmsg.len = 128;
    binder_io_init(&bio, binder_buf, sizeof(binder_buf), DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&bio,&sysmsg,sizeof(sysmsg));

    memset(&msg, 0, sizeof(msg));
    if(BINDER_STATUS_OK == binder_cmd_sync_call(ti,&bio,&msg,tar_handle,0)){
        /*parse and free buffer*/
		int sz = 0;
		char *data = NULL;
		binder_io_get_data(&msg,&data,&sz);
		printf("recv: data avalid = %d   reply = 0x%s ###\r\n",sz,data);
        binder_cmd_freebuf(ti, msg.data0);
    } else {
		printf("binder cmd sync call failed \r\n");
	}

    sleep(20);
    binder_cmd_release(ti,tar_handle);
    flush_commands(ti);
    binder_threads_shutdown();
    return 0;
}

