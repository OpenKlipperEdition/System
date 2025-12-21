/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_rtos/types.h"
/*************************************************************************//*!
   \brief Colour Description identifer (See ISO/IEC 23091-4 or ITU-T H.273)
*****************************************************************************/
typedef enum e_ColourDescription
{
  AL_COLOUR_DESC_RESERVED,
  AL_COLOUR_DESC_UNSPECIFIED,
  AL_COLOUR_DESC_BT_470_NTSC,
  AL_COLOUR_DESC_BT_601_NTSC,
  AL_COLOUR_DESC_BT_601_PAL,
  AL_COLOUR_DESC_BT_709,
  AL_COLOUR_DESC_BT_2020,
  AL_COLOUR_DESC_SMPTE_240M,
  AL_COLOUR_DESC_SMPTE_ST_428,
  AL_COLOUR_DESC_SMPTE_RP_431,
  AL_COLOUR_DESC_SMPTE_EG_432,
  AL_COLOUR_DESC_EBU_3213,
  AL_COLOUR_DESC_GENERIC_FILM,
  AL_COLOUR_DESC_MAX_ENUM,
}AL_EColourDescription;

/************************************//*!
   \brief Transfer Function identifer
****************************************/
typedef enum e_TransferCharacteristics
{
  AL_TRANSFER_CHARAC_BT709 = 1,
  AL_TRANSFER_CHARAC_UNSPECIFIED = 2,
  AL_TRANSFER_CHARAC_BT_2100_PQ = 16,
  AL_TRANSFER_CHARAC_MAX_ENUM,
}AL_ETransferCharacteristics;

/*******************************************************************************//*!
   \brief Matrix coefficient identifier used for luma/chroma computation from RGB
***********************************************************************************/
typedef enum e_ColourMatrixCoefficients
{
  AL_COLOUR_MAT_COEFF_BT709 = 1,
  AL_COLOUR_MAT_COEFF_UNSPECIFIED = 2,
  AL_COLOUR_MAT_COEFF_BT_2100_YCBCR = 9,
  AL_COLOUR_MAT_COEFF_MAX_ENUM,
}AL_EColourMatrixCoefficients;

/*******************************************************************************//*!
   \brief Normalized x and y chromaticity coordinates (CIE 1931 definition of x and y as specified in ISO 11664-1)
***********************************************************************************/
typedef struct AL_t_ChromaCoordinates
{
  uint16_t x;
  uint16_t y;
}AL_TChromaCoordinates;

/*************************************************************************//*!
   \brief Mimics structure for mastering display colour volume
*****************************************************************************/
typedef struct AL_t_MasteringDisplayColourVolume
{
  AL_TChromaCoordinates display_primaries[3];
  AL_TChromaCoordinates white_point;
  uint32_t max_display_mastering_luminance;
  uint32_t min_display_mastering_luminance;
}AL_TMasteringDisplayColourVolume;

/*************************************************************************//*!
   \brief Mimics structure for content light level information
*****************************************************************************/
typedef struct AL_t_ContentLightLevel
{
  uint16_t max_content_light_level;
  uint16_t max_pic_average_light_level;
}AL_TContentLightLevel;

/*************************************************************************//*!
   \brief Mimics structure containing HDR Related SEIs
*****************************************************************************/
typedef struct AL_t_HDRSEIs
{
  bool bHasMDCV;
  AL_TMasteringDisplayColourVolume tMDCV;

  bool bHasCLL;
  AL_TContentLightLevel tCLL;
}AL_THDRSEIs;

/*************************************************************************//*!
   \brief Initialize HDR SEIs structure
   \param[in] pHDRSEIs Pointer to the HDR SEIs
*****************************************************************************/
void AL_HDRSEIs_Reset(AL_THDRSEIs* pHDRSEIs);

/*@}*/

