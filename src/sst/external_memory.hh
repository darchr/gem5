// Copyright (c) 2023-24 The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met: redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer;
// redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution;
// neither the name of the copyright holders nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef __SST_EXTERNAL_MEMORY_HH__
#define __SST_EXTERNAL_MEMORY_HH__

#include <utility>
#include <vector>

#include "base/statistics.hh"
#include "base/trace.hh"
#include "mem/abstract_mem.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/ExternalMemory.hh"
// #include "sim/sim_object.hh"
#include "sst/sst_responder_interface.hh"

/**
 * -  ExternalMemory acts as a SimObject owning pointers to both a gem5
 * ExternalMemoryPort and an SST port (via SSTResponderInterface). This bridge
 * will forward gem5 packets from the gem5 port to the SST interface. Responses
 * from SST will be handle by ExternalMemoryPort itself. Note: the bridge
 * should be decoupled from the SST libraries so that it'll be
 * SST-version-independent. Thus, there's no translation between a gem5 packet
 * and SST Response here.
 *
 *  - ExternalMemoryPort is a specialized ResponsePort working with
 * ExternalMemory.
 */

namespace gem5 {

class ExternalMemory : public memory::AbstractMemory
{
  public:
    class ExternalMemoryPort : public ResponsePort
    {
      private:
        ExternalMemory* owner;

      public:
        ExternalMemoryPort(const std::string &name_,
                            ExternalMemory* owner_);
        ~ExternalMemoryPort();
        Tick recvAtomic(PacketPtr pkt);
        void recvFunctional(PacketPtr pkt);
        bool recvTimingReq(PacketPtr pkt);
        void recvRespRetry();
        AddrRangeList getAddrRanges() const;
    };

    // We need a boolean variable to distinguish between INIT and RUN phases in
    // SST. Gem5 does functional accesses to the SST memory when:
    //  (a) It loads the kernel (at the start of the simulation
    //  (b) During VIO/disk accesses.
    // While loading the kernel, it is easy to handle all functional accesses
    // as SST allows initializing of untimed data during its INIT phase.
    // However, functional accesses done to the SST memory during RUN phase has
    // to handled separately. In this implementation, we convert all such
    // functional accesses to timing accesses so that it is correctly read from
    // the memory.
    bool init_phase_bool;

  public:
    // we need a statistics counter for this simobject to find out how many
    // requests were sent to or received from the outgoing port.
    struct StatGroup : public statistics::Group
    {
        StatGroup(statistics::Group *parent);
        /** Count the number of outgoing packets */
        statistics::Scalar numOutgoingPackets;


        /** Cumulative size of the all outgoing packets */
        statistics::Scalar sizeOutgoingPackets;

        /** Count the number of incoming packets */
        statistics::Scalar numIncomingPackets;
        /** Cumulative size of all the incoming packets */
        statistics::Scalar sizeIncomingPackets;
    } stats;
  public:
    // a gem5 ResponsePort
    ExternalMemoryPort outgoingPort;
    // pointer to the corresponding SST responder
    SSTResponderInterface* sstResponder;
    // this vector holds the initialization data sent by gem5
    std::vector<std::pair<Addr, std::vector<uint8_t>>> initData;

    AddrRangeList physicalAddressRanges;

  public:
    ExternalMemory(const ExternalMemoryParams &params);
    ~ExternalMemory();

    // Required to let the ExternalMemoryPort to send range change request.
    void init();

    bool handleTiming(PacketPtr pkt);
    // Returns the range of addresses that the ports will handle.
    // Currently, it will return the range of [0x80000000, inf), which is
    // specific to RISCV (SiFive's HiFive boards).
    AddrRangeList getAddrRanges() const;

    // Required to return a port during gem5 instantiate phase.
    Port & getPort(const std::string &if_name, PortID idx);

    // Returns the buffered data for initialization. This is necessary as
    // when gem5 sends functional requests to memory for initialization,
    // the connection in SST Memory Hierarchy has not been constructed yet.
    // This buffer is only used during the INIT phase.
    std::vector<std::pair<Addr, std::vector<uint8_t>>> getInitData() const;

    // We need Set/Get functions to set the init_phase_bool.
    // `initPhaseComplete` is used to signal the outgoing bridge that INIT
    // phase is completed and RUN phase will start.
    void initPhaseComplete(bool value);

    // We read the value of the init_phase_bool using `getInitPhaseStatus`
    // method.
    bool getInitPhaseStatus();

    // A method is needed to clear any initialization data to free up memory
    // used in the init phase.
    void clearInitData();

    // gem5 Component (from SST) will call this function to let set the
    // bridge's corresponding SSTResponderSubComponent (which implemented
    // SSTResponderInterface). I.e., this will connect this bridge to the
    // corresponding port in SST.
    void setResponder(SSTResponderInterface* responder);

    // This function is called when SST wants to sent a timing response to gem5
    bool sendTimingResp(PacketPtr pkt);

    // This function is called when SST sends response having an invalidate .
    void sendTimingSnoopReq(PacketPtr pkt);

    // This function is called when gem5 wants to send a non-timing request
    // to SST. Should only be called during the SST construction phase, i.e.
    // not at the simulation time.
    void handleRecvFunctional(PacketPtr pkt);

    // We need a variable to store the nodeIndex. This will be later used in a
    // multi-node simulation scenario.
    unsigned int nodeIndex;

    // A variable is needed to tell gem5 whether to use SST or not.
    bool useSSTSim;
};

} // namespace gem5

#endif //__SST_EXTERNAL_MEMORY_HH__
