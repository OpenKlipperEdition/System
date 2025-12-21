/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup lib_rtos
   @{
   \file
******************************************************************************/
#pragma once

#include <stddef.h> // for NULL and size_t
#include <stdint.h>
#include <stdbool.h>

#define AL_INTROSPECT(...)

#ifdef __GNUC__

#define _CRT_SECURE_NO_WARNINGS
#define __AL_ALIGNED__(x) __attribute__((aligned(x)))
#define AL_INLINE inline
#define AL_API extern
#define AL_DEPRECATED(msg) __attribute__((deprecated(msg)))

#ifndef __cplusplus
#define static_assert _Static_assert
#endif

#else // _MSC_VER

#define __AL_ALIGNED__(x)
#define __attribute__(x)
#define AL_INLINE __inline
#define AL_API extern
#define AL_DEPRECATED(msg) __declspec(deprecated(msg))

#ifndef __cplusplus
#define static_assert(assertion, ...) _STATIC_ASSERT(assertion)
#endif

#endif

#define AL_DEPRECATED_ENUM_VALUE(eType, name, val, msg) AL_DEPRECATED(msg) static const eType name = val

typedef uint64_t AL_64U __AL_ALIGNED__ (8); // Ensure that 64bits has same alignment on all platforms
typedef int64_t AL_64S;
typedef uint8_t* AL_VADDR; // keep byte pointer
typedef uint32_t AL_PADDR; // physical address, 32-bit address registers
typedef AL_64U AL_PTR64;
typedef uint32_t AL_ERR;
typedef void* AL_HANDLE;

#define AL_MAX_NUM_REF 16
#define AL_MAX_NUM_B_PICT 15

/*@}*/

