#include <stdio.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include "openssl/crypto.h"


#include "gen_key_base.h"
static int bn_2_buff(const BIGNUM *item, unsigned char *buff, const size_t k_len)
{
	size_t e_offset = k_len - (size_t)BN_num_bytes(item);

	memset(buff, 0, k_len);

	PRINT_DEBUG("k_len = %ld e_offset = %ld %d\n", k_len, e_offset, BN_num_bytes(item));

	if (BN_bn2bin(item, buff + e_offset) < 0) {
		PRINT_ERR("write big nember to buffer fail\n");
		return -1;
	}

	return (int)k_len;
}

size_t gen_rand_num(unsigned char *out, const size_t k_len)
{
	int ret = 0;
	BIGNUM *bn = BN_new();
	if (bn == NULL) {
		return 0;
	}

	ret = BN_rand(bn, (int)k_len * 8, 0, 0);
	if (ret < 0) {
		BN_free(bn);
		return 0;
	}

	ret = bn_2_buff(bn, out, k_len);
	if (ret == 0) {
		BN_free(bn);
		PRINT_ERR("bn write buff failed\n");
		return 0;
	}

	BN_free(bn);

	return ret < 0 ? 0 : (size_t)ret;
}

int aes_ecb_enc(const char *key, const int key_len, const unsigned char *in, const size_t in_len, unsigned char *out, int enc)
{
	FILE *aes_in = NULL;
	unsigned char *key_buff = NULL;
	size_t ret = 0;
	AES_KEY aes;
	int iret = 0;
	size_t res_data_len = 0;
	size_t out_offset = 0;

	if (in_len % 16) {
		PRINT_ERR("must 16 bytes alignment\n");
		return -1;
	}

	iret = AES_set_encrypt_key(key, key_len * 8, &aes);
	if (iret != 0) {
		free(key_buff);
		PRINT_ERR("read key is fail\n");
		return -5;
	}

	free(key_buff);

	res_data_len = in_len;
	out_offset = 0;
	while (res_data_len) {
		unsigned char buff[16] = {0};
		AES_ecb_encrypt(in + out_offset, buff, &aes, enc);
		memcpy(out + out_offset, buff, 16);
		res_data_len -= 16;
		out_offset += 16;
	}

	return 0;
}
int aes_cbc_enc(const char *key, const int key_len, const unsigned char *in, const size_t in_len, unsigned char *out,
        unsigned char *ivec, int enc)
{
	FILE *aes_in = NULL;
	unsigned char *key_buff = NULL;
	size_t ret = 0;
	AES_KEY aes;
	int iret = 0;
	size_t res_data_len = 0;
	size_t out_offset = 0;

	if (in_len % 16) {
		PRINT_ERR("must 16 bytes alignment\n");
		return -1;
	}

	iret = AES_set_encrypt_key(key, key_len * 8, &aes);
	if (iret != 0) {
		free(key_buff);
		PRINT_ERR("read key is fail\n");
		return -5;
	}

    AES_cbc_encrypt(in, out, in_len , &aes, ivec, enc);


	return 0;
}
