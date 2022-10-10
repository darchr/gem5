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

#include "base/cprintf.hh"
#include "base/loader/memory_image.hh"
#include "base/loader/object_file.hh"
#include "debug/CenteralController.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

CenteralController::CenteralController(const Params& params):
    ClockedObject(params),
    system(params.system)
{
    for (auto mpu : params.mpu_vector) {
        mpuVector.push_back(mpu);
        mpu->registerCenteralController(this);
    }
}

void
CenteralController::initState()
{
    for (auto mpu: mpuVector) {
        addrRangeListMap[mpu] = mpu->getAddrRanges();
    }
    const auto& file = params().image_file;
    if (file == "")
        return;

    auto* object = loader::createObjectFile(file, true);
    fatal_if(!object, "%s: Could not load %s.", name(), file);

    loader::debugSymbolTable.insert(*object->symtab().globals());
    loader::MemoryImage image = object->buildImage();
    maxVertexAddr = image.maxAddr();

    PortProxy proxy(
    [this](PacketPtr pkt) {
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            for (auto range: range_list) {
                if (range.contains(pkt->getAddr())) {
                    mpu->recvFunctional(pkt);
                    break;
                }
            }
        }
    }, system->cacheLineSize());

    panic_if(!image.write(proxy), "%s: Unable to write image.");
}

void
CenteralController::startup()
{
    while(!initialUpdates.empty()) {
        PacketPtr front = initialUpdates.front();
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            for (auto range: range_list) {
                if (range.contains(front->getAddr())) {
                    mpu->handleIncomingUpdate(front);
                }
            }
        }
        initialUpdates.pop_front();
    }
}

PacketPtr
CenteralController::createReadPacket(Addr addr, unsigned int size)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, 0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) 0) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    return pkt;
}

template<typename T> PacketPtr
CenteralController::createUpdatePacket(Addr addr, T value)
{
    RequestPtr req = std::make_shared<Request>(addr, sizeof(T), addr, value);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) value) << 2);

    PacketPtr pkt = new Packet(req, MemCmd::UpdateWL);

    pkt->allocate();

    pkt->setLE<T>(value);

    return pkt;
}

void
CenteralController::createInitialBFSUpdate(Addr init_addr, uint32_t init_value)
{
    PacketPtr update = createUpdatePacket<uint32_t>(init_addr, init_value);
    initialUpdates.push_back(update);
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

void
CenteralController::recvDoneDrainSignal()
{
    bool done_drain = true;
    for (auto mpu: mpuVector) {
        done_drain &= mpu->doneDrain();
    }

    if (done_drain) {
        exitSimLoopNow("done draining.");
    }
}

void
CenteralController::printAnswerToHostSimout()
{
    int num_items = system->cacheLineSize() / sizeof(WorkListItem);
    WorkListItem items[num_items];
    for (Addr addr = 0; addr < maxVertexAddr; addr += system->cacheLineSize())
    {
        PacketPtr pkt = createReadPacket(addr, system->cacheLineSize());
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            if (contains(range_list, addr)) {
                mpu->recvFunctional(pkt);
            }
        }
        pkt->writeDataToBlock((uint8_t*) items, system->cacheLineSize());
        for (int i = 0; i < num_items; i++) {
            std::string print = csprintf("WorkListItem[%lu][%d]: %s.",
                                        addr, i, items[i].to_string());

            std::cout << print << std::endl;
        }
    }
}

void
CenteralController::enableDrain()
{
    bool neither_draining = true;
    for (auto mpu: mpuVector) {
        neither_draining &= !mpu->getDraining();
    }
    assert(neither_draining);

    for (auto mpu: mpuVector) {
        mpu->enableDrain();
    }
}

void
CenteralController::disableDrain()
{
    bool all_draining = true;
    for (auto mpu: mpuVector) {
        all_draining &= mpu->getDraining();
    }
    assert(all_draining);

    for (auto mpu: mpuVector) {
        mpu->disableDrain();
    }
}

void
CenteralController::enableFastMode()
{
    bool neither_fast = true;
    for (auto mpu: mpuVector) {
        neither_fast &= !mpu->getFastMode();
    }
    assert(neither_fast);

    for (auto mpu: mpuVector) {
        mpu->enableFastMode();
    }
}

void
CenteralController::disableFastMode()
{
    bool all_fast = true;
    for (auto mpu: mpuVector) {
        all_fast &= mpu->getFastMode();
    }
    assert(all_fast);

    for (auto mpu: mpuVector) {
        mpu->disableFastMode();
    }
}

void
CenteralController::resumeAfterDrain()
{
    bool neither_draining = true;
    for (auto mpu: mpuVector) {
        neither_draining &= !mpu->getDraining();
    }
    assert(neither_draining);

    for (auto mpu: mpuVector) {
        mpu->resumeAfterDrain();
    }
}

}
