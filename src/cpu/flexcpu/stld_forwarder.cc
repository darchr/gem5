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

#include "debug/SDCPUDeps.hh"
#include "debug/SDCPUForwarder.hh"

using namespace std;

StLdForwarder::StLdForwarder(string name,
                             unsigned store_buffer_size,
                             Cycles stld_forward_latency,
                             unsigned stld_forward_bandwidth):
    _name(name),
    storeBufferSize(store_buffer_size),
    stldForwardLatency(stld_forward_latency),
    stldForwardBandwidth(stld_forward_bandwidth)
{
    //
}

void
StLdForwarder::associateData(InflightInst* st_inst_ptr,
                             shared_ptr<uint8_t> data, Addr base)
{
    dataMap.emplace(st_inst_ptr, DataEntry({std::move(data), base}));
}

void
StLdForwarder::commitStore()
{
    assert(!storeBuffer.empty());

    InflightInst* const& inst_ptr = storeBuffer.front();

    DPRINTF(SDCPUForwarder, "Removing (seq %d) from front of store buffer.\n",
            inst_ptr->seqNum());

    dataMap.erase(inst_ptr);
    storeBuffer.pop_front();
    assert(!storeBuffer.empty() || dataMap.empty());
}

void
StLdForwarder::doForward(const DataEntry& src, const RequestPtr& req,
                         const function<void(PacketPtr)>& callback)
{
    ++forwardsProvided;

    PacketPtr pkt_to_provide = Packet::createRead(req);

    // We assume requests will not span page boundaries, so accessed
    // physical addresses should be contiguous.


    // NOTE If we implement stldforward latency, we have to replace
    //      this with a separate allocation and data copy. This code
    //      means that the lifespan of the forwarded packet is
    //      implicitly tied to the lifespan of the source packet.
    pkt_to_provide->dataStatic(src.data.get()
                             + (pkt_to_provide->getAddr() - src.base));

    if (DTRACE(SDCPUForwarder)) {
        switch (pkt_to_provide->getSize()) {
          case 1:
            DPRINTF(SDCPUForwarder, "Forwarder providing packet for Addr: %#x,"
                                    " Size: %d, Value: %#04x\n",
                                    req->getPaddr(), req->getSize(),
                                    *(pkt_to_provide->getPtr<uint8_t>()));
            break;
          case 2:
            DPRINTF(SDCPUForwarder, "Forwarder providing packet for Addr: %#x,"
                                    " Size: %d, Value: %#06x\n",
                                    req->getPaddr(), req->getSize(),
                                    *(pkt_to_provide->getPtr<uint16_t>()));
            break;
          case 4:
            DPRINTF(SDCPUForwarder, "Forwarder providing packet for Addr: %#x,"
                                    " Size: %d, Value: %#010x\n",
                                    req->getPaddr(), req->getSize(),
                                    *(pkt_to_provide->getPtr<uint32_t>()));
            break;
          case 8:
            DPRINTF(SDCPUForwarder, "Forwarder providing packet for Addr: %#x,"
                                    " Size: %d, Value: %#018x\n",
                                    req->getPaddr(), req->getSize(),
                                    *(pkt_to_provide->getPtr<uint64_t>()));
            break;
          default:
            DPRINTF(SDCPUForwarder, "Forwarder providing packet for Addr: %#x,"
                                    " Size: %d, First byte: %#04x\n",
                                    req->getPaddr(), req->getSize(),
                                    *(pkt_to_provide->getPtr<uint8_t>()));
            break;
        }
    }

    if (stldForwardLatency == 0) {
        callback(pkt_to_provide);
        delete pkt_to_provide;
    } else {
        // TODO schedule an event to perform the callback.
        panic("Non-zero stldForwardLatency not yet implemented!");
    }

    return;
}

void
StLdForwarder::populateMemDependencies(
    const shared_ptr<InflightInst>& inst_ptr)
{
    DPRINTF(SDCPUDeps, "Matching data dependencies through memory for (seq "
                       "%d).\n", inst_ptr->seqNum());
    // BEGIN Conservative Memory dependence ordering

    assert(inst_ptr->staticInst()->isLoad());

    // Checking all stores older than the inst_ptr. (This loop starts
    // from oldest)
    for (auto itr = storeBuffer.rbegin(); itr != storeBuffer.rend(); ++itr) {
        InflightInst* other = *itr;

        if (other->seqNum() > inst_ptr->seqNum()) continue;

        // Dependency will already exist on barrier, so older stores don't need
        // to be checked.
        if (other->staticInst()->isMemBarrier()) break;

        if (other->isEffAddred()) {
            // Case 1: other has already calculated an effective address

            if (inst_ptr->effAddrOverlap(*other)) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [mem]\n",
                        inst_ptr->seqNum(), other->seqNum());
                inst_ptr->addMemDependency(*other);
            }
        } else {
            // Case 2: other has not yet calculated an effective address

            weak_ptr<InflightInst> weak_inst = inst_ptr;
            other->addEffAddrCalculatedCallback(
                [this, weak_inst, other] {
                    shared_ptr<InflightInst> inst_ptr = weak_inst.lock();
                    if (!inst_ptr || inst_ptr->isSquashed()) return;

                    // If the later instruction is still calculating the
                    // effective address, let it create the dependency when
                    // it finishes instead, since we don't know if the
                    // dependency is necessary yet.
                    if (!inst_ptr->isEffAddred()) return;

                    if (inst_ptr->effAddrOverlap(*other)) {
                        DPRINTF(SDCPUDeps, "Dep %d -> %d [mem]\n",
                                inst_ptr->seqNum(), other->seqNum());
                        inst_ptr->addMemDependency(*other);
                    }
                }
            );

            // NOTE: It is important that this dependency is added after
            //       the callback above is added to other, since that
            //       callback needs to resolve first, to make sure that we
            //       don't accidentally notifyMemReady() before we've added
            //       all appropriate memory dependencies.
            DPRINTF(SDCPUDeps, "Dep %d -> %d [mem predicted overlap]\n",
                    inst_ptr->seqNum(), other->seqNum());
            inst_ptr->addMemEffAddrDependency(*other);
        }
    }

    // END Conservative Memory dependence ordering
}

void
StLdForwarder::registerMemBarrier(InflightInst* inst_ptr)
{
    // TODO enforce size bound.
    DPRINTF(SDCPUForwarder, "Inserting Mem Barrier in store buffer (seq %d) "
                            "Buffer size: %d\n",
                            inst_ptr->seqNum(),
                            storeBuffer.size());

    assert(inst_ptr->staticInst()->isMemBarrier());
    storeBuffer.emplace_back(inst_ptr);

    // As long as register... is always called in-order, instruction squash
    // happens from the back, and the number of callbacks called is the key.
    // Same for commit: if in-order, then removing one from front is always
    // fine.
    inst_ptr->addSquashCallback([this] {
        dataMap.erase(storeBuffer.back());
        storeBuffer.pop_back();
        assert(!storeBuffer.empty() || dataMap.empty());

        ++barriersSquashed;
    });
    inst_ptr->addCommitCallback([this] {
        commitStore();

        ++barriersCommitted;
    });

    ++barriersRegistered;
}

void
StLdForwarder::registerStore(InflightInst* inst_ptr)
{
    // TODO enforce size bound.
    DPRINTF(SDCPUForwarder, "Inserting store in store buffer (seq %d) Buffer "
                            "size: %d\n",
                            inst_ptr->seqNum(),
                            storeBuffer.size());

    assert(inst_ptr->staticInst()->isStore());
    storeBuffer.emplace_back(inst_ptr);

    // As long as registerStore is always called in-order, instruction squash
    // happens from the back, and the number of callbacks called is the key.
    // Same for commit: if in-order, then removing one from front is always
    // fine.
    inst_ptr->addSquashCallback([this] {
        dataMap.erase(storeBuffer.back());
        storeBuffer.pop_back();
        assert(!storeBuffer.empty() || dataMap.empty());

        ++storesSquashed;
    });
    // TODO consider holding onto the store for a little longer (e.g. until
    //      memory response time), so more loads can be forwarded. Pretty sure
    //      the other CPU model LSQs will hold onto items until response anyway
    //      but we haven't implemented it yet only to keep the code simple for
    //      now.
    inst_ptr->addCommitCallback([this] {
        commitStore();

        ++storesCommitted;
    });

    ++storesRegistered;
}

void
StLdForwarder::regStats(const string& name)
{
    storesRegistered
        .name(name + ".storesRegistered")
        .desc("Number of stores registered in the forwarder's store buffer.")
        ;

    storesCommitted
        .name(name + ".storesCommitted")
        .desc("Number of stores removed from the store buffer by commit.")
        ;

    storesSquashed
        .name(name + ".storesSquashed")
        .desc("Number of stores removed from the store buffer by squash.")
        ;

    barriersRegistered
        .name(name + ".barriersRegistered")
        .desc("Number of barriers registered in the forwarder's store buffer.")
        ;

    barriersCommitted
        .name(name + ".barriersCommitted")
        .desc("Number of barriers removed from the store buffer by commit.")
        ;

    barriersSquashed
        .name(name + ".barriersSquashed")
        .desc("Number of barriers removed from the store buffer by squash.")
        ;

    forwardsRequested
        .name(name + ".forwardsRequested")
        .desc("Number of requests for forwarding given to the forwarder.")
        ;

    forwardsProvided
        .name(name + ".forwardsProvided")
        .desc("Number of requests for forwarding satisfied by the forwarder.")
        ;

    forwardsBlockedByMissOrBarrier
        .name(name + ".forwardsBlockedByMissOrBarrier")
        .desc("Number of requests for forwarding unavailable because no "
              "matching store found before end of buffer, or barrier hit.")
        ;

    forwardsBlockedByOverlap
        .name(name + ".forwardsBlockedByOverlap")
        .desc("Number of requests for forwarding unavailable because an "
              "overlap occurred causing a dependency without enabling "
              "forwarding.")
        ;
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
    DPRINTF(SDCPUForwarder, "Checking if load @ %#x (seq %d) can be forwarded."
                            "\n", req->getPaddr(), inst_ptr->seqNum());

    ++forwardsRequested;

    assert(inst_ptr->isEffAddred());

    shared_ptr<LoadDepCtrlBlk> ctrl_blk;

    auto youngest_older_store_itr = storeBuffer.rbegin();
    for (auto itr = youngest_older_store_itr++; itr != storeBuffer.rend();
         ++itr) {
        InflightInst* const st_inst_ptr = *itr;

        if (st_inst_ptr->seqNum() > inst_ptr->seqNum()) {
            youngest_older_store_itr = itr;
            ++youngest_older_store_itr;
            // Offset by one so the value stored isn't the past the end reverse
            // iterator (@see cppreference.com for list<>::rbegin())
            ++youngest_older_store_itr;
            continue;
        }

        if (st_inst_ptr->staticInst()->isMemBarrier()) break; // Don't forward
                                                              // across barrier

        assert(st_inst_ptr->staticInst()->isStore());

        if (!st_inst_ptr->isEffAddred()) {
            DPRINTF(SDCPUForwarder,"Store (seq %d) needs to calculate "
                                   "effective address.\n",
                                   st_inst_ptr->seqNum());
            if (!ctrl_blk)
                ctrl_blk = make_shared<LoadDepCtrlBlk>(inst_ptr, req);

            ++ctrl_blk->remaining_unknowns;

            st_inst_ptr->addEffAddrCalculatedCallback(
                [this, ctrl_blk, youngest_older_store_itr, st_inst_ptr,
                 callback] {
                    shared_ptr<InflightInst> ld_inst_ptr =
                        ctrl_blk->load_inst.lock();

                    if (!ld_inst_ptr || ld_inst_ptr->isSquashed()) return;

                    --ctrl_blk->remaining_unknowns;

                    if (ctrl_blk->latest_overlapping_store
                     && ctrl_blk->latest_overlapping_store->seqNum()
                        > st_inst_ptr->seqNum()) {
                        // This event's instruction is no longer relevant for
                        // forwarding checking purposes, since it regardless of
                        // overlap, its effects are hidden by the later
                        // overlapping store, so we can return now.
                        DPRINTF(SDCPUForwarder, "Future store overlaps, store "
                                "(seq %d) cannot influence load (seq %d).\n",
                                st_inst_ptr->seqNum(),
                                ld_inst_ptr->seqNum());
                        return;
                    }

                    // Check if newly calculated effective address overlaps.
                    if (st_inst_ptr->effAddrOverlap(*ld_inst_ptr)) {
                        const bool superset =
                            st_inst_ptr->effPAddrSuperset(*ld_inst_ptr);

                        // Any previous value for this variable must be older
                        // or not yet calculated, otherwise would have returned
                        // early above.
                        DPRINTF(SDCPUForwarder, "Prospective latest st->ld "
                                "overlap (seq %d -> seq %d), %s superset\n",
                                st_inst_ptr->seqNum(),
                                ld_inst_ptr->seqNum(),
                                superset ? "is" : "not");

                        ctrl_blk->latest_overlapping_store = st_inst_ptr;
                        ctrl_blk->is_superset = superset;

                        st_inst_ptr->addRetireCallback([ctrl_blk, st_inst_ptr]{
                            if (ctrl_blk->latest_overlapping_store
                             == st_inst_ptr)
                                ctrl_blk->latest_overlapping_store = nullptr;
                        });
                    }

                    if (!ctrl_blk->remaining_unknowns
                     && (!ctrl_blk->latest_overlapping_store
                      || !ctrl_blk->is_superset)) {
                        // No further possible unknown stores can forward,
                        // so we don't bother doing any iteration, and can
                        // notify the callback thus.
                        DPRINTF(SDCPUForwarder, "Load (seq %d) cannot be "
                                                "forwarded.\n",
                                                ld_inst_ptr->seqNum());

                        if (ctrl_blk->latest_overlapping_store) {
                            ++forwardsBlockedByOverlap;
                        } else {
                            ++forwardsBlockedByMissOrBarrier;
                        }

                        callback(nullptr);
                        return;
                    }

                    // If there are remaining unknowns, and we haven't
                    // encountered any stores enforcing order, then not enough
                    // information is available yet to determine if forwarding
                    // is possible. So we return and let the next event check.
                    if (!ctrl_blk->latest_overlapping_store) {
                        DPRINTF(SDCPUForwarder, "Unknowns remain for load "
                                "(seq %d) and no overlaps known, not "
                                "forwarding.\n",
                                ld_inst_ptr->seqNum());
                        return;
                    }

                    // NOTE Since outer loop uses reverse iterator, ++itr goes
                    //      in the direction of younger to older instruction.
                    auto itr = youngest_older_store_itr;
                    // Offset it back one to undo the offset by one before.
                    --itr;
                    while (*itr != ctrl_blk->latest_overlapping_store) {
                        // If we see an unknown, we don't know if it might
                        // overlap, and should let that event handle it.
                        if (!(*itr)->isEffAddred()) {
                            DPRINTF(SDCPUForwarder, "Unknown (seq %d) remains "
                                    "for load (seq %d) between latest overlap "
                                    "and load, not forwarding.\n",
                                    (*itr)->seqNum(),
                                    ld_inst_ptr->seqNum());
                            return;
                        }

                        ++itr;
                    }

                    // If no unknowns are between the load and the store is
                    // overlapping but not a superset, we can't forward, and
                    // should inform the caller thus.
                    if (!ctrl_blk->is_superset) {
                        DPRINTF(SDCPUForwarder, "Load (seq %d) cannot be "
                                                "forwarded.\n",
                                                ld_inst_ptr->seqNum());

                        ++forwardsBlockedByOverlap;

                        callback(nullptr);
                        return;
                    }

                    // Otherwise we finally know for certain that the forward
                    // is possible, and have the information to do it, so we
                    // do it here.
                    // NOTE If the at() call fails, it means no packet was
                    //      registered for the store.
                    DPRINTF(SDCPUForwarder, "Load (seq %d) can be forwarded"
                        "from store (seq %d).\n",
                        ld_inst_ptr->seqNum(),
                        ctrl_blk->latest_overlapping_store->seqNum());
                    doForward(dataMap.at(ctrl_blk->latest_overlapping_store),
                              ctrl_blk->req,
                              callback);
            });

            continue;
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
                DPRINTF(SDCPUForwarder, "Load (seq %d) cannot be forwarded.\n",
                                        inst_ptr->seqNum());

                ++forwardsBlockedByOverlap;

                callback(nullptr);
                return;
            }

            // NOTE If the at() call fails, it means no packet was registered
            //      for the store.
            DPRINTF(SDCPUForwarder, "Load (seq %d) can be forwarded from "
                                    "store (seq %d).\n",
                                    inst_ptr->seqNum(),
                                    st_inst_ptr->seqNum());
            doForward(dataMap.at(st_inst_ptr), req, callback);

            return;
        } // End early resolution case

        if (!ctrl_blk->latest_overlapping_store) {
            DPRINTF(SDCPUForwarder, "Prospective latest st->ld overlap "
                    "(seq %d -> seq %d), %s superset\n", st_inst_ptr->seqNum(),
                    inst_ptr->seqNum(), superset ? "is" : "not");
            ctrl_blk->latest_overlapping_store = st_inst_ptr;
            ctrl_blk->is_superset = superset;

            st_inst_ptr->addRetireCallback([ctrl_blk, st_inst_ptr]{
                if (ctrl_blk->latest_overlapping_store
                    == st_inst_ptr)
                    ctrl_blk->latest_overlapping_store = nullptr;
            });
        }

        // No need to look older than a found overlap.
        break;
    } // END for

    // If no unknowns were encountered and we didn't see any overlaps, then we
    // have nothing to forward, and should notify the caller thus.
    if (!ctrl_blk) {
        DPRINTF(SDCPUForwarder, "Load (seq %d) cannot be forwarded.\n",
                                inst_ptr->seqNum());

        ++forwardsBlockedByMissOrBarrier;

        callback(nullptr);
        return;
    }

    DPRINTF(SDCPUForwarder, "Awaiting some prior stores' address calculation."
                            "\n");

    // TODO Implement different behavior for different memory models.
    // TODO start with most conservative approach, just stall all requests we
    //      aren't sure about. Speculation will come later.
}
