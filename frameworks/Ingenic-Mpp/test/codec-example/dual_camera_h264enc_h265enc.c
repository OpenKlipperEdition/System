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

#define TAG  "dual_camera_h264enc_h265enc_test"
#define CAMERA_BUFFER_NUM       2

struct encoder_chan {
        int stream_fd;
        char stream_filename[64];
        char chan_name[12];
        pthread_t input_tid;
        pthread_t work_tid;
        pthread_t out_tid;
        IHAL_CameraHandle_t *camera_handle;
        IHal_CodecHandle_t *encoder_handle;
};


struct encoder_chan camera_h264e;
struct encoder_chan camera_h265e;

IHAL_INT8 *h264_input_camera_node = NULL;
IHAL_INT8 *h265_input_camera_node = NULL;
IHAL_INT32 h264_image_width = 1280;
IHAL_INT32 h265_image_width = 1280;
IHAL_INT32 h264_image_height = 720;
IHAL_INT32 h265_image_height = 720;
IHAL_INT32 h264_image_fmt = -1;
IHAL_INT32 h265_image_fmt = -1;
IHAL_INT32 h264_target_bitrate = 2000;
IHAL_INT32 h265_target_bitrate = 2000;
IHAL_INT32 h264_frame_rate_num = 30;
IHAL_INT32 h265_frame_rate_num = 30;
IHAL_INT32 h264_gpo_len = 25;
IHAL_INT32 h265_gpo_len = 25;
IHAL_INT8 *h264_output_path = "camera_h264e_out.h264";
IHAL_INT8 *h265_output_path = "camera_h265e_out.h265";

static void print_help(void)
{
        printf("Usage: dual-camera-h264enc-h265enc [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -i               h264 encoded input camera device.\n"
               " -w               h264 encoded input raw data width.(default 1280)\n"
               " -h               h264 encoded input raw data height.(default 720)\n"
               " -f               h264 encoded input raw data format.(default nv12)\n"
               " -b               h264 target bitrate.(default 2000)\n"
               " -r               h264 frame rate num. The default value is 30 and the maximum value is 30.\n"
               " -g               set goplen for h264 encoding.(default 25)\n"
               " -o               output h264 encode filename.(default camera_h264e_out.h264)\n"
               " -I               h265 encoded input camera device.\n"
               " -W               h265 encoded input raw data width.(default 1280)\n"
               " -H               h265 encoded input raw data height.(default 720)\n"
               " -F               h265 encoded input raw data format.(default nv12)\n"
               " -B               h265 target bitrate.(default 2000)\n"
               " -R               h265 frame rate num. The default value is 30 and the maximum value is 30.\n"
               " -G               set goplen for h265 encoding.(default 25)\n"
               " -O               output h265 encode filename.(default camera_h265e_out.h265)\n"
               " -s               see help\n\n"
               "Video presets:\n");
}

static int set_isp_img_depth(char *depth)
{
        int fd = open("/sys/module/isp_video/parameters/isp_force_img_depth", O_RDWR);
        if (fd < 0) {
                printf("open /sys/module/isp_video/parameters/isp_force_img_depth failed");
                return -1;
        }

        write(fd, depth, strlen(depth));

        close(fd);

        return 0;
}

static int camera_encoder_close(struct encoder_chan *ch)
{
        int ret = 0;
        pthread_cancel(ch->input_tid);
        pthread_cancel(ch->work_tid);
        pthread_cancel(ch->out_tid);
        pthread_join(ch->input_tid, NULL);
        pthread_join(ch->work_tid, NULL);
        pthread_join(ch->out_tid, NULL);

        ret = IHal_Codec_Stop(ch->encoder_handle);
        if (ret) {
                printf("codec stop failed\n");
        }

        ret = IHal_CodecDestroy(ch->encoder_handle);
        if (ret) {
                printf("codec destroy failed\n");
        }

        ret = IHal_CameraStop(ch->camera_handle);
        if (ret) {
                printf("camera stop failed\n");
        }

        ret = IHal_CameraClose(ch->camera_handle);
        if (ret) {
                printf("camera close failed\n");
        }

        close(ch->stream_fd);
        return 0;
}

static int signal_handle(int sig)
{
        int ret = 0;
        camera_encoder_close(&camera_h264e);
        camera_encoder_close(&camera_h265e);
        ret = IHal_CodecDeInit();
        if (ret) {
                printf("codec deinit failed\n");
        }
        printf("##################test demo exit ~~~~~~~~~~\n");
        exit(0);
}


void *work_thread(void *arg)
{
        struct encoder_chan *ch = (struct encoder_chan *)arg;
        IMPP_BufferInfo_t encbuf;
        int ret = 0;
        int cnt = 0;
        while (1) {
                //wait src buffer use done
                ret = IHal_Codec_WaitSrcAvailable(ch->encoder_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueSrcBuffer(ch->encoder_handle, &encbuf);
                        // queue src buffer to camera
                        IHal_CameraQueuebuffer(ch->camera_handle, &encbuf);
                } else {
                        usleep(5000);
                        cnt++;
                        if (cnt == 100) {
                                printf("%s : get codec src failed 100 times\n", ch->chan_name);
                                cnt = 0;
                        }
                }
        }
}

void *input_thread(void *arg)
{
        struct encoder_chan *ch = (struct encoder_chan *)arg;
        IMPP_BufferInfo_t camera_buf;
        int ret = 0;
        int cnt = 0;
        while (1) {
                ret = IHal_Camera_WaitBufferAvailable(ch->camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        // dequeue buffer from camera
                        IHal_CameraDeQueueBuffer(ch->camera_handle, &camera_buf);
                        // queue the buffer to encoder
                        IHal_Codec_QueueSrcBuffer(ch->encoder_handle, &camera_buf);
                } else {
                        usleep(5000);
                        cnt++;
                        if (cnt == 100) {
                                printf("%s : get camera data failed 100 times\n", ch->chan_name);
                                cnt = 0;
                        }
                }
        }
}

void *out_thread(void *arg)
{
        IHAL_CodecStreamInfo_t stream;
        struct encoder_chan *ch = (struct encoder_chan *)arg;
        int ret = 0;
        unsigned int cnt = 0;
        for (;;) {
                ret = IHal_Codec_WaitDstAvailable(ch->encoder_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueDstBuffer(ch->encoder_handle, &stream);
                        write(ch->stream_fd, stream.vaddr, stream.size);
                        IHal_Codec_QueueDstBuffer(ch->encoder_handle, &stream);
                        printf("%s : get stream ok !!\n", ch->chan_name);
                } else {
                        usleep(5000);
                        cnt++;
                        if (cnt == 100) {
                                printf("%s : get streeam failed 100 times\n", ch->chan_name);
                                cnt = 0;
                        }
                }
        }
}

static int get_fmt_from_fmt_str(const char * const fmt_str)
{
    if (0 == strcasecmp(fmt_str, "NV12"))
        return IMPP_PIX_FMT_NV12;
    else if (0 == strcasecmp(fmt_str, "NV21"))
        return IMPP_PIX_FMT_NV21;
    else if (0 == strcasecmp(fmt_str, "I420"))
        return IMPP_PIX_FMT_I420;

    return -1;
}

int camera_h264e_init(void)
{
        int ret = 0;
        camera_h264e.camera_handle = IHal_CameraOpen(h264_input_camera_node);
        if (!camera_h264e.camera_handle) {
                IMPP_LOGD(TAG, "open %s failed", h264_input_camera_node);
                return -1;
        }
        memcpy(&camera_h264e.chan_name, "h264e", 5);
        IHAL_CAMERA_PARAMS camera_params = {
                .imageWidth = h264_image_width,
                .imageHeight = h264_image_height,
                .imageFmt = h264_image_fmt,
        };

        ret = IHal_CameraSetParams(camera_h264e.camera_handle, &camera_params);
        if (ret) {
                IHal_CameraClose(camera_h264e.camera_handle);
                return -1;
        }

        ret = IHal_CameraCreateBuffers(camera_h264e.camera_handle, IMPP_EXT_DMABUFFER, CAMERA_BUFFER_NUM);
        if (ret < 2) {
                IHal_CameraClose(camera_h264e.camera_handle);
                return -1;
        }

        camera_h264e.encoder_handle = IHal_CodecCreate(H264_ENC);
        if (!camera_h264e.encoder_handle) {
                printf("codec create failed\n");
                return -1;
        }

        IHal_CodecParam param;
        memset(&param, 0, sizeof(IHal_CodecParam));
        param.codec_type = H264_ENC;
        param.codecparam.h264e_param.rc_mode = IMPP_ENC_RC_MODE_VBR;
        param.codecparam.h264e_param.target_bitrate = h264_target_bitrate;
        param.codecparam.h264e_param.max_bitrate    = h264_target_bitrate + 1000;
        param.codecparam.h264e_param.gop_len        = h264_gpo_len;
        param.codecparam.h264e_param.initial_Qp     = 25;
        param.codecparam.h264e_param.mini_Qp        = 10;
        param.codecparam.h264e_param.max_Qp         = 48;
        param.codecparam.h264e_param.level          = 20;
        param.codecparam.h264e_param.maxPSNR        = 40;
        param.codecparam.h264e_param.freqIDR        = 30;
        param.codecparam.h264e_param.frameRateNum   = h264_frame_rate_num;
        param.codecparam.h264e_param.frameRateDen   = 1;
        param.codecparam.h264e_param.IFrameQp       = 30;
        param.codecparam.h264e_param.PFrameQp       = 30;

        param.codecparam.h264e_param.enc_width      = h264_image_width;
        param.codecparam.h264e_param.enc_height     = h264_image_height;
        param.codecparam.h264e_param.src_width      = h264_image_width;
        param.codecparam.h264e_param.src_height     = h264_image_height;
        param.codecparam.h264e_param.src_fmt        = h264_image_fmt;

        ret =  IHal_Codec_SetParams(camera_h264e.encoder_handle, &param);
        if (ret) {
                printf("set codec param failed\n");
                return -1;
        }
        int srcbuf_nums = IHal_Codec_CreateSrcBuffers(camera_h264e.encoder_handle, IMPP_INTERNAL_BUFFER, CAMERA_BUFFER_NUM);
        if (srcbuf_nums < 1) {
                printf("src buffer create failed\n");
                return -1;
        }

        int dstbuf_nums = IHal_Codec_CreateDstBuffer(camera_h264e.encoder_handle, IMPP_INTERNAL_BUFFER, 2);
        if (dstbuf_nums < 1) {
                printf("dst buffer create failed\n");
                return -1;
        }

        IMPP_BufferInfo_t enc_share;
        for (int i = 0; i < srcbuf_nums; i++) {
                ret = IHal_Codec_GetSrcBuffer(camera_h264e.encoder_handle, i, &enc_share);
                if (ret) {
                        printf("get src buffer %d failed", i);
                        return -1;
                }
                //printf("share type = %d  fd = %d size = %d \n",enc_share.type,enc_share.info.fd,enc_share.size);
                // set encoder buffers to camera
                ret = IHal_CameraSetBuffers(camera_h264e.camera_handle, i, &enc_share);
                if (ret) {
                        printf("set camera buffers failed");
                        return -1;
                }
        }

        camera_h264e.stream_fd = open(h264_output_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (camera_h264e.stream_fd < 0) {
                printf("create out file failed\n");
                return -1;
        }
        //  TODO
        ret = IHal_Codec_Start(camera_h264e.encoder_handle);
        if (ret) {
                printf("codec start failed\n");
                return -1;
        }

        ret = IHal_CameraStart(camera_h264e.camera_handle);
        if (ret) {
                printf("camera start failed \n");
                return -1;
        }

        pthread_create(&camera_h264e.work_tid, NULL, work_thread, (void *)&camera_h264e);
        pthread_create(&camera_h264e.out_tid, NULL, out_thread, (void *)&camera_h264e);
        pthread_create(&camera_h264e.input_tid, NULL, input_thread, (void *)&camera_h264e);
        return 0;
}

#if 1
int camera_h265e_init(void)
{
        int ret = 0;
        camera_h265e.camera_handle = IHal_CameraOpen(h265_input_camera_node);
        if (!camera_h265e.camera_handle) {
                IMPP_LOGD(TAG, "open %s failed", h265_input_camera_node);
                return -1;
        }
        memcpy(&camera_h265e.chan_name, "h265e", 5);

        IHAL_CAMERA_PARAMS camera_params = {
                .imageWidth = h265_image_width,
                .imageHeight = h265_image_height,
                .imageFmt = h265_image_fmt,
        };

        ret = IHal_CameraSetParams(camera_h265e.camera_handle, &camera_params);
        if (ret) {
                IHal_CameraClose(camera_h265e.camera_handle);
                return -1;
        }

        ret = IHal_CameraCreateBuffers(camera_h265e.camera_handle, IMPP_EXT_DMABUFFER, CAMERA_BUFFER_NUM);
        if (ret < 2) {
                IHal_CameraClose(camera_h265e.camera_handle);
                return -1;
        }

        camera_h265e.encoder_handle = IHal_CodecCreate(H265_ENC);
        if (!camera_h265e.encoder_handle) {
                printf("codec create failed\n");
                return -1;
        }

        IHal_CodecParam param;
        memset(&param, 0, sizeof(IHal_CodecParam));
        param.codec_type = H265_ENC;
        param.codecparam.h265e_param.rc_mode = IMPP_ENC_RC_MODE_VBR;
        param.codecparam.h265e_param.target_bitrate = h265_target_bitrate;
        param.codecparam.h265e_param.max_bitrate    = h265_target_bitrate + 1000;
        param.codecparam.h265e_param.gop_len        = h265_gpo_len;
        param.codecparam.h265e_param.initial_Qp     = 25;
        param.codecparam.h265e_param.mini_Qp        = 10;
        param.codecparam.h265e_param.max_Qp         = 40;
        param.codecparam.h265e_param.level          = 20;
        param.codecparam.h265e_param.maxPSNR        = 40;
        param.codecparam.h265e_param.freqIDR        = 30;
        param.codecparam.h265e_param.frameRateNum   = h265_frame_rate_num;
        param.codecparam.h265e_param.frameRateDen   = 1;

        param.codecparam.h265e_param.enc_width      = h265_image_width;
        param.codecparam.h265e_param.enc_height     = h265_image_height;
        param.codecparam.h265e_param.src_width      = h265_image_width;
        param.codecparam.h265e_param.src_height     = h265_image_height;
        param.codecparam.h265e_param.src_fmt        = h265_image_fmt;

        ret =  IHal_Codec_SetParams(camera_h265e.encoder_handle, &param);
        if (ret) {
                printf("set codec param failed\n");
                return -1;
        }
        int srcbuf_nums = IHal_Codec_CreateSrcBuffers(camera_h265e.encoder_handle, IMPP_INTERNAL_BUFFER, CAMERA_BUFFER_NUM);
        if (srcbuf_nums < 1) {
                printf("src buffer create failed\n");
                return -1;
        }

        int dstbuf_nums = IHal_Codec_CreateDstBuffer(camera_h265e.encoder_handle, IMPP_INTERNAL_BUFFER, 2);
        if (dstbuf_nums < 1) {
                printf("dst buffer create failed\n");
                return -1;
        }

        IMPP_BufferInfo_t enc_share;
        for (int i = 0; i < srcbuf_nums; i++) {
                ret = IHal_Codec_GetSrcBuffer(camera_h265e.encoder_handle, i, &enc_share);
                if (ret) {
                        printf("get src buffer %d failed", i);
                        return -1;
                }
                //printf("share type = %d  fd = %d size = %d \n",enc_share.type,enc_share.info.fd,enc_share.size);
                // set encoder buffers to camera
                ret = IHal_CameraSetBuffers(camera_h265e.camera_handle, i, &enc_share);
                if (ret) {
                        printf("set camera buffers failed");
                        return -1;
                }
        }

        camera_h265e.stream_fd = open(h265_output_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (camera_h265e.stream_fd < 0) {
                printf("create out file failed\n");
                return -1;
        }

        ret = IHal_Codec_Start(camera_h265e.encoder_handle);
        if (ret) {
                printf("codec start failed\n");
                return -1;
        }
        ret = IHal_CameraStart(camera_h265e.camera_handle);
        if (ret) {
                printf("camera start failed \n");
                return -1;
        }

        pthread_create(&camera_h265e.work_tid, NULL, work_thread, (void *)&camera_h265e);
        pthread_create(&camera_h265e.out_tid, NULL, out_thread, (void *)&camera_h265e);
        pthread_create(&camera_h265e.input_tid, NULL, input_thread, (void *)&camera_h265e);

        return 0;
}
#endif

int main(int argc, char **argv)
{
        int ret = 0;

        int opt = 0;

        while (1) {
                opt = getopt(argc, argv, "i:w:h:f:b:r:g:o:I:W:H:F:B:R:G:O:s");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'i':
                        h264_input_camera_node = optarg;
                        break;
                case 'w':
                        h264_image_width = atoi(optarg);
                        break;
                case 'h':
                        h264_image_height = atoi(optarg);
                        break;
				case 'f':
						h264_image_fmt = get_fmt_from_fmt_str(optarg);
						if (-1 == h264_image_fmt) {
							printf("The input raw data format is not supported.\n");
							return -1;
						}
						break;
                case 'b':
                        h264_target_bitrate = atoi(optarg);
                        break;
                case 'r':
                        h264_frame_rate_num = atoi(optarg);
                        break;
                case 'g':
                        h264_gpo_len = atoi(optarg);
                        break;
                case 'o':
                        h264_output_path = optarg;
                        break;
                case 'I':
                        h265_input_camera_node = optarg;
                        break;
                case 'W':
                        h265_image_width = atoi(optarg);
                        break;
                case 'H':
                        h265_image_height = atoi(optarg);
                        break;
				case 'F':
						h265_image_fmt = get_fmt_from_fmt_str(optarg);
						if (-1 == h265_image_fmt) {
							printf("The input raw data format is not supported.\n");
							return -1;
						}
						break;
                case 'B':
                        h265_target_bitrate = atoi(optarg);
                        break;
                case 'R':
                        h265_frame_rate_num = atoi(optarg);
                        break;
                case 'G':
                        h265_gpo_len = atoi(optarg);
                        break;
                case 'O':
                        h265_output_path = optarg;
                        break;
                case 's':
                        print_help();
                        return 0;
                case '?':
                        print_help();
                        return -1;
                }
        }

	if (-1 == h264_image_fmt)
		h264_image_fmt = IMPP_PIX_FMT_NV12;

	if (-1 == h265_image_fmt)
		h265_image_fmt = IMPP_PIX_FMT_NV12;

        if (NULL == h264_input_camera_node || NULL == h265_input_camera_node) {
                print_help();
                return -1;
        }

        set_isp_img_depth("12");        // nv12 is 12bit
        ret = IHal_CodecInit();
        if (ret) {
                printf("codec init error\n");
                return -1;
        }
        signal(SIGINT, signal_handle);
        memset(&camera_h264e, 0, sizeof(struct encoder_chan));
        memset(&camera_h265e, 0, sizeof(struct encoder_chan));
        camera_h264e_init();
        camera_h265e_init();
        for (;;) {
                sleep(2);
        }

        return 0;
}

