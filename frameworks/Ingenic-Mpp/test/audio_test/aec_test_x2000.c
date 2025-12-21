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
#include <impp_fifo.h>

IHal_AudioHandle_t *ao_handle = NULL;
IHal_AudioHandle_t *ai_handle = NULL;

void *aec_record_thread(void *arg)
{
        int ret = 0;
        IHal_AI_Attr_t ai_attr;
        arg = (char *)arg;
        memset(&ai_attr, 0, sizeof(IHal_AI_Attr_t));
        //ai_attr.audio_node = "plug:cap_chn0";
        ai_attr.audio_node = "hw:0,6";
        ai_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ai_attr.SampleRate = 16000;
        ai_attr.numPerSample = 320;
        ai_attr.channels = 1;
        ai_attr.bufferDeep = 20;
        IHal_AudioHandle_t *ai_handle = IHal_AI_ChanCreate(&ai_attr);
        if (!ai_handle) {
                printf("ai channel create failed\r\n");
                return (void *) -1;
        }
        IHal_AI_SingleChannelSet(ai_handle, MONO_RIGHT);

        ret = IHal_AI_ChanStart(ai_handle);
        if (ret) {
                printf("ai start failed\r\n");
                IHal_AI_ChanDestroy(ai_handle);

                return (void *) -1;
        }
        IHal_AI_EnableAec(ai_handle);

        int fd2 = open("aec_save.pcm", O_RDWR | O_CREAT | O_TRUNC, 0666);
        IHal_AudioBuffer_t buf;
        int times  = atoi(arg);
        while (times--) {
                ret = IHal_AI_GetBuffer(ai_handle, &buf, IMPP_WAIT_FOREVER);
                if (!ret) {
                        write(fd2, buf.vaddr, buf.datalen);
                        IHal_AI_ReleaseBuffer(ai_handle, &buf);
                        printf("write end (%d)....\r\n", times);
                }
        }
        sleep(1);
        IHal_AI_ChanStop(ai_handle);
        IHal_AI_DisableAec(ai_handle);
        IHal_AI_ChanDestroy(ai_handle);
        close(fd2);
        printf("%s exit ############\r\n", __func__);
        pthread_exit(0);
}

void *ref_play_thread(void *arg)
{
        int ret = 0;
        arg = (char *)arg;
        IHal_AO_Attr_t ao_attr;
        memset(&ao_attr, 0, sizeof(IHal_AO_Attr_t));
        ao_attr.audio_node = "plug:multi";
        ao_attr.SampleFmt = IMPP_SAMPLE_FMT_S16;
        ao_attr.SampleRate = 16000;
        ao_attr.numPerSample = 320;
        ao_attr.channels = 1;
        ao_attr.bufferDeep = 20;

        ao_handle = IHal_AO_ChanCreate(&ao_attr);
        if (!ao_handle) {
                printf("ao channel create failed\r\n");
                return (void *) -1;
        }

		IHal_AO_SetVolume(ao_handle, 40);

        ret = IHal_AO_ChanStart(ao_handle);
        if (ret) {
                printf("ao start failed\r\n");
                return (void *) -1;
        }

        int fd = open(arg, O_RDONLY);
        if (fd < 0) {
                printf("open play file fialed");
                return (void *) -1;
        }
        unsigned int datasize = 16 / 8 * ao_attr.channels * ao_attr.numPerSample;
        unsigned char *buffer = malloc(datasize);
        int readsize = read(fd, buffer, datasize);
        usleep(10000);
        IHal_AudioFrm_t frm;
        int cnt = 0;
        while (readsize > 0) {
                frm.vaddr = (unsigned int *)buffer;

                frm.datalen = readsize;
                ret = IHal_AO_ChanWriteData(ao_handle, &frm, IMPP_WAIT_FOREVER);
                if (ret) {
                        printf("write ao data failed");
                }
                if (ai_handle && (cnt == 0)) {
                        cnt++;
                }
                readsize = read(fd, buffer, datasize);
        }
        IHal_AO_BufferFlush(ao_handle);
        IHal_AO_ChanStop(ao_handle);
        IHal_AO_ChanDestroy(ao_handle);
        free(buffer);
        printf("%s exit ############\r\n", __func__);
        pthread_exit(0);
}

int main(int argc, char **argv)
{
        int ret = 0;
        pthread_t rec_tid;
        pthread_t play_tid;
        if (argc < 3) {
                printf("param error ./aec_test times echofile\r\n");
                return -1;
        }
        pthread_create(&play_tid, NULL, ref_play_thread, argv[2]);
        pthread_create(&rec_tid, NULL, aec_record_thread, argv[1]);
        pthread_join(play_tid, NULL);
        pthread_join(rec_tid, NULL);

        return 0;
}
