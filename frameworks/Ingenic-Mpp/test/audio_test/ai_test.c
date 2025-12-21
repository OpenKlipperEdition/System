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
#include <impp.h>
#include <audio.h>

static int fd;
static IHal_AudioHandle_t *ai_handle;

static void print_help(void)
{
        printf("Usage: ai_test [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -a             audio node.[dmic/chn0]\n"
               " -r             sample rate.(default 16000)\n"
               " -n             number of channels.(default 2)\n"
               " -v             Recording volume gain. If not set, the configuration will not be modified.\n"
               " -o             output pcm filename.(default ai_test_save.pcm)\n"
               " -d             user ring buffer num. If not set default as 20, max as 50\n"
               " -h             help\n\n"
               "Audio presets:\n");
}

static int signal_handle(int sig)
{
        IHal_AI_ChanStop(ai_handle);
        IHal_AI_ChanDestroy(ai_handle);
        close(fd);
        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;

        char *output_path = "ai_test_save.pcm";
        IHAL_INT32 buffer_deep = 20;
        IHAL_UINT32 sample_rate = 16000;
        IHAL_UINT32 channels = 2;
        IHAL_UINT32 recording_volume = -1;

        IHal_AI_Attr_t ai_attr;
		memset(&ai_attr, 0, sizeof(ai_attr));

        while (1) {
                opt = getopt(argc, argv, "a:r:n:v:o:d:h");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'a':
                        if (0 == strcmp("dmic", optarg)) {
                                ai_attr.audio_node = "plug:cap_dmic";        // dmic support 1/2/3/4 channel ,samplerate : 8K/16K
                        } else if (0 == strcmp("chn0", optarg)) {
                                ai_attr.audio_node = "plug:cap_chn0";
                        } else {
                                print_help();
                                return -1;
                        }
                        break;
                case 'r':
                        sample_rate = atoi(optarg);
                        break;
                case 'n':
                        channels = atoi(optarg);
                        break;
                case 'v':
                        recording_volume = atoi(optarg);
                        break;
                case 'o':
                        output_path = optarg;
                        break;
                case 'd':
                        buffer_deep = atoi(optarg);
                        break;
                case 'h':
                        print_help();
                        return 0;
                case '?':
                        print_help();
                        return -1;
                }
        }

        if (NULL == ai_attr.audio_node || \
			(0 != strcmp("plug:cap_dmic", ai_attr.audio_node) && 0 != strcmp("plug:cap_chn0", ai_attr.audio_node))) {
                print_help();
                return -1;
        }

        ai_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ai_attr.SampleRate = sample_rate;
        ai_attr.numPerSample = 512;
        ai_attr.channels = channels;
        ai_attr.bufferDeep = buffer_deep;

        ai_handle = IHal_AI_ChanCreate(&ai_attr);
        if (!ai_handle) {
                printf("ai channel create failed\r\n");
                return -1;
        }

        if (0 != strcmp("plug:cap_dmic", ai_attr.audio_node) && -1 != recording_volume) {
                IHal_AI_SetGain(ai_handle, recording_volume);
                int val = 0;
                IHal_AI_GetGain(ai_handle,&val);
        }

        signal(SIGINT, signal_handle);
        ret = IHal_AI_ChanStart(ai_handle);
        if (ret) {
                printf("ai start failed\r\n");
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        fd = open(output_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
                printf("open save file failed");
                IHal_AI_ChanStop(ai_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        int times = 500;

        IHal_AudioBuffer_t buf;
        while (times--) {
                ret = IHal_AI_GetBuffer(ai_handle, &buf, IMPP_WAIT_FOREVER);
                if (!ret) {
                        printf("get buffer ok data size = %d \n", buf.datalen);
                        write(fd, buf.vaddr, buf.datalen);
                        IHal_AI_ReleaseBuffer(ai_handle, &buf);
                }
        }
        IHal_AI_ChanStop(ai_handle);
        IHal_AI_ChanDestroy(ai_handle);
        close(fd);

        return 0;
}

