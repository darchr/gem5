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

#include "base/types.hh"
#include "cpu/flexcpu/inflight_inst.hh"

class StLdForwarder {
  protected:
    struct StoreEntry {
      InflightInst* inst;
      PacketPtr pkt = nullptr;
      StoreEntry(InflightInst* inst): inst(inst)
      { }

      ~StoreEntry()
      {
          if (pkt) delete pkt;
      }
    };

    /**
     * Sorted (by sequence) series of store instructions waiting to be sent to
     * memory.
     */
    std::list<StoreEntry> storeBuffer;

    unsigned storeBufferSize; // TODO
    Cycles stldForwardLatency; // TODO
    unsigned stldForwardBandwidth; // TODO

    void doForward(const PacketPtr src_pkt, const RequestPtr& req,
                   const std::function<void(PacketPtr)>& callback);
  public:
    /**
     * Constructor
     */
    StLdForwarder(unsigned store_buffer_size,
                  Cycles stld_forward_delay, unsigned stld_forward_bandwidth);

    virtual ~StLdForwarder() {}

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
     * Let the forwarder know about a store instruction. This should be done as
     * soon as the store is decoded and identified. This allows callbacks to be
     * attached so that any subsequent loads can have data forwarded as soon as
     * effective address and data is calculated for the store.
     *
     * @param inst_ptr The store to inform the forwarder about.
     */
    PacketPtr& registerStore(InflightInst* inst_ptr);

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
     *                 provided. The PacketPtr is only guaranteed to be valid
     *                 for the duration of the call to the callback.
     *                 Additionally, the callback may be made on the same call
     *                 stack (before requestLoad() returns) if the latency for
     *                 forwarding is configured to be zero. Packet is nullptr
     *                 if forwarding is not possible.
     */
    void requestLoad(const std::shared_ptr<InflightInst>& inst_ptr,
                     const RequestPtr& req,
                     const std::function<void(PacketPtr)>& callback);
};

#endif // __CPU_FLEXCPU_LSQ_HH__