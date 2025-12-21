/* Copyright (C) 2019 Allegro DVT2.  All rights reserved. */
/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_rtos/lib_rtos.h"
#include "lib_common_enc/Settings.h"

#include "lib_common/Allocator.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/BufferMeta.h"
#include "lib_common/PixMapBuffer.h"

#define AL_MAX_PLDESC_CNT   8

typedef struct TFrameInfo
{
  int iWidth;
  int iHeight;
  uint8_t iBitDepth;
  AL_EChromaMode eCMode;
} TFrameInfo;

/*************************************************************************//*!
   \brief AL_TBufPoolConfig: Used to configure the AL_TBufPool
*****************************************************************************/
typedef struct al_t_BufPoolConfig
{
  uint32_t uNumBuf; /*!< number of buffer in the pool */
  size_t zBufSize;/*!< Size of the buffers that will fill the pool */
  char const* debugName;
  AL_TMetaData* pMetaData;/*!< Metadata of the buffer that will fill the pool */
  bool bUseExtraBuf;
}AL_TBufPoolConfig;

typedef struct al_t_PixMapConfig
{
  uint32_t uNumBuf; /*!< number of buffer in the pool */
  char const* debugName;
  AL_TDimension tDim;
  TFourCC tFourCC;
  bool bUseExtraBuf;

  AL_TPlaneDescription plDesc[AL_MAX_PLDESC_CNT];
  int plDescCnt;
  size_t zBufSize;  /*!< Size of the buffers that will fill the pool */
}AL_TPixMapBufPollConfig;

typedef struct
{
  size_t m_zMaxElem;
  size_t m_zTail;
  size_t m_zHead;
  void** m_ElemBuffer;
  AL_MUTEX hMutex;
  AL_EVENT hEvent;
  int m_iBufNumber;
  bool m_isDecommited;
  AL_SEMAPHORE hSpaceSem;
}App_Fifo;

/*************************************************************************//*!
   \brief Buffer Access mode: Do we want to wait if no buffer is available or to fail fast.
*****************************************************************************/
typedef enum
{
  AL_BUF_MODE_BLOCK,
  AL_BUF_MODE_NONBLOCK,
  /* sentinel */
  AL_BUF_MODE_MAX
}AL_EBufMode;

uint32_t AL_GetWaitMode(AL_EBufMode eMode);

/*************************************************************************//*!
   \brief AL_TBufPool: Pool of buffer
*****************************************************************************/
typedef struct
{
  AL_TAllocator* pAllocator; /*! Allocator used to allocate the buffers */

  AL_TBuffer** pPool; /*! pool of allocated buffers */
  uint32_t uNumBuf; /*! Number of buffer in the pool */

  size_t zBufSize; /*! Size of the buffers in the pool */
  AL_TMetaData* pCreationMeta; /*! Metadata added at buffers creation */

  App_Fifo fifo;
}AL_TBufPool;

/*************************************************************************//*!
   \brief AL_BufPool_Init Initialize the AL_TBufPool
   \param[in] pBufPool Pointer to an AL_TBufPool
   \param[in] pAllocator Pointer to a real allocator
   \param[in] pConfig Pointer to an AL_TBufPoolConfig object
   \return return true on success, false on failure
*****************************************************************************/
bool AL_BufPool_Init(AL_TBufPool* pBufPool, AL_TAllocator* pAllocator, AL_TBufPoolConfig* pConfig);

/*************************************************************************//*!
   \brief AL_BufPool_Deinit Deiniatilize the AL_TBufPool
   \param[in] pBufPool Pointer to an AL_TBufPool
*****************************************************************************/
void AL_BufPool_Deinit(AL_TBufPool* pBufPool);

/*************************************************************************//*!
   \brief AL_BufPool_GetBuffer Get a buffer from the pool
   \param[in] pBufPool Pointer to an AL_TBufPool
   \param[in] eMode Get mode. blocking or non blocking
   \return return the buffer or NULL in case of failure in the non blocking case
*****************************************************************************/
AL_TBuffer* AL_BufPool_GetBuffer(AL_TBufPool* pBufPool, AL_EBufMode eMode);
/*************************************************************************//*!
   \brief AL_BufPool_AddMetaData creates and adds a metadata on all buffers (even if referenced)
   \param[in] pBufPool Pointer to an AL_TBufPool
   \param[in] pMeta Pointer to a metadata
   \return return true on success, false on failure
*****************************************************************************/
bool AL_BufPool_AddMetaData(AL_TBufPool* pBufPool, AL_TMetaData* pMeta);
/*************************************************************************//*!
   \brief AL_BufPool_Decommit Decommit the pool. This deblocks all the blocking
   call to AL_BufPool_GetBuffer.
   \param[in] pBufPool Pointer to an AL_TBufPool.
*****************************************************************************/
void AL_BufPool_Decommit(AL_TBufPool* pBufPool);

/*****************************************************************************/

bool Fifo_Init(App_Fifo* pFifo, size_t zMaxElem);
void Fifo_Deinit(App_Fifo* pFifo);
bool Fifo_Queue(App_Fifo* pFifo, void* pElem, uint32_t uWait);
void* Fifo_Dequeue(App_Fifo* pFifo, uint32_t uWait);
void Fifo_Decommit(App_Fifo* pFifo);
size_t Fifo_GetMaxElements(App_Fifo* pFifo);

int ComputeYPitch(int iWidth, TFourCC tFourCC, int extStride);
int GetCPlanePitch(int iPitchY, TFourCC fourCC);

AL_TBufPoolConfig GetBufPoolConfig(const char* debugName, AL_TMetaData* pMetaData, int iSize, int frameBuffersCount, bool bUseExtraBuf);
AL_TBufPoolConfig GetQpBufPoolConfig(AL_TEncSettings* Settings, AL_TEncChanParam* tChParam, int frameBuffersCount, bool bUseExtraBuf);
void GetPixMapBufPollConfig(AL_TPixMapBufPollConfig *pConfig, TFrameInfo *FrameInfo, AL_ESrcMode eSrcMode, int frameBuffersCount, int extStride, int extLine, bool bUseExtraBuf);
int PixMapBufPollInit(AL_TBufPool* pPixMapBufPool, AL_TAllocator* pAllocator, AL_TPixMapBufPollConfig *pConfig);
AL_TBufPoolConfig GetStreamBufPoolConfig(AL_TEncSettings* Settings, int iLayerID, bool bUseExtraBuf, int baseStreamCnt, uint32_t baseStreamSize);
TFrameInfo GetFrameInfo(AL_TEncChanParam *tChParam);

/*****************************************************************************/

/*@}*/
