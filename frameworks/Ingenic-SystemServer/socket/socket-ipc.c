#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "socket-ipc.h"
#include "dlog.h"
#ifdef TAG
#undef TAG
#define TAG "SOCKET-IPC"
#else
#define TAG "SOCKET-IPC"
#endif



int SockIPC_ServerInit(char* name)	/* 1.socket 2.bind 3.listen  */
{
	int ret = 0;
	int socket_fd = 0;
	struct sockaddr_un un_addr;

	if(!name)
		return -1;
	socket_fd = socket(AF_UNIX,SOCK_STREAM,0);
	if(socket_fd <= 0){
		LOGE("%s socket error\n",__func__);
		return -1;
	}

	memset(&un_addr,0,sizeof(un_addr));
	un_addr.sun_family = AF_UNIX;
	strcpy(un_addr.sun_path,name);
	unlink(name);

	int addr_len = sizeof(un_addr.sun_family) + strlen(un_addr.sun_path);
	ret = bind(socket_fd,(struct sockaddr*)&un_addr,addr_len);
	if(ret < 0){
		LOGE("%s bind error\n",__func__);
		goto error;
	}

	if(chmod(name,0666) < 0){
		goto error;
	}

	if(listen(socket_fd,6) < 0){
		goto error;
	}

	return socket_fd;
error:
	close(socket_fd);
	return -1;
}


int SockIPC_ServerAccept(int sock_fd)	/* accept client connect  */
{
	int accept_fd = 0;
	struct sockaddr_un un_addr;
	int len = sizeof(un_addr);
	accept_fd = accept(sock_fd,(struct sockaddr*)&un_addr,&len);
	if(accept_fd < 0)
		return -1;
	else
		return accept_fd;
}

int SockIPC_ServerClose(int fd)	/* close server socket_fd  */
{
	if(fd)
		close(fd);
	return 0;
}

int SockIPC_ClientInit(char* name)	/* 1.socket 2.connect  */
{
	int ret = 0;
	int socket_fd = 0;
	struct sockaddr_un un_addr;
	if(!name)
		return -1;
	socket_fd = socket(AF_UNIX,SOCK_STREAM,0);
	if(socket_fd <= 0)
		return -1;
	memset(&un_addr,0,sizeof(un_addr));
	un_addr.sun_family = AF_UNIX;
	strcpy(un_addr.sun_path,name);
	int addr_len = sizeof(un_addr.sun_family) + strlen(un_addr.sun_path);
	ret = connect(socket_fd,(struct sockaddr*)&un_addr,addr_len);
	if(ret < 0){
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

int SockIPC_ClientClose(int fd)	/* close socket_fd  */
{
	if(fd)
		close(fd);
	return 0;
}

int SockIPC_WriteMsg(int sockfd,SockIpcMsg_t *msg)
{
	if(sockfd <= 0 || !msg || msg->len <= 0)
		return 0;
	char *data = msg->buf;
	int sendpos = 0;
	int ret = 0;
	while(sendpos != msg->len){
		ret = write(sockfd,(void*)&data[sendpos],msg->len - sendpos);
		if(ret < 0){
			if(errno == EINTR)
				continue;
			else if(errno == EPIPE)
				return -1;
			else
				return -2;
		}
		sendpos += ret;
	}
	return sendpos;
}

int SockIPC_ReadMsg(int sockfd,SockIpcMsg_t *msg)
{
	if(sockfd <= 0 || !msg || msg->len == 0)
		return 0;
	char *buf = msg->buf;
	int readpos = 0;
	int ret = 0;
	fd_set fds;
	struct timeval tm;
	FD_ZERO(&fds);
	FD_SET(0,&fds);
	FD_SET(sockfd,&fds);
	tm.tv_sec = 10;
	tm.tv_usec = 0;
	ret = select(sockfd+1,NULL,&fds,NULL,&tm);
	if(ret < 0)
		return -1;
	return read(sockfd,(void*)&buf[0],msg->len);

#if 0
	while(readpos != msg->len){
		ret = read(sockfd,(void*)&buf[readpos],msg->len - readpos);
		if(ret < 0){
			if(errno == EINTR)
				continue;
			else if(errno == EAGAIN)
				return readpos;
			else
				return -1;
		}
		if(ret == 0)
			return -1;
		readpos += ret;
	}

	printf("%s end ##\n",__func__);
	return readpos;
#endif
}

int SockIPC_SendDevFd(int sockfd,SockDevfdData_t *data)
{
	int ret = 0;
	struct timeval tm;
	struct msghdr msg;
	struct iovec iov;
	char ctr_msg[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;
	fd_set fds;

	if(sockfd <= 0 || !data)
		return -1;

	FD_ZERO(&fds);
	FD_SET(0,&fds);
	FD_SET(sockfd,&fds);
	tm.tv_sec = 10;
	tm.tv_usec = 0;
	ret = select(sockfd+1,NULL,&fds,NULL,&tm);
	if(ret < 0)
		return -1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov.iov_base = data->data;
	iov.iov_len = data->len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = ctr_msg;
	msg.msg_controllen = sizeof(ctr_msg);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*((int*)CMSG_DATA(cmsg)) = data->fd;

	return sendmsg(sockfd,&msg,0);

}

int SockIPC_ReadDevFd(int sockfd,SockDevfdData_t *data)
{
	int ret = 0;
	struct timeval tm;
	struct msghdr msg;
	struct iovec iov;
	char ctr_msg[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;
	int buffd = 0;
	fd_set fds;

	if(sockfd <= 0 || !data)
		return -1;

	FD_ZERO(&fds);
	FD_SET(0,&fds);
	FD_SET(sockfd,&fds);
	tm.tv_sec = 10;
	tm.tv_usec = 0;
	ret = select(sockfd+1,NULL,&fds,NULL,&tm);
	if(ret < 0)
		return -1;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	iov.iov_base = data->data;
	iov.iov_len = data->len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = ctr_msg;
	msg.msg_controllen = sizeof(ctr_msg);

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;

	ret = recvmsg(sockfd,&msg,0);
	if(ret < 0)
		return -1;
	memcpy(&buffd,CMSG_DATA(cmsg),sizeof(buffd));
	data->fd = buffd;

	return 0;
}





