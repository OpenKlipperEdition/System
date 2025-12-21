/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once

#include "RetroCompatGen1V120Defs.h"

#define AL_ENC_CORE_REG_OFFSET(Core) (AL_ENC_COMMON_OFFSET + AL_ENC_COMMON_REG_SIZE + (Core) * AL_ENC_CORE_REG_SIZE)

#define AL_GET_JPEG_PHYSICAL_CORE_ID(Core) (Core + AL_JPEG_FIRST_CORE)
#define AL_GET_JPEG_CORE_REG_OFFSET(CoreID) (AL_ENC_BASE_JPEG + AL_ENC_COMMON_REG_SIZE + AL_ENC_CORE_REG_SIZE * (CoreID))

#define AL_ENC_CLOCK_AUTO 0x00
#define AL_ENC_CLOCK_FORCE_ON 0x01
#define AL_ENC_CLOCK_FORCE_OFF 0x02 /* force off has higher priority */
#define AL_ENC_CLOCK_STATUS_ON 0x01
#define AL_ENC_CLOCK_STATUS_OFF 0x00

#define AL_ENC_START_REGISTER 1
#define AL_ENC_START_COMMANDLIST 2
#define AL_ENC_RESET_ON 1

#define AL_ENC_IRQ_ON 1
#define AL_ENC_IRQ_OFF 0

#define AL_ENC_PERF_ON 1
#define AL_ENC_PERF_OFF 0

#define AL_ENC_STATUS_RUN_ON 1
#define AL_ENC_STATUS_RUN_OFF 0

#define AL_ENC_RAM_SLEEP_ON 1
#define AL_ENC_RAM_SLEEP_OFF 0

/* Configuration of RegisterDefs.h */
#define AL_REG_CORE_OFFSET(...) AL_ENC_CORE_REG_OFFSET(__VA_ARGS__)
#define AL_REG_COMMON_OFFSET AL_ENC_COMMON_OFFSET

#include "RegisterDefs.h"

