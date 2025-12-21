#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hash_openssl.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "hash.h"

#define DATA_LEN 64

#define DUMP

#ifdef DUMP
int dump(unsigned int *in, int len, char *chars)
{
    int i;

    printf("%s\n", chars);
    for(i = 0; i < len; i++) {
        printf("0x%08x, ", in[i]);
        if((i + 1) % 4 == 0)
            printf("\n");
    }
    printf("\n");
    printf("\n");
}
#else
int dump(unsigned int *in, int len, char *chars)
{
}
#endif

int cmp_data(unsigned int *src, unsigned int *tar, unsigned int len)
{
    int i;
    for(i = 0; i < len; i++){
        if(src[i] != tar[i]) {
		printf("src[i] = 0x%08x, tar[i] = 0x%08x\n", src[i], tar[i]);
            printf("cmp data error !!i = %d\n", i);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    int ret = 0;
    unsigned int hash[32] = {0};
    unsigned int hash_openssl[32] = {0};
    struct stat fstat;

    if(argc < 3) {
	printf("hash_test in_file out_file\n");
	return -1;
    }

    stat(argv[1], &fstat);

    FILE *srcfp = fopen(argv[1], "r");
    FILE *fp = fopen(argv[2], "w");
    unsigned int *hash_mem = (unsigned int *)malloc(fstat.st_size);
    if(hash_mem == NULL) {
        printf("calloc error !!!!\n");
        return -1;
    }
    memset(hash_mem, 0, fstat.st_size);

    fread(hash_mem, fstat.st_size, 1, srcfp);

    sha_256((unsigned char*)hash_mem, fstat.st_size, (unsigned char*)hash_openssl);

    jz_hash(fstat.st_size, hash_mem, hash);

    fwrite(hash, 32, 1, fp);

    if(cmp_data(hash, hash_openssl, 8) < 0)
    {
        printf("hash crypt error!!\n");
        return -1;
    }else{
        printf("hash crypt success!!\n");
    }

    free(hash_mem);
    fclose(srcfp);
    fclose(fp);

}
