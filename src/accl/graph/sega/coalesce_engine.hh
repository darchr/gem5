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
#include "base/cprintf.hh"
#include "base/statistics.hh"
#include "params/CoalesceEngine.hh"

#define MAX_BITVECTOR_SIZE (1 << 28)

namespace gem5
{

enum BitStatus
{
    PENDING_READ,
    IN_CACHE,
    IN_MEMORY,
    GARBAGE
};

class MPU;

class CoalesceEngine : public BaseMemoryEngine
{
  private:
    struct Block
    {
        WorkListItem* items;
        Addr addr;
        uint64_t busyMask;
        bool valid;
        bool needsApply;
        bool needsWB;
        bool pendingData;
        bool pendingApply;
        bool pendingWB;
        Tick lastChangedTick;
        // TODO: This might be useful in the future
        // Tick lastWLWriteTick;
        Block() {}
        Block(int num_elements):
          addr(0),
          busyMask(0),
          valid(false),
          needsApply(false),
          needsWB(false),
          pendingData(false),
          pendingApply(false),
          pendingWB(false),
          lastChangedTick(0)
        {
          items = new WorkListItem [num_elements];
        }

        std::string to_string() {
            return csprintf("CacheBlock{addr: %lu, busyMask: %lu, valid: %s, "
                "needsApply: %s, needsWB: %s, pendingData: %s, "
                "pendingApply: %s, pendingWB: %s, lastChangedTick: %lu}",
                addr, busyMask, valid ? "true" : "false",
                needsApply ? "true" : "false", needsWB ? "true" : "false",
                pendingData ? "true" : "false", pendingApply ? "true" : "false",
                pendingWB ? "true" : "false", lastChangedTick);
        }
    };

    struct SenderState : public Packet::SenderState
    {
      bool isRetry;
      SenderState(bool is_retry): isRetry(is_retry) {}
    };
    MPU* owner;

    int numLines;
    int numElementsPerLine;
    Block* cacheBlocks;

    int onTheFlyReqs;
    int numMSHREntries;
    int numTgtsPerMSHR;
    std::unordered_map<int, std::vector<Addr>> MSHR;
    int maxRespPerCycle;
    std::deque<std::tuple<Addr, WorkListItem, Tick>> responseQueue;

    int _workCount;
    int numPullsReceived;
    UniqueFIFO<int> applyQueue;
    std::bitset<MAX_BITVECTOR_SIZE> needsPush;
    std::deque<int> activeBits;

    int getBlockIndex(Addr addr);
    int getBitIndexBase(Addr addr);
    Addr getBlockAddrFromBitIndex(int index);
    std::tuple<BitStatus, Addr, int> getOptimalPullAddr();

    // A map from addr to sendMask. sendMask determines which bytes to
    // send for push when getting the read response from memory.
    std::unordered_map<Addr, uint64_t> pendingVertexPullReads;

    std::string workload;
    uint32_t reduce(uint32_t update, uint32_t value);

    MemoryEvent nextMemoryEvent;
    void processNextMemoryEvent();
    void processNextRead(int block_index, Tick schedule_tick);
    void processNextWriteBack(int block_index, Tick schedule_tick);
    void processNextVertexPull(int ignore, Tick schedule_tick);
    std::deque<std::tuple<
        std::function<void(int, Tick)>, int, Tick>> memoryFunctionQueue;

    EventFunctionWrapper nextResponseEvent;
    void processNextResponseEvent();

    EventFunctionWrapper nextApplyEvent;
    void processNextApplyEvent();

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
      statistics::Scalar mshrEntryShortage;
      statistics::Scalar mshrTargetShortage;
      statistics::Scalar workSearchFails;
      statistics::Scalar numDoubleMemReads;

      statistics::Formula hitRate;
      statistics::Histogram mshrEntryLength;
      statistics::Histogram bitvectorLength;
    };

    CoalesceStats stats;

  protected:
    virtual void recvMemRetry() override;
    virtual bool handleMemResp(PacketPtr pkt) override;

  public:
    PARAMS(CoalesceEngine);
    CoalesceEngine(const Params &params);
    void registerMPU(MPU* mpu);

    bool recvWLRead(Addr addr);
    void recvWLWrite(Addr addr, WorkListItem wl);

    int workCount() { return _workCount; }
    void recvVertexPull();

    bool done();
};

}

#endif // __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__
