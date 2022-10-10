/*
 * Copyright (c) 2020 The Regents of the University of California.
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

#include "accl/graph/sega/mpu.hh"

#include "accl/graph/sega/centeral_controller.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

MPU::MPU(const Params& params):
    SimObject(params),
    system(params.system),
    wlEngine(params.wl_engine),
    coalesceEngine(params.coalesce_engine),
    pushEngine(params.push_engine)
{
    wlEngine->registerMPU(this);
    coalesceEngine->registerMPU(this);
    pushEngine->registerMPU(this);
}

void
MPU::registerCenteralController(CenteralController* centeral_controller)
{
    centeralController = centeral_controller;
}

bool
MPU::handleIncomingUpdate(PacketPtr pkt)
{
    return wlEngine->handleIncomingUpdate(pkt);
}

void
MPU::handleIncomingWL(Addr addr, WorkListItem wl)
{
    wlEngine->handleIncomingWL(addr, wl);
}

WorkListItem
MPU::recvFunctionalWLRead(Addr addr)
{
    return coalesceEngine->recvFunctionalWLRead(addr);
}

void
MPU::recvWLWrite(Addr addr, WorkListItem wl)
{
    coalesceEngine->recvWLWrite(addr, wl);
}

void
MPU::recvFunctionalWLWrite(Addr addr, WorkListItem wl)
{
    coalesceEngine->recvFunctionalWLWrite(addr, wl);
}

void
MPU::recvVertexPush(Addr addr, WorkListItem wl)
{
    pushEngine->recvVertexPush(addr, wl);
}

void
MPU::recvDoneSignal()
{
    if (done()) {
        centeralController->recvDoneSignal();
    }
}

bool
MPU::done()
{
    return wlEngine->done() && coalesceEngine->done() && pushEngine->done();
}

void
MPU::recvDoneDrainSignal()
{
    if (doneDrain()) {
        centeralController->recvDoneDrainSignal();
    }
}

bool
MPU::doneDrain()
{
    return wlEngine->doneDrain() &&
        coalesceEngine->doneDrain() && pushEngine->doneDrain();
}

bool
MPU::getDraining()
{
    assert((wlEngine->getDraining() && coalesceEngine->getDraining() && pushEngine->getDraining()) ||
            (!wlEngine->getDraining() && !coalesceEngine->getDraining() && !pushEngine->getDraining()));
    return wlEngine->getDraining() &&
        coalesceEngine->getDraining() && pushEngine->getDraining();
}

void
MPU::enableDrain()
{
    wlEngine->enableDrain();
    coalesceEngine->enableDrain();
    pushEngine->enableDrain();
}

void
MPU::disableDrain()
{
    wlEngine->disableDrain();
    coalesceEngine->disableDrain();
    pushEngine->disableDrain();
}

bool
MPU::getFastMode()
{
    assert((wlEngine->getFastMode() && coalesceEngine->getFastMode() && pushEngine->getFastMode()) ||
            (!wlEngine->getFastMode() && !coalesceEngine->getFastMode() && !pushEngine->getFastMode()));
    return wlEngine->getFastMode() &&
        coalesceEngine->getFastMode() && pushEngine->getFastMode();
}

void
MPU::enableFastMode()
{
    wlEngine->enableFastMode();
    coalesceEngine->enableFastMode();
    pushEngine->enableFastMode();
}

void
MPU::disableFastMode()
{
    wlEngine->disableFastMode();
    coalesceEngine->disableFastMode();
    pushEngine->disableFastMode();
}

void
MPU::resumeAfterDrain()
{
    pushEngine->resumeAfterDrain();
}

} // namespace gem5
