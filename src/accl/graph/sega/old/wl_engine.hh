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

#ifndef __ACCL_GRAPH_SEGA_WL_ENGINE_HH__
#define __ACCL_GRAPH_SEGA_WL_ENGINE_HH__

#include <queue>
#include <unordered_map>

#include "accl/graph/base/base_wl_engine.hh"
#include "accl/graph/sega/apply_engine.hh"
#include "accl/graph/sega/lock_dir.hh"
#include "params/WLEngine.hh"

namespace gem5
{

class ApplyEngine;

class WLEngine : public BaseWLEngine
{
  private:
    class RespPort : public ResponsePort
    {
      private:
        WLEngine* owner;

      public:
        RespPort(const std::string& name, WLEngine* owner):
          ResponsePort(name, owner), owner(owner)
        {}
        virtual AddrRangeList getAddrRanges() const;

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    RespPort respPort;
    ApplyEngine* applyEngine;
    LockDirectory* lockDir;

    virtual void startup();
    void recvFunctional(PacketPtr pkt);

  protected:
    virtual bool sendWLNotif(Addr addr) override;
    virtual bool acquireAddress(Addr addr) override;
    virtual bool releaseAddress(Addr addr) override;

  public:
    PARAMS(WLEngine);
    WLEngine(const WLEngineParams &params);
    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
};

}
#endif // __ACCL_GRAPH_SEGA_WL_ENGINE_HH__
