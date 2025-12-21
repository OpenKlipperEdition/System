#ifndef __SYS_COMMON_H__
#define __SYS_COMMON_H__

#define SYSTEM_SERVICE_NAME "ingenic.systemServer.service"

typedef struct _sys_msg {
	char target_server_name[32];
	char msg[256];
	unsigned int len;	// msg
}sys_msg_t;

typedef enum
{
    ISS_SUCCESS = 0,      // 成功
	ISS_FAILED,
    ISS_CLIENT_NOT_EXIST, // 客户端不存在
    ISS_NO_IPC_MEMORY,    // reserve memory内存不够
    ISS_NO_HEAP_MEMORY,   // 堆内存不够
    ISS_PARAM_ERROR,      // 参数错误
    ISS_MISSING_NECESSARY_PARAMETERS,   // 缺少必要参数，以osd为例在开始合成前，少调用某些接口，导致缺少必要参数无法合成时返回该值
    ISS_EXEC_FAILED,      // 执行失败
	ISS_INVALID_CMD,
}ISS_ERR_CODE_t;


typedef struct {
    int32_t cmd;
    int32_t exec_result;
    int32_t retval;
}iss_remote_reply_t;


#endif // __SYS_COMMON_H__

