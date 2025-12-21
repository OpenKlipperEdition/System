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

extern size_t gen_rand_num(unsigned char *out, const size_t k_len);
unsigned char *sha_256(const unsigned char *in, const size_t in_len, unsigned char *out);

#ifdef __cplusplus
}
#endif

#endif // GEN_KEY_BASE_H
