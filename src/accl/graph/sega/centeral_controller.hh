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

#include <vector>

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/base/graph_workload.hh"
#include "accl/graph/sega/enums.hh"
#include "accl/graph/sega/mpu.hh"
#include "base/addr_range.hh"
#include "base/intmath.hh"
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
        PacketPtr blockedPacket;
        PortID _id;

      public:
        ReqPort(const std::string& name, CenteralController* owner, PortID id):
          RequestPort(name, owner),
          owner(owner), blockedPacket(nullptr), _id(id)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return (blockedPacket != nullptr); }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    struct PointerTag : public Packet::SenderState
    {
        int _id;
        PointerType _type;
        PointerTag(int id, PointerType type): _id(id), _type(type) {}
        int Id() { return _id; }
        PointerType type() { return _type; }

    };

    class MirrorReadInfoGen {
      private:
        Addr _start;
        Addr _end;
        size_t _step;
        size_t _atom;

      public:
        MirrorReadInfoGen(Addr start, Addr end, size_t step, size_t atom):
                        _start(start), _end(end), _step(step), _atom(atom)
        {}

        std::tuple<Addr, Addr, int> nextReadPacketInfo()
        {
            panic_if(done(), "Should not call nextPacketInfo when done.\n");
            Addr aligned_addr = roundDown<Addr, Addr>(_start, _atom);
            Addr offset = _start - aligned_addr;
            int num_items = 0;

            if (_end > (aligned_addr + _atom)) {
                num_items = (_atom - offset) / _step;
            } else {
                num_items = (_end - _start) / _step;
            }

            return std::make_tuple(aligned_addr, offset, num_items);
        }

        void iterate()
        {
            panic_if(done(), "Should not call iterate when done.\n");
            Addr aligned_addr = roundDown<Addr, Addr>(_start, _atom);
            _start = aligned_addr + _atom;
        }

        bool done() { return (_start >= _end); }
    };

    System* system;

    ReqPort mirrorsPort;
    ReqPort mapPort;

    Addr maxVertexAddr;

    ProcessingMode mode;

    std::vector<MPU*> mpuVector;
    std::unordered_map<MPU*, AddrRangeList> addrRangeListMap;

    // FIXME: Initialize these two.
    int currentSliceNumber;
    int totalSliceNumber;
    int lastReadPacketId;
    std::unordered_map<int, Addr> startAddrs;
    std::unordered_map<int, Addr> endAddrs;
    // TODO: Set a max size for this queue;
    std::deque<MirrorReadInfoGen> mirrorPointerQueue;
    std::unordered_map<RequestPtr, std::tuple<Addr, int>> reqInfoMap;

    std::deque<MirrorVertex> mirrorQueue;
    std::deque<PacketPtr> writeBackQueue;

    PacketPtr createReadPacket(Addr addr, unsigned int size);

    bool handleMemResp(PacketPtr pkt, PortID id);
    void recvReqRetry(PortID id);

    EventFunctionWrapper nextMirrorMapReadEvent;
    void processNextMirrorMapReadEvent();

    EventFunctionWrapper nextMirrorReadEvent;
    void processNextMirrorReadEvent();

    EventFunctionWrapper nextMirrorUpdateEvent;
    void processNextMirrorUpdateEvent();

    EventFunctionWrapper nextWriteBackEvent;
    void processNextWriteBackEvent();

  public:
    GraphWorkload* workload;

    PARAMS(CenteralController);
    CenteralController(const CenteralControllerParams &params);
    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;

    virtual void startup() override;

    void setAsyncMode() { mode = ProcessingMode::ASYNCHRONOUS; }
    void setBSPMode() { mode = ProcessingMode::BULK_SYNCHRONOUS; }
    void setPGMode() { mode = ProcessingMode::POLY_GRAPH; }

    void createPopCountDirectory(int atoms_per_block);

    void createBFSWorkload(Addr init_addr, uint32_t init_value);
    void createBFSVisitedWorkload(Addr init_addr, uint32_t init_value);
    void createSSSPWorkload(Addr init_addr, uint32_t init_value);
    void createCCWorkload();
    void createAsyncPRWorkload(float alpha, float threshold);
    void createPRWorkload(int num_nodes, float alpha);
    void createBCWorkload(Addr init_addr, uint32_t init_value);

    void recvDoneSignal();

    int workCount();
    float getPRError();
    void printAnswerToHostSimout();
};

}

#endif // __ACCL_GRAPH_SEGA_CENTERAL_CONTROLLER_HH__
