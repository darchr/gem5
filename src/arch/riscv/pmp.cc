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
#include "base/addr_range.hh"
#include "base/types.hh"
#include "math.h"
#include "mem/request.hh"
#include "params/PMP.hh"
#include "sim/sim_object.hh"

PMP::PMP(const Params &params) :
SimObject(params), maxEntries(params.max_pmp)
{

    for (int i=0; i < maxEntries; i++) {
        pmpEntry* entry = new pmpEntry;
        AddrRange thisRange(0,-1);
        entry->pmpAddr = thisRange;
        entry->pmpCfg = 0;
        pmpTable.push_back(entry);
    }

}

// check if this region can be
// accessed (else raise an exception)
bool
PMP::pmpCheck(const RequestPtr &req, BaseTLB::Mode mode)
{

    std::cout << "pmp: check" << std::endl;

    // all pmp entries need to be looked from the lowest to
    // the highest number

    bool addrMatch = false;

    int i;
    for (i = 0; i < pmpTable.size(); i++) {
        std::cout << "pmp: table check " << i << "   " << std::endl;
        if (pmpTable[i]->pmpAddr.contains(req->getPaddr())){
           //address matched
            addrMatch = true;
            break;
        }
    }

    if (!addrMatch)
        return false; //no pmp entry has a matching address


    // else check the RWX permissions from the pmp entry

    uint8_t allowedPrivs = PMP_READ | PMP_WRITE | PMP_EXEC;

    // i is the index of pmp table which matched
    allowedPrivs &= pmpTable[i]->pmpCfg;

    std::cout << "pmp: priv check " << std::endl;

    if ((mode == BaseTLB::Mode::Read) &&
            ((PMP_READ & allowedPrivs) == PMP_READ))
        return true;
    else if ((mode == BaseTLB::Mode::Write) &&
            ((PMP_WRITE & allowedPrivs) == PMP_WRITE))
        return true;
    else if ((mode == BaseTLB::Mode::Execute) &&
            ((PMP_WRITE & allowedPrivs) == PMP_EXEC))
        return true;
    else
        return false;
}

// to get a field from pmpcfg register
inline uint8_t
PMP::pmpGetAField(uint8_t cfg)
{
    uint8_t a = cfg >> 3;
    return a & 0x3;
}


void
PMP::pmpUpdateCfg(uint32_t pmpIndex, uint8_t thisCfg)
{

    std::cout << "pmp: update cfg" << std::endl;

    pmpTable[pmpIndex]->pmpCfg = thisCfg;

    Addr thisAddr = pmpTable[pmpIndex]->rawAddr;

    Addr prevAddr = 0;

    if (pmpIndex >= 1) {

        prevAddr = pmpTable[pmpIndex - 1]->pmpAddr.end();
    }

    switch (pmpGetAField(thisCfg)) {
    case PMP_OFF:
        {
        AddrRange thisRange(0, -1);
        pmpTable[pmpIndex]->pmpAddr = thisRange;
        break;
        }
    case PMP_TOR:
        {
        AddrRange thisRange(prevAddr << 2, (thisAddr << 2) - 1);
        pmpTable[pmpIndex]->pmpAddr = thisRange;
        break;
        }
    case PMP_NA4:
        {
        AddrRange thisRange(thisAddr << 2, (thisAddr + 4) - 1);
        pmpTable[pmpIndex]->pmpAddr = thisRange;
        break;
        }
    case PMP_NAPOT:
        {
        AddrRange thisRange = pmpDecodeNapot(thisAddr);
        pmpTable[pmpIndex]->pmpAddr = thisRange;
        break;
        }
    default:
        {
        AddrRange thisRange(0,0);
        pmpTable[pmpIndex]->pmpAddr = thisRange;
        break;
        }
    }


}

void
PMP::pmpUpdateAddr(uint32_t pmpIndex, Addr thisAddr)
{
    // just writing the raw addr in the pmp table
    // will convert it into a range, once cfg
    // reg is written

    std::cout << "pmp: update addr" << std::endl;

    pmpTable[pmpIndex]->rawAddr = thisAddr;

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
