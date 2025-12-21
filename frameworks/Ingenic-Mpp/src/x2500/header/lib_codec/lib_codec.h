/* Copyright (C) 2019 Ingenic.  All rights reserved. */
/**************************************************************************//*!
   \defgroup lib_codec Codec

	 The struct alcodec defines an interface to encapsulate the lib_encode which can
	 be called by the sdk and exec_codec conveniently.

   @{
   \file
 *****************************************************************************/

#ifndef __LIB_CODEC_H__
#define __LIB_CODEC_H__

#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>

#include "traces/Traces.h"
#include "lib_common/FourCC.h"
#include "lib_common_enc/Settings.h"
#include "lib_common/BufferAPI.h"
//#include <isp/ncu_common.h>
//#include <system/system.h>
typedef struct AL_t_CodecEncode AL_CodecEncode;

typedef struct AL_t_CodecParam {
  int              m_chnNum;
  AL_TEncSettings  m_Settings;
  int              m_baseStreamCnt;
  uint32_t         m_baseStreamSize;

  /***************TCfgRunInfo start***********************/
  bool             m_bUseBoard;
  TFourCC          m_fourCC;
  bool             m_bUseExtraSrcBuffer;
  bool             m_bUseExtraStreamBuffer;
  int              m_heightAlignVal;
  bool             m_printPictureType;
  char             *m_logsFile;
  bool             m_trackDma;
  char            *m_tracePath;
  AL_ETraceMode    m_traceMode;
  uint32_t         m_traceFrame;
  /***************TCfgRunInfo end*************************/
  uint8_t			extra_StreamMngrCtx_enable;
  void *extra_StreamMngrCtx;
  void *extra_pCtx;
} AL_CodecParam;

#ifndef __IMP_COMMON_H__
/**
 * IMP帧图像信息定义.
 */
typedef struct {
  int index;			/**< 帧序号 */
  int pool_idx;		/**< 帧所在的Pool的ID */

  uint32_t width;			/**< 帧宽 */
  uint32_t height;			/**< 帧高 */
  uint32_t pixfmt;			/**< 帧的图像格式 */
  uint32_t size;			/**< 帧所占用空间大小 */

  uint32_t phyAddr;	/**< 帧的物理地址 */
  uint32_t virAddr;	/**< 帧的虚拟地址 */

  int64_t timeStamp;	/**< 帧的时间戳 */
  uint32_t priv[0];	/* 私有数据 */
} IMPFrameInfo;
#endif

#ifndef __IMP_ISP_H__
typedef struct {
	unsigned int zone[15][15];    /**< 各区域信息*/
} IMPISPZone;
#endif

#ifndef __SYSTEM_H__
typedef struct {
	uint32_t	i_fps_num;
	uint32_t	i_fps_den;
	uint32_t	b_use_ncuinfo;
	uint32_t	b_use_aeinfo;
//	IMPNCUFrameInfo ncuinfo;
	//IMPISPZone ae_zone;
	uint32_t qcount;
	uint32_t dqcount;
	uint8_t frame_type;
	uint16_t frame_valid_lines;
	uint32_t valid_lines_reg_addr;
	int64_t  frame_dq_time;
	int64_t  frame_q_time;
	int64_t  encoder_frame_ready_time;
	int64_t  encoder_start_time;
	int64_t  encoder_end_hw_time;
	int64_t  encoder_end_cb_time;
	int64_t  encoder_end_imp_time;
} IMPFrameInfoPriv;
#endif

typedef struct AL_t_CodecFrame {
  IMPFrameInfo frameInfo;
  IMPFrameInfoPriv frameInfoPriv;
  void         *priv;

  int        (*waitEncodeAvail)(void *codecFrame, uint32_t uMaxCuSize);
  void       (*setReleaseFrame)(void *privData, void *releaseData);
  int        (*releaseFunc)(void *privData, void *releaseData, int is_enc_end);

  void        *releaseData;
  void        *privData;
  int         is_day;
  sem_t       *releaseFrmSem;
  pthread_mutex_t *releaseFrmMutex;
  int         releaseFrmCnt;
} AL_CodecFrame;


/*********************************************************************//*!
   \brief Create AL_Codec which manage the global resources of the encode device
	 such as AL_TIpCtrl which is used to access the hardware, AL_TAllocator which
	 is used to manage to dma memory, TScheduler which is used to manage the encode
	 channels, and so on.
   \return errorcode specifying why this codec couldn't be created, If the function
	 succeeds the return value is zero, else the return value is less than zero(fails)
*************************************************************************/
int AL_Codec_Create(void);

/*********************************************************************//*!
   \brief Destroy AL_Codec which manage the global resources of the encode device
	 such as AL_TIpCtrl which is used to access the hardware, AL_TAllocator which
	 is used to manage to dma memory, TScheduler which is used to manage the encode
	 channels, and so on. After call the function, the device is release
*************************************************************************/
void AL_Codec_Destroy(void);

void AL_Codec_Encode_SetDefaultParam(AL_CodecParam *param);

/*********************************************************************//*!
   \brief Create and configure a new instance of the AL_CodecEncode and returns
	 the handle of pCodecEncode which manages one encoder channel resources, such
	 as picture buf(option), stream buf, reconstuct buf, refference buf and so on.
	 We don't support Input/Output Format conversion.
   \param[out] pCodecEncode handle to the CodecEncode which will be created
	 \param[in] pCodecParam pointer to a AL_CodecParam structure which handle the config
	 parameters to config the CodecEncode.
   \return errorcode specifying why this codec couldn't be created, If the function
	 succeeds the return value is zero, If fails the return value is less than zero
*************************************************************************/
int AL_Codec_Encode_Create(AL_CodecEncode **pCodecEncode,  AL_CodecParam *pCodecParam);

/*********************************************************************//*!
   \brief Destroy the AL_CodecEncode which manages one encoder channel resources
   \param[in] pCodecEncode handle to the CodecEncode which will be destroyed
	 parameters to config the CodecEncode.
*************************************************************************/
void AL_Codec_Encode_Destroy(AL_CodecEncode *pCodecEncode);

/*********************************************************************//*!
   \brief Push a AL_CodecFrame to the AL_CodecEncode. According to the AL_EQpCtrlMode
	 and Gop Param, the frame could or couldn't be encoded immediately
   \param[in] pCodecEncode handle to the AL_CodecEncode object
	 \param[in] pCodecFrame pointer to a AL_CodecFrame structure which handle the frame 
	 information to be encoded.
   \return errorcode specifying why this codec couldn't be created, If the function
	 succeeds the return value is zero, If fails the return value is less than zero
*************************************************************************/
int AL_Codec_Encode_Process(AL_CodecEncode *pCodecEncode, AL_CodecFrame *pCodecFrame);
int AL_Codec_Encode_Process_V2(AL_CodecEncode *pCodecEncode,AL_TBuffer *pSrc);

int AL_Codec_Encode_Commit_FilledFifo(AL_CodecEncode *pCodecEncode);
int AL_Codec_Encode_GetStream(AL_CodecEncode *pCodecEncode, AL_TBuffer **pStream, AL_TBuffer **pSrc);
int AL_Codec_Encode_ReleaseStream(AL_CodecEncode *pCodecEncode, AL_TBuffer *pStream, AL_TBuffer *pSrc);

int AL_Codec_Encode_GetSrcFrameCntAndSize(AL_CodecEncode *pCodecEncode, uint32_t *pSrcFrameCnt, uint32_t *pSrcFrameSize);
int AL_Codec_Encode_GetSrcStreamCntAndSize(AL_CodecEncode *pCodecEncode, uint32_t *pSrcStreamCnt, uint32_t *pSrcStreamSize);
int AL_Codec_Encode_GetRcParam(AL_CodecEncode *pCodecEncode, AL_TRCParam *pRCParam);
int AL_Codec_Encode_SetRcParam(AL_CodecEncode *pCodecEncode, AL_TRCParam *pRCParam);
int AL_Codec_Encode_GetFrameRate(AL_CodecEncode *pCodecEncode, uint16_t *uFrameRate, uint16_t *uClkRatio);
int AL_Codec_Encode_SetFrameRate(AL_CodecEncode *pCodecEncode, uint16_t uFrameRate, uint16_t uClkRatio);
int AL_Codec_Encode_SetBitRate(AL_CodecEncode *pCodecEncode, int iTargetBitrate, int iMaxBitRate);
int AL_Codec_Encode_RestartGop(AL_CodecEncode *pCodecEncode);
int AL_Codec_Encode_GetGopParam(AL_CodecEncode *pCodecEncode, AL_TGopParam *pGopParam);
int AL_Codec_Encode_SetGopParam(AL_CodecEncode *pCodecEncode, AL_TGopParam *pGopParam);
int AL_Codec_Encode_SetGopLength(AL_CodecEncode *pCodecEncode, int iGopLength);
int AL_Codec_Encode_SetQpBounds(AL_CodecEncode *pCodecEncode, int iMinQP, int iMaxQP);

#ifdef __cplusplus
}
#endif

#endif //__LIB_CODEC_H__

/*@}*/
