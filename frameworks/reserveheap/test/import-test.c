#include <stdio.h>
#include <unistd.h>
#include <reserveheap.h>
#include <ipcsocket.h>
int main(int argc, char *argv[])
{
	int ret, status;
	int sockfd, shared_fd;
	int size;
	struct socket_info skinfo;
	/* This is the client part. Here 0 means client or importer */
	status = opensocket(&sockfd, SOCKET_NAME, 0);
	if (status < 0) {
		fprintf(stderr, "No exporter exists...\n");
		ret = status;
		goto err_socket;
	}

	skinfo.sockfd = sockfd;

	ret = socket_receive_fd(&skinfo);
	if (ret < 0) {
		fprintf(stderr, "Failed: socket_receive_fd\n");
		goto err_recv;
	}

	shared_fd = skinfo.datafd;
	size = skinfo.size;
	printf("Received buffer fd: %d size: %d\n", shared_fd,size);
	if (shared_fd <= 0) {
		fprintf(stderr, "ERROR: improper buf fd\n");
		ret = -1;
		goto err_fd;
	}

	struct reserveheap mem = {
		.fd = shared_fd,
		.size = size,
		.paddr = skinfo.paddr,
		.vaddr = 0
	};

	if(reserveheap_import(&mem) != 0)
		printf("SHMEM: import mem failed\n");

	while(1){
		printf("SHMEM:%s\n",mem.vaddr);
		usleep(1000000);
	}
err_import:
	reserveheap_free(&mem);
err_fd:
err_recv:
err_socket:
	closesocket(sockfd, SOCKET_NAME);

    return 0;
}
