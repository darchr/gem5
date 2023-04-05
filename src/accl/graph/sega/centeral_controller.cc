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
    system(params.system),
    mirrorsPort("mirrors_mem", this, 0), mapPort("map_port", this, 1),
    mode(ProcessingMode::NOT_SET), currentSliceNumber(0), totalSliceNumber(148),
    lastReadPacketId(0),
    nextMirrorMapReadEvent([this] { processNextMirrorMapReadEvent(); }, name()),
    nextMirrorReadEvent([this] { processNextMirrorReadEvent(); }, name()),
    nextMirrorUpdateEvent([this] { processNextMirrorUpdateEvent(); }, name()),
    nextWriteBackEvent([this] { processNextWriteBackEvent(); }, name())
{
    for (auto mpu : params.mpu_vector) {
        mpuVector.push_back(mpu);
        mpu->registerCenteralController(this);
    }
}

Port&
CenteralController::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "mirrors_mem") {
        return mirrorsPort;
    } else if (if_name == "mirrors_map_mem") {
        return mapPort;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

void
CenteralController::createBFSWorkload(Addr init_addr, uint32_t init_value)
{
    workload = new BFSWorkload(init_addr, init_value);
}

void
CenteralController::createBFSVisitedWorkload(Addr init_addr, uint32_t init_value)
{
    workload = new BFSVisitedWorkload(init_addr, init_value);
}

void
CenteralController::createSSSPWorkload(Addr init_addr, uint32_t init_value)
{
    workload = new SSSPWorkload(init_addr, init_value);
}

void
CenteralController::createCCWorkload()
{
    workload = new CCWorkload();
}

void
CenteralController::createAsyncPRWorkload(float alpha, float threshold)
{
    workload = new PRWorkload(alpha, threshold);
}

void
CenteralController::createPRWorkload(int num_nodes, float alpha)
{
    workload = new BSPPRWorkload(num_nodes, alpha);
}

void
CenteralController::createBCWorkload(Addr init_addr, uint32_t init_value)
{
    workload = new BSPBCWorkload(init_addr, init_value);
}

void
CenteralController::createPopCountDirectory(int atoms_per_block)
{
    fatal_if(mode == ProcessingMode::NOT_SET, "You should set the processing "
                        "mode by calling either setAsyncMode or setBSPMode.");
    if (mode == ProcessingMode::ASYNCHRONOUS) {
        for (auto mpu: mpuVector) {
            mpu->createAsyncPopCountDirectory(atoms_per_block);
        }
    }
    if (mode == ProcessingMode::BULK_SYNCHRONOUS) {
        for (auto mpu: mpuVector) {
            mpu->createBSPPopCountDirectory(atoms_per_block);
        }
    }
    if (mode == ProcessingMode::POLY_GRAPH) {
        for (auto mpu: mpuVector) {
            mpu->createAsyncPopCountDirectory(atoms_per_block);
        }
    }
}

void
CenteralController::startup()
{
    unsigned int vertex_atom = mpuVector.front()->vertexAtomSize();
    for (auto mpu: mpuVector) {
        addrRangeListMap[mpu] = mpu->getAddrRanges();
        mpu->setProcessingMode(mode);
        mpu->recvWorkload(workload);
    }

    const auto& vertex_file = params().vertex_image_file;
    if (vertex_file == "")
        return;

    auto* object = loader::createObjectFile(vertex_file, true);
    fatal_if(!object, "%s: Could not load %s.", name(), vertex_file);

    loader::debugSymbolTable.insert(*object->symtab().globals());
    loader::MemoryImage vertex_image = object->buildImage();
    maxVertexAddr = vertex_image.maxAddr();

    PortProxy vertex_proxy(
    [this](PacketPtr pkt) {
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            if (contains(range_list, pkt->getAddr())) {
                mpu->recvFunctional(pkt);
            }
        }
    }, vertex_atom);

    panic_if(!vertex_image.write(vertex_proxy), "%s: Unable to write image.");

    for (auto mpu: mpuVector) {
        mpu->postMemInitSetup();
        if (!mpu->running() && (mpu->workCount() > 0)) {
            mpu->start();
        }
    }
    workload->iterate();
}

void
CenteralController::ReqPort::sendPacket(PacketPtr pkt)
{
    panic_if(blockedPacket != nullptr,
            "Should never try to send if blocked!");
    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt))
    {
        DPRINTF(CenteralController, "%s: Port %d: Packet %s "
                "is blocked.\n", __func__, _id, pkt->print());
        blockedPacket = pkt;
    } else {
        DPRINTF(CenteralController, "%s: Port %d: Packet %s "
                    "sent.\n", __func__, _id, pkt->print());
    }
}

bool
CenteralController::ReqPort::recvTimingResp(PacketPtr pkt)
{
    return owner->handleMemResp(pkt, _id);
}

void
CenteralController::ReqPort::recvReqRetry()
{
    panic_if(blockedPacket == nullptr,
            "Received retry without a blockedPacket.");

    DPRINTF(CenteralController, "%s: ReqPort %d received a reqRetry. "
            "blockedPacket: %s.\n", __func__, _id, blockedPacket->print());
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);
    if (blockedPacket == nullptr) {
        DPRINTF(CenteralController, "%s: blockedPacket sent "
                                "successfully.\n", __func__);
        owner->recvReqRetry(_id);
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

void
CenteralController::recvDoneSignal()
{
    bool done = true;
    for (auto mpu : mpuVector) {
        done &= mpu->done();
    }

    if (done && mode == ProcessingMode::ASYNCHRONOUS) {
        exitSimLoopNow("no update left to process.");
    }

    if (done && mode == ProcessingMode::BULK_SYNCHRONOUS) {
        for (auto mpu: mpuVector) {
            mpu->postConsumeProcess();
            mpu->swapDirectories();
            if (!mpu->running() && (mpu->workCount() > 0)) {
                mpu->start();
            }
        }
        workload->iterate();
        exitSimLoopNow("finished an iteration.");
    }

    if (done && mode == ProcessingMode::POLY_GRAPH) {
        // assert(!nextMirrorMapReadEvent.scheduled());
        if (!nextMirrorMapReadEvent.scheduled()) {
            schedule(nextMirrorMapReadEvent, nextCycle());
        }
    }
}

void
CenteralController::processNextMirrorMapReadEvent()
{
    // TODO: In future add functionality to align start_addr and end_addr to
    // size of the vertex atom.
    Addr start_addr = currentSliceNumber * totalSliceNumber * sizeof(int);
    Addr end_addr = start_addr + totalSliceNumber * sizeof(int);
    PacketPtr start = createReadPacket(start_addr, sizeof(int));
    PointerTag* start_tag = new PointerTag(lastReadPacketId, PointerType::START);
    start->pushSenderState(start_tag);
    PacketPtr end = createReadPacket(end_addr, sizeof(int));
    PointerTag* end_tag = new PointerTag(lastReadPacketId, PointerType::END);
    end->pushSenderState(end_tag);
    lastReadPacketId++;
    mapPort.sendPacket(start);
    mapPort.sendPacket(end);
}

bool
CenteralController::handleMemResp(PacketPtr pkt, PortID id)
{
    assert(pkt->isResponse());
    if (id == 0) {
        if (pkt->isWrite()) {
            delete pkt;
            return true;
        }
        assert(reqInfoMap.find(pkt->req) != reqInfoMap.end());
        Addr offset;
        int num_mirrors;
        int pkt_size_in_mirrors = pkt->getSize() / sizeof(MirrorVertex);
        MirrorVertex data[pkt_size_in_mirrors];
        pkt->writeDataToBlock((uint8_t*) data, pkt->getSize());

        std::tie(offset, num_mirrors) = reqInfoMap[pkt->req];
        assert(num_mirrors > 0);
        offset = (int) (offset / sizeof(MirrorVertex));
        for (int i = 0; i < num_mirrors; i++) {
            mirrorQueue.push_back(data[i + offset]);
        }
        delete pkt;

        if (!nextMirrorUpdateEvent.scheduled()) {
            schedule(nextMirrorUpdateEvent, nextCycle());
        }
        return true;
    } else if (id == 1) {
        PointerTag* tag = pkt->findNextSenderState<PointerTag>();
        int read_id = tag->Id();
        PointerType read_type = tag->type();
        if (read_type == PointerType::START) {
            assert(startAddrs.find(read_id) == startAddrs.end());
            startAddrs[read_id] = pkt->getLE<int>();
            if (endAddrs.find(read_id) != endAddrs.end()) {
                int vertex_atom = mpuVector.front()->vertexAtomSize();
                mirrorPointerQueue.emplace_back(
                    startAddrs[read_id], endAddrs[read_id],
                    sizeof(MirrorVertex), vertex_atom);
                if (!nextMirrorReadEvent.scheduled()) {
                    schedule(nextMirrorReadEvent, nextCycle());
                }
            }
        } else {
            assert(read_type == PointerType::END);
            assert(endAddrs.find(read_id) == endAddrs.end());
            endAddrs[read_id] = pkt->getLE<int>();
            if (startAddrs.find(read_id) != startAddrs.end()) {
                int vertex_atom = mpuVector.front()->vertexAtomSize();
                mirrorPointerQueue.emplace_back(
                    startAddrs[read_id], endAddrs[read_id],
                    sizeof(MirrorVertex), vertex_atom);
                if (!nextMirrorReadEvent.scheduled()) {
                    schedule(nextMirrorReadEvent, nextCycle());
                }
            }
        }
        DPRINTF(CenteralController, "%s: Received pkt: %s from port %d "
                                    "with value: %d.\n", __func__,
                                    pkt->print(), id, pkt->getLE<int>());
        delete tag;
        delete pkt;
        return true;
    } else {
        panic("did not expect this.");
    }
}

void
CenteralController::recvReqRetry(PortID id) {
    if (id == 0) {
        assert(!nextMirrorReadEvent.scheduled());
        if (!mirrorPointerQueue.empty()) {
            schedule(nextMirrorReadEvent, nextCycle());
        }
    } else if (id == 1) {
        DPRINTF(CenteralController, "%s: Ignoring reqRetry "
                            "for port %d.\n", __func__, id);
    } else {
        panic("Did not expect the other.");
    }
}

void
CenteralController::processNextMirrorReadEvent()
{
    Addr aligned_addr, offset;
    int num_mirrors;

    int vertex_atom = mpuVector.front()->vertexAtomSize();
    MirrorReadInfoGen& front = mirrorPointerQueue.front();
    std::tie(aligned_addr, offset, num_mirrors) = front.nextReadPacketInfo();
    PacketPtr pkt = createReadPacket(aligned_addr, vertex_atom);
    mirrorsPort.sendPacket(pkt);
    reqInfoMap[pkt->req] = std::make_tuple(offset, num_mirrors);
    front.iterate();
    if (front.done()) {
        mirrorPointerQueue.pop_front();
    }

    if (!mirrorPointerQueue.empty() && !mirrorsPort.blocked()) {
        schedule(nextMirrorReadEvent, nextCycle());
    }
}

void
CenteralController::processNextMirrorUpdateEvent()
{
    int vertex_atom = mpuVector.front()->vertexAtomSize();
    MirrorVertex front = mirrorQueue.front();
    Addr org_addr = front.vertexId * sizeof(WorkListItem);
    Addr aligned_org_addr = roundDown<Addr, int>(org_addr, vertex_atom);
    int wl_offset = (org_addr - aligned_org_addr) / sizeof(WorkListItem);
    int num_items = vertex_atom / sizeof(WorkListItem);
    WorkListItem data[num_items];

    PacketPtr read_org = createReadPacket(aligned_org_addr, vertex_atom);
    for (auto mpu: mpuVector) {
        AddrRangeList range_list = addrRangeListMap[mpu];
        if (contains(range_list, org_addr)) {
            mpu->recvFunctional(read_org);
        }
    }
    read_org->writeDataToBlock((uint8_t*) data, vertex_atom);
    DPRINTF(CenteralController, "%s: OG: %s, CP: %s.\n", __func__,
            workload->printWorkListItem(data[wl_offset]), front.to_string());
    mirrorQueue.pop_front();
    delete read_org;
    if (!mirrorQueue.empty()) {
        schedule(nextMirrorUpdateEvent, nextCycle());
    }
}

void
CenteralController::processNextWriteBackEvent() {}

int
CenteralController::workCount()
{
    int work_count = 0;
    for (auto mpu: mpuVector) {
        work_count += mpu->workCount();
    }
    return work_count;
}

float
CenteralController::getPRError()
{
    BSPPRWorkload* pr_workload = dynamic_cast<BSPPRWorkload*>(workload);
    return pr_workload->getError();
}

void
CenteralController::printAnswerToHostSimout()
{
    unsigned int vertex_atom = mpuVector.front()->vertexAtomSize();
    int num_items = vertex_atom / sizeof(WorkListItem);
    WorkListItem items[num_items];
    for (Addr addr = 0; addr < maxVertexAddr; addr += vertex_atom)
    {
        PacketPtr pkt = createReadPacket(addr, vertex_atom);
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            if (contains(range_list, addr)) {
                mpu->recvFunctional(pkt);
            }
        }
        pkt->writeDataToBlock((uint8_t*) items, vertex_atom);
        for (int i = 0; i < num_items; i++) {
            std::string print = csprintf("WorkListItem[%lu][%d]: %s.", addr, i,
                                        workload->printWorkListItem(items[i]));

            std::cout << print << std::endl;
        }
    }
}

}
