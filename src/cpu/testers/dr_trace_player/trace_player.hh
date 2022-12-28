/*
 * Copyright (c) 2022 The Regents of the University of California.
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


#ifndef __CPU_TESTERS_DR_TRACE_PLAYER_TRACE_PLAYER_HH__
#define __CPU_TESTERS_DR_TRACE_PLAYER_TRACE_PLAYER_HH__

/**
 * @file
 * Contains the player for dynamario traces
 * This object works with the trace reader to play dynamorio traces
 */

#include "cpu/testers/dr_trace_player/trace_reader.hh"
#include "mem/port.hh"
#include "params/DRTracePlayer.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

/**
 * An object to play dynamorio traces.
 * This object represents on "core." The core can execute instructions from
 * multiple different threads. The trace reader acts as the scheduler and
 * chooses which cores will execute which thread and when.
 * The cores (players) must request the next item from the centralized trace
 * reader.
 */
class DRTracePlayer : public ClockedObject
{
  private:
    EventFunctionWrapper onClockEvent;

    // Parameters
    DRTraceReader *reader;
    int playerId;
    int requestorId;

    // State
    bool stalled = false;
    Addr curPC = 0;
    DRTraceReader::TraceRef nextRef;

    /**
     * @brief Take the current reference in nextRef and try to execute it.
     *
     * If the instruction can't be executed (e.g., something is stalled), then
     * don't get the next instruction.
     */
    void tryExecuteInsts();

    void recvReqRetry();

    bool recvTimingResp(PacketPtr pkt);

    PacketPtr getPacket(DRTraceReader::TraceRef &mem_ref);

    /**
     * @brief Send a request to memory based on the trace
     *
     * @param mem_ref
     * @return true if the port is stalled
     * @return false if the port accepted the packet
     */
    bool trySendMemRef(DRTraceReader::TraceRef &mem_ref);

  public:
    PARAMS(DRTracePlayer);
    DRTracePlayer(const Params &params);

    void startup() override;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

  private:
    class DataPort : public RequestPort
    {
      public:
        DataPort(const std::string &name, DRTracePlayer &player) :
            RequestPort(name, &player), player(player)
        { }

      protected:
        void recvReqRetry() override { player.recvReqRetry(); }

        bool recvTimingResp(PacketPtr pkt) override
        { return player.recvTimingResp(pkt); }

        void recvTimingSnoopReq(PacketPtr pkt) override { }

        void recvFunctionalSnoop(PacketPtr pkt) override { }

        Tick recvAtomicSnoop(PacketPtr pkt) override { return 0; }

      private:
        DRTracePlayer &player;
    };

    DataPort port;
};

} // namespace gem5

#endif //__CPU_TESTERS_DR_TRACE_PLAYER_TRACE_PLAYER_HH__
