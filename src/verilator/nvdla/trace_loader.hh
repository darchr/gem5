/* nvdla.cpp
 * Driver for Verilator testbench
 * NVDLA Open Source Project
 *
 * Copyright (c) 2017 NVIDIA Corporation.  Licensed under the NVDLA Open
 * Hardware License.  For more information, see the "LICENSE" file that came
 * with this distribution.
 */

/*# Copyright (c) 2019 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nima Ganjehloo
*/


#ifndef __VERILATOR_TRACE_LOADER__HH__
#define __VERILATOR_TRACE_LOADER__HH__

#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <queue>

#include "verilator/nvdla/axi_responder.hh"
#include "verilator/nvdla/csb_master.hh"

#define VERILY_READ(p, n)                      \
        do                                     \
        {                                      \
                if (read(fd, (p), (n)) != (n)) \
                {                              \
                        fatal("short read");  \
                }                              \
        } while (0)

class TraceLoader {
        enum axi_opc {
                AXI_LOADMEM,
                AXI_DUMPMEM
        };

        struct axi_op {
                axi_opc opcode;
                uint32_t addr;
                uint32_t len;
                const uint8_t *buf;
                const char *fname;
        };
        std::queue<axi_op> opq;

        CSBMaster *csb;
        AXIResponder<uint64_t> *axi_dbb, *axi_cvsram;

        int _test_passed;

public:
        enum stop_type {
                TRACE_CONTINUE = 0,
                TRACE_AXIEVENT,
                TRACE_WFI
        };

        TraceLoader(CSBMaster *_csb, AXIResponder<uint64_t> *_axi_dbb,
            AXIResponder<uint64_t> *_axi_cvsram);

        void load(const char *fname);

        void axievent();

        int test_passed();
};

#endif
