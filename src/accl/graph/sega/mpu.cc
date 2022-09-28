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
    ClockedObject(params),
    system(params.system),
    wlEngine(params.wl_engine),
    coalesceEngine(params.coalesce_engine),
    pushEngine(params.push_engine),
    inPort(name() + ".inPort", this),
    outPort(name() + ".outPort", this),
    updateQueueSize(params.update_queue_size),
    nextUpdatePushEvent([this] { processNextUpdatePushEvent(); }, name())
{
    wlEngine->registerMPU(this);
    coalesceEngine->registerMPU(this);
    pushEngine->registerMPU(this);


    for (int i = 0; i < params.port_out_ports_connection_count; ++i) {

        outports.emplace_back(name() + ".out_ports" + std::to_string(i), this);
    }
}

Port&
MPU::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "in_port") {
        return inPort;
    } else if (if_name == "out_port") {
        return outPort;
    } else if (if_name == "outPorts") {
        return outports[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
MPU::init()
{
    localAddrRange = getAddrRanges();
    inPort.sendRangeChange();
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

void
MPU::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr,
            "Should never try to send if blocked!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
    } else {
        owner->recvReqRetry();
    }
}

bool
MPU::ReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("recvTimingResp called on the request port.");
}

void
MPU::ReqPort::recvReqRetry()
{
    panic_if(blockedPacket == nullptr,
            "Received retry without a blockedPacket.");

    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
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

bool
MPU::enqueueUpdate(Update update)
{
    // Creating the packet
    Addr dst_addr = update.dst;
    bool found_locally = false;
    for (auto range : localAddrRange) {
        found_locally |= range.contains(dst_addr);
    }

    for (int i = 0; i < outports.size(); i++) {
        AddrRangeList addrList = outports[i].getAddrRanges();
        for (auto range : addrList) {
            if (range.contains(dst_addr)) {
                if (updateQueues[i].size() < updateQueueSize) {
                    updateQueues[i].emplace_back(update, curTick());
                    return true;
                } else {
                    return false;
                }
            }
        }
    }

    panic("The update created does not match to any outport.");
}

template<typename T> PacketPtr
MPU::createUpdatePacket(Addr addr, T value)
{
    RequestPtr req = std::make_shared<Request>(addr, sizeof(T), 0, 0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) 1) << 2);

    // FIXME: MemCmd::UpdateWL
    PacketPtr pkt = new Packet(req, MemCmd::UpdateWL);

    pkt->allocate();
    // pkt->setData(data);
    pkt->setLE<T>(value);

    return pkt;
}

void
MPU::processNextUpdatePushEvent()
{
    int next_time_send = 0;

    for (int i = 0; i < updateQueues.size(); i++) {
        Update update;
        Tick entrance_tick;
        std::tie(update, entrance_tick) = updateQueues[i].front();
        if (outports[i].blocked()) {
            continue;
        }
        PacketPtr pkt = createUpdatePacket<uint32_t>(update.dst, update.value);
        outports[i].sendPacket(pkt);
        updateQueues[i].pop_front();
        if (updateQueues[i].size() > 0) {
            next_time_send += 1;
        }
    }

    assert(!nextUpdatePushEvent.scheduled());
    if (next_time_send > 0) {
        schedule(nextUpdatePushEvent, nextCycle());
    }


}

void
MPU::recvVertexPush(Addr addr, WorkListItem wl)
{
    pushEngine->recvVertexPush(addr, wl);
}

void
MPU::sendPacket(PacketPtr pkt)
{
    bool found_locally = false;
    for (auto range : localAddrRange) {
        found_locally |= range.contains(pkt->getAddr());
    }

    if (found_locally) {
        // TODO: count number of local updates

    } else {
        // TOOD: count number of remote updates

    }

    outPort.sendPacket(pkt);
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
