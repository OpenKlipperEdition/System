/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \defgroup lib_rtos RTOS

   The Rtos functions wrap all calls to the operating system facilities
   like memory allocation, thread and synchronisation primitives.
   They can be reimplemented by the user to fit his platform of choice.

   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/****************************************************************************/
typedef void* AL_MUTEX;
typedef void* AL_SEMAPHORE;
typedef void* AL_EVENT;
typedef void* AL_THREAD;

/****************************************************************************/
#define AL_NO_WAIT 0
#define AL_WAIT_FOREVER 0xFFFFFFFF

/****************************************************************************/

/****************************************************************************/
/*  Memory */
/****************************************************************************/
void* Rtos_Malloc(size_t zSize);
void Rtos_Free(void* pMem);

void* Rtos_Memcpy(void* pDst, void const* pSrc, size_t zSize);
void* Rtos_Memmove(void* pDst, void const* pSrc, size_t zSize);
void* Rtos_Memset(void* pDst, int iVal, size_t zSize);
int Rtos_Memcmp(void const* pBuf1, void const* pBuf2, size_t zSize);

/****************************************************************************/
/*  Clock */
/****************************************************************************/
AL_64U Rtos_GetTime(void);
void Rtos_Sleep(uint32_t uMillisecond);

/****************************************************************************/
/*  Mutex */
/****************************************************************************/
AL_MUTEX Rtos_CreateMutex(void);
void Rtos_DeleteMutex(AL_MUTEX Mutex);
bool Rtos_GetMutex(AL_MUTEX Mutex);
bool Rtos_ReleaseMutex(AL_MUTEX Mutex);

/****************************************************************************/
/*  Semaphore */
/****************************************************************************/
AL_SEMAPHORE Rtos_CreateSemaphore(int iInitialCount);
void Rtos_DeleteSemaphore(AL_SEMAPHORE Semaphore);
bool Rtos_GetSemaphore(AL_SEMAPHORE Semaphore, uint32_t Wait);
bool Rtos_ReleaseSemaphore(AL_SEMAPHORE Semaphore);

/****************************************************************************/
/*  Event */
/****************************************************************************/
AL_EVENT Rtos_CreateEvent(bool iInitialState);
void Rtos_DeleteEvent(AL_EVENT Event);
bool Rtos_WaitEvent(AL_EVENT Event, uint32_t Wait);
bool Rtos_SetEvent(AL_EVENT Event);

/****************************************************************************/
/*  Threads */
/****************************************************************************/
AL_THREAD Rtos_CreateThread(void* (*pFunc)(void* pParam), void* pParam);
void Rtos_SetCurrentThreadName(const char* pThreadName);
bool Rtos_JoinThread(AL_THREAD Thread);
void Rtos_DeleteThread(AL_THREAD Thread);

/****************************************************************************/
/*  Driver */
/****************************************************************************/
void* Rtos_DriverOpen(char const* name);
void Rtos_DriverClose(void* drv);
int Rtos_DriverIoctl(void* drv, unsigned long int req, void* data);

#define AL_POLLPRI 1
#define AL_POLLIN 2
#define AL_POLLOUT 4
#define AL_POLLERR 8

int Rtos_DriverPoll(void* drv, int timeout, unsigned long flags);

/****************************************************************************/
/*  Atomics */
/****************************************************************************/
int32_t Rtos_AtomicIncrement(int32_t* iVal);
int32_t Rtos_AtomicDecrement(int32_t* iVal);

/****************************************************************************/
/*  Cache Memory coherency */
/****************************************************************************/
void Rtos_InvalidateCacheMemory(void* pMem, size_t zSize);
void Rtos_FlushCacheMemory(void* pMem, size_t zSize);

/****************************************************************************/

/*@}*/

