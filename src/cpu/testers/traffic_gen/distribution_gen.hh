/*
 * Copyright (c) 2021 The Regents of the University of California.
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

#ifndef __CPU_TESTERS_TRAFFIC_GEN_DISTRIBUTION_GEN_HH__
#define __CPU_TESTERS_TRAFFIC_GEN_DISTRIBUTION_GEN_HH__

#include "base/types.hh"
#include "cpu/testers/traffic_gen/distribution.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/DistributionGen.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class DistributionGen : public ClockedObject
{
  private:
    class GenPort : public RequestPort
    {
      private:
        DistributionGen* owner;
        PacketPtr blockedPacket;

      public:
        GenPort(const std::string& name, DistributionGen* owner) :
            RequestPort(name), owner(owner), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return (blockedPacket != nullptr); }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    GenPort port;

    uint32_t requestorId;

    uint32_t lsb;
    uint32_t bits;
    uint32_t match;
    uint32_t pktsize;

    Tick duration;

    Distribution* distribution;

    PacketPtr createPacket(Addr addr);

    void scheduleNextGenEvent();
    EventFunctionWrapper nextGenEvent;
    void processNextGenEvent();

  public:
    PARAMS(DistributionGen);
    DistributionGen(const Params& params);

    virtual void startup() override;
    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;

    void recvReqRetry();

    void setRayleighDistribution(Tick time_unit, double lambda, double sigma);
};

}
#endif // __CPU_TESTERS_TRAFFIC_GEN_DISTRIBUTION_GEN_HH__
