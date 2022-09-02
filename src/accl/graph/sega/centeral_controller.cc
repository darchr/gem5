/*
 * Copyright (c) 2021 The Regents of the University of California.
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

#include "accl/graph/sega/centeral_controller.hh"

#include <iostream>

#include "base/loader/memory_image.hh"
#include "base/loader/object_file.hh"
#include "debug/CenteralController.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

CenteralController::CenteralController
                    (const CenteralControllerParams &params):
    ClockedObject(params),
    system(params.system),
    reqPort(name() + ".req_port", this),
    addr(params.addr),
    value(params.value)
{
    for (auto mpu : params.mpu_vector) {
        mpuVector.push_back(mpu);
        mpu->registerCenteralController(this);
    }
}

Port&
CenteralController::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "req_port") {
        return reqPort;
    } else {
        return SimObject::getPort(if_name, idx);
    }
}

void
CenteralController::initState()
{
    ClockedObject::initState();

    const auto &file = params().image_file;
    if (file == "")
        return;

    auto *object = loader::createObjectFile(file, true);
    fatal_if(!object, "%s: Could not load %s.", name(), file);

    loader::debugSymbolTable.insert(*object->symtab().globals());
    loader::MemoryImage image = object->buildImage();
    PortProxy proxy([this](PacketPtr pkt) { functionalAccess(pkt); },
                    system->cacheLineSize());

    panic_if(!image.write(proxy), "%s: Unable to write image.");
}

void
CenteralController::startup()
{
    PacketPtr first_update = createUpdatePacket<uint32_t>(addr, value);

    if (!reqPort.blocked()) {
        reqPort.sendPacket(first_update);
    }
}

template<typename T> PacketPtr
CenteralController::createUpdatePacket(Addr addr, T value)
{
    RequestPtr req = std::make_shared<Request>(
                addr, sizeof(T), addr, value);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) value) << 2);

    PacketPtr pkt = new Packet(req, MemCmd::UpdateWL);

    pkt->allocate();
    // pkt->setData(data);
    pkt->setLE<T>(value);

    return pkt;
}

void
CenteralController::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(_blocked, "Should never try to send if blocked MemSide!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        blockedPacket = pkt;
        _blocked = true;
    }
}

bool
CenteralController::ReqPort::recvTimingResp(PacketPtr pkt)
{
    panic("recvTimingResp called on the request port.");
}

void
CenteralController::ReqPort::recvReqRetry()
{
    panic_if(!(_blocked && blockedPacket), "Received retry without a blockedPacket");

    _blocked = false;
    sendPacket(blockedPacket);

    if (!_blocked) {
        blockedPacket = nullptr;
    }
}

void
CenteralController::functionalAccess(PacketPtr pkt)
{
    DPRINTF(CenteralController,
                "%s: Functional access for pkt->addr: %lu, pkt->size: %lu.\n",
                __func__, pkt->getAddr(), pkt->getSize());
    reqPort.sendFunctional(pkt);
}

void
CenteralController::recvDoneSignal()
{
    bool done = true;
    for (auto mpu : mpuVector) {
        done &= mpu->done();
    }

    if (done) {
        exitSimLoopNow("no update left to process.");
    }
}

}