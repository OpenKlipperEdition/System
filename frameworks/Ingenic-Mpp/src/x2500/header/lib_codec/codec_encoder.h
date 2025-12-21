/* Copyright (C) 2019 Ingenic.  All rights reserved. */

/**************************************************************************//*!
   \addtogroup Codec
   @{
   \file
******************************************************************************/

#ifndef __CODEC_ENCODER_H__
#define __CODEC_ENCODER_H__

#pragma once

#include "lib_common/Allocator.h"
#include "lib_perfs/Logger.h"
#include "lib_common_enc/Settings.h"
#include "lib_encode/lib_encoder.h"
#include "BufPool.h"
#include "lib_codec.h"
#include "lib_codec/ROIMngr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AL_MAX_NUM_CODEC_ENCODE			6
#define AL_MAX_DEBUGNAME_LEN        32

typedef struct t_Scheduler TScheduler;

typedef struct AL_t_Codec AL_Codec;

typedef struct AL_t_Codec {
	AL_TIpCtrl        *m_pIpCtrl;
	AL_TAllocator     *m_pAllocator;
	AL_IEncScheduler  *m_pScheduler;
	AL_Timer          *m_pTimer;
	AL_MUTEX          m_codecMutex;
	AL_CodecEncode    *m_pCodecEncode[AL_MAX_NUM_CODEC_ENCODE];
	int               m_codecNum;
} AL_Codec;

struct AL_t_CodecEncode {
	AL_Codec             *m_pCodec;
	AL_CodecParam        m_codecParam;
	AL_HEncoder          m_hEncoder;
  AL_EVENT             m_doneEvent;
	AL_CB_EndEncoding    m_onEndEncoding;
	int                  m_indexInALCodec;
	AL_TBufPoolConfig    m_StreamBufPoolConfig;
	AL_TBufPool          m_StreamBufPool;
	AL_TBufPoolConfig    m_QpBufPoolConfig;
	AL_TBufPool          m_QpBufPool;
	AL_TBuffer           *m_pQpbuf;
	uint8_t              *m_pQps;
	AL_TRoiMngrCtx       *m_RoiCtx;
	int                  m_iNumQPPerLCU;
	int                  m_iNumBytesPerLCU;
	App_Fifo             m_pStreamFilledFifo;
	App_Fifo             m_pSrcFilledFifo;
	AL_TPixMapBufPollConfig  m_SrcBufPoolConfig;
	AL_TBufPool          m_SrcBufPool;
	TFourCC              m_SrcFourCC;
	int                  m_frameBuffersCount;
	int                  m_pictureType;
};

const char* AL_Encoder_ErrorToString(AL_ERR eErr);

#ifdef __cplusplus
}
#endif

#endif // __CODEC_ENCODER_H__
/*@}*/
