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

#include <cmath>
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
    BaseMemoryEngine(params),
    mapPort("map_port", this, 1), mode(ProcessingMode::NOT_SET),
    mirrorsMem(params.mirrors_mem), currentSliceId(0), totalUpdatesLeft(0),
    chooseBest(params.choose_best),
    nextSliceSwitchEvent([this] { processNextSliceSwitchEvent(); }, name()),
    stats(*this)
{
    uint64_t total_cache_size = 0;
    for (auto mpu : params.mpu_vector) {
        mpuVector.push_back(mpu);
        mpu->registerCenteralController(this);
        total_cache_size += mpu->getCacheSize();
    }
    verticesPerSlice = std::floor(total_cache_size / sizeof(WorkListItem));
}

Port&
CenteralController::getPort(const std::string& if_name, PortID idx)
{
    if (if_name == "mirrors_map_mem") {
        return mapPort;
    } else if (if_name == "mem_port") {
        return BaseMemoryEngine::getPort("mem_port", idx);
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
        for (auto range: mpu->getAddrRanges()) {
            mpuAddrMap.insert(range, mpu);
        }
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

    int num_total_vertices = (maxVertexAddr / sizeof(WorkListItem));
    numTotalSlices = std::ceil((double) num_total_vertices / verticesPerSlice);

    numPendingUpdates = new int [numTotalSlices];
    bestPendingUpdate = new uint32_t [numTotalSlices];
    for (int i = 0; i < numTotalSlices; i++) {
        numPendingUpdates[i] = 0;
        bestPendingUpdate[i] = -1;
    }

    PortProxy vertex_proxy(
    [this](PacketPtr pkt) {
        auto routing_entry = mpuAddrMap.contains(pkt->getAddr());
        routing_entry->second->recvFunctional(pkt);
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
    panic("recvTimingResp should not be called at all");
}

void
CenteralController::ReqPort::recvReqRetry()
{
    panic("recvReqRetry should not be called at all");
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
        DPRINTF(CenteralController, "%s: Received done signal.\n", __func__);
        exitSimLoopNow("Finished processing a slice.");
        if (!nextSliceSwitchEvent.scheduled()) {
            schedule(nextSliceSwitchEvent, nextCycle());
        }
    }
}

int
CenteralController::chooseNextSlice()
{
    int ret_slice_id = -1;
    int max_pending_count = 0;
    // TODO: Make this generalizable for all workloads.
    uint32_t best_update = -1;
    for (int i = 0; i < numTotalSlices; i++) {
        if (numPendingUpdates[i] > max_pending_count) {
            max_pending_count = numPendingUpdates[i];
        }
        if (numPendingUpdates[i] > 0 &&
            workload->betterThan(bestPendingUpdate[i], best_update)) {
            best_update = bestPendingUpdate[i];
        }
    }
    if (chooseBest) {
        int max_count = 0;
        for (int i = 0; i < numTotalSlices; i++) {
            if (numPendingUpdates[i] > max_count &&
                bestPendingUpdate[i] == best_update) {
                max_count = numPendingUpdates[i];
                ret_slice_id = i;
            }
        }
    } else {
        uint32_t best_value = -1;
        for (int i = 0; i < numTotalSlices; i++) {
            if (numPendingUpdates[i] == max_pending_count &&
                workload->betterThan(bestPendingUpdate[i], best_value)) {
                best_value = bestPendingUpdate[i];
                ret_slice_id = i;
            }
        }
    }
    return ret_slice_id;
}

void
CenteralController::processNextSliceSwitchEvent()
{
    int vertex_atom = mpuVector.front()->vertexAtomSize();
    int vertices_per_atom = (int) vertex_atom / sizeof(WorkListItem);
    int bytes_accessed = 0;
    int updates_generated_total =  0;
    for (int dst_id = 0; dst_id < numTotalSlices; dst_id++) {
        if (dst_id == currentSliceId) {
            continue;
        }
        int updates_generated = 0;
        Addr start_pointer = (currentSliceId * numTotalSlices + dst_id) * sizeof(uint64_t);
        Addr end_pointer = (currentSliceId * numTotalSlices + dst_id + 1) * sizeof(uint64_t);
        PacketPtr start = createReadPacket(start_pointer, sizeof(uint64_t));
        PacketPtr end = createReadPacket(end_pointer, sizeof(uint64_t));
        mapPort.sendFunctional(start);
        mapPort.sendFunctional(end);
        Addr start_addr = start->getLE<uint64_t>();
        Addr end_addr = end->getLE<uint64_t>();
        delete start;
        delete end;
        DPRINTF(CenteralController, "%s: %d->%d: [%lu, %lu].\n", __func__,
                            currentSliceId, dst_id, start_addr, end_addr);

        int num_bytes = end_addr - start_addr;
        int num_mirrors = (int) (end_addr - start_addr) / sizeof(MirrorVertex);
        MirrorVertex* mirrors = new MirrorVertex [num_mirrors];

        PacketPtr read_mirrors = createReadPacket(start_addr, num_bytes);
        memPort.sendFunctional(read_mirrors);
        read_mirrors->writeData((uint8_t*) mirrors);
        delete read_mirrors;

        WorkListItem vertices [vertices_per_atom];
        for (int i = 0; i < num_mirrors; i++) {
            Addr org_addr = mirrors[i].vertexId * sizeof(WorkListItem);
            Addr aligned_org_addr = roundDown<Addr, int>(org_addr, vertex_atom);
            int wl_offset = (int) (org_addr - aligned_org_addr) / sizeof(WorkListItem);
            PacketPtr read_org = createReadPacket(aligned_org_addr, vertex_atom);
            auto routing_entry = mpuAddrMap.contains(aligned_org_addr);
            routing_entry->second->recvFunctional(read_org);
            read_org->writeDataToBlock((uint8_t*) vertices, vertex_atom);
            delete read_org;
             if (vertices[wl_offset].tempProp != vertices[wl_offset].prop) {
                assert(vertices[wl_offset].degree == 0);
                vertices[wl_offset].prop = vertices[wl_offset].tempProp;
            }
            if (mirrors[i].prop != vertices[wl_offset].prop) {
                mirrors[i].prop = vertices[wl_offset].prop;
                if (!mirrors[i].activeNow) {
                    mirrors[i].activeNow = true;
                    numPendingUpdates[dst_id]++;
                    totalUpdatesLeft++;
                    updates_generated++;
                }
                bestPendingUpdate[dst_id] =
                    workload->betterThan(mirrors[i].prop, bestPendingUpdate[dst_id]);
            }
        }
        PacketPtr write_mirrors =
                    createWritePacket(start_addr, num_bytes, (uint8_t*) mirrors);
        memPort.sendFunctional(write_mirrors);
        delete write_mirrors;
        delete [] mirrors;
        DPRINTF(CenteralController, "%s: Done scattering updates from slice "
                        "%d to slice %d.\n", __func__, currentSliceId, dst_id);
        DPRINTF(CenteralController, "%s: Generated %d updates from slice "
                                        "%d to slice %d.\n", __func__,
                                    updates_generated, currentSliceId, dst_id);
        updates_generated_total += updates_generated;
        bytes_accessed += 2 * num_bytes;
    }
    DPRINTF(CenteralController, "%s: Done with slice %d.\n", __func__, currentSliceId);
    DPRINTF(CenteralController, "%s: Generated a total of %d updates.\n",
                                        __func__, updates_generated_total);
    DPRINTF(CenteralController, "%s: There are a total of %d "
                                "updates left.\n", __func__, totalUpdatesLeft);
    if (totalUpdatesLeft > 0) {
        currentSliceId = chooseNextSlice();
    } else {
        exitSimLoopNow("Done with all the slices.");
        return;
    }
    DPRINTF(CenteralController, "%s: Chose %d as the "
                                    "next slice.\n", __func__, currentSliceId);

    for (int src_id = 0; src_id < numTotalSlices; src_id++) {
        if (src_id == currentSliceId) {
            continue;
        }
        Addr start_pointer = (src_id * numTotalSlices + currentSliceId) * sizeof(uint64_t);
        Addr end_pointer = (src_id * numTotalSlices + currentSliceId + 1) * sizeof(uint64_t);
        PacketPtr start = createReadPacket(start_pointer, sizeof(uint64_t));
        PacketPtr end = createReadPacket(end_pointer, sizeof(uint64_t));
        mapPort.sendFunctional(start);
        mapPort.sendFunctional(end);
        Addr start_addr = start->getLE<uint64_t>();
        Addr end_addr = end->getLE<uint64_t>();
        delete start;
        delete end;

        int num_bytes = end_addr - start_addr;
        int num_mirrors = (int) (end_addr - start_addr) / sizeof(MirrorVertex);
        MirrorVertex* mirrors = new MirrorVertex [num_mirrors];

        PacketPtr read_mirrors = createReadPacket(start_addr, num_bytes);
        memPort.sendFunctional(read_mirrors);
        read_mirrors->writeData((uint8_t*) mirrors);
        delete read_mirrors;
        for (int i = 0; i < num_mirrors; i++) {
            if (mirrors[i].activeNow) {
                Addr org_addr = mirrors[i].vertexId * sizeof(WorkListItem);
                auto routing_entry = mpuAddrMap.contains(org_addr);
                routing_entry->second->recvMirrorPush(org_addr, mirrors[i].prop,
                                        mirrors[i].edgeIndex, mirrors[i].degree);
                mirrors[i].activeNow = false;
                numPendingUpdates[currentSliceId]--;
                totalUpdatesLeft--;
            }
        }
        PacketPtr write_mirrors =
                    createWritePacket(start_addr, num_bytes, (uint8_t*) mirrors);
        memPort.sendFunctional(write_mirrors);
        delete write_mirrors;
        delete [] mirrors;
        DPRINTF(CenteralController, "%s: Done gathering updates from slice "
                        "%d to slice %d.\n", __func__, src_id, currentSliceId);
        bytes_accessed += num_bytes;
    }

    double mirror_mem_bw = mirrorsMem->getBW();
    Tick time_to_switch = bytes_accessed * mirror_mem_bw;
    stats.switchTicks += time_to_switch;
    stats.switchedBytes += bytes_accessed;
    stats.numSwitches++;
    for (auto mpu: mpuVector) {
        mpu->startProcessingMirrors(time_to_switch);
    }
    exitSimLoopNow("Done with slice switch.");
}

bool
CenteralController::handleMemResp(PacketPtr pkt)
{
    panic("handleMemResp should not be called at all");
}

void
CenteralController::recvMemRetry()
{
    panic("recvMemRetry should not be called at all");
}

void
CenteralController::recvFunctional(PacketPtr pkt)
{
    panic("recvFunctional should not be called at all");
}

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
        auto routing_entry = mpuAddrMap.contains(pkt->getAddr());
        routing_entry->second->recvFunctional(pkt);
        pkt->writeDataToBlock((uint8_t*) items, vertex_atom);
        for (int i = 0; i < num_items; i++) {
            std::string print = csprintf("WorkListItem[%lu][%d]: %s.", addr, i,
                                        workload->printWorkListItem(items[i]));

            std::cout << print << std::endl;
        }
        delete pkt;
    }
}

CenteralController::ControllerStats::ControllerStats(CenteralController& _ctrl):
    statistics::Group(&_ctrl), ctrl(_ctrl),
    ADD_STAT(numSwitches, statistics::units::Byte::get(),
             "Number of slices switches completed."),
    ADD_STAT(switchedBytes, statistics::units::Byte::get(),
             "Number of bytes accessed during slice switching."),
    ADD_STAT(switchTicks, statistics::units::Tick::get(),
             "Number of ticks spent switching slices."),
    ADD_STAT(switchSeconds, statistics::units::Second::get(),
             "Traversed Edges Per Second.")
{
}

void
CenteralController::ControllerStats::regStats()
{
    using namespace statistics;

    switchSeconds = switchTicks / simFreq;
}

}
