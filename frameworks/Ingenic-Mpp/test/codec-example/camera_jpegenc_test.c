#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>


#include "log.h"
#include "icamera.h"
#include "codec.h"

#define TAG  "camera_jpegenc_test"

#define CAMERA_BUFFER_NUM       2

IHAL_CameraHandle_t *camera_handle = NULL;

IHal_CodecHandle_t *codec_handle = NULL;

pthread_t input_tid;
pthread_t work_tid;

int save_fd = 0;

static void print_help(void)
{
        printf("Usage: camera-jpegenc [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -i                input camera device.\n"
               " -w                width.(default 640)\n"
               " -h                height.(default 480)\n"
			   " -f                input raw data format.(default nv12)\n"
               " -o                output jpeg encode filename.(default camera_jpeg.jpg)\n"
               " -H                help\n\n"
               "Video presets:\n");
}

static int demo_exit(void)
{
        int ret = 0;
        pthread_cancel(input_tid);
        pthread_cancel(work_tid);
        pthread_join(input_tid, NULL);
        pthread_join(work_tid, NULL);
        ret = IHal_Codec_Stop(codec_handle);
        if (ret) {
                printf("codec stop failed\n");
        }

        ret = IHal_CodecDestroy(codec_handle);
        if (ret) {
                printf("codec destroy failed\n");
        }

        ret = IHal_CodecDeInit();
        if (ret) {
                printf("codec deinit failed\n");
        }

        ret = IHal_CameraStop(camera_handle);
        if (ret) {
                printf("camera stop failed\n");
        }

        ret = IHal_CameraClose(camera_handle);
        if (ret) {
                printf("camera close failed\n");
        }

        close(save_fd);
        printf("##################test demo exit ~~~~~~~~~~\n");
        exit(0);
}

void *input_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t camera_buf;
        while (1) {
                ret = IHal_Camera_WaitBufferAvailable(camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        // dequeue buffer from camera
                        ret = IHal_CameraDeQueueBuffer(camera_handle, &camera_buf);
                        if (!ret) {
                                // queue the buffer to encoder
                                IHal_Codec_QueueSrcBuffer(codec_handle, &camera_buf);
                        }
                } else {
                        usleep(5000);
                }
        }
}

void *work_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t encbuf;
        for (;;) {
                ret = IHal_Codec_WaitSrcAvailable(codec_handle, IMPP_NO_WAIT);
                if (!ret) {
                        ret = IHal_Codec_DequeueSrcBuffer(codec_handle, &encbuf);
                        if (!ret) {
                                IHal_CameraQueuebuffer(camera_handle, &encbuf);
                        }
                } else {
                        usleep(5000);
                }
        }
}

static int get_fmt_from_fmt_str(const char * const fmt_str)
{
	if (0 == strcasecmp(fmt_str, "NV12"))
		return IMPP_PIX_FMT_NV12;
	else if (0 == strcasecmp(fmt_str, "NV21"))
		return IMPP_PIX_FMT_NV21;
	else if (0 == strcasecmp(fmt_str, "NV16"))
		return IMPP_PIX_FMT_NV16;
	else if (0 == strcasecmp(fmt_str, "YUV444"))
		return IMPP_PIX_FMT_YUV444;
	else if (0 == strcasecmp(fmt_str, "YUYV"))
		return IMPP_PIX_FMT_YUYV;
	else if (0 == strcasecmp(fmt_str, "RGB888"))
		return IMPP_PIX_FMT_RGB_888;
	else if (0 == strcasecmp(fmt_str, "RGBA8888"))
		return IMPP_PIX_FMT_RGBA_8888;
	else if (0 == strcasecmp(fmt_str, "BGRA8888"))
		return IMPP_PIX_FMT_BGRA_8888;
	else if (0 == strcasecmp(fmt_str, "GREY"))
		return IMPP_PIX_FMT_GREY;

	return -1;
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;
        IHAL_INT8 *input_camera_node = NULL;
        IHAL_INT32 pic_width = 640;
        IHAL_INT32 pic_height = 480;
		IMPP_PIX_FMT image_fmt = -1;
        IHAL_INT8 *output_path = "camera_jpeg.jpg";

        while (1) {
                opt = getopt(argc, argv, "i:w:h:f:o:H");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'i':
                        input_camera_node = optarg;
                        break;
                case 'w':
                        pic_width = atoi(optarg);
                        break;
                case 'h':
                        pic_height = atoi(optarg);
                        break;
				case 'f':
						image_fmt = get_fmt_from_fmt_str(optarg);
						 if (-1 == image_fmt) {
							 printf("The input raw data format is not supported.\n");
							 return -1;
						 }
						 break;
                case 'o':
                        output_path = optarg;
                        break;
                case 'H':
                        print_help();
                        return 0;
                case '?':
                        print_help();
                        return -1;
                }
        }

		if (-1 == image_fmt)
			image_fmt = IMPP_PIX_FMT_NV12; // default

        if (NULL == input_camera_node) {
                print_help();
                return -1;
        }

        camera_handle = IHal_CameraOpen(input_camera_node);
        if (!camera_handle) {
                IMPP_LOGD(TAG, "open camera %s failed", input_camera_node);
                return -1;
        }

        IHAL_CAMERA_PARAMS camera_params = {
                .imageWidth = pic_width,
                .imageHeight = pic_height,
                .imageFmt = image_fmt,
        };

        ret = IHal_CameraSetParams(camera_handle, &camera_params);
        if (ret) {
                printf("Camera set params failed!\r\n");
                goto close_camera;
        }

        // camera request buffer under mmap mode
        ret = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, CAMERA_BUFFER_NUM);
        if (ret < CAMERA_BUFFER_NUM) {
                goto close_camera;
        }

        ret = IHal_CodecInit();
        if (ret) {
                printf("codec init error\n");
                goto close_camera;
        }

        codec_handle = IHal_CodecCreate(JPEG_ENC);
        if (!codec_handle) {
                printf("codec create failed\n");
                goto deinit_codec;
        }

        IHal_CodecParam param;
        memset(&param, 0, sizeof(IHal_CodecParam));
        param.codec_type = JPEG_ENC;
        param.codecparam.jpegenc_param.quality = 40;

        /* param.h264e_param.maxPictureSize = pic_width * pic_height; */
        param.codecparam.jpegenc_param.enc_width      = pic_width;
        param.codecparam.jpegenc_param.enc_height     = pic_height;
        param.codecparam.jpegenc_param.src_width      = pic_width;
        param.codecparam.jpegenc_param.src_height     = pic_height;
        param.codecparam.jpegenc_param.src_fmt        = image_fmt;

        ret =  IHal_Codec_SetParams(codec_handle, &param);
        if (ret) {
                printf("set codec param failed\n");
                goto destroy_codec;
        }

        int srcbuf_nums = IHal_Codec_CreateSrcBuffers(codec_handle, IMPP_EXT_DMABUFFER, CAMERA_BUFFER_NUM);
        if (srcbuf_nums < 1) {
                printf("src buffer create failed\n");
                goto destroy_codec;
        }

        // dmabuf import to codec
        IMPP_BufferInfo_t camera_share;
        for (int i = 0; i < CAMERA_BUFFER_NUM; i++) {
                /* camera_share.type = SHARE_TYPE_USERPTR; */
                ret = IHal_GetCameraBuffers(camera_handle, i, &camera_share);
                if (ret) {
                        printf("get camera buffer failed\r\n");
                        goto destroy_codec;
                }
                ret = IHal_Codec_SetSrcBuffer(codec_handle, i, &camera_share);
                if (ret) {
                        printf("set jpeg encoder src buffer failed\r\n");
                        goto destroy_codec;
                }
        }

        int dstbuf_nums = IHal_Codec_CreateDstBuffer(codec_handle, IMPP_INTERNAL_BUFFER, 2);
        if (dstbuf_nums < 1) {
                printf("dst buffer create failed\n");
                goto destroy_codec;
        }

        ret = IHal_Codec_Start(codec_handle);
        if (ret) {
                printf("codec start failed\n");
                goto destroy_codec;
        }

        ret = IHal_CameraStart(camera_handle);
        if (ret) {
                printf("camera start failed \n");
                goto stop_codec;
        }

        pthread_create(&input_tid, NULL, input_thread, (void *)camera_handle);
        pthread_create(&work_tid, NULL, work_thread, (void *)codec_handle);
        save_fd = open(output_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (save_fd < 0) {
                printf("create out file failed\n");
                goto stop_camera;
        }

        IHAL_CodecStreamInfo_t stream;
        int times = 100;
        while (times--) {
                ret = IHal_Codec_WaitDstAvailable(codec_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        ret = IHal_Codec_DequeueDstBuffer(codec_handle, &stream);
                        if (!ret) {
                                printf("dq dstbuf->index = %d paklen = %d #######\n", stream.index, stream.size);
                                lseek(save_fd, 0, SEEK_SET);
                                write(save_fd, stream.vaddr, stream.size);
                                IHal_Codec_QueueDstBuffer(codec_handle, &stream);
                        }
                } else {
                        usleep(5000);
                }
        }

        demo_exit();
        return 0;

stop_camera:
        IHal_CameraStop(camera_handle);
stop_codec:
        IHal_Codec_Stop(codec_handle);
destroy_codec:
        IHal_CodecDestroy(codec_handle);
deinit_codec:
        IHal_CodecDeInit();
close_camera:
        IHal_CameraClose(camera_handle);

        return -1;
}
