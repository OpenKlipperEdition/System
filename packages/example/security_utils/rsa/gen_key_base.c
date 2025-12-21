#include <stdio.h>
#include <string.h>

#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "openssl/sha.h"
#include "openssl/err.h"
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

int gen_rsa_key(const char *pr_path, const size_t len)
{
	int ret = 0;
	RSA *rsa = NULL;
	BIO *out = NULL;

	BIGNUM *bn = BN_new();

	if (bn == NULL) {
		PRINT_ERR("new big number failed\n");
		return -1;
	}

	ret = BN_set_word(bn, RSA_F4);
	if (ret < 0) {
		PRINT_ERR("set big number failed\n");
		return -2;
	}

	rsa = RSA_new();
	if(rsa == NULL) {
		return -3;
	}

	ret = RSA_generate_key_ex(rsa, (int)len, bn, NULL);
	if (ret < 0) {
		RSA_free(rsa);
		return -4;
	}

	out = BIO_new_file(pr_path, "w");
	if (out == NULL) {
		RSA_free(rsa);
		return -5;
	}

	ret = PEM_write_bio_RSAPrivateKey(out, rsa, NULL, NULL, 0, NULL, NULL);
	if (ret < 0) {
		RSA_free(rsa);
		BIO_free(out);
		return -6;
	}

	BIO_ctrl(out, BIO_CTRL_FLUSH, 0, NULL);
	BIO_free(out);

	RSA_free(rsa);

	return 0;
}

int get_n_e(const char *f_rsa, unsigned char *n, unsigned char *e, unsigned char *d, const size_t key_len)
{
	BIO *rsa_in = NULL;
	RSA *rsa = NULL;
	const BIGNUM *n_t = NULL;
	const BIGNUM *e_t = NULL;
	const BIGNUM *d_t = NULL;

	if (f_rsa == NULL || n == NULL || e == NULL) {
		PRINT_ERR("invalid parameter\n");
		return -1;
	}

	rsa_in = BIO_new_file(f_rsa, "r");
	if (rsa_in == NULL) {
		PRINT_ERR("private key file open fail\n");
		return -1;
	}

	rsa = PEM_read_bio_RSAPrivateKey(rsa_in, NULL, NULL, NULL);
	if (rsa == NULL) {
		BIO_free(rsa_in);
		PRINT_ERR("read private key fail\n");
		return -2;
	}

	BIO_free(rsa_in);

	if (key_len != (size_t)RSA_size(rsa)) {
		RSA_free(rsa);
		PRINT_ERR("key len err\n");
		return -3;
	}

	RSA_get0_key(rsa, &n_t, &e_t, &d_t);
	if (n_t == NULL || e_t == NULL) {
		PRINT_ERR("get n or e failed\n");
		RSA_free(rsa);
		return -4;
	}

	if (bn_2_buff(n_t, n, key_len) < 0) {
		RSA_free(rsa);
		PRINT_ERR("get n fail\n");
		return -5;
	}

	if (bn_2_buff(e_t, e, key_len) < 0) {
		RSA_free(rsa);
		PRINT_ERR("get e fail\n");
		return -6;
	}
	if (bn_2_buff(d_t, d, key_len) < 0) {
		RSA_free(rsa);
		PRINT_ERR("get e fail\n");
		return -6;
	}

	RSA_free(rsa);


	return (int)key_len;
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

/*copy from security boot*/
size_t big_endian(unsigned int *out, unsigned int *input, unsigned int len)
{
	int tmp;
	int i;
	for (i=0; i<len; i++)
		out[i] |= (((input[i] & 0xff000000) >> 24) | \

				((input[i] & 0x00ff0000) >> 8) | \

				((input[i] & 0x0000ff00) << 8) | \

				((input[i] & 0x000000ff) << 24) \

			  );

	return i;
}

unsigned char *sha_256(const unsigned char *in, const size_t in_len, unsigned char *out)
{
	if (in == NULL || out == NULL) {
		PRINT_ERR("invalid parameter\n");
		return out;
	}

	return SHA256(in, in_len, out);
}

#define HASH_SIZE 256
int rsa_sha256_emsa_pass_sig(const char *f_rsa, const unsigned char *in,
		const size_t in_len, unsigned char *out,
		unsigned char * em, unsigned char *de_data,
		unsigned int *padding_data)
{
	RSA *rsa = NULL;
	unsigned char m_hash[HASH_SIZE] = {0};
	int ret = 0;
	BIO *rsa_in = BIO_new_file(f_rsa, "r");
	if (in == NULL) {
		PRINT_ERR("open key is fail\n");
		return -1;
	}

	PRINT_DEBUG("private key file: %s\n", f_rsa);

	rsa = PEM_read_bio_RSAPrivateKey(rsa_in, NULL, NULL, NULL);
	if (rsa == NULL) {
		BIO_free(rsa_in);
		PRINT_ERR("read key is fail\n");
		return -2;
	}

	BIO_free(rsa_in);

	SHA256(in, in_len, m_hash);

	ret = RSA_padding_add_PKCS1_PSS(rsa, em, m_hash, EVP_sha256(), -1);
	if (ret != 1) {
		RSA_free(rsa);
		PRINT_ERR("rsa padding add fail\n");
		return -3;
	}

	memcpy(padding_data, em, 256);

	ret = RSA_private_encrypt(HASH_SIZE, em, out, rsa, RSA_NO_PADDING);
	PRINT_DEBUG("ret = %d\n", ret);
	if (ret <= 0) {
		RSA_free(rsa);
		PRINT_ERR("rsa encryption fail\n");
		return -4;
	}
	ret = RSA_public_decrypt(HASH_SIZE, out, de_data, rsa, RSA_NO_PADDING);
	PRINT_DEBUG("ret = %d\n", ret);
	if (ret <= 0) {
		RSA_free(rsa);
		PRINT_ERR("rsa decryption fail\n");
		return -5;
	}

	RSA_free(rsa);

	return ret;
}



