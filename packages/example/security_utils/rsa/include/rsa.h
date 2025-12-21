/*****************************************************************************
Filename    : rsa.h
Author      : Terrantsh (tanshanhe@foxmail.com)
Date        : 2018-9-20 11:22:22
Description : RSA加密头文件
*****************************************************************************/
#ifndef __RSA_H__
#define __RSA_H__

#include <stdint.h>

// RSA key lengths
#define RSA_MAX_MODULUS_BITS                2048
#define RSA_MAX_MODULUS_LEN                 ((RSA_MAX_MODULUS_BITS + 7) / 8)

typedef uint64_t dbn_t;
typedef uint32_t bn_t;

typedef struct {
    uint32_t bits;
    uint8_t  modulus[RSA_MAX_MODULUS_LEN];
    uint8_t  exponent[RSA_MAX_MODULUS_LEN];
} rsa_pk_t;

int rsa_public_decrypt (uint8_t *out, uint8_t *in, uint32_t in_len, rsa_pk_t *pk);
int rsa_public_decrypt_modify (uint8_t *out, uint32_t *in, uint32_t in_len, rsa_pk_t *pk);


#endif  // __RSA_H__
