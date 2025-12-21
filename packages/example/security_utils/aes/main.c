#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gen_key_base.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "aes.h"

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
            printf("cmp data error !!i = %d\n", i);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    unsigned int key[8] ={0};
    unsigned int ivec[4] = {0};

    struct stat fstat;
    if(argc < 5) {
	printf("aes_test in_file out_file key_file op(en:1, de:0)\n");
	return -1;
    }

    int op = atoi(argv[4]);

    if(op != 0 && op != 1){
	    printf("op value error\n");
	    return -1;
    }

    stat(argv[1], &fstat);

    FILE *srcfp = fopen(argv[1], "r");
    FILE *keyfp = fopen(argv[3], "r");
    FILE *fp = fopen(argv[2], "w");

    unsigned int *input = (unsigned int *)malloc((fstat.st_size + 16) /16 *16);
    unsigned int *output = (unsigned int *)malloc((fstat.st_size + 16) /16 *16);

    fread(input, fstat.st_size, 1, srcfp);
    fread(key, 32, 1, keyfp);
    fread(ivec, 16, 1, keyfp);

    aes_crypt((void *)input, fstat.st_size, (void *)output, (void *)key, (void *)ivec, op, ECB, 256);

    fwrite(output, fstat.st_size, 1, fp);

    free(input);
    free(output);
    fclose(srcfp);
    fclose(keyfp);
    fclose(fp);


}
