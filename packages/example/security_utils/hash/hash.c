#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <linux/if_alg.h>
#include <errno.h>

#ifndef AF_ALG

#define AF_ALG 38

#define SOL_ALG 279

#endif

int jz_hash(unsigned int code_len,unsigned int *input, unsigned int *output)
{
	int opfd;
	int tfmfd;
	struct sockaddr_alg sa = {0};
	sa.salg_family = AF_ALG;
	memcpy(sa.salg_type, "hash", sizeof("hash"));
	memcpy(sa.salg_name, "sha256", sizeof("sha256"));

	tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);

	if (tfmfd < 0) {
		fprintf(stderr, "sockfd: %s\n", strerror(errno));

		return -1;
	}
	int on = 1;


	if(bind(tfmfd, (struct sockaddr *)&sa, sizeof(sa))) {
		fprintf(stderr, "bind: %s\n", strerror(errno));
		close(tfmfd);

		return -1;
	}

	opfd = accept(tfmfd, NULL, 0);

	write(opfd, input, code_len);

	read(opfd, output, 16 * 4);

	close(opfd);
	close(tfmfd);

	return 0;
}
