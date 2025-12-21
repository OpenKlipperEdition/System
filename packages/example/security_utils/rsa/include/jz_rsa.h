#ifndef JZ_RSA_H
#define JZ_RSA_H

void check_nku(unsigned int *M, unsigned int *D, unsigned int *N, unsigned int *P,unsigned int *data);

int jz_rsa_de(unsigned int *n, unsigned int *e, unsigned int key_len,
        unsigned int *input, unsigned int *output);

#endif
