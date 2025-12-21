#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <dma-buf.h>
#include <reserveheap.h>

static void* alloc_node[10];
static int curId = 0;

/**
 * 先把alloc的空间保存到alloc_node中，当alloc node 足够了然后再统一释放。
 */
int save_mem(void *m)
{
	alloc_node[curId] = m;
	curId++;
	return curId < 10;
}

int main(int argc, char *argv[])
{
	int alloc_size[] = {10*1024,20*1024,30*1024,40*1024,50*1024};
	printf("total size: %x\n",reserveheap_get_size());
	for(int i = 0;i < 1000;i++) {
		int len = rand() % (sizeof(alloc_size) / sizeof(alloc_size[0]));
		printf("reserve memory size: %d\n",reserveheap_get_available());
		printf("test-> memsize: %x\n",alloc_size[len]);

		struct reserveheap *mem = malloc(sizeof(struct reserveheap));
		int ret = reserveheap_alloc(alloc_size[len],mem);
		assert(ret == 0);
		void* p = mem->vaddr;
		printf("vmem: vaddr->%x paddr->%x fd->%d size->%d\n",mem->vaddr,mem->paddr,mem->fd,mem->size);
		dmabuf_sync(mem->fd,DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ);
		memset(p,0,len);
		dmabuf_sync(mem->fd,DMA_BUF_SYNC_END | DMA_BUF_SYNC_WRITE);
		if(!save_mem(mem)) {
			for(int i = 0;i < curId;i++) {
				reserveheap_free(alloc_node[i]);
				free(alloc_node[i]);
			}
			curId = 0;
		}
	}

    return 0;
}
