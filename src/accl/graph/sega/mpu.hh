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

#ifndef __ACCL_GRAPH_SEGA_MPU_HH__
#define __ACCL_GRAPH_SEGA_MPU_HH__

#include "accl/graph/sega/apply_engine.hh"
#include "accl/graph/sega/push_engine.hh"
#include "accl/graph/sega/wl_engine.hh"
#include "base/addr_range.hh"
#include "mem/port.hh"
#include "mem/packet.hh"
#include "params/MPU.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class MPU : public ClockedObject
{
  private:
    class MPURespPort : public ResponsePort
    {
      private:
        MPU* owner;

      public:
        MPURespPort(const std::string& name, MPU* owner):
          ResponsePort(name, owner), owner(owner)
        {}
        virtual AddrRangeList getAddrRanges() const;

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    class MPUReqPort : public RequestPort
    {
      private:
        MPU* owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        MPUReqPort(const std::string& name, MPU* owner):
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    class MPUMemPort : public RequestPort
    {
      private:
        MPU* owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        MPUMemPort(const std::string& name, MPU* owner):
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}

        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    virtual void startup();

    RequestorID nextRequestorId;

    MPURespPort respPort;
    MPUReqPort reqPort;
    MPUMemPort memPort;

    ApplyEngine* applyEngine;
    PushEngine* pushEngine;
    WLEngine* wlEngine;

    AddrRangeList getAddrRanges();
    void recvFunctional(PacketPtr pkt);

  public:
    PARAMS(MPU);
    MPU(const MPUParams &params);

    Port& getPort(const std::string &if_name,
                PortID idx=InvalidPortID) override;

    bool handleMemReq(PacketPtr pkt);
    void handleMemResp(PacketPtr pkt);

    bool handleWLUpdate(PacketPtr pkt);
    bool recvPushUpdate(PacketPtr pkt);
};

}

#endif // __ACCL_GRAPH_SEGA_MPU_HH__
