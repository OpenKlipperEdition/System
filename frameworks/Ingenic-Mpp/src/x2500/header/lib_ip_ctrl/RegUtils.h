/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once

#include "lib_ip_ctrl/IpCtrl.h"
#include "lib_rtos/types.h"

static AL_INLINE void ReadModifyRegister(AL_TIpCtrl* pIpCtrl, uint32_t uAddr, uint32_t uVal, uint32_t uMask)
{
  uint32_t uWord = AL_IpCtrl_ReadRegister(pIpCtrl, uAddr);
  uWord &= ~uMask;
  uWord |= (uVal & uMask);
  AL_IpCtrl_WriteRegister(pIpCtrl, uAddr, uWord);
}

/* AL_Reg_ReadParam will extract the parameter value from its register and give you the result
 * with the correct type. */
#define AL_Reg_ReadParam(IpCtrl, Name, Core) \
  (AL_Reg_GetParamValue ## Name(AL_IpCtrl_ReadRegister((IpCtrl), AL_Reg_GetRegAddr ## Name((Core))), Core))

#define AL_Reg_ReadTopParam(IpCtrl, Name) \
  (AL_Reg_GetParamValue ## Name(AL_IpCtrl_ReadRegister((IpCtrl), AL_Reg_GetRegAddr ## Name())))

/* AL_Reg_WriteTopParam and AL_Reg_WriteParam will write the parameter in its register
 * and take care to not overwrite the other parameters that might exist in the same register.
 * The Top version is for registers that exist in the TOP register bank and do not depend
 * on core offsets */
#define AL_Reg_WriteTopParam(IpCtrl, Name, Val) \
  (ReadModifyRegister((IpCtrl), \
                      AL_Reg_GetRegAddr ## Name(), \
                      AL_Reg_GetRegValue ## Name((Val)), \
                      AL_Reg_GetRegMask ## Name())) \

#define AL_Reg_WriteParam(IpCtrl, Name, Val, Core) \
  (ReadModifyRegister((IpCtrl), \
                      AL_Reg_GetRegAddr ## Name((Core)), \
                      AL_Reg_GetRegValue ## Name((Val), (Core)), \
                      AL_Reg_GetRegMask ## Name((Core)))) \

/* If you know the parameter is alone in its register and will stay that way,
 * or if the parameter is write only and can't be read back,
 * you can use OverWriteParam to skip the read-modify-write process */
#define AL_Reg_OverWriteParam(IpCtrl, Name, Val, Core) \
  (AL_IpCtrl_WriteRegister((IpCtrl), \
                           AL_Reg_GetRegAddr ## Name((Core)), \
                           AL_Reg_GetRegValue ## Name((Val), (Core)))) \

#define AL_Reg_OverWriteTopParam(IpCtrl, Name, Val) \
  (AL_IpCtrl_WriteRegister((IpCtrl), \
                           AL_Reg_GetRegAddr ## Name(), \
                           AL_Reg_GetRegValue ## Name(Val))) \


