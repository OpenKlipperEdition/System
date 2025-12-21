/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/SliceConsts.h"
#include "lib_common/PixMapBuffer.h"

/*************************************************************************//*!
   \brief Reconstructed picture information
*****************************************************************************/
typedef struct
{
  uint32_t uID;
  AL_EPicStruct ePicStruct; /*!< Specifies the pic_struct: subset of table D-1 */
  uint32_t iPOC; /*!< the Picture Order Count of the frame buffer */
  AL_TDimension tPicDim; /*!< The dimension of the reconstructed frame buffer */
}AL_TReconstructedInfo;

typedef struct t_RecPic
{
  AL_TBuffer* pBuf;
  AL_TReconstructedInfo tInfo;
}AL_TRecPic;

uint32_t AL_GetRecPitch(uint32_t uBitDepth, uint32_t uWidth);
void AL_EncRecBuffer_FillPlaneDesc(AL_TPlaneDescription* pPlaneDesc, AL_TDimension tDim, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bIsAvc);

/*@}*/

