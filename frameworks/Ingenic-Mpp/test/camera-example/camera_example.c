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
#include <icamera.h>

static void print_help(void)
{
        printf("Usage: impp-camera-example [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -i                 input camera device.\n"
               " -f                 pix fmt string. If the default NV12 format is not set.\n"
               " -w                 width of image.(default 1280)\n"
               " -h                 height of image.(default 720)\n"
               " -n                 num of buffers to use. default:3\n"
               " -o                 output yuv image filename.(default camera_test.yuv)\n"
               " -H                 help.\n\n"
               "Camera presets:\n");
}


static unsigned long long get_time_ms(void)
{
        struct timeval tv;
        gettimeofday(&tv, NULL);

        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int calc_fps(void)
{

        static unsigned long long last_time = 0;
        static int framecount = 0;
        static int fps = 0;
        unsigned long long cur_time = get_time_ms();

        if (last_time == 0) {
                last_time = get_time_ms();
        }

        framecount ++;
        if (cur_time - last_time >= 1000) {
                fps = framecount;
                framecount = 0;
                last_time = cur_time;
                printf("fps:%d\n", fps);
        }

        return fps;
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;

        char *input_camera_node = NULL;
        IHAL_INT32 image_width = 1280;
        IHAL_INT32 image_height = 720;
        char *output_yuv_filename = "camera_test.yuv";
        char *fmt_str = NULL;
        int num_buffers = 3;

        while (1) {
                opt = getopt(argc, argv, "i:f:w:h:n:o:H");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'i':
                        input_camera_node = optarg;
                        break;
                case 'f':
                        fmt_str = optarg;
                        break;
                case 'w':
                        image_width = atoi(optarg);
                        break;
                case 'h':
                        image_height = atoi(optarg);
                        break;
                case 'n':
                        num_buffers = atoi(optarg);
                        break;
                case 'o':
                        output_yuv_filename = optarg;
                        break;
                case 'H':
                        print_help();
                        return 0;
                case '?':
                        print_help();
                        return -1;
                }
        }

        if (NULL == input_camera_node) {
                print_help();
                return -1;
        }

        int yuvfd = open(output_yuv_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);

        IHAL_CameraHandle_t *handle = IHal_CameraOpen(input_camera_node);
        if (!handle) {
                return -1;
        }

        IHAL_CAMERA_PARAMS params = {
                .imageWidth = image_width,
                .imageHeight = image_height,
                .imageFmt = IMPP_PIX_FMT_NV12,
                .imageFmtStr = fmt_str,
        };

        ret = IHal_CameraSetParams(handle, &params);
        if (ret) {
                goto close;
        }

        ret = IHal_CameraCreateBuffers(handle, IMPP_INTERNAL_BUFFER, num_buffers);
        if (ret < 1) {
                goto close;
        }

        ret = IHal_CameraStart(handle);
        if (ret != IHAL_ROK) {
                goto close;
        }

        IMPP_BufferInfo_t camerabuf;
        while (1) {
                IHal_Camera_WaitBufferAvailable(handle, IMPP_WAIT_FOREVER);
                ret = IHal_CameraDeQueueBuffer(handle, &camerabuf);
                if (ret != IHAL_ROK) {
                        break;
                }

                calc_fps();

                lseek(yuvfd, 0, SEEK_SET);
                write(yuvfd, camerabuf.vaddr, camerabuf.size);
                ret = IHal_CameraQueuebuffer(handle, &camerabuf);
                if (ret != IHAL_ROK) {
                        break;
                }
        }

        ret = IHal_CameraStop(handle);
        if (ret) {
                goto close;
        }
        close(yuvfd);
close:
        IHal_CameraClose(handle);
        close(yuvfd);
        return 0;
}
