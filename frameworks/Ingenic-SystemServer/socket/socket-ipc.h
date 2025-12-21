#ifndef __SOCK_IPC_H__
#define __SOCK_IPC_H__

typedef struct _sock_msg {
	char* buf;
	//unsigned int bufsize;	/* data buf size  */
	unsigned int len;	/*read or write data len*/
}SockIpcMsg_t;

typedef struct _sock_devfd {
	int fd;		//devfd
	char *data;	// helper data
	int len;	//data len
}SockDevfdData_t;

/**
 * @brief Socket IPC Server初始化
 * @param [in] name : sun_path
 * @retval socket_fd 成功
 * @retval -1        失败
 */
int SockIPC_ServerInit(char* name);	

/**
 * @brief Socket IPC Server接受连接
 * @param [in] socket_fd
 * @retval accept_fd 成功
 * @retval -1        失败
 */
int SockIPC_ServerAccept(int sock_fd);

/**
 * @brief Socket IPC Server关闭
 * @param [in] fd : socket_fd
 * @retval 0         成功
 * @retval -1        失败
 */
int SockIPC_ServerClose(int fd);

/**
 * @brief Socket IPC Client初始化
 * @param [in] name : sun_path 通常是/tmp/xxx
 * @retval socket_fd 成功
 * @retval -1        失败
 */
int SockIPC_ClientInit(char* name);

/**
 * @brief Socket IPC Client关闭
 * @param [in] fd : socket fd
 * @retval 0		 成功
 * @retval -1        失败
 */
int SockIPC_ClientClose(int fd);

/**
 * @brief Socket IPC 发送消息
 * @param [in] sockfd
 * @retval n         成功发送的字节数
 * @retval -1        失败
 */
int SockIPC_WriteMsg(int sockfd,SockIpcMsg_t *msg);

/**
 * @brief Socket IPC 读取消息
 * @param [in] sockfd
 * @retval n         成功读取的字节数
 * @retval -1        失败
 */
int SockIPC_ReadMsg(int sockfd,SockIpcMsg_t *msg);

/**
 * @brief Socket IPC 发送文件描述符和数据
 * @param [in] Sockdevfddata_t
 * @retval 0         成功
 * @retval -1        失败
 */
int SockIPC_SendDevFd(int sockfd,SockDevfdData_t *data);

/**
 * @brief Socket IPC 接收文件描述符和数据
 * @param [in] Sockdevfddata_t
 * @retval 0         成功
 * @retval -1        失败
 */
int SockIPC_ReadDevFd(int sockfd,SockDevfdData_t *data);

#endif	//__SOCK_IPC_H__




