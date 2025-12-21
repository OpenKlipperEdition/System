#ifndef __RESERVEHEAP__
#define __RESERVEHEAP__

/**
 * 根据dma-buf fd 获取物理地址
 */
#define SHAREDMEM_ATTACH_PHY_ADDR		_IOW('F', 0x1, unsigned int)
#define SHAREDMEM_DEATTACH_PHY_ADDR		_IOW('F', 0x2, unsigned int)
#define SHAREDMEM_AVAILABLE		        _IOW('F', 0x3, unsigned int)
#define SHAREDMEM_TOTAL_SIZE            _IOW('F', 0x4, unsigned int)

struct reserveheap
{
	unsigned int paddr;  // 物理地址
	unsigned int size;   // 虚拟地址
	int fd;              // 虚拟地址 文件句柄fd
	void *vaddr;
};

/**
 *  dma buf 操作API接口
 *  dmabuf_alloc_fd: 根据内存大小分配 dma fd
 *  dmabuf_mmap:     根据fd map 虚拟内存
 *  dmabuf_munmap:   unmap
 *  dmabuf_phyaddr:  导出物理地址
 *  dmabuf_sync:     刷cache函数
 *  dmabuf_close:    关闭dmau句柄
 */

int  dmabuf_alloc_fd(int size);
void *dmabuf_mmap(int fd,int size);
void dmabuf_munmap(void *p,int size);
void dmabuf_sync(int fd,unsigned int flags);
void dmabuf_close(int fd);

int  reserveheap_open(void);
int  reserveheap_close(int heapfd);
int  reserveheap_attach_phyaddr(int heapfd,int dmabuf_fd);
int  reserveheap_available(int heapfd);
int  reserveheap_size(int heapfd);
int  reserveheap_deattach(int heapfd,int fd);

int  reserveheap_alloc(int size,struct reserveheap* mem);
int  reserveheap_get_available(void);
int  reserveheap_get_size(void);
int  reserveheap_import(struct reserveheap* mem);
void reserveheap_free(struct reserveheap* mem);

struct socket_info {
	int sockfd;
	int datafd;
	unsigned long size;
	unsigned int paddr;
};


#define SOCKET_NAME "shmem_socket"

int socket_receive_fd(struct socket_info *info);
int socket_send_fd(struct socket_info *info);

#endif /* __RESERVEHEAP__ */
