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

#ifndef __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__
#define __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__

#include <bitset>

#include "accl/graph/sega/base_memory_engine.hh"
#include "accl/graph/base/data_structs.hh"
#include "accl/graph/sega/push_engine.hh"
#include "base/statistics.hh"
#include "params/CoalesceEngine.hh"

#define MAX_BITVECTOR_SIZE (1 << 28)

namespace gem5
{

class WLEngine;

class CoalesceEngine : public BaseMemoryEngine
{
  private:
    struct Block
    {
        WorkListItem* items;
        Addr addr;
        uint8_t busyMask;
        bool allocated;
        bool valid;
        bool hasConflict;
        bool dirty;
        // TODO: This might be useful in the future
        // Tick lastWLWriteTick;
        Block() {}
        Block(int num_elements):
          addr(0),
          busyMask(0),
          allocated(false),
          valid(false),
          hasConflict(false),
          dirty(false)
        {
          items = new WorkListItem [num_elements];
        }
    };

    struct SenderState : public Packet::SenderState
    {
      bool isRetry;
      SenderState(bool is_retry): isRetry(is_retry) {}
    };

    // int nmpu;
    // Addr memoryAddressOffset;

    WLEngine* peerWLEngine;
    PushEngine* peerPushEngine;

    Block* cacheBlocks;

    std::string workload;
    float thereshold;
    int numLines;
    int numElementsPerLine;

    int numMSHREntries;
    int numTgtsPerMSHR;
    std::unordered_map<int, std::vector<Addr>> MSHR;

    std::deque<int> fillQueue;

    std::deque<std::tuple<Addr, WorkListItem>> responseQueue;

    int currentBitSliceIndex;
    int numRetriesReceived;
    InOutSet<int> applyQueue;
    std::bitset<MAX_BITVECTOR_SIZE> needsPush;

    InOutSet<int> writeBackQueue;

    int getBlockIndex(Addr addr);
    bool applyCondition(uint32_t value, uint32_t update);
    int getBitIndexBase(Addr addr);
    Addr getBlockAddrFromBitIndex(int index);
    std::tuple<bool, int> getOptimalBitVectorSlice();

    std::deque<std::string> pendingEventQueue;

    MemoryEvent nextMemoryReadEvent;
    void processNextMemoryReadEvent();

    EventFunctionWrapper nextRespondEvent;
    void processNextRespondEvent();

    EventFunctionWrapper nextApplyEvent;
    void processNextApplyEvent();

    MemoryEvent nextWriteBackEvent;
    void processNextWriteBackEvent();

    MemoryEvent nextRecvPushRetryEvent;
    void processNextRecvPushRetryEvent();

    struct CoalesceStats : public statistics::Group
    {
      CoalesceStats(CoalesceEngine &coalesce);

      void regStats() override;

      CoalesceEngine &coalesce;

      statistics::Scalar numVertexReads;
      statistics::Scalar numVertexWrites;
      statistics::Scalar readHits;
      statistics::Scalar readMisses;
      statistics::Scalar readHitUnderMisses;
      statistics::Scalar readRejections;
      statistics::Scalar falseApplySchedules;
      statistics::Scalar falseEvictSchedules;
    };

    CoalesceStats stats;

  protected:
    virtual void recvMemRetry() override;
    virtual bool handleMemResp(PacketPtr pkt) override;

  public:
    PARAMS(CoalesceEngine);

    CoalesceEngine(const Params &params);

    bool recvWLRead(Addr addr);
    void recvWLWrite(Addr addr, WorkListItem wl);

    void registerWLEngine(WLEngine* wl_engine);

    void recvPushRetry();

    // virtual void startup() override;
};

}

#endif // __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__
