/*
** Copyright 2013, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "../utility.h"
#include "../machine.h"

void dump_memory_and_code(log_t*, pid_t) {
}

void dump_registers(log_t* log, pid_t tid) {
    struct user_regs_struct r;
    if (ptrace(PTRACE_GETREGS, tid, 0, &r) == -1) {
        _LOG(log, logtype::ERROR, "cannot get registers: %s\n", strerror(errno));
        return;
    }
    _LOG(log, logtype::REGISTERS, "    rax %016llx  rbx %016llx  rcx %016llx  rdx %016llx\n",
         r.rax, r.rbx, r.rcx, r.rdx);
    _LOG(log, logtype::REGISTERS, "    rsi %016llx  rdi %016llx\n",
         r.rsi, r.rdi);
    _LOG(log, logtype::REGISTERS, "    r8  %016llx  r9  %016llx  r10 %016llx  r11 %016llx\n",
         r.r8, r.r9, r.r10, r.r11);
    _LOG(log, logtype::REGISTERS, "    r12 %016llx  r13 %016llx  r14 %016llx  r15 %016llx\n",
         r.r12, r.r13, r.r14, r.r15);
    _LOG(log, logtype::REGISTERS, "    cs  %016llx  ss  %016llx\n",
         r.cs, r.ss);
    _LOG(log, logtype::REGISTERS, "    rip %016llx  rbp %016llx  rsp %016llx  eflags %016llx\n",
         r.rip, r.rbp, r.rsp, r.eflags);
}
