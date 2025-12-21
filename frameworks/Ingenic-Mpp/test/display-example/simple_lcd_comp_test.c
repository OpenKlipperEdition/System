#include <fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <impp.h>
#include <display.h>
#include <icamera.h>
#include <assert.h>

#define WIDTH   720
#define HEIGHT  1280

IHal_SampleFB_Handle_t *fb0_handle;
IHal_SampleFB_Handle_t *fb1_handle;
IHAL_CameraHandle_t    *cam_handle;

static int signal_handle(int sig)
{
        IHal_CameraStop(cam_handle);
        IHal_CameraClose(cam_handle);
        IHal_SimpleFB_DeInit(fb0_handle);
        IHal_SimpleFB_DeInit(fb1_handle);

        printf("###################################### exit ####################################\r\n");
        exit(0);
}

int main(int argc, char **argv)
{
        int ret = 0;
	IHal_SampleFB_LayerAttr_t g_attr_layer0;
	IHal_SampleFB_LayerAttr_t g_attr_layer1;
        cam_handle = IHal_CameraOpen("/dev/video4");
        IHAL_CAMERA_PARAMS cam_params = {
                .imageWidth = WIDTH,
                .imageHeight = HEIGHT,
                .imageFmt = IMPP_PIX_FMT_NV12,
        };
        IHal_CameraSetParams(cam_handle, &cam_params);
        IHal_CameraCreateBuffers(cam_handle, IMPP_INTERNAL_BUFFER, 3); // create 3 buf

	memset(&g_attr_layer0, 0, sizeof(IHal_SampleFB_LayerAttr_t));
	sprintf(&g_attr_layer0.node[0],"%s","/dev/fb0");
	g_attr_layer0.lay_en = 1;
	g_attr_layer0.srcWidth = 720;
	g_attr_layer0.srcHeight = 1280;
	g_attr_layer0.scaleWidth = 720;
	g_attr_layer0.scaleHeight = 1280;
	g_attr_layer0.scale_enable = 1;
	g_attr_layer0.alpha = 255;
	g_attr_layer0.osd_posX = 0;
	g_attr_layer0.osd_posY = 0;
	g_attr_layer0.srcCropx = 0;
	g_attr_layer0.srcCropy = 0;
	g_attr_layer0.srcCropw = 0;
	g_attr_layer0.srcCroph = 0;
	g_attr_layer0.srcFmt = IMPP_PIX_FMT_NV12;
	g_attr_layer0.osd_order = Order_3;
	fb0_handle = IHal_SimpleFB_Init(&g_attr_layer0);
	if(!fb0_handle){
		printf("init error");
		return -1;
	}

	memset(&g_attr_layer1, 0, sizeof(IHal_SampleFB_LayerAttr_t));
	sprintf(&g_attr_layer1.node[0],"%s","/dev/fb1");
	g_attr_layer1.lay_en = 1;
	g_attr_layer1.srcWidth = 720;
	g_attr_layer1.srcHeight = 1280;
	g_attr_layer1.scaleWidth = 720;
	g_attr_layer1.scaleHeight = 1280;
	g_attr_layer1.scale_enable = 1;
	g_attr_layer1.alpha = 255;
	g_attr_layer1.osd_posX = 0;
	g_attr_layer1.osd_posY = 0;
	g_attr_layer1.srcCropx = 0;
	g_attr_layer1.srcCropy = 0;
	g_attr_layer1.srcCropw = 0;
	g_attr_layer1.srcCroph = 0;
	g_attr_layer1.srcFmt = IMPP_PIX_FMT_NV12;
	g_attr_layer1.osd_order = Order_2;
	fb1_handle = IHal_SimpleFB_Init(&g_attr_layer1);
	if(!fb1_handle){
		printf("init error");
		return -1;
	}

        IHal_CameraStart(cam_handle);

        signal(SIGINT, signal_handle);

        IMPP_BufferInfo_t cam_buf;
        IMPP_BufferInfo_t fb0_buf;
        IMPP_BufferInfo_t fb1_buf;
        int vsync = 0;
        while (1) {
                ret = IHal_Camera_WaitBufferAvailable(cam_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        ret = IHal_CameraDeQueueBuffer(cam_handle, &cam_buf);
                        IHal_SampleFB_GetMem(fb0_handle, &fb0_buf);
                        IHal_SampleFB_GetMem(fb1_handle, &fb1_buf);
                        memcpy(fb0_buf.vaddr, cam_buf.vaddr, cam_buf.size);
                        memcpy(fb1_buf.vaddr, cam_buf.vaddr, cam_buf.size);
			IHal_SimpleFB_UpdateWithAttr(fb0_handle, &fb0_buf, &g_attr_layer0);
			IHal_SimpleFB_UpdateWithAttr(fb1_handle, &fb1_buf, &g_attr_layer1);
                        IHal_CameraQueuebuffer(cam_handle, &cam_buf);
                }
        }

        IHal_CameraStop(cam_handle);
        IHal_CameraClose(cam_handle);
	IHal_SimpleFB_DeInit(fb0_handle);
	IHal_SimpleFB_DeInit(fb1_handle);

        return 0;
}
