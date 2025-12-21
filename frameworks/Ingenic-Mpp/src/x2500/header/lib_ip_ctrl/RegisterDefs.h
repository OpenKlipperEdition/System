/* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved. */
#pragma once
#include "config.h"
#include "CmdRegs.h"
#include "lib_rtos/types.h"

/* RegisterDefs needs to be configured before inclusion. Or at least where it is used.
 * It depends on the following macros:
 *
 * - AL_REG_CORE_OFFSET(Core) which is used to get the first address of a core registers.
 * - AL_REG_COMMON_OFFSET which is used to get the first address of the top registers.
 *
 * Register Interface
 * ------------------
 *
 * The register interface generates the following functions.
 * The CoreId might not exist if you register doesn't belong to a core (TOP register)
 *
 * Get the register address of the parameter.
 * AL_Reg_GetRegAddr{RegisterParamName}([CoreId]) -> RegAddr
 *
 * Get the mask of the parameter in the register.
 * (Which bits belong to the parameter in the register)
 * AL_Reg_GetRegMask{RegisterParamName}([CoreId]) -> RegMask
 *
 * Get the value in the register from the parameter value.
 * AL_Reg_GetRegValue{RegisterParamName}(Val, [CoreId]) -> RegVal
 *
 * Get the parameter value from the value in the register.
 * AL_Reg_GetRegParamValue{RegisterParamName}(RegVal, [CoreId]) -> Val
 *
 * This is a low level API, and you will want to use the include/lib_ip_ctrl/RegUtils.h
 * higher level API for most of your registers access.
 *
 * CommandList Interface
 * ---------------------
 *
 * The command list interface generates the following functions:
 *
 * AL_Reg_Set{RegisterParamName}(CmdListArray, Val)
 * AL_Reg_Get{RegisterParamName}(CmdListArray) -> Val
 * */

/* Example of a classic registers implementation
 * -------------- Start of MMIO region
 *  ^
 *  | Common offset.
 *  v
 * -------------- Start of the IP registers
 *  ^
 *  | Top registers. (Registers that drive the whole IP)
 *  v
 * -------------- Start of Core 0 registers
 *  ^
 *  | Core 0 Registers (Registers that drive one core of the IP)
 *  v
 * -------------- Start of Core n registers
 *  .
 *  . Core n Registers
 *  .
 *
 * The top only has one bank. You describe top registers by
 * specifying the offset directly.
 *
 * Each core is divided in several banks
 *
 * ------------- Start of Core registers
 * ^
 * | CMDTOP bank
 * v
 * .
 * ^
 * | CMD bank
 * v
 * .
 * ^
 * | STATUS bank
 * v
 *
 * So to describe a register you will:
 *
 * Tell it is a core register, then select a bank and finally give the offset inside that bank.
 *
 * When using this register, you will need to tell which core is being targeted.
 */

#define AL_REGADDR uint32_t

#define AL_ASSERT_REG64(Name, Addr) \
  static_assert(((Addr) & 7) == 0, "Invalid 64b command definition");

#define AL_ASSERT_REG32(Name, NumBit, FirstBit) \
  static_assert(((FirstBit) + ((NumBit) - 1)) < 32, "Invalid command definition");

#define AL_MASK32(NumBit, FirstBit) ((0xFFFFFFFF >> (32 - (NumBit))) << (FirstBit))

static AL_INLINE uint32_t SetValue(uint32_t RegVal, int FirstBit, int NumBit, uint32_t Val)
{
  return (RegVal & ~AL_MASK32(NumBit, FirstBit)) | ((Val & AL_MASK32(NumBit, 0)) << FirstBit);
}

static AL_INLINE uint32_t GetUnsignedValue(uint32_t RegVal, int FirstBit, int NumBit)
{
  return (RegVal >> FirstBit) & AL_MASK32(NumBit, 0);
}

static AL_INLINE int32_t GetSignedValue(uint32_t RegVal, int FirstBit, int NumBit)
{
  return ((int32_t)(RegVal << (32 - NumBit - FirstBit))) >> (32 - NumBit);
}

#define AL_DEFINE_SET_REG64(Name, Addr) \
  static AL_INLINE void AL_Reg_Set ## Name(TCmdRegs * pRegs, uint64_t Val) \
  { \
    (*pRegs)[(Addr) >> 2] = (uint32_t)(Val); \
    (*pRegs)[((Addr) >> 2) + 1] = (uint32_t)((Val) >> 32); \
  } \

#define AL_DEFINE_GET_REG64(Name, Addr) \
  static AL_INLINE uint64_t AL_Reg_Get ## Name(TCmdRegs * pRegs) \
  { \
    return (((uint64_t)(*pRegs)[((Addr) >> 2) + 1]) << 32) | (*pRegs)[(Addr) >> 2]; \
  } \

#define AL_DEFINE_SET_REG32(Name, Addr, NumBit, FirstBit, Type) \
  static AL_INLINE void AL_Reg_Set ## Name(TCmdRegs * pRegs, Type Val) \
  { \
    (*pRegs)[(Addr) >> 2] = SetValue((*pRegs)[(Addr) >> 2], FirstBit, NumBit, (uint32_t)(Val)); \
  }

#define AL_DEFINE_GET_REG32U(Name, Addr, NumBit, FirstBit, Type) \
  static AL_INLINE Type AL_Reg_Get ## Name(TCmdRegs * pRegs) \
  { \
    return (Type)(GetUnsignedValue((*pRegs)[(Addr) >> 2], FirstBit, NumBit)); \
  } \

#define AL_DEFINE_REMOVED_GET_REG32(Name, Type) \
  static AL_INLINE Type AL_Reg_Get ## Name(TCmdRegs * pRegs) \
  { \
    (void)pRegs; \
    return 0; \
  } \

#define AL_DEFINE_REMOVED_SET_REG32(Name, Type) \
  static AL_INLINE void AL_Reg_Set ## Name(TCmdRegs * pRegs, Type Val) \
  { \
    (void)pRegs, (void)Val; \
  } \

#define AL_DEFINE_REMOVED_ADDR(Name) \
  static AL_INLINE AL_REGADDR AL_Reg_GetRegAddr ## Name(int Core) \
  { \
    (void)Core; \
    return -1; \
  } \

#define AL_DEFINE_TOP_REMOVED_ADDR(Name) \
  static AL_INLINE AL_REGADDR AL_Reg_GetRegAddr ## Name(void) \
  { \
    return -1; \
  } \

#define AL_DEFINE_TOP_REMOVED_VALUE(Name, Type) \
  static AL_INLINE uint32_t AL_Reg_GetRegValue ## Name(int Val) \
  { \
    (void)Val; \
    return 0; \
  } \
  static AL_INLINE Type AL_Reg_GetParamValue ## Name(uint32_t RegVal) \
  { \
    (void)RegVal; \
    return 0; \
  } \

#define AL_DEFINE_REMOVED_VALUE(Name, Type) \
  static AL_INLINE uint32_t AL_Reg_GetRegValue ## Name(int Val, int Core) \
  { \
    (void)Val, (void)Core; \
    return 0; \
  } \
  static AL_INLINE Type AL_Reg_GetParamValue ## Name(uint32_t RegVal, int Core) \
  { \
    (void)RegVal, (void)Core; \
    return 0; \
  } \

#define AL_DEFINE_TOP_REMOVED_MASK(Name) \
  static AL_INLINE uint32_t AL_Reg_GetRegMask ## Name(void) \
  { \
    return 0; \
  } \

#define AL_DEFINE_REMOVED_MASK(Name) \
  static AL_INLINE uint32_t AL_Reg_GetRegMask ## Name(int Core) \
  { \
    (void)Core; \
    return 0; \
  } \


#define AL_DEFINE_GET_REG32S(Name, Addr, NumBit, FirstBit, Type) \
  static AL_INLINE Type AL_Reg_Get ## Name(TCmdRegs * pRegs) \
  { \
    return (Type)(GetSignedValue((*pRegs)[(Addr) >> 2], FirstBit, NumBit)); \
  } \

#define AL_DEFINE_CORE_ADDR(Name, Addr) \
  static AL_INLINE AL_REGADDR AL_Reg_GetRegAddr ## Name(int Core) \
  { \
    return (AL_REGADDR)(AL_REG_CORE_OFFSET(Core) + (Addr)); \
  } \

#define AL_DEFINE_TOP_ADDR(Name, Addr) \
  static AL_INLINE AL_REGADDR AL_Reg_GetRegAddr ## Name(void) \
  { \
    return (AL_REGADDR)(AL_REG_COMMON_OFFSET + (Addr)); \
  } \

#define AL_DEFINE_TOP_CORE_ADDR(Name, Addr, Offset) \
  static AL_INLINE AL_REGADDR AL_Reg_GetRegAddr ## Name(int Core) \
  { \
    return (AL_REGADDR)(AL_REG_COMMON_OFFSET + (Addr) + ((Core) * (Offset))); \
  } \

#define AL_DEFINE_TOP_VALUE(Name, NumBit, FirstBit, Type) \
  static AL_INLINE uint32_t AL_Reg_GetRegValue ## Name(int Val) \
  { \
    return (uint32_t)(((Val) & AL_MASK32(NumBit, 0)) << (FirstBit)); \
  } \
  static AL_INLINE Type AL_Reg_GetParamValue ## Name(uint32_t RegVal) \
  { \
    return (Type)(((RegVal) >> FirstBit) & AL_MASK32(NumBit, 0)); \
  } \

#define AL_DEFINE_MULTICORE_VALUE(Name, NumBit, FirstBit, Shift, Type) \
  static AL_INLINE uint32_t AL_Reg_GetRegValue ## Name(int Val, int Core) \
  { \
    return (uint32_t)(((Val) & AL_MASK32(NumBit, 0)) << ((FirstBit) + ((Shift) * (Core)))); \
  } \
  static AL_INLINE Type AL_Reg_GetParamValue ## Name(uint32_t RegVal, int Core) \
  { \
    return (Type)(((RegVal) >> ((FirstBit) + ((Shift) * (Core)))) & AL_MASK32(NumBit, 0)); \
  } \

#define AL_DEFINE_CORE_UNSIGNED_VALUE(Name, NumBit, FirstBit, Type) \
  static AL_INLINE uint32_t AL_Reg_GetRegValue ## Name(int Val, int Core) \
  { \
    (void)(Core); \
    return (uint32_t)(SetValue(0, FirstBit, NumBit, Val)); \
  } \
  static AL_INLINE Type AL_Reg_GetParamValue ## Name(uint32_t RegVal, int Core) \
  { \
    (void)Core; \
    return (Type)(GetUnsignedValue(RegVal, FirstBit, NumBit)); \
  } \

#define AL_DEFINE_CORE_SIGNED_VALUE(Name, NumBit, FirstBit, Type) \
  static AL_INLINE uint32_t AL_Reg_GetRegValue ## Name(int Val, int Core) \
  { \
    (void)(Core); \
    return (uint32_t)(SetValue(0, FirstBit, NumBit, Val)); \
  } \
  static AL_INLINE Type AL_Reg_GetParamValue ## Name(uint32_t RegVal, int Core) \
  { \
    (void)Core; \
    return (Type)(GetSignedValue(RegVal, FirstBit, NumBit)); \
  } \

#define AL_DEFINE_TOP_MASK(Name, NumBit, FirstBit) \
  static AL_INLINE uint32_t AL_Reg_GetRegMask ## Name(void) \
  { \
    return (uint32_t)(AL_MASK32(NumBit, FirstBit)); \
  } \

#define AL_DEFINE_MULTICORE_MASK(Name, NumBit, FirstBit, Shift) \
  static AL_INLINE uint32_t AL_Reg_GetRegMask ## Name(int Core) \
  { \
    return (uint32_t)(AL_MASK32(NumBit, ((FirstBit) + ((Shift) * (Core))))); \
  } \

#define AL_DEFINE_CORE_MASK(Name, Mask) \
  static AL_INLINE uint32_t AL_Reg_GetRegMask ## Name(int Core) \
  { \
    (void)(Core); \
    return (uint32_t)(Mask); \
  } \

/* This define creates the register interface for a top register param.
 * Top register params exist in one exemplar. They drive the whole ip behavior and
 * do not depend on cores. */
#define AL_DEFINE_TOP_REG32(Name, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_TOP_ADDR(Name, Addr) \
  AL_DEFINE_TOP_VALUE(Name, NumBit, FirstBit, Type) \
  AL_DEFINE_TOP_MASK(Name, NumBit, FirstBit) \

/* This define is for registers param that affect one core but are still in the TOP bank.
 * There are two variants which can be combined:
 * With ShiftValue != 0, to get the param for one specific core, the register value is shifted by a number of (ShiftValue * CoreId) bits.
 * With ShiftAddr != 0, to get the param for one specific core, the register address is shifted by a number of (ShiftAddr * CoreId) bytes.
 * */
#define AL_DEFINE_TOP_MULTICORE_REG32(Name, Addr, NumBit, FirstBit, ShiftAddr, ShiftValue, Type) \
  AL_DEFINE_TOP_CORE_ADDR(Name, Addr, ShiftAddr) \
  AL_DEFINE_MULTICORE_VALUE(Name, NumBit, FirstBit, ShiftValue, Type) \
  AL_DEFINE_MULTICORE_MASK(Name, NumBit, FirstBit, ShiftValue) \

/* This define creates the register interface for an unsigned core register param. */
#define AL_DEFINE_REG32U(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_CORE_ADDR(Name, BankOffset + Addr) \
  AL_DEFINE_CORE_UNSIGNED_VALUE(Name, NumBit, FirstBit, Type) \
  AL_DEFINE_CORE_MASK(Name, AL_MASK32(NumBit, FirstBit)) \

/* This define creates the register interface for a signed core register param. */
#define AL_DEFINE_REG32S(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_CORE_ADDR(Name, BankOffset + Addr) \
  AL_DEFINE_CORE_SIGNED_VALUE(Name, NumBit, FirstBit, Type) \
  AL_DEFINE_CORE_MASK(Name, AL_MASK32(NumBit, FirstBit)) \

/* This define creates the register interface for a cmdtop register param.
 * A cmdtop register is a core register that isn't in a commandlist.
 * This define doesn't create the cmdlist interface for the register param. */
#define AL_DEFINE_CMDTOP_REG32(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_REG32U(Name, BankOffset, Addr, NumBit, FirstBit, Type) \

/* This define make it possible to get a value that spans on two consecutive registers.
 * The LSB needs to be the first register, followed by the MSB. The generated function
 * will return and take an uint64_t. */
/* /!\ hack: define as a CMDTOP_REG32 waiting 64 bit address support */
#define AL_DEFINE_CMD_REG64(Name, BankOffset, Addr) \
  AL_ASSERT_REG64(Name, BankOffset + Addr) \
  AL_ASSERT_REG64(Name, Addr) \
  AL_DEFINE_SET_REG64(Name, Addr) \
  AL_DEFINE_GET_REG64(Name, Addr) \
  AL_DEFINE_REG32U(Name, BankOffset, Addr, 32, 0, uint32_t)

/* This define creates the commandlist interface and the register interface for an unsigned register parameter. This version is used for the command register param for documentation sake */
#define AL_DEFINE_CMD_REG32U(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_ASSERT_REG32(Name, NumBit, FirstBit) \
  AL_DEFINE_SET_REG32(Name, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_GET_REG32U(Name, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_REG32U(Name, BankOffset, Addr, NumBit, FirstBit, Type)

/* This define creates the commandlist interface and the register interface for a signed register parameter. This version is used for the command register param for documentation sake */
#define AL_DEFINE_CMD_REG32S(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_ASSERT_REG32(Name, NumBit, FirstBit) \
  AL_DEFINE_SET_REG32(Name, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_GET_REG32S(Name, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_REG32S(Name, BankOffset, Addr, NumBit, FirstBit, Type)

/* This define creates the commandlist interface and the register interface for an unsigned register parameter. This version is used for the status register param for documentation sake */
#define AL_DEFINE_STATUS_REG32U(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_CMD_REG32U(Name, BankOffset, Addr, NumBit, FirstBit, Type) \

/* This define creates the commandlist interface and the register interface for a signed register parameter. This version is used for the status register param for documentation sake */
#define AL_DEFINE_STATUS_REG32S(Name, BankOffset, Addr, NumBit, FirstBit, Type) \
  AL_DEFINE_CMD_REG32S(Name, BankOffset, Addr, NumBit, FirstBit, Type) \

/* Do not use this define except if you know exactly what you are doing. It exists for retrocompatiblity issues. */
#define AL_DEFINE_STATUS_HACK_REG32U(Name, BankOffset, AddrReg, AddrCmdList, NumBit, FirstBit, Type) \
  AL_ASSERT_REG32(Name, NumBit, FirstBit) \
  AL_DEFINE_SET_REG32(Name, AddrCmdList, NumBit, FirstBit, Type) \
  AL_DEFINE_GET_REG32U(Name, AddrCmdList, NumBit, FirstBit, Type) \
  AL_DEFINE_REG32U(Name, BankOffset, AddrReg, NumBit, FirstBit, Type)

#define AL_DEFINE_REMOVED_REG(Name, Type) \
  AL_DEFINE_REMOVED_SET_REG32(Name, Type) \
  AL_DEFINE_REMOVED_GET_REG32(Name, Type) \
  AL_DEFINE_REMOVED_ADDR(Name) \
  AL_DEFINE_REMOVED_VALUE(Name, Type) \
  AL_DEFINE_REMOVED_MASK(Name) \

#define AL_DEFINE_TOP_REMOVED_REG(Name, Type) \
  AL_DEFINE_REMOVED_SET_REG32(Name, Type) \
  AL_DEFINE_REMOVED_GET_REG32(Name, Type) \
  AL_DEFINE_TOP_REMOVED_ADDR(Name) \
  AL_DEFINE_TOP_REMOVED_VALUE(Name, Type) \
  AL_DEFINE_TOP_REMOVED_MASK(Name) \

/* This define makes it possible to create a zone.
 * A bank is a segment of register in a core.
 * The bank offset is relative to the start of the core.
 * To define a zone, you give the offset of the bank start then the offset in that bank
 * and the number of register in the bank.
 * This defines a segment of registers.
 * A zone is relative to a core, so a bank offset of 0 means the start of the core.
 */
#define AL_ZONE(BankOffset, OffsetInBank, NumRegs) { ((BankOffset) + (OffsetInBank)), ((OffsetInBank) / 4), (NumRegs) }

/* Do not use this define except if you know exactly what you are doing. It exists for retrocompatiblity issues. */
#define AL_ZONE_HACK(BankOffset, RegOffset, CmdOffset, NumRegs) { (BankOffset) + (RegOffset), (CmdOffset) / 4, (NumRegs) }

/* Defines a zone array.
 * A zone array is an ensemble of zones that regroups registers which have the same purpose. */
#define AL_DEFINE_ZONE_ARRAY(name, ...) static const AL_TRegZone name[] = { __VA_ARGS__ };

/* Get the number of zones in the zone array.
 * This macro should be used on the real array. It will fail if the array becomes a AL_TRegZone*. */
#define AL_ZONE_ARRAY_SIZE(name) (sizeof(name) / sizeof(AL_TRegZone))

