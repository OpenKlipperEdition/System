#include <stdio.h>
#include <string.h>

#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/crypto.h>


#include "hash_openssl.h"

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

unsigned char *sha_256(const unsigned char *in, const size_t in_len, unsigned char *out)
{
    if (in == NULL || out == NULL) {
        PRINT_ERR("invalid parameter\n");
        return out;
    }

    return SHA256(in, in_len, out);
}

