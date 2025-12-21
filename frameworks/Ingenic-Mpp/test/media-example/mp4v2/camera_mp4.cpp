#define _FILE_OFFSET_BITS 64
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

extern "C" {
#include "log.h"
#include "icamera.h"
#include "codec.h"
#include "audio.h"
#include "mp4Encoder.h"
#include <mp4v2.h>
}


struct configInfo {
        char *audio_node;
        int  audio_sampleRate;
        int  audio_channels;
        int  audio_bufferDeep;
        char *video_node;
        int      width;
        int  height;
        int num_buffers;
        int target_bitrate;
        int frame_rate_num;
        int gop_len;
        char *outfilename;
};


static IHAL_CameraHandle_t *camera_handle = NULL;
static IHal_CodecHandle_t *handle = NULL;
static IHal_AudioHandle_t *ai_handle;
static IHal_AudioEnc_Handle_t *aenc_handle;
static IHal_Mp4EncoderHandle_t *mp4Handle;
static struct configInfo input_params;

pthread_t input_tid;
pthread_t work_tid;
pthread_t audio_tid;
pthread_t audio_write_tid;

static struct option long_options[] = {
        { "audio_node    ", required_argument, NULL, 'D'},
        { "sample_rate   ", required_argument, NULL, 'r'},
        { "channels      ", required_argument, NULL, 'c'},
        { "buffer_deep   ", required_argument, NULL, 'd'},
        { "video_node    ", required_argument, NULL, 'i'},
        { "pixel_size    ", required_argument, NULL, 's'},
        { "num_buffers   ", required_argument, NULL, 'n'},
        { "target_bitrate", required_argument, NULL, 'R'},
        { "frame_rate    ", required_argument, NULL, 'f'},
        { "gop_len       ", required_argument, NULL, 'g'},
        { "output_file   ", required_argument, NULL, 'o'},
        { "help          ", no_argument,       NULL, 'h'},
        {NULL, 0, NULL, 0},
};

#define print_opt_help(opt_index, help_str)             \
        do {                                \
                        printf("   -%c or --%s          %s", (char)long_options[opt_index].val, long_options[opt_index].name, help_str); \
        } while (0)

static void usage()
{
        printf("\ncamera-mp4-recoder usage:\n");
        print_opt_help(0,  "Select audio node. The default value is plug:cap_chn0.\n");
        print_opt_help(1,  "Audio sampling rate in Hertz. The default rate is 16000 Hertz.\n");
        print_opt_help(2,  "The number of audio channels. The default is 2 channels.\n");
        print_opt_help(3,  "Audio buffer num. The default is 16.\n");
        print_opt_help(4,  "Video input node. The default value is /dev/video4.\n");
        print_opt_help(5,  "Camera pixel resolution size. The format is WxH. The default is 1280x720.\n");
        print_opt_help(6,  "Num of buffers to use. The default is 3.\n");
        print_opt_help(7,  "H264 encoding target bitrate. The default value is 2000.\n");
        print_opt_help(8,  "H264 encoding frame rate num. The default value is 30 and the maximum value is 30.\n");
        print_opt_help(9,  "H264 encoding gop length. The default is 60.\n");
        print_opt_help(10, "Output file name and its path. The default is test.mp4.\n");
        print_opt_help(11, "Show help.\n");
}

static int get_paramt(int argc, char *argv[])
{
        int opt = 0;
        memset(&input_params, 0, sizeof(struct configInfo));

        /* The default value*/
        input_params.audio_node = "plug:cap_chn0";
        input_params.audio_sampleRate = 16000;
        input_params.audio_channels = 2;
        input_params.audio_bufferDeep = 16;
        input_params.video_node = "/dev/video4";
        input_params.width = 1280;
        input_params.height = 720;
        input_params.num_buffers = 3;
        input_params.target_bitrate = 2000;
        input_params.frame_rate_num = 30;
        input_params.gop_len = 60;
        input_params.outfilename = "test.mp4";

        char optstring[] = "D:r:c:d:i:s:n:R:f:g:o:h";
        while ((opt = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
                switch (opt) {
                case 'D':
                        input_params.audio_node = optarg;
                        break;
                case 'r':
                        input_params.audio_sampleRate = atoi(optarg);
                        break;
                case 'c':
                        input_params.audio_channels = atoi(optarg);
                        break;
                case 'd':
                        input_params.audio_bufferDeep = atoi(optarg);
                        break;
                case 'i':
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
                case 'R':
                        input_params.target_bitrate = atoi(optarg);
                        break;
                case 'f':
                        input_params.frame_rate_num = atoi(optarg);
                        break;
                case 'g':
                        input_params.gop_len = atoi(optarg);
                        break;
                case 'o':
                        input_params.outfilename = optarg;
                        break;
                case 'H':
                        usage();
                        exit(0);
                case '?':
                default:
                        usage();
                        exit(-1);
                }
        }

        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
        printf("Audio_node : %s\r\n", input_params.audio_node);
        printf("Audio_sampleRate = %d\t Audio_channels = %d\t audio_bufferDeep = %d\r\n", \
               input_params.audio_sampleRate, input_params.audio_channels, input_params.audio_bufferDeep);
        printf("Video_node : %s\r\n", input_params.video_node);
        printf("Width = %d\t Height = %d\r\n", input_params.width, input_params.height);
        printf("num_buffers : %d\r\n", input_params.num_buffers);
        printf("target_bitrate : %d     frame_rate_num : %d     gop_len : %d\r\n", \
               input_params.target_bitrate, input_params.frame_rate_num, input_params.gop_len);
        printf("Output file name : %s\r\n", input_params.outfilename);
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");

        return 0;
}


static void signal_handle(int sig)
{
        int ret = 0;
        pthread_cancel(work_tid);
        pthread_join(work_tid, NULL);
        pthread_cancel(input_tid);
        pthread_join(input_tid, NULL);
        pthread_cancel(audio_tid);
        pthread_join(audio_tid, NULL);
        pthread_cancel(audio_write_tid);
        pthread_join(audio_write_tid, NULL);

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
        IHal_AI_ChanStop(ai_handle);
        IHal_AI_ChanDestroy(ai_handle);
        IHal_AudioEnc_DestroyChn(aenc_handle);
        IHal_Mp4FileClose(mp4Handle);
        exit(0);
}

void *audio_cap_thread(void *arg)
{
        int ret = 0;
        IHal_AudioBuffer_t buf;
        IHal_AudioStream_t frame;
        for (;;) {
                ret = IHal_AI_GetBuffer(ai_handle, &buf, IMPP_NO_WAIT);
                if (!ret) {
                        frame.vaddr = buf.vaddr;
                        frame.size = buf.datalen;
                        IHal_AudioEnc_PutFrame(aenc_handle, &frame, IMPP_WAIT_FOREVER);
                        IHal_AI_ReleaseBuffer(ai_handle, &buf);
                } else {
                        usleep(10000);
                }
        }
}

void *audio_write_thread(void *arg)
{
        IHal_AudioStream_t stream;
        int ret = 0;
        for (;;) {
                ret = IHal_AudioEnc_GetStream(aenc_handle, &stream, IMPP_NO_WAIT);
                if (!ret) {
                        IHal_Mp4WriteAAC(mp4Handle, (unsigned char *)stream.vaddr, stream.size);
                        IHal_AudioEnc_ReleaseStream(aenc_handle, &stream);
                        printf("audio write ok !!!!!\r\n");
                } else {
                        usleep(5000);
                }
        }
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
                        usleep(10000);
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

        get_paramt(argc, argv);

        int times = 800;
        IHal_AI_Attr_t ai_attr;
        memset(&ai_attr, 0, sizeof(IHal_AI_Attr_t));
        ai_attr.audio_node = input_params.audio_node;
        ai_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ai_attr.SampleRate = input_params.audio_sampleRate;
        ai_attr.numPerSample = 1024;
        ai_attr.channels = input_params.audio_channels;
        ai_attr.bufferDeep = input_params.audio_bufferDeep;
        ai_handle = IHal_AI_ChanCreate(&ai_attr);
        if (!ai_handle) {
                printf("ai channel create failed\r\n");
                return -1;
        }

        IHal_AudioEnc_Attr_t aenc_attr;
        memset(&aenc_attr, 0, sizeof(IHal_AudioEnc_Attr_t));
        aenc_attr.type = Codec_AAC_ADIF;
        aenc_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;              // src pcm-data format
        aenc_attr.BitRate = 16000;
        aenc_attr.SampleRate = input_params.audio_sampleRate;
        aenc_attr.Quality = 0;
        aenc_attr.Channels = input_params.audio_channels;
        aenc_attr.numPerSample = 1024;
        aenc_attr.bufsize = input_params.audio_bufferDeep;              // the num of buffer
        aenc_handle = IHal_AudioEnc_CreateChn(&aenc_attr);
        if (!aenc_handle) {
                printf("audio encoder chn create failed\r\n");
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        camera_handle = IHal_CameraOpen(input_params.video_node);
        if (!camera_handle) {
                printf("open camera %s failed\r\n", input_params.video_node);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        IHAL_CAMERA_PARAMS camera_params;
        memset(&camera_params, 0, sizeof(IHAL_CAMERA_PARAMS));
        camera_params.imageWidth = input_params.width;
        camera_params.imageHeight = input_params.height;
        camera_params.imageFmt = IMPP_PIX_FMT_NV12;
        camera_params.wdr_enable = 0;
        camera_params.bin_select = 0;
        //memset(camera_params.binpath,0,sizeof(camera_params.binpath));
        ret = IHal_CameraSetParams(camera_handle, &camera_params);
        if (ret) {
                printf("Camera set params failed!\r\n");
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        // camera request buffer under mmap mode
        int num_buffers = 0;
        num_buffers = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, input_params.num_buffers);
        if (num_buffers < 1) {
                printf("Camera create buffers failed!\r\n");
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        ret = IHal_CodecInit();
        if (ret) {
                printf("codec init error\n");
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }
        handle = IHal_CodecCreate(H264_ENC);
        if (!handle) {
                printf("codec create failed\n");
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

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
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        int srcbuf_nums = IHal_Codec_CreateSrcBuffers(handle, IMPP_EXT_DMABUFFER, num_buffers);
        if (srcbuf_nums < 1) {
                printf("src buffer create failed\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        // dmabuf import to codec
        IMPP_BufferInfo_t share;
        for (int i = 0; i < num_buffers; i++) {
                ret = IHal_GetCameraBuffers(camera_handle, i, &share);
                if (ret) {
                        printf("get camera buffer failed");
                        IHal_CodecDestroy(handle);
                        IHal_CodecDestroy(handle);
                        IHal_CodecDeInit();
                        IHal_CameraClose(camera_handle);
                        IHal_AudioEnc_DestroyChn(aenc_handle);
                        IHal_AI_ChanDestroy(ai_handle);

                        return -1;
                }
                ret = IHal_Codec_SetSrcBuffer(handle, i, &share);
                if (ret) {
                        printf("set h264 encoder src buffer failed");
                        IHal_CodecDestroy(handle);
                        IHal_CodecDeInit();
                        IHal_CameraClose(camera_handle);
                        IHal_AudioEnc_DestroyChn(aenc_handle);
                        IHal_AI_ChanDestroy(ai_handle);

                        return -1;
                }
                printf("v4l2 expt-dma-fd = %d ----------------------------------\r\n", share.fd);
        }

        int dstbuf_nums = IHal_Codec_CreateDstBuffer(handle, IMPP_INTERNAL_BUFFER, num_buffers);
        if (dstbuf_nums < 1) {
                printf("dst buffer create failed\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        ret = IHal_AI_ChanStart(ai_handle);
        if (ret) {
                printf("ai start failed\r\n");
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        ret = IHal_CameraStart(camera_handle);
        if (ret) {
                printf("camera start failed \n");
                IHal_AI_ChanStop(ai_handle);
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        ret = IHal_Codec_Start(handle);
        if (ret) {
                printf("codec start failed\n");
                IHal_CameraStop(camera_handle);
                IHal_AI_ChanStop(ai_handle);
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        IHal_Mp4EncoderInit_t mp4_init;
        memset(&mp4_init, 0, sizeof(mp4_init));
        mp4_init.videowidth = input_params.width;
        mp4_init.videoheight = input_params.height;
        mp4_init.timescale = 90000;
        mp4_init.framerate = input_params.frame_rate_num;
        mp4_init.audioSamplerate = input_params.audio_sampleRate;
        memcpy(mp4_init.filename, input_params.outfilename, strlen(input_params.outfilename));
        mp4Handle = IHal_Mp4FileCreate(&mp4_init);
        if (!mp4Handle) {
                printf("mp4 init error\r\n");
                IHal_Codec_Stop(handle);
                IHal_CameraStop(camera_handle);
                IHal_AI_ChanStop(ai_handle);
                IHal_CodecDestroy(handle);
                IHal_CodecDeInit();
                IHal_CameraClose(camera_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        signal(SIGINT, signal_handle);

        pthread_create(&work_tid, NULL, work_thread, (void *)handle);
        pthread_create(&input_tid, NULL, input_thread, (void *)camera_handle);
        pthread_create(&audio_tid, NULL, audio_cap_thread, NULL);
        pthread_create(&audio_write_tid, NULL, audio_write_thread, NULL);

        /* pthread_create(&out_tid,NULL,out_thread,NULL); */
        IHAL_CodecStreamInfo_t stream;
        IHal_AudioStream_t audiostream;
        unsigned char *pBuffer;
        unsigned int packlen = 0;
        while (times--) {
                ret = IHal_Codec_WaitDstAvailable(handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_Codec_DequeueDstBuffer(handle, &stream);
                        /* printf("pack num = %d \r\n",stream.pack.packcnt); */
                        for (int i = 0; i < stream.pack.packcnt; i++) {
                                pBuffer = (unsigned char *)(stream.vaddr + stream.pack.pack[i].offset);
                                packlen = stream.pack.pack[i].length;
                                IHal_Mp4WriteH264(mp4Handle, pBuffer, packlen);
                                printf("video write ok !!!!!\r\n");
                        }
                        IHal_Codec_QueueDstBuffer(handle, &stream);
                }
        }
        signal_handle(0);

        return 0;
}

