/*
 * Copyright (c) 2003-2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Gabe Black
 *          Nathan Binkert
 *          Ali Saidi
 */

#include <gem5/asm/generic/m5ops.h>

#ifdef M5OP_ADDR
/* Use the memory mapped m5op interface */
#define TWO_BYTE_OP(name, number, subfunc) \
        .globl name;                      \
        .func name;                       \
name:                                     \
        mov m5_mem, %r11;                 \
        mov $number, %rax;                \
        shl $8, %rax;                     \
        or  $subfunc, %rax;               \
        mov 0(%r11, %rax, 1), %rax;       \
        ret;                              \
        .endfunc;

#else
/* Use the magic instruction based m5op interface. This does not work
 * in virtualized environments.
 */

#define TWO_BYTE_OP(name, number, subfunc) \
        .globl name;                      \
        .func name;                       \
name:                                     \
        .byte 0x0F, 0x04;                 \
        .word number;                     \
        ret;                              \
        .endfunc;

#endif

TWO_BYTE_OP(m5_arm, M5OP_ARM, 0)
TWO_BYTE_OP(m5_quiesce, M5OP_QUIESCE, 0)
TWO_BYTE_OP(m5_quiesce_ns, M5OP_QUIESCE_NS, 0)
TWO_BYTE_OP(m5_quiesce_cycle, M5OP_QUIESCE_CYCLE, 0)
TWO_BYTE_OP(m5_quiesce_time, M5OP_QUIESCE_TIME, 0)
TWO_BYTE_OP(m5_rpns, M5OP_RPNS, 0)
TWO_BYTE_OP(m5_exit, M5OP_EXIT, 0)
TWO_BYTE_OP(m5_fail, M5OP_FAIL, 0)
TWO_BYTE_OP(m5_init_param, M5OP_INIT_PARAM, 0)
TWO_BYTE_OP(m5_load_symbol, M5OP_LOAD_SYMBOL, 0)
TWO_BYTE_OP(m5_reset_stats, M5OP_RESET_STATS, 0)
TWO_BYTE_OP(m5_dump_stats, M5OP_DUMP_STATS, 0)
TWO_BYTE_OP(m5_dump_reset_stats, M5OP_DUMP_RESET_STATS, 0)
TWO_BYTE_OP(m5_checkpoint, M5OP_CHECKPOINT, 0)
TWO_BYTE_OP(m5_read_file, M5OP_READ_FILE, 0)
TWO_BYTE_OP(m5_write_file, M5OP_WRITE_FILE, 0)
TWO_BYTE_OP(m5_debug_break, M5OP_DEBUG_BREAK, 0)
TWO_BYTE_OP(m5_switch_cpu, M5OP_SWITCH_CPU, 0)
TWO_BYTE_OP(m5_add_symbol, M5OP_ADD_SYMBOL, 0)
TWO_BYTE_OP(m5_panic, M5OP_PANIC, 0)
TWO_BYTE_OP(m5_work_begin, M5OP_WORK_BEGIN, 0)
TWO_BYTE_OP(m5_work_end, M5OP_WORK_END, 0)
TWO_BYTE_OP(m5_dist_toggle_sync, M5OP_DIST_TOGGLE_SYNC, 0)

TWO_BYTE_OP(m5_annotate_task_begin, M5OP_ANNOTATE, 0)
TWO_BYTE_OP(m5_annotate_task_end, M5OP_ANNOTATE, M5OP_AN_TASKEND)
