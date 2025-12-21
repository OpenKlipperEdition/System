/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Allocator
   @{
   \file
 *****************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_common/Allocator.h"

/*************************************************************************//*!
   \brief Retrieves Allocator interface
*****************************************************************************/
AL_TAllocator* AL_LibRef_GetAllocator(void);
AL_TAllocator* AL_LibRef_GetAllocatorWithBaseAddr(AL_PADDR BaseAddr);
AL_VADDR AL_LibRef_GetRegAddr(AL_PADDR RegAddr);

#ifdef __cplusplus
}
#endif

/*@}*/

