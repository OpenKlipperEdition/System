#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "jz_rsa.h"

#define DEV_NAME "/dev/rsa"

struct rsa_key {
    unsigned int *e_or_d;
    unsigned int *n;
    unsigned int rsa_mode;
};
struct rsa_data {
    unsigned int *input;
    unsigned int inlen;
    unsigned int *output;
};

int jz_rsa_de(unsigned int *n, unsigned int *e, unsigned int key_len,
        unsigned int *input, unsigned int *output)
{
    struct rsa_key key;
    struct rsa_data data;
    int fd;

    key.e_or_d = e;
    key.n = n;
    key.rsa_mode = key_len * 8;

    fd = open(DEV_NAME, O_RDONLY);
    if(fd < 0)
    {
        printf("open %s false\n", DEV_NAME);
        return -1;
    }


    ioctl(fd, 0x1, &key);

	data.output = output;
	data.input = input;
	data.inlen = 64;
	ioctl(fd, 0x2, &data);

	close(fd);
}
