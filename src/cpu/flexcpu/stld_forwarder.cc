/*
 * Copyright (c) 2019 The Regents of The University of California
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
 *
 * Authors: Bradley Wang
 */

#include "cpu/flexcpu/stld_forwarder.hh"

using namespace std;

StLdForwarder::StLdForwarder(unsigned store_buffer_size,
                             Cycles stld_forward_latency,
                             unsigned stld_forward_bandwidth):
    storeBufferSize(store_buffer_size),
    stldForwardLatency(stld_forward_latency),
    stldForwardBandwidth(stld_forward_bandwidth)
{
    //
}

void
StLdForwarder::commitStore()
{
    assert(!storeBuffer.empty());
    assert(!storeBuffer.front().inst.expired());
    assert(storeBuffer.front().inst.lock()->isComplete());

    storeBuffer.pop_front();
}

PacketPtr&
StLdForwarder::registerStore(const shared_ptr<InflightInst>& inst_ptr)
{
    // TODO enforce size bound.


    storeBuffer.emplace_back(inst_ptr, nullptr);

    // As long as registerStore is always called, we will only ever have
    // dead instructions at the back, and the number of squash callbacks should
    // equal the number of times we need to remove something from the buffer.
    inst_ptr->addSquashCallback([this]{ storeBuffer.pop_back(); });
    inst_ptr->addCommitCallback([this]{ commitStore(); });
    // TODO consider holding onto the store for a little longer (e.g. until
    //      memory response time), so more loads can be forwarded. Pretty sure
    //      the other CPU model LSQs will hold onto items until response anyway
    //      but we haven't implemented it yet only to keep the code simple for
    //      now.
    return storeBuffer.back().pkt;
}

bool
StLdForwarder::requestLoad(const shared_ptr<InflightInst>& inst_ptr,
                           const RequestPtr& req,
                           const function<void(PacketPtr)>& callback)
{
    // TODO enforce bandwidth bound

    for (auto itr = storeBuffer.rbegin(); itr != storeBuffer.rend(); ++itr) {
        shared_ptr<InflightInst> st_inst_ptr = itr->inst.lock();

        if (st_inst_ptr->seqNum() > inst_ptr->seqNum()) {
            continue;
        }

        assert(st_inst_ptr->isEffAddred()); // Assume we have a strict memory
                                            // dependence policy for now

        // If the memory access is unrelated, we keep searching.
        if (!st_inst_ptr->effAddrOverlap(*inst_ptr)) continue;

        // If the most recent related memory access intersects, but cannot
        // provide the complete data for forwarding, then we cannot forward.
        if (!st_inst_ptr->effPAddrSuperset(*inst_ptr)) return false;

        PacketPtr pkt_to_provide = Packet::createRead(req);

        // We assume requests will not span page boundaries, so accessed
        // physical addresses should be contiguous.

        PacketPtr& srcPkt = itr->pkt;

        // NOTE If we implement stldforward latency, we have to replace this
        //      with a separate allocation and data copy. This code means that
        //      the lifespan of the forwarded packet is implicitly tied to the
        //      lifespan of the source packet.
        pkt_to_provide->dataStatic(srcPkt->getPtr<uint8_t>()
            + (pkt_to_provide->getAddr() - srcPkt->getAddr()));

        // TODO accomplish the forward.
        if (stldForwardLatency == 0) {
            callback(pkt_to_provide);
            delete pkt_to_provide;
        } else {
            // TODO schedule an event to perform the callback.
            panic("Non-zero stldForwardLatency not yet implemented!");
        }

        return true;
    }

    // TODO Implement different behavior for different memory models.
    // TODO start with most conservative approach, just stall all requests we
    //      aren't sure about. Speculation will come later.
    return false;
}
