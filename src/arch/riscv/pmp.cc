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

#include "arch/riscv/pmp.hh"

#include "arch/generic/tlb.hh"
#include "arch/riscv/isa.hh"
#include "base/addr_range.hh"
#include "base/types.hh"
#include "cpu/thread_context.hh"
#include "math.h"
#include "mem/request.hh"
#include "params/PMP.hh"
#include "sim/sim_object.hh"

PMP::PMP(const Params &params) :
    SimObject(params),
    maxEntries(params.max_pmp)
{
    for (int i=0; i < maxEntries; i++) {
        pmpEntry* entry = new pmpEntry;
        AddrRange thisRange(-1,-2);
        entry->pmpAddr = thisRange;
        entry->pmpCfg = 0;
        pmpTable.push_back(entry);
    }

    num_rules = 0;

}

bool
PMP::pmpCheck(const RequestPtr &req, BaseTLB::Mode mode,
              RiscvISA::PrivilegeMode pmode)
{
    // This function checks if the request
    // is allowed based on PMP rules
    if (num_rules == 0)
        return true;

    int ret = -1;

    // all pmp entries need to be looked from the lowest to
    // the highest number

    int match_index = -1;

    for (int i = 0; i < pmpTable.size(); i++) {
        //std::cout << "pmp: table check " << i << "   " << std::endl;
        if (pmpTable[i]->pmpAddr.contains(req->getPaddr())){
           //address matched
            match_index = i;
        }

        if ((PMP_OFF != pmpGetAField(pmpTable[match_index]->pmpCfg))
                                            && (match_index > -1)){
            // check the RWX permissions from the pmp entry
            uint8_t allowed_privs = PMP_READ | PMP_WRITE | PMP_EXEC;

            // i is the index of pmp table which matched
            allowed_privs &= pmpTable[match_index]->pmpCfg;

            if ((mode == BaseTLB::Mode::Read) && (PMP_READ & allowed_privs))
                {ret = 1;
                break;
                }
            else if ((mode == BaseTLB::Mode::Write) &&
                                            (PMP_WRITE & allowed_privs))
                {ret = 1;
                break;
                }
            else if ((mode == BaseTLB::Mode::Execute) &&
                                            (PMP_EXEC & allowed_privs))
                {ret = 1;
                break;
                }
            else{
                ret = 0;
                break;
                }
        }

    }

    // No PMP entry matched
    if (ret == -1) {
        if (pmode == RiscvISA::PrivilegeMode::PRV_M) {
            // Based on spec v1.10, M mode access
            // should still succeed
            ret = 1;
        } else {
            // Other modes should not succeed without
            // an entry match
            ret = 0;
        }
    }

    return ret == 1 ? true : false;
}

inline uint8_t
PMP::pmpGetAField(uint8_t cfg)
{
    // to get a field from pmpcfg register
    uint8_t a = cfg >> 3;
    return a & 0x03;
}


void
PMP::pmpUpdateCfg(uint32_t pmp_index, uint8_t this_cfg)
{
    pmpTable[pmp_index]->pmpCfg = this_cfg;
    pmpUpdateRule(pmp_index);

}

void
PMP::pmpUpdateRule(uint32_t pmp_index)
{
    // In qemu, the rule is updated whenever
    // pmpaddr/pmpcfg is written

    num_rules = 0;
    Addr prevAddr = 0;

    if (pmp_index >= 1) {
        prevAddr = pmpTable[pmp_index - 1]->rawAddr;
    }

    Addr this_addr = pmpTable[pmp_index]->rawAddr;
    uint8_t this_cfg = pmpTable[pmp_index]->pmpCfg;

    switch (pmpGetAField(this_cfg)) {
    case PMP_OFF:
        {
        AddrRange thisRange(-1, -2);
        pmpTable[pmp_index]->pmpAddr = thisRange;
        break;
        }
    case PMP_TOR:
        {
        AddrRange thisRange(prevAddr << 2, (this_addr << 2) - 1);
        pmpTable[pmp_index]->pmpAddr = thisRange;
        break;
        }
    case PMP_NA4:
        {
        AddrRange thisRange(this_addr << 2, (this_addr + 4) - 1);
        pmpTable[pmp_index]->pmpAddr = thisRange;
        break;
        }
    case PMP_NAPOT:
        {
        AddrRange thisRange = pmpDecodeNapot(this_addr);
        pmpTable[pmp_index]->pmpAddr = thisRange;
        break;
        }
    default:
        {
        AddrRange thisRange(-1,-2);
        pmpTable[pmp_index]->pmpAddr = thisRange;
        break;
        }
    }
        for (int i = 0; i < maxEntries; i++) {
        const uint8_t a_field =
            pmpGetAField(pmpTable[i]->pmpCfg);
        if (PMP_OFF != a_field) {
            num_rules++;
        }
    }

}

void
PMP::pmpUpdateAddr(uint32_t pmp_index, Addr this_addr)
{
    // just writing the raw addr in the pmp table
    // will convert it into a range, once cfg
    // reg is written

    pmpTable[pmp_index]->rawAddr = this_addr;

    pmpUpdateRule(pmp_index);

}

bool
PMP::shouldCheckPMP(RiscvISA::PrivilegeMode pmode,
            BaseTLB::Mode mode, ThreadContext *tc)
{
    // instruction fetch in S and U mode
    bool cond1 = (mode == BaseTLB::Execute &&
            (pmode != RiscvISA::PrivilegeMode::PRV_M));

    // data access in S and U mode when MPRV in mstatus is clear
    RiscvISA::STATUS status =
            tc->readMiscRegNoEffect(RiscvISA::MISCREG_STATUS);

    bool cond2 = (mode != BaseTLB::Execute &&
                 (pmode != RiscvISA::PrivilegeMode::PRV_M)
                 && (!status.mprv));

    // data access in any mode when MPRV bit in mstatus is set
    // and the MPP field in mstatus is S or U
    bool cond3 = (mode != BaseTLB::Execute && (status.mprv)
    && (status.mpp != RiscvISA::PrivilegeMode::PRV_M));

    return (cond1 || cond2 || cond3);
}

AddrRange
PMP::pmpDecodeNapot(Addr a)
{
    if (a == -1) {
      AddrRange thisRange(0u, -1);
      return thisRange;
    } else {
        uint64_t t1 = ctz64(~a);
        uint64_t range = std::pow(2,t1+3);
        uint64_t base = bits(a, 63, t1+1);
        AddrRange thisRange(base, base+range);
        return thisRange;
    }

}
