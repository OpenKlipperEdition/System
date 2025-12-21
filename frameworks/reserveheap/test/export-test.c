#include <stdio.h>
#include <reserveheap.h>
#include <ipcsocket.h>

int main(int argc, char *argv[])
{
	int status;
	int sockfd;
	int ret;
	struct socket_info skinfo;
	/* This is server: open the socket connection first */
	/* Here; 1 indicates server or exporter */
	status = opensocket(&sockfd, SOCKET_NAME, 1);
	if (status < 0) {
		fprintf(stderr, "<%s>: Failed opensocket.\n", __func__);
		goto err_socket;
	}
	skinfo.sockfd = sockfd;

	struct reserveheap mem = {0};
	if(reserveheap_alloc(10*1024,&mem) != 0)
		printf("SHMEM: alloc mem failed!\n");
	printf("SHMEM: vaddr: %x paddr: %x size: %d fd: %d\n",mem.vaddr,mem.paddr,mem.size,mem.fd);
	snprintf(mem.vaddr,100,"this is a test");

	skinfo.datafd = mem.fd;
	skinfo.size = mem.size;

	ret = socket_send_fd(&skinfo);
	if (ret < 0) {
		fprintf(stderr, "FAILED: socket_send_fd\n");
		goto err_send;
	}

err_send:
err_export:
	reserveheap_free(&mem);
err_socket:
	closesocket(sockfd, SOCKET_NAME);
	printf("Finish\n");
    return 0;
}
