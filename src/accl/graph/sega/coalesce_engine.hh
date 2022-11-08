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

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/base/graph_workload.hh"
#include "accl/graph/sega/base_memory_engine.hh"
#include "accl/graph/sega/enums.hh"
#include "accl/graph/sega/work_directory.hh"
#include "base/cprintf.hh"
#include "base/statistics.hh"
#include "params/CoalesceEngine.hh"

namespace gem5
{

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
        bool dirty;
        bool hasConflict;
        CacheState state;
        Tick lastChangedTick;
        Block() {}
        Block(int num_elements):
          addr(-1),
          busyMask(0),
          valid(false),
          dirty(false),
          hasConflict(false),
          state(CacheState::INVALID),
          lastChangedTick(0)
        {
          items = new WorkListItem [num_elements];
        }

        void reset() {
            addr = -1;
            busyMask = 0;
            valid = false;
            dirty = false;
            hasConflict = false;
            state = CacheState::INVALID;
            lastChangedTick = 0;
        }

        std::string to_string() {
            return csprintf("CacheBlock{addr: %lu, busyMask: %lu, valid: %s, "
                "dirty: %s, hasConflict: %s, state: %s, lastChangedTick: %lu}",
                addr, busyMask, valid ? "true" : "false",
                dirty ? "true" : "false", hasConflict ? "true" : "false",
                cacheStateStrings[state], lastChangedTick);
        }
    };

    struct ReadPurpose : public Packet::SenderState
    {
      ReadDestination _dest;
      ReadPurpose(ReadDestination dest): _dest(dest) {}
      ReadDestination dest() { return _dest; }
    };

    MPU* owner;
    WorkDirectory* directory;
    GraphWorkload* graphWorkload;

    Addr lastAtomAddr;

    int numLines;
    int numElementsPerLine;
    Block* cacheBlocks;

    int onTheFlyReqs;
    std::unordered_map<int, std::vector<Addr>> MSHR;

    // Response route to WLEngine
    int maxRespPerCycle;
    std::deque<std::tuple<Addr, WorkListItem, Tick>> responseQueue;

    // Tracking work in cache
    int pullsReceived;
    // NOTE: Remember to erase from this upon eviction from cache
    UniqueFIFO<int> activeCacheBlocks;

    int pullsScheduled;
    int pendingPullReads;
    // A map from addr to sendMask. sendMask determines which bytes to
    // send for push when getting the read response from memory.
    std::unordered_set<Addr> pendingPullAddrs;

    int activeBufferSize;
    int postPushWBQueueSize;
    std::deque<std::tuple<PacketPtr, Tick>> activeBuffer;
    std::deque<std::tuple<PacketPtr, Tick>> postPushWBQueue;

    bool timeToPull();
    bool canSchedulePull();
    bool workLeftInMem();
    bool pullCondition();
    int getBlockIndex(Addr addr);

    MemoryEvent nextMemoryEvent;
    void processNextMemoryEvent();
    void processNextRead(int block_index, Tick schedule_tick);
    void processNextWriteBack(int block_index, Tick schedule_tick);
    void processNextVertexPull(int ignore, Tick schedule_tick);
    void processNextPostPushWB(int ignore, Tick schedule_tick);
    std::deque<std::tuple<
        std::function<void(int, Tick)>, int, Tick>> memoryFunctionQueue;

    EventFunctionWrapper nextResponseEvent;
    void processNextResponseEvent();

    EventFunctionWrapper nextApplyEvent;
    void processNextApplyEvent();

    struct CoalesceStats : public statistics::Group
    {
        CoalesceStats(CoalesceEngine &coalesce);

        virtual void regStats() override;

        virtual void resetStats() override;

        CoalesceEngine &coalesce;

        Tick lastResetTick;

        statistics::Scalar numVertexReads;
        statistics::Scalar numVertexWrites;
        statistics::Scalar readHits;
        statistics::Scalar readMisses;
        statistics::Scalar readHitUnderMisses;
        statistics::Scalar numConflicts;
        statistics::Scalar responsePortShortage;
        statistics::Scalar numMemoryBlocks;
        statistics::Scalar verticesPulled;
        statistics::Scalar verticesPushed;
        statistics::Scalar lastVertexPullTime;
        statistics::Scalar lastVertexPushTime;
        statistics::Scalar numInvalidWriteBacks;

        statistics::Formula hitRate;
        statistics::Formula vertexPullBW;
        statistics::Formula vertexPushBW;

        statistics::Histogram frontierSize;
        statistics::Histogram blockActiveCount;
        statistics::Histogram responseQueueLatency;
        statistics::Histogram memoryFunctionLatency;
    };

    CoalesceStats stats;

  protected:
    virtual void recvMemRetry() override;
    virtual bool handleMemResp(PacketPtr pkt) override;

  public:
    PARAMS(CoalesceEngine);
    CoalesceEngine(const Params &params);
    void registerMPU(MPU* mpu);

    void recvWorkload(GraphWorkload* workload) { graphWorkload = workload; }
    virtual void recvFunctional(PacketPtr pkt);

    void postMemInitSetup();

    void createPopCountDirectory(int atoms_per_block);

    ReadReturnStatus recvWLRead(Addr addr);
    void recvWLWrite(Addr addr, WorkListItem wl);

    int workCount();
    void recvVertexPull();

    bool done();
};

}

#endif // __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__
