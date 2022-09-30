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
    pushEngine(params.push_engine),
    updateQueueSize(params.update_queue_size),
    nextUpdatePushEvent([this] { processNextUpdatePushEvent(); }, name())
{
    wlEngine->registerMPU(this);
    coalesceEngine->registerMPU(this);
    pushEngine->registerMPU(this);


    for (int i = 0; i < params.port_out_ports_connection_count; ++i) {
        outPorts.emplace_back(
                            name() + ".out_ports" + std::to_string(i), this, i);
        updateQueues.emplace_back();
    }

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
    } else if (if_name == "out_ports") {
        return outPorts[idx];
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
MPU::init()
{
    localAddrRange = getAddrRanges();
    for (int i = 0; i < inPorts.size(); i++){
        inPorts[i].sendRangeChange();
    }
    for (int i = 0; i < outPorts.size(); i++){
        portAddrMap[outPorts[i].id()] = outPorts[i].getAddrRanges();
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

void
MPU::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr,
            "Should never try to send if blocked!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
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
    if (blockedPacket == nullptr) {
        owner->recvReqRetry();
    }
}

void
MPU::recvReqRetry()
{
    if (!nextUpdatePushEvent.scheduled()) {
        schedule(nextUpdatePushEvent, nextCycle());
    }
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
    Addr dst_addr = update.dst;
    bool found_locally = false;
    bool accepted = false;
    for (auto range : localAddrRange) {
        found_locally |= range.contains(dst_addr);
    }
    for (int i = 0; i < outPorts.size(); i++) {
        AddrRangeList addr_range_list = portAddrMap[outPorts[i].id()];
        for (auto range : addr_range_list) {
            if (range.contains(dst_addr)) {
                if (updateQueues[outPorts[i].id()].size() < updateQueueSize) {
                    DPRINTF(MPU, "%s: Queue %d received an update.\n", __func__, i);
                    updateQueues[outPorts[i].id()].emplace_back(update, curTick());
                    accepted = true;
                    break;
                }
            }
        }
    }

    if (accepted && (!nextUpdatePushEvent.scheduled())) {
        schedule(nextUpdatePushEvent, nextCycle());
    }

    return accepted;
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
        if (updateQueues[i].empty()) {
            continue;
        }
        if (outPorts[i].blocked()) {
            continue;
        }
        Update update;
        Tick entrance_tick;
        std::tie(update, entrance_tick) = updateQueues[i].front();
        PacketPtr pkt = createUpdatePacket<uint32_t>(update.dst, update.value);
        outPorts[i].sendPacket(pkt);
        DPRINTF(MPU, "%s: Sent update from addr: %lu to addr: %lu with value: "
                    "%d.\n", __func__, update.src, update.dst, update.value);
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
