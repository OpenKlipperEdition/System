/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/**************************************************************************//*!
   \addtogroup lib_ip_ctrl
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_ip_ctrl/IpCtrl.h"

#define AL_BOARD_ENCODER_NAME        "/dev/avpu"

/*********************************************************************//*!
   \brief ip control interface implementation that access the hardware registers
   Checks the presence of the hardware ip, connect to the hardware driver.
   \param[in] deviceFile Specify the file descriptor associated to device
   \param[in] uIntReg Specify the interrupt register
   \param[in] uMskReg Specify the interrupt mask register
   \param[in] uIntMask Specify the interrupt mask Value
   \return Pointer on AL_TIpCtrl object
*************************************************************************/
AL_TIpCtrl* AL_Board_Create(const char* deviceFile, uint32_t uIntReg, uint32_t uMskReg, uint32_t uIntMask);

/*********************************************************************//*!
   \brief destrory the ip control interface implementation.
   \param[in] pIpCtrl ip control interface implementation which will be destroyed
*************************************************************************/
void AL_Board_Destroy(AL_TIpCtrl* pIpCtrl);

/*********************************************************************//*!
   \brief show board infomation relative tho ipctrl interface
   \param[in] ipCtrl ipctrl interface through which can access the hardware register
   \return when success, return the value AL_SUCCESS, else return AL_ERROR
*************************************************************************/
int ShowBoardInformation(AL_TIpCtrl* ipCtrl);

/*@}*/

