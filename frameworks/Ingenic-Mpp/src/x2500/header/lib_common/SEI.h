/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/
#pragma once

/****************************************************************************/
typedef enum e_SeiFlag
{
  AL_SEI_NONE = 0x00000000, // no SEI
  // prefix (16 LSBs)
  AL_SEI_BP = 0x00000001, // Buffering period
  AL_SEI_PT = 0x00000002, // Picture Timing
  AL_SEI_RP = 0x00000004, // Recovery Point
  AL_SEI_MDCV = 0x0000008, // Mastering Display Colour Volume (HDR)
  AL_SEI_CLL = 0x00000010,  // Content Light Level (HDR)
  // suffix (16 MSBs)

  AL_SEI_ALL = 0xFFFFFFFF, // All supported SEI
}AL_ESeiFlag;

/****************************************************************************/
static AL_INLINE bool AL_HAS_SEI_SUFFIX(AL_ESeiFlag seiFlag)
{
  return (seiFlag & 0xFFFF0000) != 0;
}

/****************************************************************************/
static AL_INLINE bool AL_HAS_SEI_PREFIX(AL_ESeiFlag seiFlag)
{
  return (seiFlag & 0x0000FFFF) != 0;
}

/****************************************************************************/
static uint8_t const SEI_PREFIX_USER_DATA_UNREGISTERED_UUID[16] =
{
  0xb1, 0xe1, 0x67, 0xa4, 0xd9, 0xca, 0x11, 0xe7,
  0xb1, 0x9b, 0x00, 0x50, 0xc2, 0x49, 0x00, 0x48
};

