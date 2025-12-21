/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/BufferMeta.h"
#include "lib_common/BufferAPI.h"
#include "lib_common/SliceConsts.h"
#include "lib_common_enc/RateCtrlStats.h"

/*************************************************************************//*!
   \brief MetaData gathering encode-statistics useful for rate-control
   algorithms
*****************************************************************************/
typedef struct AL_t_RateCtrlMetaData
{
  AL_TMetaData tMeta;
  bool bFilled;
  AL_RateCtrl_Statistics tRateCtrlStats;
  AL_TBuffer* pMVBuf;
}AL_TRateCtrlMetaData;

/*************************************************************************//*!
   \brief Create a RateCtrl metadata.
   \return Pointer to a RateCtrl Metadata if success, NULL otherwise
*****************************************************************************/
AL_TRateCtrlMetaData* AL_RateCtrlMetaData_Create(AL_TAllocator* pAllocator, AL_TDimension tDim, uint8_t uLcuSize, AL_ECodec eCodec);

/*@}*/

