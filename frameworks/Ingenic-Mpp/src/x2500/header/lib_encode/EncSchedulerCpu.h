/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once

typedef struct AL_i_EncScheduler AL_IEncScheduler;

#include "lib_common/Allocator.h"
#include "lib_ip_ctrl/IpCtrl.h"

AL_IEncScheduler* AL_SchedulerCpu_Create(AL_TIpCtrl* pIpCtrl, AL_TAllocator* pDmaAllocator);
void AL_SchedulerCpu_Destroy(AL_IEncScheduler *pScheduler);

