/*
 * Copyright (c) 2021 The Regents of the University of California
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
 */

#ifndef __ARCH_RISCV_PMP_HH__
#define __ARCH_RISCV_PMP_HH__

#include "arch/generic/tlb.hh"
#include "arch/riscv/isa.hh"
#include "base/addr_range.hh"
#include "base/types.hh"
#include "mem/packet.hh"
#include "params/PMP.hh"
#include "sim/sim_object.hh"

/**
 * Place-holder for Doxygen comments on this class
 */

class PMP : public SimObject
{
  public:

    typedef PMPParams Params;

    const Params &
    params() const
    {
        return dynamic_cast<const Params &>(_params);
    }

    int maxEntries;

    PMP(const Params &params);

    // For encoding of address matching
    // mode of PMP address register
    // A (3-4) bits of pmpcfg register
    typedef enum {
        PMP_OFF,
        PMP_TOR,
        PMP_NA4,
        PMP_NAPOT
    } pmpAmatch;


    // PMP range permissions
    const uint8_t PMP_READ  =  1 << 0;
    const uint8_t PMP_WRITE = 1 << 1;
    const uint8_t PMP_EXEC  = 1 << 2;
    const uint8_t PMP_LOCK  = 1 << 7;

    int num_rules;

    // struct corresponding to a single
    // PMP entry
    typedef struct {
        AddrRange pmpAddr;
        Addr rawAddr;
        uint8_t  pmpCfg;
    } pmpEntry;

    std::vector<pmpEntry*> pmpTable;

    bool pmpCheck(const RequestPtr &req, BaseTLB::Mode mode,
                RiscvISA::PrivilegeMode pmode);
    inline uint8_t pmpGetAField(uint8_t cfg);
    void pmpUpdateCfg(uint32_t pmp_index, uint8_t this_cfg);
    void pmpUpdateAddr(uint32_t pmp_index, Addr this_addr);
    void pmpUpdateRule(uint32_t pmp_index);
    bool shouldCheckPMP(RiscvISA::PrivilegeMode pmode,
                BaseTLB::Mode mode, ThreadContext *tc);
    inline AddrRange pmpDecodeNapot(Addr a);

};

#endif // __ARCH_RISCV_PMP_HH__
