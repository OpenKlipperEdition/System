/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once
#include "lib_rtos/types.h"

typedef enum
{
  DRIVER_SUCCESS,
  DRIVER_ERROR_UNKNOWN,
  DRIVER_ERROR_NO_MEMORY,
  DRIVER_ERROR_CHANNEL,
  DRIVER_TIMEOUT,
}AL_EDriverError;

#define AL_POLL_MSG 0xfffffffc

typedef struct AL_t_driver AL_TDriver;
typedef struct
{
  int (* pfnOpen)(AL_TDriver* driver, const char* device);
  void (* pfnClose)(AL_TDriver* driver, int fd);
  AL_EDriverError (* pfnPostMessage)(AL_TDriver* driver, int fd, long unsigned int messageId, void* data);
}AL_DriverVtable;

struct AL_t_driver
{
  const AL_DriverVtable* vtable;
};

static AL_INLINE
int AL_Driver_Open(AL_TDriver* driver, const char* device)
{
  return driver->vtable->pfnOpen(driver, device);
}

static AL_INLINE
void AL_Driver_Close(AL_TDriver* driver, int fd)
{
  driver->vtable->pfnClose(driver, fd);
}

static AL_INLINE
AL_EDriverError AL_Driver_PostMessage(AL_TDriver* driver, int fd, long unsigned int messageId, void* data)
{
  return driver->vtable->pfnPostMessage(driver, fd, messageId, data);
}

