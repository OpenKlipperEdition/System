/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#pragma once

#include "config.h"
#include "lib_rtos/types.h"

/*************************************************************************//*!
   \brief Command List set
*****************************************************************************/
typedef uint32_t TCmdRegs[NUM_COMMAND_REGISTER];

typedef struct AL_t_RegZone
{
  uint32_t RegOffset; // offset in byte from the start of the core
  uint16_t CmdIndex; // Index in the commandlist. (offset in 32-bit words from the start of the commandlist)
  uint16_t NumRegs;
}AL_TRegZone;

static AL_INLINE bool AL_RegZone_IsInZone(AL_TRegZone const* Zone, uint32_t Addr, uint32_t CoreOffset)
{
  uint32_t AbsoluteRegOffset = CoreOffset + Zone->RegOffset;
  return (Addr >= AbsoluteRegOffset) && (Addr < AbsoluteRegOffset + Zone->NumRegs * sizeof(uint32_t));
}

#define AL_RegZoneArray_IsIn(ZoneArray, Addr, CoreId) \
  AL_RegZoneArray_IsIn__(ZoneArray, AL_ZONE_ARRAY_SIZE(ZoneArray), Addr, AL_REG_CORE_OFFSET(CoreId))

static AL_INLINE bool AL_RegZoneArray_IsIn__(AL_TRegZone const* Zones, unsigned int NumZones, uint32_t Addr, uint32_t CoreOffset)
{
  for(unsigned int i = 0; i < NumZones; ++i)
  {
    if(AL_RegZone_IsInZone(&Zones[i], Addr, CoreOffset))
      return true;
  }

  return false;
}

/*@}*/

