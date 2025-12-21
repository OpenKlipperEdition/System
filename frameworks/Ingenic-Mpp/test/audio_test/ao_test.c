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
static IHal_AudioHandle_t *ao_handle;

static void print_help(void)
{
        printf("Usage: ao_test [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -i             input pcm filename.\n"
               " -r             sample rate.(default 16000)\n"
               " -n             number of channels.(default 2)\n"
               " -v             Playback volume. If not set default as 50, and the value ranges from 0 to 100.\n"
               " -d             user ring buffer num. If not set default as 20, max as 50\n"
               " -h             help.\n\n"
               "Audio presets:\n");
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;

        char *input_file = NULL;
        IHAL_INT32 buffer_deep = 20;
        IHAL_INT32 playbak_volume = 50;
        IHAL_UINT32 sample_rate = 16000;
        IHAL_UINT32 channels = 2;

        while (1) {
                opt = getopt(argc, argv, "i:r:n:d:v:h");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'i':
                        input_file = optarg;
                        break;
                case 'r':
                        sample_rate = atoi(optarg);
                        break;
                case 'n':
                        channels = atoi(optarg);
                        break;
                case 'd':
                        buffer_deep = atoi(optarg);
                        break;
                case 'v':
                        playbak_volume = atoi(optarg);
                        break;
                case 'h':
                        print_help();
                        return 0;
                case '?':
                        print_help();
                        return -1;
                }
        }

        if (NULL == input_file) {
                print_help();
                return -1;
        }

        IHal_AO_Attr_t ao_attr;
        ao_attr.audio_node = "plug:play_chn0";
        ao_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ao_attr.SampleRate = sample_rate;
        ao_attr.numPerSample = 512;
        ao_attr.channels = channels;
        ao_attr.bufferDeep = buffer_deep;

        ao_handle = IHal_AO_ChanCreate(&ao_attr);
        if (!ao_handle) {
                printf("ao channel create failed\r\n");
                return -1;
        }

        ret = IHal_AO_ChanStart(ao_handle);
        if (ret) {
                printf("ao start failed\r\n");
                return -1;
        }

        fd = open(input_file, O_RDONLY);
        if (fd < 0) {
                printf("open save file failed \r\n");
                return -1;
        }
        IHal_AO_SetVolume(ao_handle, playbak_volume);
        int times = 500;
        IHal_AudioBuffer_t buf;
        IHal_AudioFrm_t frm;
        unsigned int datasize = 16 / 8 * ao_attr.channels * ao_attr.numPerSample;  // bytewidth * channels * numpersample
        char *buffer = (char *)malloc(datasize);
        unsigned int readsize = 0;
        readsize = read(fd, buffer, datasize);
        while (readsize > 0) {
                printf("read data size = %d \n", readsize);
                frm.vaddr = buffer;
                frm.datalen = readsize;
                IHal_AO_ChanWriteData(ao_handle, &frm, IMPP_WAIT_FOREVER);
                readsize = read(fd, buffer, datasize);
        }
        IHal_AO_BufferFlush(ao_handle);
        IHal_AO_ChanStop(ao_handle);
        IHal_AO_ChanDestroy(ao_handle);
        free(buffer);
        return 0;
}

