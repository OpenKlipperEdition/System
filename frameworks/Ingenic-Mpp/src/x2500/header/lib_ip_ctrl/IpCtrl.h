/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/*************************************************************************//*!
   \defgroup lib_ip_ctrl Ip Control

   The AL_TIpCtrl structure defines an interface object used to access the
   hardware ip registers and to register actions to execute when an interrupt
   is received from the hardware ip.
   The following API is an abstraction layer allowing various implementations
   of the ip control interface.
   The user can provide his own implementation if the provided ones don't fit
   his constraints

   @{
   \file
*****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief IP callback prototype
*****************************************************************************/
typedef void (* AL_PFN_IpCtrl_CallBack) (void* pUserData);

/*************************************************************************//*!
   \brief Generic interface to access the registers of the hardware ip.
*****************************************************************************/
typedef struct AL_t_IpCtrl AL_TIpCtrl;

/*!\cond*/
typedef struct
{
  void (* pfnDestroy)(AL_TIpCtrl* pThis);
  uint32_t (* pfnReadRegister)(AL_TIpCtrl* pThis, uint32_t uReg);
  void (* pfnWriteRegister)(AL_TIpCtrl* pThis, uint32_t uReg, uint32_t uVal);
  void (* pfnRegisterCallBack)(AL_TIpCtrl* pThis, AL_PFN_IpCtrl_CallBack pfnCallBack, void* pUserData, uint8_t uNumInt);
}AL_IpCtrlVtable;
/*!\endcond*/

struct AL_t_IpCtrl
{
  const AL_IpCtrlVtable* vtable;
};

/*********************************************************************//*!
   \brief Destroys an AL_TIpCtrl object
   \param[in] pIpCtrl pointer to a AL_TIpCtrl interface
*************************************************************************/
static AL_INLINE void AL_IpCtrl_Destroy(AL_TIpCtrl* pIpCtrl)
{
  if(!pIpCtrl)
    return;

  pIpCtrl->vtable->pfnDestroy(pIpCtrl);
}

/*********************************************************************//*!
   \brief Retrieves Current value of the specified 32 bits register
   The register must be 32bits aligned;

   If the register address is UINT32_MAX, AL_IpCtrl_ReadRegister returns a value of 0.
   This makes it easier to deprecate registers in certain version.
   \param[in] pIpCtrl pointer to a AL_TIpCtrl interface
   \param[in] uReg Register address
   \return the current value of the register identified by uReg
*************************************************************************/
static AL_INLINE uint32_t AL_IpCtrl_ReadRegister(AL_TIpCtrl* pIpCtrl, uint32_t uReg)
{
  if(uReg == UINT32_MAX)
    return 0;

  return pIpCtrl->vtable->pfnReadRegister(pIpCtrl, uReg);
}

/*********************************************************************//*!
   \brief Writes a new value to the specified 32 bits register
   The register must be 32bits aligned;

   If the register address is UINT32_MAX, AL_IpCtrl_WriteRegister acts as a noop.
   This makes it easier to deprecate registers in certain version.
   \param[in] pIpCtrl pointer to a AL_TIpCtrl interface
   \param[in] uReg Register address
   \param[in] uVal New value
*************************************************************************/
static AL_INLINE void AL_IpCtrl_WriteRegister(AL_TIpCtrl* pIpCtrl, uint32_t uReg, uint32_t uVal)
{
  if(uReg == UINT32_MAX)
    return;

  pIpCtrl->vtable->pfnWriteRegister(pIpCtrl, uReg, uVal);
}

/*********************************************************************//*!
   \brief Associates a callback function with an interrupt number
   To unregister a callback, register a NULL pointer as a callback.
   After calling this function, the user knows that the callback will never be called again.

   An implementation is required to finish the callback being unregistered if it
   is running before returning from this function.

   Implementations can raise a warning if the underlying IP tries to raise
   an interrupt which doesn't have a callback but shouldn't raise one if the
   interrupt was unregistered.

   A callback cannot be registered inside another callback.

   \param[in] pIpCtrl pointer to a TIpCtrl interface
   \param[in] pfnCallBack Pointer to the function to be called when
   corresponding interrupt is received.
   \param[in] pUserData   Pointer to the call back function parameter
   \param[in] uIntNum     Interrupt number associated to pfn_CallBack
*************************************************************************/
static AL_INLINE void AL_IpCtrl_RegisterCallBack(AL_TIpCtrl* pIpCtrl, AL_PFN_IpCtrl_CallBack pfnCallBack, void* pUserData, uint8_t uIntNum)
{
  pIpCtrl->vtable->pfnRegisterCallBack(pIpCtrl, pfnCallBack, pUserData, uIntNum);
}

/*@}*/

