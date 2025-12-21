#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <dma-buf.h>
#include <dma-heap.h>

#include <reserveheap.h>
#include <ipcsocket.h>

#define EXPORT_SHAREDMEM_NAME "/dev/dma_heap/sharedmem"
#define SHAREDMEM_NAME "/dev/sharedmem"

#define LOGE(...) printf(__VA_ARGS__)
#define LOGI(...) printf(__VA_ARGS__)

static int __dma_heap_fd = -1;

static int __reserveheap_fd = -1;
static int __reserveheap_fd_ref = 0;

/**
 * @brief dmabuf_alloc_fd
 * @details 根据申请内存的大小分配DMABUF的句柄
 * @param[in] size 要分配的大小，需要页对齐
 * @return 分配的文件句柄
 */

int dmabuf_alloc_fd(int size)
{
	int ret;
	if(__dma_heap_fd < 0) {
		__dma_heap_fd = open(EXPORT_SHAREDMEM_NAME, O_RDWR);
		if(__dma_heap_fd < 0) {
			LOGE("open %s Failed!\n",EXPORT_SHAREDMEM_NAME);
			return -1;
		}
	}
	struct dma_heap_allocation_data data = {
		.len = size,
		.fd = 0,
		.fd_flags = O_RDWR | O_CLOEXEC,
		.heap_flags = 0,
	};

	ret = ioctl(__dma_heap_fd, DMA_HEAP_IOCTL_ALLOC, &data);
	if (ret < 0) {
		LOGE(" DMA_HEAP_IOCTL_ALLOC Failed %d!\n",ret);
		return -1;
	}
	return data.fd;
}

/**
 * @brief dmabuf_mmap
 * @details 根据文件句柄和内存大小映射虚拟内存地址
 * @param[in] fd 文件句柄
 * @param[in] size 内存大小
 * @return 虚拟内存指针
 */

void *dmabuf_mmap(int fd,int size)
{

	void* p = mmap(NULL,
			 size,
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED,
			 fd,
			 0);
	if (p == MAP_FAILED) {
		LOGE("mmap() failed: %s fd:%d size: %d\n",strerror(errno),fd,size);
		return NULL;
	}
	return p;
}

/**
 * @brief dmabuf_munmap
 * @details 虚拟地址反映射
 * @param[inout] p 虚拟内存，是dmabuf_mmap映射出来的地址
 * @param[in] size 虚拟内存大小
 */

void dmabuf_munmap(void *p,int size)
{
	if(p)
		munmap(p,size);
}

/**
 * @brief reserveheap_open
 * @details 打开sharedmem heap的节点
 * @return 返回文件句柄
 */

int reserveheap_open(void)
{
	int ifd = open(SHAREDMEM_NAME, O_RDWR);
	if(ifd < 0) {
		LOGE("open %s Failed! %s\n",SHAREDMEM_NAME,strerror(errno));
		return 0;
	}
	return ifd;
}

/**
 * @brief reservehep_close
 * @details 关闭sharedmem文件句柄
 * @param[in] heapfd 文件句柄
 */

void reservehep_close(int heapfd)
{
	if(heapfd)
		close(heapfd);
}

/**
 * @brief reservehead_attach_phyaddr
 * @details 根据文件句柄从sharedmem heap上获取物理地址，并保持不管闭
 * @param[in] heapfd sharedmem heap文件句柄
 * @param[in] dmabuf fd 文件句柄
 * @return 物理地址
 */

int reserveheap_attach_phyaddr(int heapfd,int dmabuf_fd)
{
	unsigned int phyaddr;
	int ret;

	phyaddr = dmabuf_fd;
	ret = ioctl(heapfd, SHAREDMEM_ATTACH_PHY_ADDR, &phyaddr);
	if(ret < 0) {
		LOGE("SHAREDMEM_ATTACH_PHY_ADDR: %s\n",strerror(errno));
		phyaddr = 0;
	}

	return phyaddr;
}

/**
 * @brief reserveheap_available
 * @details 根据文件句柄从sharedmem heap上获取reserve memory的剩余空间
 * @param[in] heapfd sharedmem heap文件句柄
 * @return 尺寸
 */

int reserveheap_available(int heapfd)
{
	unsigned int available = 0;
	int ret;

	ret = ioctl(heapfd, SHAREDMEM_AVAILABLE, &available);
	if(ret < 0) {
		LOGE("SHAREDMEM_AVAILABLE: %s\n",strerror(errno));
	}

	return available;
}


/**
 * @brief reserveheap_available
 * @details 根据文件句柄从sharedmem heap上获取reserve memory的所有空间
 * @param[in] heapfd sharedmem heap文件句柄
 * @return 尺寸
 */

int reserveheap_size(int heapfd)
{
	unsigned int total = 0;
	int ret;

	ret = ioctl(heapfd, SHAREDMEM_TOTAL_SIZE, &total);
	if(ret < 0) {
		LOGE("SHAREDMEM_AVAILABLE: %s\n",strerror(errno));
	}

	return total;
}
/**
 * @brief reserveheap_deattach
 * @details 把从sharedmem heap 中的物理地址deattach，
 * @param[in] heapfd sharedmem heap的文件句柄
 * @param[in] fd dmabuf的文件句柄
 * @return 0: OK, -1 失败
 */

int reserveheap_deattach(int heapfd,int fd)
{
	int ret;
	ret = ioctl(heapfd, SHAREDMEM_DEATTACH_PHY_ADDR, &fd);
	if(ret < 0) {
		LOGE("SHAREDMEM_DEATTACH_PHY_ADDR: %s\n",strerror(errno));
	}
	return ret;
}


/**
 * @brief dmabuf_sync
 * @details 刷cache函数，在地址需要给其他设备使用的时候需要刷cache，保证cache与DDR数据一致
 *          在从其他设备接收到的内存需要读写内存前刷cache
 *          把写好的内存发给其他设备，需要写好内存后，刷内存。
 * @param[in] fd 文件句柄，有dmabuf_alloc分配出来。
 * @param[in] flags 刷Cache的标识，
 *           当从设备读取的数据时候,需使用 (DMA_BUF_SYNC_READ|DMA_BUF_SYNC_START)
 *           当把数据写给设备的时候,需使用 (DMA_BUF_SYNC_WRITE|DMA_BUF_SYNC_END)
 */

void dmabuf_sync(int fd,unsigned int flags)
{
	struct dma_buf_sync sync = {
		.flags = flags,
	};
	int ret;

	ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (ret)
		LOGE("sync failed %s\n", strerror(errno));
}

/**
 * @brief dmabuf_close
 * @details 关闭由dmabuf_alloc分配的句柄
 * @param[in] fd 文件句柄
 */

void dmabuf_close(int fd)
{
	if(fd >= 0)
		close(fd);
}

/**
 * @brief reserveheap_get_available
 * @details 获取reserve memory的剩余空间
 * @return  可以获取的空间大小
 */

int reserveheap_get_available(void)
{
	if(__reserveheap_fd < 0)
		__reserveheap_fd = reserveheap_open();

	if(__reserveheap_fd < 0)
		return 0;
	return reserveheap_available(__reserveheap_fd);
}

/**
 * @brief reserveheap_get_size
 * @details 获取reserve memory的g总共空间
 * @return  可以获取的空间大小
 */

int reserveheap_get_size(void)
{
	if(__reserveheap_fd < 0)
		__reserveheap_fd = reserveheap_open();

	if(__reserveheap_fd < 0)
		return 0;
	return reserveheap_size(__reserveheap_fd);
}

/**
 * @brief reserveheap_alloc
 * @details 共享内存分配，使用dmabuf分配内存，是dmabuf的二次封装
 * @param[in] size 需要分配的内存大小，要小于内核reserve的大小。
 * @param[out] mem 内存申请返回的结构体，参考struct reserveheap
 * @return 0: OK, -1: FAIL
 */
int reserveheap_alloc(int size,struct reserveheap* mem)
{
	int fd;

 	if(!mem) {
		LOGE("ERROR: mem is NULL.\n");
		return -1;
	}
	fd = dmabuf_alloc_fd(size);
	if(fd < 0) {
		return -1;
	}
	mem->size = size;
	mem->vaddr = dmabuf_mmap(fd,size);
	mem->fd = fd;
	if(!mem->vaddr)	{
		return -1;
	}
	if(__reserveheap_fd < 0)
		__reserveheap_fd = reserveheap_open();

	if(__reserveheap_fd < 0)
		return -1;

	__reserveheap_fd_ref++;
	mem->paddr = reserveheap_attach_phyaddr(__reserveheap_fd,fd);
	if(!mem->paddr)	{
		return -1;
	}
	return 0;
}

/**
 * @brief reserveheap_import
 * @details 导入内存，存在fd，和映射的size时，映射内存的操作
 * @param[inout] mem 传入的mem结构，注意fd，size必须赋值，
                    paddr如果小于等于0表示需要获取，否则不需要获取
 * @return 0: OK, -1: FAIL
 */

int reserveheap_import(struct reserveheap* mem)
{
	if(!mem || mem->fd < 0 || mem->size <= 0) {
		LOGE("ERROR: argments is invalid.\n");
		return -1;
	}

	mem->vaddr = dmabuf_mmap(mem->fd,mem->size);
	if(__reserveheap_fd < 0)
		__reserveheap_fd = reserveheap_open();
	if(__reserveheap_fd < 0)
		return -1;
	__reserveheap_fd_ref++;

	mem->paddr = reserveheap_attach_phyaddr(__reserveheap_fd,mem->fd);
	if(!mem->paddr) {
		return -1;
	}
	return 0;
}


/**
 * @brief reserveheap_free
 * @details 释放由reserveheap_alloc分配来的内存，并关闭文件句柄
 * @param[inout] mem reserveheap结构体
 */

void reserveheap_free(struct reserveheap* mem)
{
	if(mem == NULL)
		return;
	if(__reserveheap_fd >= 0)
		reserveheap_deattach(__reserveheap_fd,mem->fd);
	dmabuf_munmap(mem->vaddr,mem->size);
	dmabuf_close(mem->fd);
	if(--__reserveheap_fd_ref == 0)
	{
		reservehep_close(__reserveheap_fd);
		__reserveheap_fd = -1;
	}
}

/**
 * @brief socket_send_fd
 * @details 利用socket向其他经常发送本进程申请的dmabuf的文件句柄fd
 * @param[inout] info socket 发送的信息，包括要传输的fd，和size
 * @return 等于0表示发送成功，其他都是错误
 */

int socket_send_fd(struct socket_info *info)
{
	int status;
	int fd, sockfd;
	struct socketdata skdata;

	if (!info) {
		LOGE("<%s>: Invalid socket info\n", __func__);
		return -1;
	}

	sockfd = info->sockfd;
	fd = info->datafd;
	memset(&skdata, 0, sizeof(skdata));
	skdata.fd = fd;
	skdata.data[0] = info->size;
	skdata.data[1] = info->paddr;
	skdata.len = 8;

	status = sendtosocket(sockfd, &skdata);
	if (status < 0) {
		LOGE("<%s>: Failed: sendtosocket\n", __func__);
		return -1;
	}

	return 0;
}

/**
 * @brief socket_receive_fd
 * @details 接收由其他进程发送来的dmabuf的文件句柄fd，和size信息
 * @param[inout] info socket信息
 * @return 等于0表示发送成功，其他都是错误
 */

int socket_receive_fd(struct socket_info *info)
{
	int status;
	int fd, sockfd;
	struct socketdata skdata;

	if (!info) {
		fprintf(stderr, "<%s>: Invalid socket info\n", __func__);
		return -1;
	}

	sockfd = info->sockfd;
	memset(&skdata, 0, sizeof(skdata));
	skdata.len = 8;
	status = receivefromsocket(sockfd, &skdata);
	if (status < 0) {
		LOGE("<%s>: Failed: receivefromsocket\n", __func__);
		return -1;
	}
	info->datafd = skdata.fd;
	info->size = skdata.data[0];
	info->paddr = skdata.data[1];

	return status;
}
