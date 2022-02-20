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

#ifndef __ACCL_APPLY_HH__
#define __ACCL_APPLY_HH__

#include <queue>
#include <unordered_map>

#include "mem/packet.hh"
#include "mem/port.hh"
#include "params/BaseApplyEngine.hh"
#include "sim/clocked_object.hh"
#include "sim/port.hh"

namespace gem5
{

class BaseApplyEngine : public ClockedObject
{
  private:
    //FIXME: Remove queue defenition from here.
    struct ApplyQueue{
        std::queue<PacketPtr> applyQueue;
        const uint32_t queueSize;
        bool sendPktRetry;

        bool blocked(){
            return (applyQueue.size() == queueSize);
        }
        bool empty(){
            return applyQueue.empty();
        }
        void push(PacketPtr pkt){
            applyQueue.push(pkt);
        }

        void pop(){
            applyQueue.pop();
        }

        PacketPtr front(){
            return applyQueue.front();
        }

        ApplyQueue(uint32_t qSize):
          queueSize(qSize)
        {}
    };

    const RequestorID requestorId;

    ApplyQueue applyReadQueue;
    ApplyQueue applyWriteQueue;

    std::unordered_map<RequestPtr, int> requestOffset;

    bool handleWL(PacketPtr pkt);
    EventFunctionWrapper nextApplyCheckEvent;
    void processNextApplyCheckEvent();
    //FIXME: make void
    bool handleMemResp(PacketPtr resp);
    EventFunctionWrapper nextApplyEvent;
    void processNextApplyEvent();

  protected:
    virtual void sendMemReq(PacketPtr pkt) = 0;

  public:
    BaseApplyEngine(const ApplyParams &apply);

    Port& getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    RequestorID getRequestorId();
    void setRequestorId(RequestorId requestorId);
};

}

#endif // __ACCL_APPLY_HH__
