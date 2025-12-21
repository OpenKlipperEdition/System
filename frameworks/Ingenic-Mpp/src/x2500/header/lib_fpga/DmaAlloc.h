/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Allocator
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_common/Allocator.h"

/**************************************************************************//*!
   \brief Create an allocator supporting dma allocations
   Dma buffers are required for all the buffers used by the hardware ip.
   On a typical platform, use AL_BOARD_ENCODER_NAME for the encoder and
   "AL_BOARD_DECODER_NAME" for the decoder
   \param[in] deviceFile the device file of the driver that provides
   the dma allocation facilities
 *****************************************************************************/
AL_TAllocator* AL_DmaAlloc_Create(const char* deviceFile);
AL_TAllocator* AL_DmaAlloc_GetAllocator(int poolId);

/**************************************************************************//*!
   \brief Destroy an allocator supporting dma allocations
   \param[in] pAllocator point to a dma allocator which will be destroyed
 *****************************************************************************/
void AL_DmaAlloc_Destroy(AL_TAllocator* pAllocator);
int AL_DmaAlloc_FlushCache(void *vaddr, size_t size, uint32_t dir);

/*@}*/

