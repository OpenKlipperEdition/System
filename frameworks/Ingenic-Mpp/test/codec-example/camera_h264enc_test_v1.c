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

#define TAG  "camera_h264enc_test"

#define CAMERA_BUFFER_NUM       2

IHAL_CameraHandle_t *camera_handle = NULL;

IHal_CodecHandle_t *handle = NULL;

pthread_t input_tid;
pthread_t work_tid;

int save_fd = 0;

static void print_help(void)
{
        printf("Usage: camera-h264enc [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -i                  input camera device.\n"
               " -w                  input raw data width.(default 1920)\n"
               " -h                  input raw data height.(default 1080)\n"
			   " -f                  input raw data format.(default nv12)\n"
               " -b                  target bitrate.(default 6000)\n"
               " -r                  frame rate num. The default value is 30 and the maximum value is 30.\n"
               " -g                  set goplen.(default 90)\n"
               " -o                  output h264 encode filename.(default camera_enc_out_v1.h264)\n"
               " -H                  help\n\n"
               "Video presets:\n");
}

static int signal_handle(int sig)
{
        int ret = 0;
        pthread_cancel(work_tid);
        pthread_join(work_tid, NULL);
        pthread_cancel(input_tid);
        pthread_join(input_tid, NULL);

        ret = IHal_CameraStop(camera_handle);
        if (ret) {
                printf("camera stop failed\n");
        }

        ret = IHal_CameraClose(camera_handle);
        if (ret) {
                printf("camera close failed\n");
        }

        ret = IHal_Codec_Stop(handle);
        if (ret) {
                printf("codec stop failed\n");
        }

        ret = IHal_CodecDestroy(handle);
        if (ret) {
                printf("codec destroy failed\n");
        }

        ret = IHal_CodecDeInit();
        if (ret) {
                printf("codec deinit failed\n");
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
                        IHal_CameraDeQueueBuffer(camera_handle, &camera_buf);
                        // queue the buffer to encoder
                        IHal_Codec_QueueSrcBuffer(handle, &camera_buf);
                } else {
                        usleep(5000);
                }
        }
}

void *work_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t eSrcbuf;
        for (;;) {
                ret = IHal_Codec_WaitSrcAvailable(handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        ret = IHal_Codec_DequeueSrcBuffer(handle, &eSrcbuf);
                        if (!ret) {
                                IHal_CameraQueuebuffer(camera_handle, &eSrcbuf);
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
    else if (0 == strcasecmp(fmt_str, "YUV420P"))
        return IMPP_PIX_FMT_YUV420p;
    else if (0 == strcasecmp(fmt_str, "I420"))
        return IMPP_PIX_FMT_I420;

    return -1;
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;
        IHAL_INT8 *input_camera_node = NULL;
        IHAL_INT32 image_width = 1920;
        IHAL_INT32 image_height = 1080;
		IMPP_PIX_FMT image_fmt = -1;
        IHAL_INT32 gop_len = 90;
        IHAL_INT8 *output_path = "camera_enc_out_v1.h264";
        IHAL_INT32 target_bitrate = 6000;
        IHAL_INT32 frame_rate_num = 30;

        while (1) {
                opt = getopt(argc, argv, "i:w:h:f:b:r:g:o:H");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'i':
                        input_camera_node = optarg;
                        break;
                case 'w':
                        image_width = atoi(optarg);
                        break;
                case 'h':
                        image_height = atoi(optarg);
                        break;
				case 'f':
						image_fmt = get_fmt_from_fmt_str(optarg);
						if (-1 == image_fmt) {
							printf("The input raw data format is not supported.\n");
							return -1;
						}
						break;
                case 'b':
                        target_bitrate = atoi(optarg);
                        break;
                case 'r':
                        frame_rate_num = atoi(optarg);
                        break;
                case 'g':
                        gop_len = atoi(optarg);
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
                IMPP_LOGD(TAG, "open camera %s failed\r\n", input_camera_node);
                return -1;
        }

        IHAL_CAMERA_PARAMS camera_params = {
                .imageWidth = image_width,
                .imageHeight = image_height,
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
        handle = IHal_CodecCreate(H264_ENC);
        if (!handle) {
                printf("codec create failed\n");
                goto deinit_codec;
        }

        IHal_CodecParam param;
        memset(&param, 0, sizeof(IHal_CodecParam));
        param.codec_type = H264_ENC;
        param.codecparam.h264e_param.rc_mode = IMPP_ENC_RC_MODE_VBR;
        param.codecparam.h264e_param.target_bitrate = target_bitrate;
        param.codecparam.h264e_param.max_bitrate    = target_bitrate + 1000;
        param.codecparam.h264e_param.gop_len        = gop_len;
        param.codecparam.h264e_param.initial_Qp     = 25;
        param.codecparam.h264e_param.mini_Qp        = 10;
        param.codecparam.h264e_param.max_Qp         = 48;
        param.codecparam.h264e_param.level          = 20;
        param.codecparam.h264e_param.maxPSNR        = 40;
        param.codecparam.h264e_param.freqIDR        = 30;

        param.codecparam.h264e_param.frameRateNum   = frame_rate_num;
        param.codecparam.h264e_param.frameRateDen   = 1;
        /* param.codecparam.h264e_param.maxPictureSize = image_width * image_height; */
        param.codecparam.h264e_param.enc_width      = image_width;
        param.codecparam.h264e_param.enc_height     = image_height;
        param.codecparam.h264e_param.src_width      = image_width;
        param.codecparam.h264e_param.src_height     = image_height;
        param.codecparam.h264e_param.src_fmt        = image_fmt;

        ret =  IHal_Codec_SetParams(handle, &param);
        if (ret) {
                printf("set codec param failed\n");
                goto destroy_codec;
        }
        int srcbuf_nums = IHal_Codec_CreateSrcBuffers(handle, IMPP_EXT_DMABUFFER, CAMERA_BUFFER_NUM);
        if (srcbuf_nums < 1) {
                printf("src buffer create failed\n");
                goto destroy_codec;
        }
        // dmabuf import to codec
        IMPP_BufferInfo_t share;
        for (int i = 0; i < CAMERA_BUFFER_NUM; i++) {
                ret = IHal_GetCameraBuffers(camera_handle, i, &share);
                if (ret) {
                        printf("get camera buffer failed\r\n");
                        goto destroy_codec;
                }
                printf("v4l2 expt-dma-fd = %d\n", share.fd);
                ret = IHal_Codec_SetSrcBuffer(handle, i, &share);
                if (ret) {
                        printf("set h264 encoder src buffer failed\r\n");
                        goto destroy_codec;
                }
        }

        int dstbuf_nums = IHal_Codec_CreateDstBuffer(handle, IMPP_INTERNAL_BUFFER, 2);
        if (dstbuf_nums < 1) {
                printf("dst buffer create failed\n");
                goto destroy_codec;
        }

        ret = IHal_CameraStart(camera_handle);
        if (ret) {
                printf("camera start failed \n");
                goto destroy_codec;
        }

        ret = IHal_Codec_Start(handle);
        if (ret) {
                printf("codec start failed\n");
                goto stop_camera;
        }

        signal(SIGINT, signal_handle);

        pthread_create(&work_tid, NULL, work_thread, (void *)handle);
        pthread_create(&input_tid, NULL, input_thread, (void *)camera_handle);

        save_fd = open(output_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (save_fd < 0) {
                printf("create out file failed\n");
                goto stop_codec;
        }

        IHAL_CodecStreamInfo_t stream;

        IHal_CodecControlParam_t ctrol;

        unsigned int count = 0;
        int i = 0;
        while (1) {
                ret = IHal_Codec_WaitDstAvailable(handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueDstBuffer(handle, &stream);
                        printf("pack num = %d \r\n", stream.pack.packcnt);
                        for (i = 0; i < stream.pack.packcnt; i++) {
                                write(save_fd, (void *)(stream.vaddr + stream.pack.pack[i].offset), stream.pack.pack[i].length);
                                printf("stream-silce type = %d naltype = %d\r\n", stream.pack.pack[i].sliceType, stream.pack.pack[i].nalType);
                        }
                        IHal_Codec_QueueDstBuffer(handle, &stream);
                } else {
                        usleep(5000);
                }
                if ((count % 10) == 0) {
                        ctrol.cmd = ENCODER_IDR_REQUEST;
                        //IHal_Codec_Control(handle,&ctrol);
                }
                count += 1;
        }

        return 0;

stop_codec:
        IHal_Codec_Stop(handle);
stop_camera:
        IHal_CameraStop(camera_handle);
destroy_codec:
        IHal_CodecDestroy(handle);
deinit_codec:
        IHal_CodecDeInit();
close_camera:
        IHal_CameraClose(camera_handle);

        return -1;
}

