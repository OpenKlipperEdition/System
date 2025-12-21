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
#include <getopt.h>

#include "log.h"
#include "icamera.h"
#include "codec.h"
#include <rtsp_server.h>


struct configInfo {
        char *video_node;
        int width;
        int height;
        int num_buffers;
        int target_bitrate;
        int frame_rate_num;
        int gop_len;
        char *ethernet;
        char *ipaddr;
        int port;
};

IHAL_CameraHandle_t *camera_handle = NULL;
IHal_CodecHandle_t *handle = NULL;
static struct configInfo input_params;

pthread_t input_tid;
pthread_t work_tid;

static struct option long_options[] = {
        { "video_node    ", required_argument, NULL, 'd'},
        { "pixel_size    ", required_argument, NULL, 's'},
        { "num_buffers   ", required_argument, NULL, 'n'},
        { "target_bitrate", required_argument, NULL, 'r'},
        { "frame_rate    ", required_argument, NULL, 'f'},
        { "gop_len       ", required_argument, NULL, 'g'},
        { "ethernet      ", required_argument, NULL, 'e'},
        { "ip_addr       ", required_argument, NULL, 'i'},
        { "port          ", required_argument, NULL, 'p'},
        { "help          ", no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0},
};

#define print_opt_help(opt_index, help_str)             \
        do {                                \
                        printf("   -%c or --%s          %s", (char)long_options[opt_index].val, long_options[opt_index].name, help_str); \
        } while (0)

static void usage()
{
        printf("\ncamera-rtsp usage:\n");
        print_opt_help(0, "video input node. The default is /dev/video4.\n");
        print_opt_help(1, "Camera pixel resolution size. The format is WxH. The default is 1280x720.\n");
        print_opt_help(2, "Num of buffers to use. The default is 3.\n");
        print_opt_help(3, "H264 encoding target bitrate. The default value is 2000.\n");
        print_opt_help(4, "H264 encoding frame rate num. The default value is 30 and the maximum value is 30.\n");
        print_opt_help(5, "H264 encoding gop length. The default is 60.\n");
        print_opt_help(6, "The name of the Ethernet card, such as eth0.\n");
        print_opt_help(7, "IP address of RTSP server.\n");
        print_opt_help(8, "port number of RTSP server. The default is 8888.\n");
        print_opt_help(9, "show help.\n");
}

static int get_paramt(int argc, char *argv[])
{
        int ret = 0;
        memset(&input_params, 0, sizeof(struct configInfo));

        /* The default value */
        input_params.video_node = "/dev/video4";
        input_params.width = 1280;
        input_params.height = 720;
        input_params.num_buffers = 3;
        input_params.target_bitrate = 2000;
        input_params.frame_rate_num = 30;
        input_params.gop_len = 60;
        input_params.port = 8888;

        char optstring[] = "d:s:n:r:f:g:e:i:p:h";
        while ((ret = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
                switch (ret) {
                case 'd':
                        input_params.video_node = optarg;
                        break;
                case 's':
                        if (2 != sscanf(optarg, "%dx%d", &input_params.width, &input_params.height)) {
                                printf("The resolution format of the camera pixel resolution size is incorrect!\r\n");
                                exit(-1);
                        }
                        break;
                case 'n':
                        input_params.num_buffers = atoi(optarg);
                        break;
                case 'r':
                        input_params.target_bitrate = atoi(optarg);
                        break;
                case 'f':
                        input_params.frame_rate_num = atoi(optarg);
                        break;
                case 'g':
                        input_params.gop_len = atoi(optarg);
                        break;
                case 'e':
                        input_params.ethernet = optarg;
                        break;
                case 'i':
                        input_params.ipaddr = optarg;
                        break;
                case 'p':
                        input_params.port = atoi(optarg);
                        break;
                case 'h':
                        usage();
                        exit(0);
                case '?':
                default:
                        usage();
                        exit(-1);
                }
        }

        if (NULL == input_params.ipaddr || NULL == input_params.ethernet) {
                printf("Please input paramt!!!\r\n");
                usage();
                exit(-1);
        }

        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
        printf("video_node = %s\t width = %d\t height = %d\t\r\n", input_params.video_node, input_params.width, input_params.height);
        printf("num_buffers : %d\r\n", input_params.num_buffers);
        printf("target_bitrate : %d     frame_rate_num : %d     gop_len : %d\r\n", \
               input_params.target_bitrate, input_params.frame_rate_num, input_params.gop_len);
        printf("ethernet = %s\t ipaddr = %s\t port = %d\r\n", input_params.ethernet, input_params.ipaddr, input_params.port);
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");

        return 0;
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
        rtsp_uinit();
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

int main(int argc, char **argv)
{
        int ret = 0;
        char shell_cmd[128] = {0};

        get_paramt(argc, argv);

        /* set test ip */
        sprintf(shell_cmd, "ifconfig %s %s", input_params.ethernet, input_params.ipaddr);
        ret = system(shell_cmd);
        if (0 != ret) {
                printf("Failed to configure IP address.\r\n");
                return -1;
        }

        camera_handle = IHal_CameraOpen(input_params.video_node);
        if (!camera_handle) {
                printf("open camera %s failed\r\n", input_params.video_node);
                return -1;
        }

        IHAL_CAMERA_PARAMS camera_params = {
                .imageWidth = input_params.width,
                .imageHeight = input_params.height,
                .imageFmt = IMPP_PIX_FMT_NV12,
        };

        ret = IHal_CameraSetParams(camera_handle, &camera_params);
        if (ret) {
                printf("Camera set params fialed\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        // camera request buffer under mmap mode
        int num_buffers = 0;
        num_buffers = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, input_params.num_buffers);
        if (num_buffers < 1) {
                printf("Camera create buffers failed\r\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        ret = IHal_CodecInit();
        if (ret) {
                printf("codec init error\n");
                IHal_CameraClose(camera_handle);
                return -1;
        }

        handle = IHal_CodecCreate(H264_ENC);
        if (!handle) {
                printf("codec create failed\n");
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        IHal_CodecParam param;
        memset(&param, 0, sizeof(IHal_CodecParam));
        param.codec_type = H264_ENC;
        param.codecparam.h264e_param.rc_mode = IMPP_ENC_RC_MODE_VBR;
        param.codecparam.h264e_param.target_bitrate = input_params.target_bitrate;
        param.codecparam.h264e_param.max_bitrate    = input_params.target_bitrate + 1000;
        param.codecparam.h264e_param.gop_len        = input_params.gop_len;
        param.codecparam.h264e_param.initial_Qp     = 25;
        param.codecparam.h264e_param.mini_Qp        = 10;
        param.codecparam.h264e_param.max_Qp         = 40;
        param.codecparam.h264e_param.level          = 20;
        param.codecparam.h264e_param.maxPSNR        = 40;
        param.codecparam.h264e_param.freqIDR        = 30;

        param.codecparam.h264e_param.frameRateNum   = input_params.frame_rate_num;
        param.codecparam.h264e_param.frameRateDen   = 1;
        /* param.codecparam.h264e_param.maxPictureSize = WIDTH * HEIGHT; */
        param.codecparam.h264e_param.enc_width      = camera_params.imageWidth;
        param.codecparam.h264e_param.enc_height     = camera_params.imageHeight;
        param.codecparam.h264e_param.src_width      = camera_params.imageWidth;
        param.codecparam.h264e_param.src_height     = camera_params.imageHeight;
        param.codecparam.h264e_param.src_fmt        = IMPP_PIX_FMT_NV12;

        ret =  IHal_Codec_SetParams(handle, &param);
        if (ret) {
                printf("set codec param failed\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        int srcbuf_nums = IHal_Codec_CreateSrcBuffers(handle, IMPP_EXT_DMABUFFER, num_buffers);
        if (srcbuf_nums < 1) {
                printf("src buffer create failed\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        // dmabuf import to codec
        IMPP_BufferInfo_t share;
        for (int i = 0; i < num_buffers; i++) {
                ret = IHal_GetCameraBuffers(camera_handle, i, &share);
                if (ret) {
                        printf("get camera buffer failed");
                        IHal_CodecDestroy(handle);
                        IHal_CodecDeInit();
                        IHal_CameraClose(camera_handle);
                        return -1;
                }
                printf("v4l2 expt-dma-fd = %d\n", share.fd);
                ret = IHal_Codec_SetSrcBuffer(handle, i, &share);
                if (ret) {
                        printf("set h264 encoder src buffer failed");
                        IHal_CodecDestroy(handle);
                        IHal_CodecDeInit();
                        IHal_CameraClose(camera_handle);
                        return -1;
                }
        }

        int dstbuf_nums = IHal_Codec_CreateDstBuffer(handle, IMPP_INTERNAL_BUFFER, num_buffers);
        if (dstbuf_nums < 1) {
                printf("dst buffer create failed\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        /* rtsp init  */
        rtsp_cfg cfg;
        memset(cfg.ipaddr, 0, sizeof(cfg.ipaddr));
        strcpy(cfg.ipaddr, input_params.ipaddr);
        cfg.ip_len = strlen(cfg.ipaddr);
        cfg.session_count = 1;
        cfg.session_cfg[0].port = input_params.port;
        cfg.session_cfg[0].videoCodec = RTSP_CODEC_ID_VIDEO_H264;
        cfg.session_cfg[0].audioCodec = RTSP_CODEC_ID_NONE;
        sprintf(cfg.session_cfg[0].suffix, "/live/0/h264");

        rtsp_sdp sdp;
        sdp.session_count = 1;

        ret = rtsp_init(&cfg, &sdp, NULL, NULL);
        if (ret) {
                printf("rtsp init fialed\r\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        ret = IHal_CameraStart(camera_handle);
        if (ret) {
                printf("camera start failed \n");
                rtsp_uinit();
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        ret = IHal_Codec_Start(handle);
        if (ret) {
                printf("codec start failed\n");
                rtsp_uinit();
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                return -1;
        }

        signal(SIGINT, signal_handle);

        pthread_create(&work_tid, NULL, work_thread, (void *)handle);
        pthread_create(&input_tid, NULL, input_thread, (void *)camera_handle);

        IHAL_CodecStreamInfo_t stream;

        unsigned int count = 0;
        int i = 0;
        struct timeval tv;
        while (1) {
                ret = IHal_Codec_WaitDstAvailable(handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueDstBuffer(handle, &stream);
                        printf("pack num = %d \r\n", stream.pack.packcnt);
                        //ret = rtsp_server_write_video(0,(void*)stream.vaddr,stream.size,tv.tv_sec * 1000 + tv.tv_usec / 1000);
                        for (i = 0; i < stream.pack.packcnt; i++) {
                                gettimeofday(&tv, NULL);
                                ret = rtsp_server_write_video(0, (void *)(stream.vaddr + stream.pack.pack[i].offset), stream.pack.pack[i].length, tv.tv_sec * 1000 + tv.tv_usec / 1000);
                        }
                        IHal_Codec_QueueDstBuffer(handle, &stream);
                } else {
                        usleep(5000);
                }
        }

        return 0;
}

