#ifndef __ENCODER_SERVICE_H__
#define __ENCODER_SERVICE_H__




typedef enum {
	ENCODER_CLI_REGISTER,
	ENCODER_CLI_UNREGISTER,
	ENCODER_CHN_INIT,
	ENCODER_CHN_DEINIT,
	ENCODER_CHN_START,
	ENCODER_CHN_STOP,
	ENCODER_SYNC_STREAM,
	ENCODER_RELEASE_STREAM,
	ENCODER_SEND_SRC_BUF,
	ENCODER_GET_SRC_BUF,
	ENCODER_QP_BOUNDS_SET,
	ENCODER_IDR_REQ,
	ENCODER_CMD_MAX,
}iss_encoder_cmd_t;

typedef struct {
    int32_t cmd;
    char data[128];
    int32_t data_len;
    int32_t chn;
} iss_encoder_msg_t;

typedef struct {
	int32_t cmd;
	int32_t exec_result;
	int32_t retval;
}iss_encoder_reply_t;

#endif // __ENCODER_SERVICE_H__

