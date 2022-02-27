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

#ifndef __ACCL_GRAPH_SEGA_WL_DIRECTORY_HH__
#define __ACCL_GRAPH_SEGA_WL_DIRECTORY_HH__

#include <queue>
#include <unordered_map>

#include "base/addr_range.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/BaseEngine.hh"
#include "sim/clocked_object.hh"

namespace gem5
{

class WLDirectory : public ClockedObject
{
  private:
    class RespPort : public ResponsePort
    {
      private:
        WLDirectory* owner;
        PortID _id;

      public:
        RespPort(const std::string& name, WLDirectory* owner, PortID id):
          ResponsePort(name, owner), owner(owner), _id(id)
        {}
        virtual AddrRangeList getAddrRanges() const;
        PortID Id() { return _id; }

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    class MemPort : public RequestPort
    {
      private:
        WLDirectory* owner;
        bool _blocked;
        PacketPtr blockedPacket;

        public:
        MemPort(const std::string& name, WLDirectory* owner):
            RequestPort(name, owner), owner(owner),
            _blocked(false), blockedPacket(nullptr)
        {}

        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

        protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    RespPort worklistPort;
    RespPort applyPort;
    MemPort memPort;

    std::unordered_map<Addr, PortID> addrResidenceMap;
    std::unordered_map<RequestPtr, PortID> routeBackMap;

    std::queue<PacketPtr> pendingReads;
    std::queue<PacketPtr> pendingReadPorts;

    AddrRangeList getAddrRanges();

    bool handleRequest(PacketPtr pkt);

  public:
    PARAMS(WLDirectory);

    WLDirectory(const WLDirectoryParams &params);

    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};

}

#endif