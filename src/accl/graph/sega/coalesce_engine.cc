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

#include "accl/sega/coalesce_engine.hh"

#include "accl/sega/wl_engine.hh"

namespace gem5
{

CoalesceEngine::CoalesceEngine(const CoalesceEngineParams &params):
    BaseReadEngine(params),
    reqQueueSize(params.req_queue_size),
    conflictAddrQueueSize(params.conflict_addr_queue_size),
    nextWorkListSendEvent([this] { processNextWorkListSendEvent(); }, name()),
    nextApplyAndCommitEvent([this] { processNextApplyAndCommitEvent(); }, name())
{}

CoalesceEngine::~CoalesceEngine()
{}

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
    assert(reqQueue.size() <= reqQueueSize);
    if (reqQueue.size() == reqQueueSize) {
        return false;
    }

    reqQueue.push(addr);
    if ((!nextRespondEvent.scheduled()) && (!reqQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }
    return true;
}

void
CoalesceEngine::processNextRespondEvent()
{
    // TODO: Investigate this for optimization
    Addr addr = reqQueue.front();
    Addr alligned_addr = (addr / 64) * 64;
    int block_index = alligned_addr % 256;
    int wl_offset = (addr - alligned_addr) / 16;

    if (cacheBlocks[block_index].allocated) {
        // Hit
        // TODO: I guess this piece of code code could be optimized.
        // Not the code per se. The design it represents.
        if (cacheBlocks[block_index].addr == alligned_addr) {
            if (!cacheBlocks[block_index].taken[wl_offset]) {
                if (cacheBlocks[block_index].valid) {
                    peerWLEngine->handleIncomingWL(addr,
                        cacheBlocks[block_index].items[wl_offset]);
                    cacheBlocks[block_index].taken[wl_offset] = true;
                } else {
                    cacheBlocks[block_index].pending[wl_offset] = true;
                }
                reqQueue.pop();
            }
        } else { // conflict
            assert(conflictAddrQueue.size() <= conflictAddrQueueSize);
            if (conflictAddrQueue.size() < conflictAddrQueueSize) {
                cacheBlocks[block_index].numConflicts += 1;
                conflictAddrQueue.push(addr);
                reqQueue.pop();
            }
        }
    } else {
        // miss
        cacheBlocks[block_index].addr = alligned_addr;
        cacheBlocks[block_index].numConflicts = 0;
        cacheBlocks[block_index].pending = {false, false, false, false};
        cacheBlocks[block_index].pending[wl_offset] = true;
        cacheBlocks[block_index].taken = {false, false, false, false};
        cacheBlocks[block_index].valid = false;
        cacheBlocks[block_index].allocated = true;

        PacketPtr pkt = getReadPacket(alligned_addr, 64, _requestorId);

        if (!memPortBlocked()) {
            sendMemReq(pkt);
            reqQueue.pop();
        }
    }

    if ((!nextRespondEvent.scheduled()) && (!reqQueue.empty())) {
        schedule(nextRespondEvent, nextCycle());
    }
}

/*
    void recvWLWrite(Addr addr, WorkListItem wl);
*/

bool
CoalesceEngine::handleMemResp(PacketPtr pkt)
{
    if (pkt->isResp() && pkt->isWrite()) {
        return true;
    }

    Addr addr = pkt->getAddr();
    uint8_t data = pkt->getPtr<uint8_t>();

    int block_index = addr % 256;
    cacheBlocks[block_index].valid = true;

    for (i = 0; i < 4; i++) {
        cacheBlocks[block_index].items[i] = memoryToWorkList(data + (i * 16));
        cacheBlocks[block_index].taken[i] = false;
        if (cacheBlocks[block_index].pending[i]) {
            peerWLEngine->handleIncomingWL(addr + (i * 16),
                cacheBlocks[block_index].items[i]);
            cacheBlocks[block_index].taken[i] = true;
        }
        cacheBlocks[block_index].pending = false;
    }
}

void
CoalesceEngine::recvWLWrite(Addr addr, WorkListItem wl)
{
    Addr alligned_addr = (addr / 64) * 64;
    int block_index = alligned_addr % 256;
    int wl_offset = (addr - alligned_addr) / 16;

    assert(cacheBlocks[block_index].taken[wl_offset]);
    cacheBlocks[block_index].item[wl_offset] = wl;
    cacheBlocks[block_index].taken[wl_offset] = false;

    bool taken_item = false;
    taken_item &= (cacheBlocks[block_index].taken[0] &
                    cacheBlocks[block_index].taken[1] &
                    cacheBlocks[block_index].taken[2] &
                    cacheBlocks[block_index].taken[3]);

    if (!taken_item) {
        for (auto conflictAddr : conflictAddrQueue) {
            int conflict_block_index = ((conflictAddr / 64) * 64) % 256;
            if (conflict_block_index == block_index) {
                // Evict cacheBlocks[block_index]
                // Respond to conflictAddr
            }
        }
    }

}

}
