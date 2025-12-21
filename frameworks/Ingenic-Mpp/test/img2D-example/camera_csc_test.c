#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <ipu.h>
#include <icamera.h>

struct configInfo {
        char *video_node;
        unsigned int sWidth;
        unsigned int sHeight;
        unsigned int num_buffers;
        IMPP_PIX_FMT dstfmt;
        char *dstfile;
};

static struct option long_options[] = {
        { "video_node ", required_argument, NULL, 'i'},
        { "pixel_size ", required_argument, NULL, 's'},
        { "num_buffers", required_argument, NULL, 'n'},
        { "dst_fmt    ", required_argument, NULL, 'f'},
        { "dst_file   ", required_argument, NULL, 'o'},
        { "help       ", no_argument,       NULL, 'h'},
};

#define print_opt_help(opt_index, help_str)                             \
        do {                                                            \
                        printf("   -%c or --%s          %s", (char)long_options[opt_index].val, long_options[opt_index].name, help_str); \
        } while (0)

static void usage()
{
        printf("\ncam-csc-example usage:\n");
        print_opt_help(0, "Input camera device. The default is /dev/video4.\n");
        print_opt_help(1, "Camera pixel resolution size. The format is WxH. The default is 1280x720.\n");
        print_opt_help(2, "Num of buffers to use. The default is 3.\n");
        print_opt_help(3, "Target Format. The default value is RGBA8888.\n");
        print_opt_help(4, "The output file. The default is cam_1280x720_csc_out.rgba.\n");
        print_opt_help(5, "Help.\n");
}

static IMPP_PIX_FMT csc_str_to_imppfmt(const char *strfmt)
{
        assert(strfmt);

        if (0 == strcasecmp("ABGR8888", optarg)) {
                return IMPP_PIX_FMT_ABGR_8888;
        } else if (0 == strcasecmp("ARGB8888", optarg)) {
                return IMPP_PIX_FMT_ARGB_8888;
        } else if (0 == strcasecmp("BGRA8888", optarg)) {
                return IMPP_PIX_FMT_BGRA_8888;
        } else if (0 == strcasecmp("BGRX8888", optarg)) {
                return IMPP_PIX_FMT_BGRX_8888;
        } else if (0 == strcasecmp("HSV", optarg)) {
                return IMPP_PIX_FMT_HSV;
        } else if (0 == strcasecmp("NV12", optarg)) {
                return IMPP_PIX_FMT_NV12;
        } else if (0 == strcasecmp("NV21", optarg)) {
                return IMPP_PIX_FMT_NV21;
        } else if (0 == strcasecmp("RGBA8888", optarg)) {
                return IMPP_PIX_FMT_RGBA_8888;
        } else if (0 == strcasecmp("RGBX8888", optarg)) {
                return IMPP_PIX_FMT_RGBX_8888;
        } else {
                printf("Format not supported!\r\n");
                return -1;
        }
}

static struct configInfo input_param;

static int get_paramt(int argc, char *argv[])
{
        int ret = 0;

        memset(&input_param, 0, sizeof(struct configInfo));
        input_param.video_node = "/dev/video4";
        input_param.sWidth = 1280;
        input_param.sHeight = 720;
        input_param.num_buffers = 3;
        input_param.dstfmt = IMPP_PIX_FMT_RGBA_8888;
        input_param.dstfile = "cam_1280x720_csc_out.rgba";

        char optstring[] = "i:s:n:f:o:h";
        while ((ret = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
                switch (ret) {
                case 'i':
                        input_param.video_node = optarg;
                        break;
                case 's':
                        if (2 != sscanf(optarg, "%dx%d", &input_param.sWidth, &input_param.sHeight)) {
                                printf("The resolution format of the source image is incorrect!\r\n");
                                exit(-1);
                        }
                        break;
                case 'n':
                        input_param.num_buffers = atoi(optarg);
                        break;
                case 'f':
                        input_param.dstfmt = csc_str_to_imppfmt(optarg);
                        if (-1 == input_param.dstfmt) {
                                exit(-1);
                        }
                        break;
                case 'o':
                        input_param.dstfile = optarg;
                        break;
                case 'h':
                        usage();
                        exit(0);
                case '?':
                        usage();
                        exit(-1);
                }
        }

        return 0;
}


static IHAL_CameraHandle_t *camera_handle = NULL;
static IHAL_CAMERA_PARAMS   camera_params;
static IHal_CSC_Handle_t *csc_handle = NULL;
static IHal_CSC_ChanAttr_t csc_chan_attr;
static unsigned int num_buffers;
static int dstfd = -1;
static pthread_t camera_csc_thid;

void *camera_csc_thread(void *arg)
{
        int ret = 0;
        IMPP_BufferInfo_t    camera_buf;
        IMPP_FrameInfo_t     csc_sframe;

        while (1) {
                memset(&camera_buf, 0, sizeof(camera_buf));
                memset(&csc_sframe, 0, sizeof(IMPP_FrameInfo_t));

                ret = IHal_Camera_WaitBufferAvailable(camera_handle, IMPP_WAIT_FOREVER);
                if (!ret) {
                        IHal_CameraDeQueueBuffer(camera_handle, &camera_buf);
                        csc_sframe.fd = camera_buf.fd;
                        csc_sframe.vaddr = camera_buf.vaddr;
                        csc_sframe.paddr = camera_buf.paddr;
                        csc_sframe.index = camera_buf.index;
                        csc_sframe.size = camera_buf.size;
                        csc_sframe.fmt = IMPP_PIX_FMT_NV12;
                        ret = IHal_CSC_FrameProcess(csc_handle, &csc_sframe);
                        if (!ret) {
                                IHal_CameraQueuebuffer(camera_handle, &camera_buf);
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

        camera_handle = IHal_CameraOpen(input_param.video_node);
        if (!camera_handle) {
                printf("Open camera %s failed!\r\n", input_param.video_node);
                return -1;
        }

        memset(&camera_params, 0, sizeof(camera_params));
        camera_params.imageWidth = input_param.sWidth;
        camera_params.imageHeight = input_param.sHeight;
        camera_params.imageFmt = IMPP_PIX_FMT_NV12;
        IHal_CameraSetParams(camera_handle, &camera_params);

        num_buffers = IHal_CameraCreateBuffers(camera_handle, IMPP_INTERNAL_BUFFER, input_param.num_buffers);
        if (0 > num_buffers) {
                printf("Camera create buffers failed!\r\n");
                goto close_camera;
        }

        memset(&csc_chan_attr, 0, sizeof(csc_chan_attr));
        csc_chan_attr.sWidth = camera_params.imageWidth;
        csc_chan_attr.sHeight = camera_params.imageHeight;
        csc_chan_attr.isSrcUseExtDmaBuf = 1;
        csc_chan_attr.isDstUseExtDmaBuf = 0;
        csc_chan_attr.numSrcBuf = num_buffers;
        csc_chan_attr.numDstBuf = num_buffers;
        csc_chan_attr.srcFmt = camera_params.imageFmt;
        csc_chan_attr.dstFmt = input_param.dstfmt;
        csc_handle = IHal_CSC_ChanCreate(&csc_chan_attr);
        if (!csc_handle) {
                printf("Create csc channel failed!\r\n");
                goto close_camera;
        }

        IMPP_BufferInfo_t share_buf;
        for (int i = 0; i < num_buffers; i++) {
                memset(&share_buf, 0, sizeof(share_buf));
                IHal_GetCameraBuffers(camera_handle, i, &share_buf);
                IHal_CSC_SetSrcExtBuf(csc_handle, &share_buf, i);
        }

        IHal_CameraStart(camera_handle);

        dstfd = open(input_param.dstfile, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (-1 == dstfd) {
                printf("Open dstfile failed!\r\n");
                goto stop_camera;
        }

        pthread_create(&camera_csc_thid,   NULL, camera_csc_thread,   NULL);

        IMPP_FrameInfo_t     csc_dframe;
        for (int i = 0; i < 150; i++) {
                memset(&csc_dframe, 0, sizeof(IMPP_FrameInfo_t));
                IHal_CSC_GetDstFrame(csc_handle, &csc_dframe, IMPP_WAIT_FOREVER);
                lseek(dstfd, 0, SEEK_SET);
                write(dstfd, (void *)csc_dframe.vaddr, csc_dframe.size);
                IHal_CSC_ReleaseDstFrame(csc_handle, &csc_dframe);
                printf("The processing of frame %d data is completed......\r\n", i + 1);
        }

        pthread_cancel(camera_csc_thid);
        pthread_join(camera_csc_thid, NULL);
        close(dstfd);
        IHal_CameraStop(camera_handle);
        IHal_CSC_DestroyChan(csc_handle);
        IHal_CameraClose(camera_handle);

        return 0;

stop_camera:
        IHal_CameraStop(camera_handle);
        IHal_CSC_DestroyChan(csc_handle);
close_camera:
        IHal_CameraClose(camera_handle);

        return -1;
}

