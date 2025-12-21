#ifndef __PHY_MEMORY_ALLOCATOR_H__
#define __PHY_MEMORY_ALLOCATOR_H__

#include <linux/dma-buf.h>
#include "impp.h"

enum flush_flags
{
	MEM_FLUSH_START = 0,
	MEM_FLUSH_END = 4,
};

struct IHal_MemHandle *IHal_MemInit(char *name);
int IHal_MemDeinit(struct IHal_MemHandle *handler);
void *IHal_MemAlloc(struct IHal_MemHandle *handler, int size, IMPP_BufferInfo_t *out_info);
int IHal_MemFree(struct IHal_MemHandle *handler, IMPP_BufferInfo_t info);
int IHal_MemFlush(struct IHal_MemHandle *handler, IMPP_BufferInfo_t info, enum flush_flags flag);

#endif
