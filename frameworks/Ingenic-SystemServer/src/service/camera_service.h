#ifndef __CAMERA_SERVICE_H__
#define __CAMERA_SERVICE_H__

#include "iss_common.h"



typedef enum {
	BIND_WITH_DEVICE,
	CAM_DATA_REQUEST,
}cam_use_type_t;

typedef struct {
	int32_t cmd;
	uint32_t ipc_magic;
	char data[128];
	int32_t data_len;
	int32_t target_dev_index;
} iss_camera_msg_t;


typedef enum {
	BIND_REQUEST,
	UNBIND_REQUEST,
	INIT_REQUEST,
	DEINIT_REQUEST,
	DATA_RELEASE,
	DATA_BUF_SYNC,
	CAMERA_START,
	CAMERA_STOP,
	CLIENT_REGISTER,
	CLIENT_UNREGISTER,
	OPS_CMD_MAX,
}camera_ops_cmd_t;





#endif // __CAMERA_SERVICE_H__



