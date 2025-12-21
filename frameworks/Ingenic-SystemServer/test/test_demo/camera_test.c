#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "systemserver_config.h"
#include "iss_camera.h"

static int g_cnt = 0;


void* get_data_thread(void* arg)
{
	ISS_CamHandle_t handle = (ISS_CamHandle_t)arg;
	DataBuffer_t buf;
	char file_name[32];
	sprintf(file_name,"%s%d.nv12",__func__,g_cnt++);
	int sfd = open(file_name,O_CREAT | O_RDWR | O_TRUNC,0666);
	int ret = 0;
	for(;;){
		ret = ISS_GetCameraData(handle,&buf);
		if(!ret){
			lseek(sfd,0,SEEK_SET);
			write(sfd,buf.vaddr,buf.size);
			printf("save file: %s vaddr = 0x%x ##\r\n",file_name,buf.vaddr);
			ISS_ReleaseCameraData(handle,&buf);
		}
	}
}


int main(int argc,char** argv)
{
	pthread_t tid0;
	pthread_t tid1;
	pthread_t tid2;
	CameraInitParam_t cam_param;
	memset(&cam_param,0,sizeof(cam_param));
	cam_param.fmt = PIX_FMT_NV12;
	cam_param.width = 1280;
	cam_param.height = 720;
	cam_param.buffer_num = 3;
	sprintf(cam_param.node,"/dev/video4");
	ISS_CamHandle_t cam0_handle = ISS_CameraInit(&cam_param);
	if(!cam0_handle)
		return -1;
	pthread_create(&tid0,NULL,get_data_thread,cam0_handle);

	memset(&cam_param,0,sizeof(cam_param));
	cam_param.fmt = PIX_FMT_NV12;
	cam_param.width = 640;
	cam_param.height = 480;
	cam_param.buffer_num = 3;
	sprintf(cam_param.node,"/dev/video8");
	ISS_CamHandle_t cam1_handle = ISS_CameraInit(&cam_param);
	if(!cam1_handle)
		return -1;
	pthread_create(&tid1,NULL,get_data_thread,cam1_handle);

	memset(&cam_param,0,sizeof(cam_param));
	cam_param.fmt = PIX_FMT_NV12;
	cam_param.width = 320;
	cam_param.height = 240;
	cam_param.buffer_num = 3;
	sprintf(cam_param.node,"/dev/video9");
	ISS_CamHandle_t cam2_handle = ISS_CameraInit(&cam_param);
	if(!cam2_handle)
		return -1;
	pthread_create(&tid2,NULL,get_data_thread,cam2_handle);


	int times = 10;
	while(times--){
		sleep(1);
	}

	pthread_cancel(tid0);
	pthread_join(tid0,NULL);
	pthread_cancel(tid1);
	pthread_join(tid1,NULL);
	pthread_cancel(tid2);
	pthread_join(tid2,NULL);

	ISS_CameraDeInit(cam0_handle);
	ISS_CameraDeInit(cam1_handle);
	ISS_CameraDeInit(cam2_handle);
	return 0;
}


