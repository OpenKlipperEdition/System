#include<stdio.h>
#include<stdlib.h>
#include<linux/watchdog.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>

#define DTRNG_NAME "/dev/hwrng"

int main(int argc, char *argv[])
{
        int fd ,retval;
        char buf[64] = {0};
        int cmd = 0;

	if(argc < 2) {
		printf("dtrng_test out_file\n");
		return -1;
	}

	FILE *fp = fopen(argv[1], "w");
        fd = open(DTRNG_NAME, O_RDONLY);
        if(fd < 0)
        {
                printf("open %s false\n",DTRNG_NAME);
                exit(errno);
        }
        if(read(fd,buf,32) < 0)
                printf(" read false\n");
        else{
                printf("random_num register value 0x%08x\n",*(unsigned int *)buf);
		fwrite(buf, 32, 1, fp);
        }


        close(fd);
	fclose(fp);
        return retval;
}
