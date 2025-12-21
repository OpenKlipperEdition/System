#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "gen_key_base.h"
#include "jz_rsa.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "rsa.h"
#include "keys.h"

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
#if  1
void decode(uint32_t *bn, uint32_t digits, uint8_t *hexarr, uint32_t size)
{
	bn_t t;
	int j;
	uint32_t i, u;
	for(i=0,j=size-1; i<digits && j>=0; i++) {
		t = 0;
		for(u=0; j>=0 && u<32; j--, u+=8) {
			t |= ((bn_t)hexarr[j]) << u;
		}
		bn[i] = t;
	}

	for(; i<digits; i++) {
		bn[i] = 0;
	}
}
#endif

#define KEY_LEN 64

int main(int argc, char const *argv[])
{
	int ret = 0;
	unsigned int e[KEY_LEN] = {0};
	unsigned int n[KEY_LEN] = {0};
	unsigned int d[KEY_LEN] = {0};
	unsigned int e_in[KEY_LEN] = {0};
	unsigned int n_in[KEY_LEN] = {0};

	unsigned int de_data[KEY_LEN] = {0};
	unsigned int data_rsa[KEY_LEN] = {0};
	unsigned int de_data_openssl[KEY_LEN] = {0};
	unsigned int en_data[KEY_LEN] = {0};
	unsigned int en_data_in[KEY_LEN] = {0};
	unsigned int em[KEY_LEN] = {0};
	unsigned int padding_data[KEY_LEN] = {0};

	if(argc < 4) {
		printf("rsa_test in_file out_file rsa_key\n");
		return -1;
	}

	FILE *srcfp = fopen(argv[1], "r");
	FILE *fp = fopen(argv[2], "w");

	get_n_e(argv[3], (unsigned char*)n, (unsigned char*)e, (unsigned char*)d, KEY_LEN * 4);

	fread(en_data, KEY_LEN * 4, 1, srcfp);
	big_endian(e_in, e, KEY_LEN);
	big_endian(n_in, n, KEY_LEN);
	big_endian(en_data_in, en_data, KEY_LEN);

	//ingenic rsa decryption
	jz_rsa_de(n_in, e_in, 256, en_data_in, data_rsa);

	big_endian(de_data, data_rsa, KEY_LEN);
	fwrite(de_data, KEY_LEN * 4, 1, fp);

	fclose(srcfp);
	fclose(fp);

}
