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

#include "dev/lupio/lupio_tmr.hh"

#include "debug/LupioTMR.hh"
#include "mem/packet_access.hh"
#include "params/LupioTMR.hh"

// Specific fields for CTRL
#define LUPIO_TMR_IE    0x1
#define LUPIO_TMR_PD    0x2

// Specific fields for STAT
#define LUPIO_TMR_EX    0x1

#define UINT32_MASK (0xFFFFFFFFULL)

namespace gem5
{

LupioTMR::LupioTMR(const Params &params) :
    BasicPioDevice(params, params.pio_size),
    tmrEvent([this]{ lupioTMRCallback(); }, name()), 
    pic(params.pic),
    lupioTMRIntID(params.int_id)
{
    start = curTick(); 
    DPRINTF(LupioTMR, "LupioTMR initalized\n");
}

uint64_t
LupioTMR::lupioTMRCurrentTime()
{
    return (curTick() - start) / sim_clock::as_int::ns; 
}

void
LupioTMR::lupioTMRSet()
{
    //reload: 10MHz = some million cycles per sec 
    //curTick() - start ??
    // cycle: converted to number of ticks automatically 
    //next = curTick() + sim_clock::as_int::ns * reload; 
    startCycle = curCycle(); 
    schedule(tmrEvent, Cycles(reload)); 
}

void
LupioTMR::lupioTMRCallback()
{
    /* Signal expiration */
    expired = true;
    if (ie) {
        pic->post(lupioTMRIntID);
        //qemu_irq_raise(s->irq);
    }

    /* If periodic timer, reload */
    if (pd && reload) {
        lupioTMRSet();
    }
}

uint64_t
LupioTMR::lupioTMRRead(uint8_t addr, int size)
{
    uint32_t r = 0;
    
    switch (addr >> 2) {
        case LUPIO_TMR_TIML:
            if (size == 4) {
                r = lupioTMRCurrentTime() & UINT32_MASK;
            } else {
                r = lupioTMRCurrentTime();
            }
            break;
        case LUPIO_TMR_TIMH:
            r = (lupioTMRCurrentTime() >> 32) & UINT32_MASK;
            break;

        case LUPIO_TMR_RLDL:
            if (size == 4) {
                r = reload & UINT32_MASK;
            } else {
                r = reload;
            }
            break;
        case LUPIO_TMR_RLDH:
            r = (reload >> 32) & UINT32_MASK;
            break;

        case LUPIO_TMR_STAT:
            if (expired) {
                r |= LUPIO_TMR_EX;
            }
            /* Acknowledge expiration */
            expired = false;
            pic->clear(lupioTMRIntID);
          //  qemu_irq_lower(s->irq);
            break;

        default:
            panic("Unexpected read to the LupioTMR device at address %#llx!",
                    addr);
            break;
    }
    return r;
}

void
LupioTMR::lupioTMRWrite(uint8_t addr, uint64_t val64, int size)
{
    uint32_t val = val64;

    switch (addr >> 2) {
        case LUPIO_TMR_RLDL:
            if (size == 4) {
                reload = (reload & ~UINT32_MASK) | val;
            } else {
                reload = val64;
            }
            break;
        case LUPIO_TMR_RLDH:
            reload = ((uint64_t)val << 32) | (reload & UINT32_MASK);
            break;

        case LUPIO_TMR_CTRL:
			ie = val & LUPIO_TMR_IE;
			pd = val & LUPIO_TMR_PD;

            /* Stop current timer if any */
            if (curCycle() < startCycle + reload) {
                deschedule(tmrEvent);
            }
            /*if (timer_pending(&s->timer)) {
                timer_del(&s->timer);
            }*/
            /* If reload isn't 0, start a new one */
            if (reload) {
                lupioTMRSet();
            }
            break;

        default:
            panic("Unexpected write to the LupioTMR device at address %#llx!",
                    addr);
            break;
    }
}

Tick
LupioTMR::read(PacketPtr pkt)
{
    Addr pic_addr = pkt->getAddr() - pioAddr;

    DPRINTF(LupioTMR,
        "Read request - addr: %#x, size: %#x\n", pic_addr, pkt->getSize());

    uint64_t read_val = lupioTMRRead(pic_addr, pkt->getSize());
    DPRINTF(LupioTMR, "Packet Read: %#x\n", read_val);
    pkt->setUintX(read_val, byteOrder);
    pkt->makeResponse();

    return pioDelay;
}

Tick
LupioTMR::write(PacketPtr pkt)
{
    Addr pic_addr = pkt->getAddr() - pioAddr;

    DPRINTF(LupioTMR, "Write register %#x value %#x\n", pic_addr,
            pkt->getUintX(byteOrder));

    lupioTMRWrite(pic_addr, pkt->getUintX(byteOrder), pkt->getSize());
    DPRINTF(LupioTMR, "Packet Write Value: %d\n", pkt->getUintX(byteOrder));

    pkt->makeResponse();

    return pioDelay;
}
} // namespace gem5
