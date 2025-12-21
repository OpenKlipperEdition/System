#ifndef __ISS_CAMERA_H__
#define __ISS_CAMERA_H__
#include "iss_common.h"
typedef struct {
    char node[32];
    PixFmt_t fmt;
    int32_t width;
    int32_t height;
	int32_t buffer_num;
}CameraInitParam_t;

typedef void* ISS_CamHandle_t;

ISS_CamHandle_t ISS_CameraInit(CameraInitParam_t *param);

int32_t ISS_CameraDeInit(ISS_CamHandle_t handle);

int32_t ISS_GetCameraData(ISS_CamHandle_t handle,DataBuffer_t* data);

int32_t ISS_ReleaseCameraData(ISS_CamHandle_t handle,DataBuffer_t* data);



#endif // __ISS_CAMERA_H__



