#ifndef GEN_KEY_BASE_H
#define GEN_KEY_BASE_H

#include <stdio.h>

//#define DEBUG

#ifdef DEBUG
#ifndef WIN32
#define PRINT_DEBUG(frame, ...) fprintf(stderr, "%s %s %d :" frame, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define PRINT_DEBUG(frame, ...) fprintf(stderr, "%s %s %d :" frame, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif
#else
#define PRINT_DEBUG(frame, ...)
#endif

#ifndef WIN32
#define PRINT_ERR(frame, ...) fprintf(stderr, "%s %s %d :" frame, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define PRINT_ERR(frame, ...) fprintf(stderr, "%s %s %d :" frame, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern size_t big_endian(unsigned int *out, unsigned int *input, unsigned int len);
extern int gen_rsa_key(const char *pr_path, const size_t len);
extern int get_n_e(const char *f_rsa, unsigned char *n, unsigned char *e, unsigned char *d, const size_t key_len);
extern size_t gen_rand_num(unsigned char *out, const size_t k_len);
int rsa_sha256_emsa_pass_sig(const char *f_rsa, const unsigned char *in,
        const size_t in_len, unsigned char *out, unsigned char * em,
        unsigned char *de_data, unsigned int *padding_data);

#ifdef __cplusplus
}
#endif

#endif // GEN_KEY_BASE_H
