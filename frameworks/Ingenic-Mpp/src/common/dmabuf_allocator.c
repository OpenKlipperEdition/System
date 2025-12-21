#include <stdlib.h>
#include <string.h>
#include <dma-buf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dma-heap.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "impp.h"
#include "list.h"
#include "log.h"

char LOGTAG[20] = "DMABUF_ALLOC";

#define SHAREMEMNODE "/dev/sharedmem"
#define HEAPNODE     "/dev/dma_heap/sharedmem"

#define NAME_LEN 20

#define SHAREDMEM_ATTACH_PHY_ADDR               _IOW('F', 0x1, unsigned int)
#define SHAREDMEM_DEATTACH_PHY_ADDR             _IOW('F', 0x2, unsigned int)
#define SHAREDMEM_AVAILABLE                     _IOW('F', 0x3, unsigned int)
#define SHAREDMEM_TOTAL_SIZE                    _IOW('F', 0x4, unsigned int)


struct IHal_MemHandle {
        struct list_head list;  // 链表头，存储申请到的buffer的基础信息
        char ownner[NAME_LEN];    // 句柄拥有者信息
        int sharememFd;     // /dev下的sharemem节点，通过该节点获取物理地址
        int heapFd;         // /dev/heap下的sharemem节点,通过该节点申请内存
};

enum flush_flags
{
	MEM_FLUSH_START = 0,
	MEM_FLUSH_END = 4,
};

struct MemChunk
{
   struct list_head list;
   IMPP_BufferInfo_t info;
};


struct IHal_MemHandle *IHal_MemInit(char *name)
{
    struct IHal_MemHandle *handler = NULL;
    int fd = 0;

    handler = malloc(sizeof(struct IHal_MemHandle));
    if (handler == NULL)
    {
        IMPP_LOGE(LOGTAG, "can't malloc struct IHal_MemHandle memory\n");
        return NULL;
    }

    if (name != NULL)
    {
        strncpy((char *)&handler->ownner, name, NAME_LEN);
        strncpy(LOGTAG, name, NAME_LEN);
    }

    INIT_LIST_HEAD(&handler->list);

    fd = open(SHAREMEMNODE, O_RDWR);
    if (fd < 0)
    {
        IMPP_LOGE(LOGTAG, "can't open %s\n", SHAREMEMNODE);
        goto free_handler;
    }

    handler->sharememFd = fd;

    fd = open(HEAPNODE, O_RDWR);
    if (fd < 0)
    {
        IMPP_LOGE(LOGTAG, "can't open %s\n", HEAPNODE);
        goto close_sharememfd;
    }

    handler->heapFd = fd;

    return handler;
close_sharememfd:
    close(handler->sharememFd);
free_handler:
    free(handler);

    return NULL;
}


int IHal_MemDeinit(struct IHal_MemHandle *handler)
{
    if (handler == NULL)
    {
        IMPP_LOGE(LOGTAG, "param error\n");
        return -1;
    }

    int ret = 0;
    struct MemChunk *temp = NULL;
    struct MemChunk *prev_entry = NULL;

    if (!list_empty(&handler->list))
    {
        list_for_each_entry(temp, &handler->list, list)
        {
	    prev_entry = container_of(temp->list.prev, struct MemChunk, list);
            ret = munmap((void *)temp->info.vaddr, temp->info.size);
            if (ret < 0)
            {
                IMPP_LOGE(LOGTAG, "munmap error\n");
                return -2;
            }

            close(temp->info.fd);
            list_del(&temp->list);
            free(temp);
	    temp = prev_entry;
        }
    }

    close(handler->heapFd);
    close(handler->sharememFd);

    return 0;
}

void *IHal_MemAlloc(struct IHal_MemHandle *handler, int size, IMPP_BufferInfo_t *out_info)
{
    if (handler == NULL || out_info == NULL)
    {
        IMPP_LOGE(LOGTAG, "param error\n");
        return NULL;
    }

    struct MemChunk *chunk = NULL;
    chunk = malloc(sizeof(struct MemChunk));
    if (chunk == NULL)
    {
        IMPP_LOGE(LOGTAG, "memory not enough\n");
        return NULL;
    }

    int ret;
    struct dma_heap_allocation_data data = {
        .len = size,
        .fd = 0,
        .fd_flags = O_RDWR | O_CLOEXEC,
        .heap_flags = 0,
    };

    ret = ioctl(handler->heapFd, DMA_HEAP_IOCTL_ALLOC, &data);
    if (ret < 0)
    {
        IMPP_LOGE(LOGTAG, "dma-buf alloc error\n");
        return NULL;
    }

    void* ptr = mmap(NULL,
                 size,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 data.fd,
                 0);
    if (ptr == MAP_FAILED) {
        IMPP_LOGE(LOGTAG, "mmap() failed: %s fd:%d size: %d\n", strerror(errno), data.fd, size);
        goto map_err;
    }

    unsigned int mem_phyaddr = 0;
    mem_phyaddr = data.fd;

    ret = ioctl(handler->sharememFd, SHAREDMEM_ATTACH_PHY_ADDR, &mem_phyaddr);
    if(ret < 0) {
        IMPP_LOGE(LOGTAG, "SHAREDMEM_ATTACH_PHY_ADDR: %s\n", strerror(errno));
        mem_phyaddr = 0;
        goto attach_phy_err;
    }

    ret = ioctl(handler->sharememFd, SHAREDMEM_DEATTACH_PHY_ADDR, &data.fd);
    if(ret < 0) {
        IMPP_LOGE(LOGTAG, "SHAREDMEM_DEATTACH_PHY_ADDR: %s\n", strerror(errno));
        goto deattach_phy_err;
    }

    out_info->fd = (int)data.fd;
    out_info->paddr = mem_phyaddr;
    out_info->vaddr = (IHAL_UINT32)ptr;
    out_info->size  = size;

    INIT_LIST_HEAD(&chunk->list);
    chunk->info.fd     =  data.fd;
    chunk->info.paddr  =  mem_phyaddr;
    chunk->info.vaddr  =  (IHAL_UINT32)ptr;
    chunk->info.size   =  size;
    list_add_tail(&chunk->list, &handler->list);
    IMPP_LOGI(LOGTAG, "alloc phy memory success fd = %d vaddr = %#x paddr = %#x size = %d\n", data.fd, ptr, mem_phyaddr, size);
    return ptr;

deattach_phy_err:
    ioctl(handler->sharememFd, SHAREDMEM_DEATTACH_PHY_ADDR, &data.fd);
attach_phy_err:
    munmap(ptr, size);
map_err:
    close(data.fd);

    return NULL;
}

int IHal_MemFree(struct IHal_MemHandle *handler, IMPP_BufferInfo_t info)
{
    if (handler == NULL)
    {
        IMPP_LOGE(LOGTAG, "param error\n");
        return -1;
    }

    struct MemChunk *temp = NULL;
    int ret = 0;

    list_for_each_entry(temp, &handler->list, list)
    {
        if (temp->info.paddr == info.paddr)
            break;
    }

    if (&temp->list == &handler->list)
    {
        IMPP_LOGE(LOGTAG, "the param phyaddr error\n");
        return -2;
    }

    ret = munmap((void *)temp->info.vaddr, temp->info.size);
    if (ret < 0)
    {
        IMPP_LOGE(LOGTAG, "munmap error\n");
        return -3;
    }

    close(temp->info.fd);
    list_del(&temp->list);
    free(temp);

    return 0;
}

int IHal_MemFlush(struct IHal_MemHandle *handler, IMPP_BufferInfo_t info, enum flush_flags flag)
{
    if (handler == NULL)
    {
        IMPP_LOGE(LOGTAG, "param error\n");
        return -1;
    }

    struct dma_buf_sync sync = {
        .flags = flag | DMA_BUF_SYNC_RW,
    };
    int ret = 0;

    ret = ioctl(info.fd, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret)
        IMPP_LOGE(LOGTAG, "sync failed %d\n", errno);

    return ret;
}

