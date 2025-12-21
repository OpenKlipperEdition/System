/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once

#include "lib_rtos/lib_rtos.h"

typedef struct
{
  uint64_t timestamp;
  char label[32];
}LogEvent;

typedef struct AL_t_Timer AL_Timer;
typedef struct
{
  uint32_t (* pfnGetTime)(AL_Timer* timer);
}AL_TimerVtable;

struct AL_t_Timer
{
  const AL_TimerVtable* vtable;
};

#include "lib_ip_ctrl/IpCtrl.h"
typedef struct
{
  const AL_TimerVtable* vtable;
  AL_TIpCtrl* ip;
}AL_HwTimer;
AL_Timer* AL_HwTimerInit(AL_HwTimer* timer, AL_TIpCtrl* ip);
extern AL_HwTimer g_HwTimer;

typedef struct
{
  const AL_TimerVtable* vtable;
}AL_CpuTimer;
AL_Timer* AL_CpuTimerInit(AL_CpuTimer* timer);
extern AL_CpuTimer g_CpuTimer;

typedef struct
{
  AL_Timer* timer;
  LogEvent* events;
  int count;
  int maxCount;
  AL_MUTEX mutex;
}AL_Logger;

static AL_INLINE uint32_t AL_Timer_GetTime(AL_Timer* timer)
{
  return timer->vtable->pfnGetTime(timer);
}

extern AL_Logger g_Logger;

void AL_LoggerInit(AL_Logger* logger, AL_Timer* timer, LogEvent* buffer, int maxCount);
void AL_LoggerDeinit(AL_Logger* logger);
void AL_Log(AL_Logger* logger, const char* label);

