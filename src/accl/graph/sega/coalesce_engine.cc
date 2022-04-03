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

#include "accl/graph/sega/coalesce_engine.hh"

#include "accl/graph/sega/wl_engine.hh"
#include "debug/MPU.hh"
#include "mem/packet_access.hh"

namespace gem5
{

CoalesceEngine::CoalesceEngine(const CoalesceEngineParams &params):
    BaseReadEngine(params),
    peerPushEngine(params.peer_push_engine),
    numMSHREntry(params.num_mshr_entry),
    numTgtsPerMSHR(params.num_tgts_per_mshr),
    outstandingMemReqQueueSize(params.outstanding_mem_req_queue_size),
    nextMemReqEvent([this] { processNextMemReqEvent(); }, name()),
    nextRespondEvent([this] { processNextRespondEvent(); }, name()),
    nextApplyAndCommitEvent([this] { processNextApplyAndCommitEvent(); }, name()),
    stats(*this)
{}

void
CoalesceEngine::startup()
{
    for (int i = 0; i < 256; i++) {
        cacheBlocks[i].takenMask = 0;
        cacheBlocks[i].allocated = false;
        cacheBlocks[i].valid = false;
        cacheBlocks[i].hasConflict = false;
    }
}

void
CoalesceEngine::recvFunctional(PacketPtr pkt)
{
    sendMemFunctional(pkt);
}

void
CoalesceEngine::registerWLEngine(WLEngine* wl_engine)
{
    peerWLEngine = wl_engine;
}

bool
CoalesceEngine::recvReadAddr(Addr addr)
{
    assert(MSHRMap.size() <= numMSHREntry);
    DPRINTF(MPU, "%s: Received a read request for address: %lu.\n",
                                                    __func__, addr);
    Addr aligned_addr = (addr / 64) * 64;
    int block_index = aligned_addr % 256;
    int wl_offset = (addr - aligned_addr) / sizeof(WorkListItem);

    if ((cacheBlocks[block_index].addr == aligned_addr) &&
        (cacheBlocks[block_index].valid)) {
        // Hit
        DPRINTF(MPU, "%s: Read request with addr: %lu hit in the cache.\n"
                        , __func__, addr);
        // TODO: Make addrQueue and wlQueue into one std::pair<Addr, WL>
        addrResponseQueue.push_back(addr);
        worklistResponseQueue.push_back(cacheBlocks[block_index].items[wl_offset]);
        // TODO: Use a bitset instead of unsigned int for takenMask
        cacheBlocks[block_index].takenMask |= (1 << wl_offset);

        stats.readHits++;
        stats.numVertexReads++;

        assert(!worklistResponseQueue.empty() && !addrResponseQueue.empty());
        if (!nextRespondEvent.scheduled()) {
            schedule(nextRespondEvent, nextCycle());
        }
        return true;
    } else {
        // miss
        if (MSHRMap.find(block_index) == MSHRMap.end()) {
            assert(MSHRMap.size() <= numMSHREntry);
            if (MSHRMap.size() == numMSHREntry) {
                // Out of MSHR entries
                return false;
            } else {
                if (cacheBlocks[block_index].allocated) {
                    assert(MSHRMap[block_index].size() <= numTgtsPerMSHR);
                    if (MSHRMap[block_index].size() == numTgtsPerMSHR) {
                        return false;
                    }
                    // MSHR available but conflict
                    DPRINTF(MPU, "%s: Read request with addr: %lu missed with "
                                "conflict. Making a request for "
                                "aligned_addr: %lu.\n",
                                __func__, addr, aligned_addr);
                    cacheBlocks[block_index].hasConflict = true;
                    MSHRMap[block_index].push_back(addr);
                    return true;
                } else {
                    // TODO: Set valid to false every deallocation and
                    // assert valid == false here.
                    // MSHR available and no conflict
                    assert(
                        outstandingMemReqQueue.size() <=
                        outstandingMemReqQueueSize);
                    if (outstandingMemReqQueue.size() ==
                        outstandingMemReqQueueSize) {
                        return false;
                    }
                    DPRINTF(MPU, "%s: Read request with addr: "
                                "%lu missed with no conflict. "
                                "Making a request for aligned_addr: %lu.\n"
                                , __func__, addr, aligned_addr);
                    cacheBlocks[block_index].addr = aligned_addr;
                    cacheBlocks[block_index].takenMask = 0;
                    cacheBlocks[block_index].allocated = true;
                    cacheBlocks[block_index].valid = false;
                    cacheBlocks[block_index].hasConflict = false;

                    MSHRMap[block_index].push_back(addr);
                    // TODO: Parameterize 64 to memory atom size
                    PacketPtr pkt = createReadPacket(aligned_addr, 64);
                    outstandingMemReqQueue.push_back(pkt);

                    stats.numVertexBlockReads++;

                    assert(!outstandingMemReqQueue.empty());
                    if (!nextMemReqEvent.scheduled()) {
                        schedule(nextMemReqEvent, nextCycle());
                    }
                    return true;
                }
            }
        } else {
            if (MSHRMap[block_index].size() == numTgtsPerMSHR) {
                return false;
            }
            if ((!cacheBlocks[block_index].hasConflict) &&
                (aligned_addr != cacheBlocks[block_index].addr)) {
                cacheBlocks[block_index].hasConflict = true;
            }
            MSHRMap[block_index].push_back(addr);
            return true;
        }
    }
}

void
CoalesceEngine::processNextMemReqEvent()
{
    PacketPtr pkt = outstandingMemReqQueue.front();

    if (!memPortBlocked()) {
        sendMemReq(pkt);
        outstandingMemReqQueue.pop_front();
    }

    if ((!nextMemReqEvent.scheduled()) &&
        (!outstandingMemReqQueue.empty())) {
        schedule(nextMemReqEvent, nextCycle());
    }
}

void
CoalesceEngine::processNextRespondEvent()
{
    Addr addr_response = addrResponseQueue.front();
    WorkListItem worklist_response = worklistResponseQueue.front();

    peerWLEngine->handleIncomingWL(addr_response, worklist_response);

    addrResponseQueue.pop_front();
    worklistResponseQueue.pop_front();

    if ((!nextRespondEvent.scheduled()) &&
        (!worklistResponseQueue.empty()) &&
        (!addrResponseQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }
}

bool
CoalesceEngine::handleMemResp(PacketPtr pkt)
{
    assert(pkt->isResponse());
    if (pkt->isWrite()) {
        return true;
    }

    Addr addr = pkt->getAddr();
    uint8_t* data = pkt->getPtr<uint8_t>();
    int block_index = addr % 256; // TODO: After parameterizing the cache size
                                  // this 256 number should change to the cache
                                  // size parameter.

    assert((cacheBlocks[block_index].allocated) && // allocated cache block
            (!cacheBlocks[block_index].valid) &&    // valid is false
            (!(MSHRMap.find(block_index) == MSHRMap.end()))); // allocated MSHR

    for (int i = 0; i < 4; i++) {
        cacheBlocks[block_index].items[i] = *((WorkListItem*) (
                                data + (i * sizeof(WorkListItem))));
    }
    cacheBlocks[block_index].valid = true;

    int bias = 0;
    std::vector<int> servicedIndices;
    for (int i = 0; i < MSHRMap[block_index].size(); i++) {
        Addr miss_addr = MSHRMap[block_index][i];
        Addr alligned_miss_addr = (miss_addr / 64) * 64;

        if (alligned_miss_addr == addr) {
            int wl_offset = (miss_addr - alligned_miss_addr) / 16;
            addrResponseQueue.push_back(miss_addr);
            worklistResponseQueue.push_back(
                cacheBlocks[block_index].items[wl_offset]);
            cacheBlocks[block_index].takenMask |= (1 << wl_offset);
            stats.numVertexReads++;
            servicedIndices.push_back(i);
        }
    }
    // TODO: We Can use taken instead of this
    for (int i = 0; i < servicedIndices.size(); i++) {
        MSHRMap[block_index].erase(MSHRMap[block_index].begin() +
                                    servicedIndices[i] - bias);
        bias++;
    }

    if (MSHRMap[block_index].empty()) {
        MSHRMap.erase(block_index);
        cacheBlocks[block_index].hasConflict = false;
    } else {
        cacheBlocks[block_index].hasConflict = true;
    }

    if ((!nextRespondEvent.scheduled()) &&
        (!worklistResponseQueue.empty()) &&
        (!addrResponseQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }

    return true;
}

PacketPtr
CoalesceEngine::createWritePacket(Addr addr, unsigned int size, uint8_t* data)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, _requestorId);

    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) _requestorId) << 2);

    PacketPtr pkt = new Packet(req, MemCmd::WriteReq);
    pkt->allocate();
    pkt->setData(data);

    return pkt;
}

void
CoalesceEngine::recvWLWrite(Addr addr, WorkListItem wl)
{
    Addr aligned_addr = (addr / 64) * 64;
    int block_index = aligned_addr % 256;
    int wl_offset = (addr - aligned_addr) / 16;
    DPRINTF(MPU, "%s: Recieved a WorkList write. addr: %lu, wl: %s.\n",
                                    __func__, addr, wl.to_string());
    DPRINTF(MPU, "%s: aligned_addr: %lu, block_index: %d, wl_offset: %d, "
            "takenMask: %u.\n", __func__, aligned_addr,
            block_index, wl_offset, cacheBlocks[block_index].takenMask);
    assert((cacheBlocks[block_index].takenMask & (1 << wl_offset)) ==
            (1 << wl_offset));
    cacheBlocks[block_index].items[wl_offset] = wl;
    cacheBlocks[block_index].takenMask &= ~(1 << wl_offset);
    stats.numVertexWrites++;

    // TODO: Make this more general and programmable.
    // && (cacheBlocks[block_index].hasConflict)
    if ((cacheBlocks[block_index].takenMask == 0)) {
        evictQueue.push_back(block_index);
    }

    if ((!nextApplyAndCommitEvent.scheduled()) &&
        (!evictQueue.empty())) {
        schedule(nextApplyAndCommitEvent, nextCycle());
    }

}

void
CoalesceEngine::processNextApplyAndCommitEvent()
{
    int block_index = evictQueue.front();
    uint8_t changedMask = 0;
    // TODO: parameterize 64 to memory atom size
    uint8_t* wl_data;
    uint8_t data[64];

    for (int i = 0; i < 4; i++) {
        uint32_t old_prop = cacheBlocks[block_index].items[i].prop;
        cacheBlocks[block_index].items[i].prop = std::min(
            cacheBlocks[block_index].items[i].prop,
            cacheBlocks[block_index].items[i].tempProp);
        if (old_prop != cacheBlocks[block_index].items[i].prop) {
            changedMask |= (1 << i);
        }
        DPRINTF(MPU, "%s: Writing WorkListItem[%lu[%d]] to memory. "
                    "WLItem: %s.\n", __func__, cacheBlocks[block_index].addr,
                    i, cacheBlocks[block_index].items[i].to_string());
        wl_data = (uint8_t*) (cacheBlocks[block_index].items + i);
        std::memcpy(data + (i * sizeof(WorkListItem)),
                    wl_data, sizeof(WorkListItem));
    }

    if (changedMask) {
        assert(outstandingMemReqQueue.size() <= outstandingMemReqQueueSize);
        PacketPtr write_pkt = createWritePacket(
            cacheBlocks[block_index].addr, 64, data);

        if ((cacheBlocks[block_index].hasConflict) &&
            (outstandingMemReqQueue.size() < outstandingMemReqQueueSize - 1)){
            Addr miss_addr = MSHRMap[block_index][0];
            // TODO: Make sure this trick works;
            Addr alligned_miss_addr = (miss_addr / 64) * 64;
            PacketPtr read_pkt = createReadPacket(alligned_miss_addr, 64);
            outstandingMemReqQueue.push_back(write_pkt);
            outstandingMemReqQueue.push_back(read_pkt);
            // TODO: This should be improved
            if ((changedMask & (1)) == 1) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[0]);
            }
            if ((changedMask & (2)) == 2) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[1]);
            }
            if ((changedMask & (4)) == 4) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[2]);
            }
            if ((changedMask & (8)) == 8) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[3]);
            }
            cacheBlocks[block_index].takenMask = 0;
            cacheBlocks[block_index].allocated = true;
            cacheBlocks[block_index].valid = false;
            cacheBlocks[block_index].hasConflict = true;
            evictQueue.pop_front();
            DPRINTF(MPU, "%s: evictQueue.size: %u.\n",
                __func__, evictQueue.size());
        } else if ((!cacheBlocks[block_index].hasConflict) &&
            (outstandingMemReqQueue.size() < outstandingMemReqQueueSize)) {
            outstandingMemReqQueue.push_back(write_pkt);
            // TODO: This should be improved
            if ((changedMask & (1)) == 1) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[0]);
            }
            if ((changedMask & (2)) == 2) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[1]);
            }
            if ((changedMask & (4)) == 4) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[2]);
            }
            if ((changedMask & (8)) == 8) {
                peerPushEngine->recvWLItem(cacheBlocks[block_index].items[3]);
            }
            cacheBlocks[block_index].takenMask = 0;
            cacheBlocks[block_index].allocated = false;
            cacheBlocks[block_index].valid = false;
            cacheBlocks[block_index].hasConflict = false;
            evictQueue.pop_front();
            DPRINTF(MPU, "%s: evictQueue.size: %u.\n",
                __func__, evictQueue.size());
        } else {
            DPRINTF(MPU, "%s: Commit failed due to full reqQueue.\n" ,
                __func__);
        }
    } else {
        evictQueue.pop_front();
    }

    if ((!nextMemReqEvent.scheduled()) &&
        (!outstandingMemReqQueue.empty())) {
        stats.numVertexBlockWrites++;
        schedule(nextMemReqEvent, nextCycle());
    }

    if ((!nextApplyAndCommitEvent.scheduled()) &&
        (!evictQueue.empty())) {
        schedule(nextApplyAndCommitEvent, nextCycle());
    }
}

CoalesceEngine::CoalesceStats::CoalesceStats(CoalesceEngine &_coalesce)
    : statistics::Group(&_coalesce),
    coalesce(_coalesce),

    ADD_STAT(numVertexBlockReads, statistics::units::Count::get(),
             "Number of memory blocks read for vertecies"),
    ADD_STAT(numVertexBlockWrites, statistics::units::Count::get(),
             "Number of memory blocks writes for vertecies"),
    ADD_STAT(numVertexReads, statistics::units::Count::get(),
             "Number of memory vertecies read from cache."),
    ADD_STAT(numVertexWrites, statistics::units::Count::get(),
             "Number of memory vertecies written to cache."),
    ADD_STAT(readHits, statistics::units::Count::get(),
             "Number of cache hits.")
{
}

void
CoalesceEngine::CoalesceStats::regStats()
{
    using namespace statistics;
}

}
