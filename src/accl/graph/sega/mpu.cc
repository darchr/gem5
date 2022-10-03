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
#include "debug/MPU.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

MPU::MPU(const Params& params):
    ClockedObject(params),
    system(params.system),
    wlEngine(params.wl_engine),
    coalesceEngine(params.coalesce_engine),
    pushEngine(params.push_engine)
{
    wlEngine->registerMPU(this);
    coalesceEngine->registerMPU(this);
    pushEngine->registerMPU(this);

    for (int i = 0; i < params.port_in_ports_connection_count; ++i) {
        inPorts.emplace_back(
                            name() + ".in_ports" + std::to_string(i), this, i);
    }
}

Port&
MPU::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "in_ports") {
        return inPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
MPU::init()
{
    for (int i = 0; i < inPorts.size(); i++){
        inPorts[i].sendRangeChange();
    }
}

void
MPU::registerCenteralController(CenteralController* centeral_controller)
{
    centeralController = centeral_controller;
}

AddrRangeList
MPU::RespPort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

void
MPU::RespPort::checkRetryReq()
{
    if (needSendRetryReq) {
        sendRetryReq();
        needSendRetryReq = false;
    }
}

void
MPU::checkRetryReq()
{
    for (int i = 0; i < inPorts.size(); ++i) {
        inPorts[i].checkRetryReq();
    }
}

bool
MPU::RespPort::recvTimingReq(PacketPtr pkt)
{
    if (!owner->handleIncomingUpdate(pkt)) {
        needSendRetryReq = true;
        return false;
    }

    return true;
}

Tick
MPU::RespPort::recvAtomic(PacketPtr pkt)
{
    panic("recvAtomic unimpl.");
}

void
MPU::RespPort::recvFunctional(PacketPtr pkt)
{
    owner->recvFunctional(pkt);
}

void
MPU::RespPort::recvRespRetry()
{
    panic("recvRespRetry from response port is called.");
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

void
MPU::recvWLWrite(Addr addr, WorkListItem wl)
{
    coalesceEngine->recvWLWrite(addr, wl);
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

} // namespace gem5
