/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once

#include <assert.h>
#include "lib_rtos/types.h"
static AL_INLINE int AL_GetIntIdx(uint32_t uIntMask)
{
  int iIntIdx = 0;

  while((uIntMask & (1u << iIntIdx)) == 0)
    ++iIntIdx;

  assert(uIntMask == (1u << iIntIdx));
  return iIntIdx;
}

#define AL_REG_BASE 0x8000u

// Common Registers
#define AL_REG_WDT_CTRL (AL_REG_BASE + 0x028) // Shared between WatchDog & Timer
#define AL_REG_WDT_TIMEOUT (AL_REG_BASE + 0x02C)
#define AL_REG_TIMER_CTRL (AL_REG_BASE + 0x028) // Shared between WatchDog & Timer
#define AL_REG_TIMER_VALUE (AL_REG_BASE + 0x030)

// Reset register values

// Watchdog control register values
#define STOP_WDT 0
#define START_WDT 1
#define PAUSE_WDT 2
#define RESTART_WDT 3

