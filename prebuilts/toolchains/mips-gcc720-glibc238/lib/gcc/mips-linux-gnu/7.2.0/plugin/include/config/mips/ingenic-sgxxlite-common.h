/* Target macros for mips*-mti-linux* targets.
   Copyright (C) 2012-2015 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef MIPS_INGENIC_SPEC_PRIVATE
#define MIPS_INGENIC_SPEC_PRIVATE \
  /* If no FP ABI option is specified, infer one from the		\
     ABI/ISA level unless there is a conflicting option.  */		\
  "%{!msingle-float: %{!msoft-float: %{!mfp*: %{mabi=32: -mfp64}}}}"	
#endif
/* Ingenic toolchains specail spec. */
#define MIPS_INGENIC_SELF_SPEC			\
  "%{mmxu3.1:%{!mmxu3: -mmxu3}}"        \
  MIPS_INGENIC_SPEC_PRIVATE,			\
  MIPS_INGENIC_ABS2008_SPEC,			\
  MIPS_INGENIC_NAN2008_SPEC,			\
  "%{!mfp32:%{!mno-fix-t40-x2000: -mfix-t40-x2000}}"


/* A spec condition that matches architectures should be targeted with
   o32 FP64 as default. */
#define MIPS_INGENIC_ARCH_O32_FP64_SPEC \
  "march=xburst2"

#define MIPS_INGENIC_O32_FP64_SPEC \
  "mfp64|mmsa|" MIPS_INGENIC_ARCH_O32_FP64_SPEC

/* Ingenic mfp64 supported arch, default support -mabs=2008 */
#define MIPS_INGENIC_ABS2008_SPEC					\
  "%{mabs*:;: %{mips32r2 :%{" MIPS_INGENIC_O32_FP64_SPEC ": -mabs=2008}}}"

#undef MIPS_INGENIC_NAN2008_SPEC
/* Infer a -mnan=2008 setting from a -mips argument.  */
#define MIPS_INGENIC_NAN2008_SPEC \
  "%{mnan*:;" MIPS_INGENIC_O32_FP64_SPEC ":-mnan=2008}"

#define TARGET_ENDIAN_DEFAULT 0

/* TNODE: set ISA_LEVEL_SPEC %{march=xburst*: -mips32r2} in 
   macro MIPS_ISA_LEVEL_SPEC */

/* sysroot of multi-lib*/
#ifndef SYSROOT_MULTILIB_PRIVATE
#define SYSROOT_MULTILIB_PRIVATE \
     "%{muclibc:/uclibc}"	 \
     "%{mfp32:/mfp32;mfpxx:/mfp32}"
#endif

#define SYSROOT_MULTILIB_INGENIC \
    SYSROOT_MULTILIB_PRIVATE

/* sysroot of header*/
#ifndef SYSROOT_HEADERS_PRIVATE
#define SYSROOT_HEADERS_PRIVATE \
     "%{muclibc:/uclibc}" 
#endif

#define SYSROOT_HEADERS_INGENIC \
    SYSROOT_HEADERS_PRIVATE

