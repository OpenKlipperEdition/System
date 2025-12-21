/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup lib_ip_ctrl
   @{
   \file
 *****************************************************************************/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lib_ip_ctrl/IpCtrl.h"

#define ALREFENC_CAPS_MAXWIDTH "maxWidth"
#define ALREFENC_CAPS_MAXHEIGHT "maxHeight"
#define ALREFENC_CAPS_LCUSIZE "lcuSize"
#define ALREFENC_CAPS_CHROMAMODE "chromaMode"
#define ALREFENC_CAPS_VQDESC "vqDesc"
#define ALREFENC_CAPS_ENTCOMP "entComp"

/*************************************************************************//*!
   \brief Opaque RefEnc configuration, represents how the IP was synthesized
*****************************************************************************/
struct AL_RefEnc_Config;

/*************************************************************************//*!
   \brief Creates a default config object
   \return Opaque configuration
*****************************************************************************/
struct AL_RefEnc_Config* AL_RefEnc_NewConfig();

/*************************************************************************//*!
   \brief Deletes a config object
   \param[in] pConfig Opaque configuration
*****************************************************************************/
void AL_RefEnc_DeleteConfig(struct AL_RefEnc_Config* pConfig);

/*************************************************************************//*!
   \brief Sets a RefEnc capability.
   \param[in] pConfig Opaque configuration
   \param[in] sCap Capabitilty name
   \param[in] iValue Capabitlity value
   \return True if sCap is recognized, false otherwise.
*****************************************************************************/
bool AL_RefEnc_SetCap(struct AL_RefEnc_Config* pConfig, const char* sCap, int iValue);
bool AL_RefEnc_SetCap2(struct AL_RefEnc_Config* pConfig, const char* sCap, const char* sValue);

/*************************************************************************//*!
   \brief Sets a function to convert physical address from register to virtual address.
   \param[in] pConfig Opaque configuration
   \param[in] pfnPAddrToVAddr pointer to the translation function. NULL to use the
              default one.
*****************************************************************************/
void AL_RefEnc_SetPAddrToVAddrCB(struct AL_RefEnc_Config* pConfig, AL_VADDR (* pfnPAddrToVAddr)(AL_PADDR));

/*************************************************************************//*!
   \brief Creates and initializes a Reference Encoder and returns an IpCtrl interface
   to access it through.
   \param[in] pConfig Opaque configuration
   \return Handle to access the reference encoder
*****************************************************************************/
AL_TIpCtrl* AL_RefEnc_New(struct AL_RefEnc_Config* pConfig);

/***************************************************************************
   \brief Legacy API. This function is deprecated, please use AL_RefEnc_New
   Creates and initializes a Reference Encoder and returns an IpCtrl interface.
   \param[in] iWidth Width of the widest picture to be handled by the encoder
   \param[in] iHeight Height of the highest picture to be handled by the encoder
   \param[in] uLcuSize Maximum size of a coding unit
   \param[in] uChromaMode Chroma Mode
   \param[in] eVQDescr Video Quality Description
   \return Handle to access the reference encoder
 *****************************************************************************/
AL_DEPRECATED("Legacy API. Use AL_RefEnc_New") AL_TIpCtrl * AL_RefEnc_Create(int iWidth, int iHeight, uint8_t uLcuSize, uint32_t uChromaMode, int eVQDescr);

#ifdef __cplusplus
}
#endif

/*@}*/

