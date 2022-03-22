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

#include "accl/base/base_read_engine.hh"

namespace gem5
{

class WLEngine;

class CoalesceEngine : public BaseReadEngine
{
  private:
    struct Block
    {
        WorkListItem items[4];
        Addr addr;
        int numConflicts;
        bool pending[4];
        bool taken[4];
        bool valid;
        bool allocated;
    };

    WLEngine* peerWLEngine;

    Block cacheBlocks[256];

    int reqQueueSize;
    std::queue<Addr> reqQueue;

    int conflictAddrQueueSize;
    std::queue<Addr> conflictAddrQueue;

    EventFunctionWrapper nextRespondEvent;
    void processNextRespondEvent();

    EventFunctionWrapper nextApplyAndCommitEvent;
    void processNextApplyAndCommitEvent();

  protected:
    virtual bool handleMemResp(PacketPtr pkt);

  public:
    PARAMS(CoalesceEngine);

    CoalesceEngine(const CoalesceEngineParams &params);
    ~CoalesceEngine();

    void recvFunctional(PacketPtr pkt);

    bool recvReadAddr(Addr addr);
    void recvWLWrite(Addr addr, WorkListItem wl);

    void registerWLEngine(WLEngine* wl_engine);
}

}

#endif // __ACCL_GRAPH_SEGA_COALESCE_ENGINE_HH__
