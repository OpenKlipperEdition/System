#include "binder_common.h"
#include "binder_ipc.h"
#include "binder_io.h"
#include <sys/time.h>
#include <sys/mman.h>
#include "sys_common.h"
#include "iss_camera.h"
#include "camera_service.h"
#include "fifo.h"
#include "systemserver_config.h"

#undef CU_TRUE
#undef CU_FALSE
#define CU_TRUE 0
#define CU_FALSE 1
#include <assert.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include <CUnit/Console.h>

static int received_cb(uint32_t code, tBinderIo* msg, tBinderIo* reply, uint32_t flag)
{
	return 0;
}

static tBinderService func_cb = {
    .transact_cb = received_cb,
    .link_to_death_cb = NULL,
    .unlink_to_death_cb = NULL,
    .death_notify_cb = NULL,
};

static void clean_global_source(void)
{
	/* ipc_handle = -1; */
	/* magic = 0; */
}


static uint32_t register_client(int32_t handle,tBinderService* cb)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};

	tIpcThreadInfo* ti = binder_get_thread_info();

	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CLIENT_REGISTER;
	cam_msg->ipc_magic = 0;

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	binder_io_append_obj(&data, cb);

	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        uint32_t ipc_magic = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
		return ipc_magic;
	}
	return 0;
}

static int32_t unregister_client(uint32_t magic,int32_t handle)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CLIENT_UNREGISTER;
	cam_msg->ipc_magic = magic;

	tIpcThreadInfo* ti = binder_get_thread_info();

    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply,handle, 0);
    if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t camera_dev_bind(int32_t handle,uint32_t magic,char* node)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  BIND_REQUEST;
	cam_msg->ipc_magic = magic;
	strncpy(cam_msg->data,node,strlen(node));

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
		/* printf("%s ret = 0x%x ###\r\n",__func__,retval); */
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}


static int32_t camera_dev_unbind(int32_t handle,uint32_t magic,char* node)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  UNBIND_REQUEST;
	cam_msg->ipc_magic = magic;
	strncpy(cam_msg->data,node,strlen(node));

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
		/* printf("%s ret = 0x%x ###\r\n",__func__,retval); */
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

static int32_t camera_dev_init(int32_t handle,uint32_t magic,char* node,int w,int h,int fmt,int index)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  INIT_REQUEST;
	cam_msg->ipc_magic = magic;
	cam_msg->data_len = sizeof(CameraInitParam_t);
	cam_msg->target_dev_index = index;
	CameraInitParam_t* param = &cam_msg->data[0];
	memcpy(param->node,node,strlen(node));
	/* param->node = node; */
	param->fmt = fmt;
	param->width = w;
	param->height = h;
	param->buffer_num = 2;

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
		printf("%s retval = %d ##\r\n",__func__,retval);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF || retval != index)
		return -1;
	else
		return 0;
}

int32_t camera_dev_deinit(int32_t handle,uint32_t magic,int32_t index)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  DEINIT_REQUEST;
	cam_msg->ipc_magic = magic;
	cam_msg->target_dev_index = index;

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
		printf("%s ret = 0x%x ###\r\n",__func__,retval);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;
}

int32_t camera_dev_start(int32_t handle,uint32_t magic,int32_t index)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CAMERA_START;
	cam_msg->ipc_magic = magic;
	cam_msg->target_dev_index = index;

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;

}

int32_t camera_dev_stop(int32_t handle,uint32_t magic,int32_t index)
{
	sys_msg_t sys_msg;
    char binder_buf[384] = {0};
	memset(&sys_msg,0,sizeof(sys_msg_t));
	iss_camera_msg_t* cam_msg = &sys_msg.msg[0];
	sys_msg.len = sizeof(iss_camera_msg_t);
	cam_msg->cmd =  CAMERA_STOP;
	cam_msg->ipc_magic = magic;
	cam_msg->target_dev_index = index;

	tIpcThreadInfo* ti = binder_get_thread_info();
    tBinderIo data, reply;
    binder_io_init(&data, binder_buf, sizeof(binder_buf),DEFAULT_OFFSET_LIST_SIZE);
    binder_io_append_data(&data, (char *)&sys_msg, sizeof(sys_msg));
	uint32_t retval = 0;
	int ret = binder_cmd_sync_call(ti, &data, &reply, handle, 0);
	if (BINDER_STATUS_OK == ret)
    {
        retval = binder_io_get_uint32(&reply);
        binder_cmd_freebuf(ti, reply.data0);
	}
	if(retval == 0xFFFF)
		return -1;
	else
		return 0;

}

void TEST_SERVER_REGISTER_000(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);
	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;


	uint32_t magic = register_client(ipc_handle,&cb);
	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}

void TEST_SERVER_UNREGISTER_001(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);
	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);
	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_NOT_EQUAL_FATAL(unregister_client(0x12,ipc_handle),0);

	CU_ASSERT_EQUAL_FATAL(unregister_client(magic,ipc_handle),0);

	clean_global_source();
}


void TEST_SERVER_BIND_002(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_bind(ipc_handle,magic,"/dev/video4"),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_unbind(ipc_handle,magic,"/dev/video4"),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}


void TEST_SERVER_BIND_003(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_bind(ipc_handle,magic,"/dev/video4"),0);

	CU_ASSERT_NOT_EQUAL_FATAL(camera_dev_bind(ipc_handle,magic,"/dev/video4"),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_unbind(ipc_handle,magic,"/dev/video4"),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}



void TEST_SERVER_BIND_004(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;

	tBinderService cb1;
	cb1.transact_cb = received_cb;
	cb1.link_to_death_cb = NULL;
	cb1.unlink_to_death_cb = NULL;
	cb1.death_notify_cb = NULL;

	uint32_t magic = register_client(ipc_handle,&cb);
	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	uint32_t magic1 = register_client(ipc_handle,&cb1);
	CU_ASSERT_NOT_EQUAL_FATAL(magic1,0);


	CU_ASSERT_EQUAL_FATAL(camera_dev_bind(ipc_handle,magic,"/dev/video5"),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_bind(ipc_handle,magic1,"/dev/video8"),0);


	CU_ASSERT_EQUAL_FATAL(camera_dev_unbind(ipc_handle,magic,"/dev/video5"),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_unbind(ipc_handle,magic1,"/dev/video8"),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}

void TEST_SERVER_UNBIND_005(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_bind(ipc_handle,magic,"/dev/video5"),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_unbind(ipc_handle,magic,"/dev/video5"),0);
	CU_ASSERT_NOT_EQUAL_FATAL(camera_dev_unbind(ipc_handle,magic,"/dev/video5"),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}

void TEST_SERVER_DEV_INIT_006(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}

void TEST_SERVER_DEV_INIT_007(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);

	CU_ASSERT_NOT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,1),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}


void TEST_SERVER_DEV_INIT_008(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;

	tBinderService cb1;
	cb1.transact_cb = received_cb;
	cb1.link_to_death_cb = NULL;
	cb1.unlink_to_death_cb = NULL;
	cb1.death_notify_cb = NULL;

	uint32_t magic = register_client(ipc_handle,&cb);
	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	uint32_t magic1 = register_client(ipc_handle,&cb1);
	CU_ASSERT_NOT_EQUAL_FATAL(magic1,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic1,"/dev/video5",1280,720,PIX_FMT_NV12,1),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic1,1),0);

	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	CU_ASSERT_EQUAL(unregister_client(magic1,ipc_handle),0);
	clean_global_source();

}

void TEST_SERVER_DEV_START_009(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_start(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_stop(ipc_handle,magic,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}

// test call start two times
void TEST_SERVER_DEV_START_010(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_start(ipc_handle,magic,0),0);
	CU_ASSERT_NOT_EQUAL_FATAL(camera_dev_start(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_stop(ipc_handle,magic,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}

void TEST_SERVER_DEV_STOP_011(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);

	CU_ASSERT_NOT_EQUAL_FATAL(camera_dev_stop(ipc_handle,magic,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}


void TEST_SERVER_DEV_STOP_012(void)
{
	int32_t ipc_handle = binder_get_service(CAMERA_SERVICE_NAME);

	tBinderService cb;
	cb.transact_cb = received_cb;
	cb.link_to_death_cb = NULL;
	cb.unlink_to_death_cb = NULL;
	cb.death_notify_cb = NULL;
	uint32_t magic = register_client(ipc_handle,&cb);

	CU_ASSERT_NOT_EQUAL_FATAL(magic,0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_init(ipc_handle,magic,"/dev/video4",1280,720,PIX_FMT_NV12,0),0);
	CU_ASSERT_EQUAL_FATAL(camera_dev_start(ipc_handle,magic,0),0);

	CU_ASSERT_NOT_EQUAL_FATAL(camera_dev_stop(ipc_handle,magic,1),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_stop(ipc_handle,magic,0),0);

	CU_ASSERT_EQUAL_FATAL(camera_dev_deinit(ipc_handle,magic,0),0);
	CU_ASSERT_EQUAL(unregister_client(magic,ipc_handle),0);
	clean_global_source();

}


CU_TestInfo camera_ser_test[] = {
	{"TEST_SERVER_REGISTER_000",TEST_SERVER_REGISTER_000},
	{"TEST_SERVER_UNREGISTER_001",TEST_SERVER_UNREGISTER_001},
	{"TEST_SERVER_BIND_002",TEST_SERVER_BIND_002},
	{"TEST_SERVER_BIND_003",TEST_SERVER_BIND_003},
	{"TEST_SERVER_BIND_004",TEST_SERVER_BIND_004},
	{"TEST_SERVER_UNBIND_005",TEST_SERVER_UNBIND_005},
	{"TEST_SERVER_DEV_INIT_006",TEST_SERVER_DEV_INIT_006},
	{"TEST_SERVER_DEV_INIT_007",TEST_SERVER_DEV_INIT_007},
	{"TEST_SERVER_DEV_INIT_008",TEST_SERVER_DEV_INIT_008},
	{"TEST_SERVER_DEV_START_009",TEST_SERVER_DEV_START_009},
	{"TEST_SERVER_DEV_START_010",TEST_SERVER_DEV_START_010},
	{"TEST_SERVER_DEV_STOP_011",TEST_SERVER_DEV_STOP_011},
	{"TEST_SERVER_DEV_STOP_012",TEST_SERVER_DEV_STOP_012},
	CU_TEST_INFO_NULL,
};

CU_SuiteInfo camera_ser_test_suite[] = {
	{"camera service API unit test",NULL,NULL,NULL,NULL,camera_ser_test},
	CU_TEST_INFO_NULL,
};



static void add_test(void)
{
    assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());
    if(CUE_SUCCESS != CU_register_suites(camera_ser_test_suite)){
        exit(-1);
    }
}

static int run_unit_test(void)
{
    if(CU_initialize_registry()){
        printf("initialize CU failed\r\n");
        exit(-1);
    } else {
        add_test();
        // auto mode
        /* CU_set_output_filename("audioInputTest_out"); */
        /* CU_list_tests_to_file(); */
        /* CU_automated_run_tests(); */

        // console mode
        CU_console_run_tests();

        CU_cleanup_registry();
        return CU_get_error();
        return 0;
    }
}

int main(int argc,char** argv)
{
    run_unit_test();
    return 0;
}




