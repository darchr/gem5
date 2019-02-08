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
    assert(storeBuffer.front().inst->isComplete());

    storeBuffer.pop_front();
}

void
StLdForwarder::doForward(const PacketPtr src_pkt, const RequestPtr& req,
                         const function<void(PacketPtr)>& callback)
{

    PacketPtr pkt_to_provide = Packet::createRead(req);

    // We assume requests will not span page boundaries, so accessed
    // physical addresses should be contiguous.


    // NOTE If we implement stldforward latency, we have to replace
    //      this with a separate allocation and data copy. This code
    //      means that the lifespan of the forwarded packet is
    //      implicitly tied to the lifespan of the source packet.
    pkt_to_provide->dataStatic(src_pkt->getPtr<uint8_t>()
        + (pkt_to_provide->getAddr() - src_pkt->getAddr()));

    // TODO accomplish the forward.
    if (stldForwardLatency == 0) {
        callback(pkt_to_provide);
        delete pkt_to_provide;
    } else {
        // TODO schedule an event to perform the callback.
        panic("Non-zero stldForwardLatency not yet implemented!");
    }

    return;
}

PacketPtr&
StLdForwarder::registerStore(InflightInst* inst_ptr)
{
    // TODO enforce size bound.


    storeBuffer.emplace_back(inst_ptr);

    // As long as registerStore is always called in-order, instruction squash
    // happens from the back, and the number of callbacks called is the key.
    // Same for commit: if in-order, then removing one from front is always
    // fine.
    inst_ptr->addSquashCallback([this]{ storeBuffer.pop_back(); });
    // TODO consider holding onto the store for a little longer (e.g. until
    //      memory response time), so more loads can be forwarded. Pretty sure
    //      the other CPU model LSQs will hold onto items until response anyway
    //      but we haven't implemented it yet only to keep the code simple for
    //      now.
    inst_ptr->addCommitCallback([this]{ commitStore(); });

    return storeBuffer.back().pkt;
}

// anonymous namespace for internal linkage only.
namespace {
struct LoadDepCtrlBlk {
    weak_ptr<InflightInst> load_inst;
    RequestPtr req;
    size_t remaining_unknowns = 0;
    InflightInst* latest_overlapping_store = nullptr;
        // If a store before the relevant load is squashed, the load will be
        // squashed too. Therefore we don't need to use weak_ptr here to check
        // liveness, as a check on the load will be sufficient.
    bool is_superset = false;

    LoadDepCtrlBlk(weak_ptr<InflightInst> load_inst,
                   const RequestPtr& req): load_inst(load_inst), req(req)
    { }
};
};

void
StLdForwarder::requestLoad(const shared_ptr<InflightInst>& inst_ptr,
                           const RequestPtr& req,
                           const function<void(PacketPtr)>& callback)
{
    // TODO enforce bandwidth bound

    assert(inst_ptr->isEffAddred());

    shared_ptr<LoadDepCtrlBlk> ctrl_blk;

    auto youngest_older_store_itr = storeBuffer.rbegin();
    for (auto itr = youngest_older_store_itr; itr != storeBuffer.rend();
         ++itr) {
        InflightInst* const st_inst_ptr = itr->inst;

        if (st_inst_ptr->seqNum() > inst_ptr->seqNum()) {
            youngest_older_store_itr = itr;
            ++youngest_older_store_itr;
            continue;
        }

        if (!st_inst_ptr->isEffAddred()) {
            if (!ctrl_blk) ctrl_blk = make_shared<LoadDepCtrlBlk>(inst_ptr,
                                                                  req);

            ++ctrl_blk->remaining_unknowns;

            st_inst_ptr->addEffAddrCalculatedCallback(
                [this, ctrl_blk, youngest_older_store_itr, st_inst_ptr,
                 callback] {
                    shared_ptr<InflightInst> load_inst_ptr =
                        ctrl_blk->load_inst.lock();

                    if (!load_inst_ptr || load_inst_ptr->isSquashed()) return;

                    --ctrl_blk->remaining_unknowns;

                    if (ctrl_blk->latest_overlapping_store
                     && ctrl_blk->latest_overlapping_store->seqNum()
                        > st_inst_ptr->seqNum()) {
                        // This event's instruction is no longer relevant for
                        // forwarding checking purposes, since it regardless of
                        // overlap, its effects are hidden by the later
                        // overlapping store, so we can return now.
                        return;
                    }

                    // Check if newly calculated effective address overlaps.
                    if (st_inst_ptr->effAddrOverlap(*load_inst_ptr)) {
                        const bool superset =
                            st_inst_ptr->effPAddrSuperset(*load_inst_ptr);

                        // Any previous value for this variable must be older
                        // or not yet calculated, otherwise would have returned
                        // early above.
                        ctrl_blk->latest_overlapping_store = st_inst_ptr;
                        ctrl_blk->is_superset = superset;
                    }

                    if (!ctrl_blk->remaining_unknowns
                     && (!ctrl_blk->latest_overlapping_store
                      || !ctrl_blk->is_superset)) {
                        // No further possible unknown stores can forward,
                        // so we don't bother doing any iteration, and can
                        // notify the callback thus.
                        callback(nullptr);
                        return;
                    }

                    // If there are remaining unknowns, and we haven't
                    // encountered any stores enforcing order, then not enough
                    // information is available yet to determine if forwarding
                    // is possible. So we return and let the next event check.
                    if (!ctrl_blk->latest_overlapping_store) return;

                    // NOTE Since outer loop uses reverse iterator, ++itr goes
                    //      in the direction of younger to older instruction.
                    auto itr = youngest_older_store_itr;
                    while (itr->inst != ctrl_blk->latest_overlapping_store) {
                        // If we see an unknown, we don't know if it might
                        // overlap, and should let that event handle it.
                        if (!itr->inst->isEffAddred()) return;

                        ++itr;
                    }

                    // If no unknowns are between the load and the store is
                    // overlapping but not a superset, we can't forward, and
                    // should inform the caller thus.
                    if (!ctrl_blk->is_superset) {
                        callback(nullptr);
                        return;
                    }

                    // Otherwise we finally know for certain that the forward
                    // is possible, and have the information to do it, so we
                    // do it here.
                    PacketPtr& srcPkt = itr->pkt;
                    doForward(srcPkt, ctrl_blk->req, callback);
            });
        }

        // If the memory access is unrelated, we keep searching.
        if (!st_inst_ptr->effAddrOverlap(*inst_ptr)) continue;
        const bool superset = st_inst_ptr->effPAddrSuperset(*inst_ptr);

        if (!ctrl_blk) { // early resolution case
            // If a ctrl_blk is not observed, it means we haven't
            // encountered a store that hasn't had its effective
            // address calculated yet.

            // If the most recent related memory access intersects, but cannot
            // provide the complete data for forwarding, then we cannot
            // forward.
            if (!superset) {
                callback(nullptr);
                return;
            }

            PacketPtr pkt_to_provide = Packet::createRead(req);

            // We assume requests will not span page boundaries, so accessed
            // physical addresses should be contiguous.

            PacketPtr& srcPkt = itr->pkt;

            // NOTE If we implement stldforward latency, we have to replace
            //      this with a separate allocation and data copy. This code
            //      means that the lifespan of the forwarded packet is
            //      implicitly tied to the lifespan of the source packet.
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

            return;
        } // End early resolution case

        if (!ctrl_blk->latest_overlapping_store) {
            ctrl_blk->latest_overlapping_store = st_inst_ptr;
            ctrl_blk->is_superset = superset;
        }
    } // END for

    // If no unknowns were encountered and we didn't see any overlaps, then we
    // have nothing to forward, and should notify the caller thus.
    if (!ctrl_blk) callback(nullptr);

    // TODO Implement different behavior for different memory models.
    // TODO start with most conservative approach, just stall all requests we
    //      aren't sure about. Speculation will come later.
}
