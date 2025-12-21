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
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <impp.h>
#include <audio.h>


int main(int argc, char **argv)
{
        if (argc < 5) {
                printf("%s filename insamplerate outsamplerate outfile\r\n", argv[0]);
                return 0;
        }
        IHal_ResamplerAttr_t rattr;
        memset(&rattr, 0, sizeof(IHal_ResamplerAttr_t));
        rattr.InSamplerate = atoi(argv[2]);
        rattr.OutSamplerate = atoi(argv[3]);
        rattr.Channels = 2;
        rattr.Quality = 6;
        rattr.MaxNumPersamples = 160;           // such  as 160 / 320....
        short *buffer = malloc(sizeof(short) * rattr.MaxNumPersamples * rattr.Channels);
        short *out;
        int ret = 0;
        int readsize = rattr.MaxNumPersamples * rattr.Channels * sizeof(short);
        int ifd = open(argv[1], O_RDWR);
        int ofd = open(argv[4], O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (ifd < 0 || ofd < 0) {
                printf("file open failed \r\n");
                free(buffer);
                return -1;
        }
        IHal_Resampler_Handle_t *rhandle = IHal_ResamlperInit(&rattr);
        if (rhandle == NULL) {
                printf("resampler init failed \r\n");
                free(buffer);
                return -1;
        }

        ret = read(ifd, buffer, readsize);
        int osize = 0;
        printf("InSamplerate = %d OutSamplerate = %d \r\n", rattr.InSamplerate, rattr.OutSamplerate);
        while (ret > 0) {
                IHal_ResamlperProcess(rhandle, buffer, &out, readsize, &osize);
                write(ofd, out, osize);
                printf("process ok!! outsize = %d  \r\n", osize);
                ret = read(ifd, buffer, readsize);
        }

        IHal_ResamlperDeInit(rhandle);
        free(buffer);
        close(ifd);
        close(ofd);

        return 0;
}
