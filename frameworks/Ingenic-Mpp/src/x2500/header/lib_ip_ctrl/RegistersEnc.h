/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup lib_ip_ctrl
   @{
   \file
******************************************************************************/
#pragma once

#include "RegistersCommon.h"
#include "RetroCompatGen1V120.h"

/*****************************************************************************/

#define AL_ENC_IS_CORE_REG(Reg) ((Reg) >= AL_ENC_CORE_REG_OFFSET(0) && (Reg) < AL_ENC_CORE_REG_OFFSET(AL_ENC_NUM_CORES))
#define AL_ENC_IS_COMMON_REG(Reg) (((int)uReg) >= ((int)AL_ENC_COMMON_OFFSET) && (uReg) < (AL_ENC_COMMON_OFFSET + AL_ENC_COMMON_REG_SIZE))

#define AL_ENC_GET_CORE_ID(uReg) (((uReg) - AL_ENC_COMMON_REG_SIZE - AL_ENC_COMMON_OFFSET) / AL_ENC_CORE_REG_SIZE)

/******************************************************************************/

#define AL_IS_JPEG_REG(Reg) \
  ((Reg) >= AL_GET_JPEG_CORE_REG_OFFSET(AL_GET_JPEG_PHYSICAL_CORE_ID(0)) && \
   (Reg) < AL_GET_JPEG_CORE_REG_OFFSET(AL_GET_JPEG_CORE_REG_OFFSET(0)) + AL_GET_JPEG_CORE_REG_OFFSET(AL_NUM_CORE_JPEG))

#define AL_GET_JPEG_CORE_ID(uReg) (((uReg) - AL_ENC_COMMON_REG_SIZE - AL_ENC_BASE_JPEG) / AL_ENC_CORE_REG_SIZE)

/*@}*/

