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

#include "accl/graph/base/base_mem_engine.hh"
#include "accl/graph/base/data_structs.hh"
#include "accl/graph/sega/push_engine.hh"
#include "base/statistics.hh"
#include "params/CoalesceEngine.hh"

#define MAX_BITVECTOR_SIZE (1 << 30)

// TODO: Add parameters for size, memory atom size, type size,
// length of items in the blocks.
namespace gem5
{

class WLEngine;

class CoalesceEngine : public BaseMemEngine
{
  private:
    class MemoryEvent : public EventFunctionWrapper
    {
      private:
        bool _pending;
      public:
        MemoryEvent(const std::function<void(void)> &callback,
                    const std::string &name):
            EventFunctionWrapper(callback, name), _pending(false)
        {}
        bool pending() { return _pending; }
        void sleep() { _pending = true; }
        void wake() { _pending = false; }
    };

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

    int nmpu;
    Addr memoryAddressOffset;

    WLEngine* peerWLEngine;
    PushEngine* peerPushEngine;

    Block* cacheBlocks;

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
    InOutSet<int> replaceQueue;

    int getBlockIndex(Addr addr);
    int getBitIndexBase(Addr addr);
    Addr getBlockAddrFromBitIndex(int index);

    MemoryEvent nextMemoryReadEvent;
    void processNextMemoryReadEvent();

    EventFunctionWrapper nextRespondEvent;
    void processNextRespondEvent();

    EventFunctionWrapper nextApplyEvent;
    void processNextApplyEvent();

    MemoryEvent nextWriteBackEvent;
    void processNextWriteBackEvent();

    EventFunctionWrapper nextSendRetryEvent;
    void processNextSendRetryEvent();

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
    virtual int respBuffSize() { return -1; }
    virtual void recvMemRetry();
    virtual bool handleMemResp(PacketPtr pkt);

  public:
    PARAMS(CoalesceEngine);

    CoalesceEngine(const CoalesceEngineParams &params);

    bool recvWLRead(Addr addr);
    void recvWLWrite(Addr addr, WorkListItem wl);

    void registerWLEngine(WLEngine* wl_engine);

    void recvPushRetry();

    void recvFunctional(PacketPtr pkt);

    virtual void startup();
};

}

#endif // __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__
