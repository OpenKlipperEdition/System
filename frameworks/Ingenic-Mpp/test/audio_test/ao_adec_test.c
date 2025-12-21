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
static IHal_AudioDec_Handle_t *adec_handle;

#define G726_RATIO_16K  8
#define ADPCM_RATIO     4

struct configInfo {
        int ratio;
        IHal_AudioCodecType_t decoding_fmt;
        char *input_file;
        IHAL_INT32 buffer_deep;
        IHAL_INT32 playbak_volume;
        IHAL_UINT32 sample_rate;
        IHAL_UINT32 channels;
};

static struct configInfo input_params;

static void print_help(void)
{
        printf("Usage: ao_adec_test [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -f             Decoding format. If not set default as adpcm.\n"
               " -i             Input the encoding filename.\n"
               " -r             sample rate.(default 16000)\n"
               " -n             number of channels.(default 2)\n"
               " -v             Playback volume. If not set default as 50, and the value ranges from 0 to 100.\n"
               " -d             user ring buffer num. If not set default as 20, max as 50\n"
               " -h             help.\n\n"
               "Audio decoding presets:\n");
}

static int get_paramt(int argc, char *argv[])
{
        int ret = 0;
        int opt = 0;
        memset(&input_params, 0, sizeof(struct configInfo));

        /* The default value */
        input_params.ratio = ADPCM_RATIO;
        input_params.decoding_fmt = Codec_ADPCM;
        input_params.input_file = NULL;
        input_params.buffer_deep = 20;
        input_params.playbak_volume = 50;
        input_params.sample_rate = 16000;
        input_params.channels = 2;

        while (1) {
                opt = getopt(argc, argv, "f:i:r:n:d:v:h");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'f':
                        if (0 == strcmp("adpcm", optarg)) {
                                input_params.ratio = ADPCM_RATIO;
                                input_params.decoding_fmt = Codec_ADPCM;
                        } else if (0 == strcmp("g711a", optarg)) {
                                input_params.ratio = 2;
                                input_params.decoding_fmt = Codec_G711A;
                        } else if (0 == strcmp("g711u", optarg)) {
                                input_params.ratio = 2;
                                input_params.decoding_fmt = Codec_G711U;
                        } else if (0 == strcmp("g726", optarg)) {
                                input_params.ratio = G726_RATIO_16K;
                                input_params.decoding_fmt = Codec_G726;
                        } else if (0 == strcmp("aac-adts", optarg)) {
                                input_params.ratio = 8;
                                input_params.decoding_fmt = Codec_AAC_ADTS;
                        } else if (0 == strcmp("aac-adif", optarg)) {
                                input_params.ratio = 8;
                                input_params.decoding_fmt = Codec_AAC_ADIF;
                        } else if (0 == strcmp("aac-raw", optarg)) {
                                input_params.ratio = 8;
                                input_params.decoding_fmt = Codec_AAC_RAW;
                        } else {
                                printf("The audio decoding format is incorrectly entered.\n");
                                exit(-1);
                        }
                        break;
                case 'i':
                        input_params.input_file = optarg;
                        break;
                case 'r':
                        input_params.sample_rate = atoi(optarg);
                        break;
                case 'n':
                        input_params.channels = atoi(optarg);
                        break;
                case 'd':
                        input_params.buffer_deep = atoi(optarg);
                        break;
                case 'v':
                        input_params.playbak_volume = atoi(optarg);
                        break;
                case 'h':
                        print_help();
                        exit(0);
                case '?':
                        print_help();
                        exit(-1);
                }
        }

        if (NULL == input_params.input_file) {
                printf("Please input paramt!!!\r\n");
                print_help();
                exit(-1);
        }

        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;

        get_paramt(argc, argv);

        IHal_AO_Attr_t ao_attr;
        memset(&ao_attr, 0, sizeof(IHal_AO_Attr_t));
        ao_attr.audio_node = "plug:play_chn0";
        ao_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ao_attr.SampleRate = input_params.sample_rate;
        ao_attr.numPerSample = 1024;            // aac decoder must set as 1024
        ao_attr.channels = input_params.channels;
        ao_attr.bufferDeep = input_params.buffer_deep;
        ao_handle = IHal_AO_ChanCreate(&ao_attr);
        if (!ao_handle) {
                printf("ao channel create failed\r\n");
                return -1;
        }

        IHal_AudioDec_Attr_t adec_attr;
        memset(&adec_attr, 0, sizeof(IHal_AudioDec_Attr_t));
        adec_attr.type = input_params.decoding_fmt;
        adec_attr.BitRate = 16000;      // for g726
        adec_attr.SampleRate = input_params.sample_rate;
        adec_attr.AO_numPerSample = 1024;       // aac decoder must set as 1024
        adec_attr.Channels = input_params.channels;
        adec_attr.bufsize = input_params.buffer_deep;          // the num of buffer

        adec_handle = IHal_AudioDec_CreateChn(&adec_attr);

        ret = IHal_AO_ChanStart(ao_handle);
        if (ret) {
                printf("ao start failed\r\n");
                return -1;
        }

        IHal_AO_SetVolume(ao_handle, input_params.playbak_volume);

        fd = open(input_params.input_file, O_RDONLY);
        if (fd < 0) {
                printf("open save file failed \r\n");
                return -1;
        }

        char pcm_file[128] = {0};
        sprintf(pcm_file, "%s.pcm", input_params.input_file);
        int pcm_fd = open(pcm_file, O_RDWR | O_CREAT | O_TRUNC, 0666);

        IHal_AudioFrm_t frm;
        IHal_AudioStream_t stream;

        unsigned int datasize = 0;
        if (adec_attr.type == Codec_AAC_ADTS) {
                datasize = 256;//sizeof(short) * ao_attr.channels * ao_attr.numPerSample / input_params.ratio;
        } else {
                datasize = sizeof(short) * ao_attr.channels * ao_attr.numPerSample / input_params.ratio;
        }

        char *buffer = (char *)malloc(datasize);
        unsigned int readsize = 0;
        unsigned int cnt = 0;
        readsize = read(fd, buffer, datasize);

        while (readsize > 0) {
                printf("read data size = %d \n", readsize);
                stream.vaddr = buffer;
                stream.size = readsize;
                IHal_AudioDec_PutStream(adec_handle, &stream, IMPP_WAIT_FOREVER);
                printf("decode cnt  = %d \r\n", cnt++);
                IHal_AudioDec_GetFrame(adec_handle, &stream, IMPP_WAIT_FOREVER);
                frm.vaddr = stream.vaddr;
                frm.datalen = stream.size;
                printf("adec frame size = %d \r\n", stream.size);
                if (stream.size > 0) {
                        IHal_AO_ChanWriteData(ao_handle, &frm, IMPP_WAIT_FOREVER);
                        write(pcm_fd, frm.vaddr, frm.datalen);
                }
                IHal_AudioDec_ReleaseFrame(adec_handle, &stream);
                readsize = read(fd, buffer, datasize);
        }
        printf("play end ........\r\n");
        IHal_AudioDec_DestroyChn(adec_handle);
        IHal_AO_BufferFlush(ao_handle);
        IHal_AO_ChanStop(ao_handle);
        IHal_AO_ChanDestroy(ao_handle);
        free(buffer);
        close(fd);
        close(pcm_fd);
        return 0;
}

