/*
 * sec_test.c - Ingenic security driver test app
 *
 * Copyright (C) 2015 Ingenic Semiconductor Co., Ltd.
 * Author: liu yang <king.lyang@ingenic.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <errno.h>
#include "sec_test.h"


/*#define DEBUG*/
unsigned int aes_key[4] = {
0x2b7e1516, 0x28aed2a6, 0xabf71588,0x09cf4f3c,
};

int do_rsa(unsigned int fd, unsigned int orig_aes_len, unsigned int * rsa_key, unsigned int * n, unsigned int * input, unsigned int *output,unsigned int mode)
{
	int ret = 0;
	struct rsa_param rsa_p;
	if(mode){
		rsa_p.in_len = orig_aes_len;
		rsa_p.out_len = 31;
	} else {
		rsa_p.in_len = 31;
		rsa_p.out_len = orig_aes_len;
	}
	rsa_p.key_len = 31;
	rsa_p.n_len = 31;
	/*rsa_p.out_len = 31;*/

	rsa_p.input = input;
	rsa_p.key = rsa_key;
	rsa_p.n = n;
	rsa_p.output = output;
	rsa_p.mode = mode;
#ifdef DEBUG
		printf("ly-test ---> rsa_p.input_len = %d\n",rsa_p.in_len);
		printf("ly-test ---> rsa_p.key_len	= %d\n",rsa_p.key_len);
		printf("ly-test ---> rsa_p.n_len	= %d\n",rsa_p.n_len);
		printf("ly-test ---> rsa_p.out_len	= %d\n",rsa_p.out_len);
		printf("ly-test ---> rsa_p.input	= %x\n",(unsigned int)rsa_p.input);
		printf("ly-test ---> rsa_p.key		= %x\n",(unsigned int)rsa_p.key);
		printf("ly-test ---> rsa_p.n		= %x\n",(unsigned int)rsa_p.n);
		printf("ly-test ---> rsa_p.output	= %x\n",(unsigned int)rsa_p.output);
#endif
	ret = ioctl(fd, SECURITY_RSA, &rsa_p);//private key encode
	if(ret < 0) {
		printf("ERROR! in ioctl SECURITY_RSA, ret = %d\n",ret);
		return -1;
	}
	return 0;
}

int cmp_data(unsigned int *src, unsigned int *dst, unsigned int len)
{
	unsigned int *start = src;
	unsigned int *end_src = src + len;
	while(start < end_src)
	{
		if(*start++ != *dst++) {
			printf("cmp data error: src:%08x, dst:%08x\n", *(start-1), *(dst-1));
			return (start - src);
		}
	}
	return 0;
}
