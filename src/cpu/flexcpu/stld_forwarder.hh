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

#ifndef __CPU_FLEXCPU_LSQ_HH__
#define __CPU_FLEXCPU_LSQ_HH__

#include <list>
#include <memory>
#include <unordered_map>

#include "base/types.hh"
#include "cpu/flexcpu/inflight_inst.hh"

class StLdForwarder {
  protected:
    struct DataEntry {
        std::shared_ptr<uint8_t> data;
        Addr base;
    };

    /**
     * We store a name so name() can be called by tracing functions.
     */
    std::string _name;

    Stats::Scalar storesRegistered;
    Stats::Scalar storesCommitted;
    Stats::Scalar storesSquashed;
    Stats::Scalar barriersRegistered;
    Stats::Scalar barriersCommitted;
    Stats::Scalar barriersSquashed;

    Stats::Scalar forwardsRequested;
    Stats::Scalar forwardsProvided;
    Stats::Scalar forwardsBlockedByMissOrBarrier;
    Stats::Scalar forwardsBlockedByOverlap;

    /**
     * Sorted (by sequence) series of store instructions waiting to be sent to
     * memory.
     */
    std::list<InflightInst*> storeBuffer;
    std::unordered_map<InflightInst*, DataEntry> dataMap;

    unsigned storeBufferSize; // TODO

    bool stldForwardEnabled;
    Cycles stldForwardLatency; // TODO
    unsigned stldForwardBandwidth; // TODO

    void doForward(const DataEntry& src, const RequestPtr& req,
                   const std::function<void(PacketPtr)>& callback);
  public:
    /**
     * Constructor
     */
    StLdForwarder(std::string name, unsigned store_buffer_size,
                  bool stld_forward_enabled, Cycles stld_forward_delay,
                  unsigned stld_forward_bandwidth);

    virtual ~StLdForwarder() {}

    void associateData(InflightInst* st_inst_ptr,
                       std::shared_ptr<uint8_t> data, Addr base);

    /**
     * Let the forwarder know that a store should be committed, and necessary
     * state changes sent to memory. No parameter should be necessary, since
     * commits should happen in-order, and therefore the oldest unsquashed
     * store is the only such instruction to consider.
     *
     * *Note: An assumption is being made that all stores (to be committed)
     *        have been registered through registerStore() beforehand!*
     */
    void commitStore();

    /**
     * Accessor for the name field. Added for use with tracing.
     *
     * @return The name of the StLdForwarder.
     */
    const std::string& name() const
    { return _name; }

    /**
     * Use the store buffer data to populate the data dependencies through
     * memory for a load, absent forwarding. Should be called only after the
     * load has had its effective address calculated.
     *
     * @param inst_ptr A reference to the load instruction for which to
     *                 populate dependencies.
     */
    void populateMemDependencies(
        const std::shared_ptr<InflightInst>& inst_ptr);

    /**
     * Let the forwarder know about a memory barrier instruction. This should
     * be done as soon as the barrier is decoded and identified. This allows
     * searches through the store buffer to be shortened, since the barrier
     * will impose stricter restrictions than older stores.
     *
     * @param inst_ptr The barrier to inform the forwarder about.
     */
    void registerMemBarrier(InflightInst* inst_ptr);

    /**
     * Let the forwarder know about a store instruction. This should be done as
     * soon as the store is decoded and identified. This allows callbacks to be
     * attached so that any subsequent loads can have data forwarded as soon as
     * effective address and data is calculated for the store.
     *
     * @param inst_ptr The store to inform the forwarder about.
     */
    void registerStore(InflightInst* inst_ptr);

    /**
     * Register forwarder-specific stats.
     * The owner should call this function in its regStats with the forwarder-
     * specific name
     *
     * @param name from the owner for this forwarder.
     */
    void regStats(const std::string &name);

    /**
     * Request data for a load instruction. If a store has data for the load,
     * then the data should be forwarded as soon as possilble. Otherwise, as
     * early as is possible, the forwarder will forward the data through the
     * callback, or inform that such is not possible by calling the callback
     * with a nullptr packet.
     *
     * @param inst_ptr The load instruction for which data should be provided.
     * @param req The (translated) request containing the address of the load.
     * @param callback The callback through which the forwarded packet will be
     *                 provided. The PacketPtr is allocated with new and it is
     *                 the callback's job to delete it when it no longer needs
     *                 it. Additionally, the callback may be made on the same
     *                 call stack (before requestLoad() returns) if the latency
     *                 for forwarding is configured to be zero. Packet is
     *                 nullptr if forwarding is not possible.
     */
    void requestLoad(const std::shared_ptr<InflightInst>& inst_ptr,
                     const RequestPtr& req,
                     const std::function<void(PacketPtr)>& callback);
};

#endif // __CPU_FLEXCPU_LSQ_HH__
