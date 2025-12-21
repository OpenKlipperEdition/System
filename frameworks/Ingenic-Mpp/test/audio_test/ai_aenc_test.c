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
static IHal_AudioEnc_Handle_t *aenc_handle;

static void print_help(void)
{
        printf("Usage: ai_aenc_test [OPTIONS] [SLICES PATH]\n\n"
               "Options:\n"
               " -f             Coding format. If not set default as adpcm.\n"
               " -r             Sample rate.(default 16000)\n"
               " -n             Number of channels.(default 2)\n"
               " -v             Recording volume gain. If not set, the configuration will not be modified.\n"
               " -o             Output the encoded filename.\n"
               " -d             user ring buffer num. If not set default as 20, max as 50\n"
               " -h             help\n\n"
               "Audio coding presets:\n");
}

static int signal_handle(int sig)
{
        IHal_AI_ChanStop(ai_handle);
        IHal_AI_ChanDestroy(ai_handle);
        IHal_AudioEnc_DestroyChn(aenc_handle);
        close(fd);
        return 0;
}

int main(int argc, char **argv)
{
        int ret = 0;
        int opt = 0;

        IHal_AudioCodecType_t coding_fmt = Codec_ADPCM;
        char *output_path = NULL;
        IHAL_INT32 buffer_deep = 20;
        IHAL_UINT32 sample_rate = 16000;
        IHAL_UINT32 channels = 2;
         IHAL_UINT32 recording_volume = -1;

        while (1) {
                opt = getopt(argc, argv, "f:r:n:v:o:d:h");

                if (opt == -1) {
                        break;
                }

                switch (opt) {
                case 'f':
                        if (0 == strcmp("adpcm", optarg)) {
                                coding_fmt = Codec_ADPCM;
                        } else if (0 == strcmp("g711a", optarg)) {
                                coding_fmt = Codec_G711A;
                        } else if (0 == strcmp("g711u", optarg)) {
                                coding_fmt = Codec_G711U;
                        } else if (0 == strcmp("g726", optarg)) {
                                coding_fmt = Codec_G726;
                        } else if (0 == strcmp("aac-adts", optarg)) {
                                coding_fmt = Codec_AAC_ADTS;
                        } else if (0 == strcmp("aac-adif", optarg)) {
                                coding_fmt = Codec_AAC_ADIF;
                        } else if (0 == strcmp("aac-raw", optarg)) {
                                coding_fmt = Codec_AAC_RAW;
                        } else {
                                printf("The audio encoding format is incorrectly entered.\n");
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

        if (NULL == output_path) {
                print_help();
                return -1;
        }

        IHal_AI_Attr_t ai_attr;
        memset(&ai_attr, 0, sizeof(IHal_AI_Attr_t));
        ai_attr.audio_node = "plug:cap_chn0";
        ai_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ai_attr.SampleRate = sample_rate;
        ai_attr.numPerSample = 1024;
        ai_attr.channels = channels;
        ai_attr.bufferDeep = buffer_deep;

        ai_handle = IHal_AI_ChanCreate(&ai_attr);
        if (!ai_handle) {
                printf("ai channel create failed\r\n");
                return -1;
        }

        if (-1 != recording_volume) {
                IHal_AI_SetGain(ai_handle, recording_volume);
                int val = 0;
                IHal_AI_GetGain(ai_handle,&val);
        }

        IHal_AudioEnc_Attr_t aenc_attr;
        memset(&aenc_attr, 0, sizeof(IHal_AudioEnc_Attr_t));
        aenc_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;              // src pcm-data format
        aenc_attr.type = coding_fmt;
        aenc_attr.BitRate = 16000;
        aenc_attr.SampleRate = sample_rate;
        aenc_attr.Quality = 0;
        aenc_attr.Channels = channels;
        aenc_attr.numPerSample = 1024;
        aenc_attr.bufsize = buffer_deep;         // the num of buffer

        aenc_handle = IHal_AudioEnc_CreateChn(&aenc_attr);
        if (!aenc_handle) {
                printf("audio encoder chn create failed\r\n");
                IHal_AI_ChanDestroy(ai_handle);
                return -1;
        }
        signal(SIGINT, signal_handle);
        ret = IHal_AI_ChanStart(ai_handle);
        if (ret) {
                printf("ai start failed\r\n");
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        fd = open(output_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
                printf("open save file failed\r\n");
                IHal_AI_ChanStop(ai_handle);
                IHal_AudioEnc_DestroyChn(aenc_handle);
                IHal_AI_ChanDestroy(ai_handle);

                return -1;
        }

        int times = 500;

        IHal_AudioBuffer_t buf;
        IHal_AudioStream_t stream;
        while (times--) {
                ret = IHal_AI_GetBuffer(ai_handle, &buf, IMPP_WAIT_FOREVER);
                if (!ret) {
                        printf("get buffer ok data size = %d \n", buf.datalen);
                        stream.vaddr = buf.vaddr;
                        stream.size = buf.datalen;
                        IHal_AudioEnc_PutFrame(aenc_handle, &stream, IMPP_WAIT_FOREVER);
                        IHal_AI_ReleaseBuffer(ai_handle, &buf);
                        ret = IHal_AudioEnc_GetStream(aenc_handle, &stream, IMPP_WAIT_FOREVER);
                        if (!ret) {
                                printf("affter encode data size = %d \r\n", stream.size);
                                if (stream.size > 0) {
                                        write(fd, stream.vaddr, stream.size);
                                }
                                IHal_AudioEnc_ReleaseStream(aenc_handle, &stream);
                        }
                }
        }
        IHal_AI_ChanStop(ai_handle);
        IHal_AI_ChanDestroy(ai_handle);
        IHal_AudioEnc_DestroyChn(aenc_handle);
        close(fd);
        return 0;
}
