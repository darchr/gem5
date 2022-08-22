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

#ifndef __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__
#define __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__

#include "accl/graph/base/data_structs.hh"
#include "params/CenteralController.hh"
#include "sim/clocked_object.hh"
#include "sim/system.hh"

namespace gem5
{

class CenteralController : public ClockedObject
{
  private:
    class ReqPort : public RequestPort
    {
      private:
        CenteralController* owner;
        bool _blocked;
        PacketPtr blockedPacket;

      public:
        ReqPort(const std::string& name, CenteralController* owner) :
          RequestPort(name, owner), owner(owner),
          _blocked(false), blockedPacket(nullptr)
        {}

        void sendPacket(PacketPtr pkt);
        bool blocked() { return _blocked; }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    System* system;
    ReqPort reqPort;

    Addr addr;
    uint32_t value;

    template<typename T> PacketPtr
                              createUpdatePacket(Addr addr, T value);

    virtual void initState();
    virtual void startup();

    void functionalAccess(PacketPtr pkt);

  public:
    PARAMS(CenteralController);
    CenteralController(const CenteralControllerParams &params);

    Port& getPort(const std::string &if_name,
                PortID idx=InvalidPortID) override;
};

}

#endif // __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__
