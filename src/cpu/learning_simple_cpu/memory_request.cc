/*
 * Copyright (c) 2017 Jason Lowe-Power
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
 *
 * Authors: Jason Lowe-Power
 */

#include "cpu/learning_simple_cpu/memory_request.hh"

#include "arch/locked_mem.hh"
#include "arch/mmapped_ipr.hh"
#include "arch/utility.hh"
#include "cpu/learning_simple_cpu/cpu.hh"
#include "debug/LearningSimpleCPU.hh"

MemoryRequest::MemoryRequest(LearningSimpleCPU &cpu, SimpleThread &thread,
                         StaticInstPtr inst, Addr addr, unsigned int size,
                         Request::Flags flags, uint8_t *_data, uint64_t *res)
    : cpu(cpu), thread(thread), inst(inst), data(nullptr), res(res),
      isFetch(false), isRead(_data ? false : true), isSplit(false),
      req(nullptr), pkt(nullptr), fault(NoFault)
{
    req = new Request(0 /* asid */, addr, size, flags, cpu.dataMasterId(),
                          thread.instAddr(), thread.contextId());

    // Check to see if access crosses cache line boundary.
    if (((addr + size - 1) / cpu.cacheLineSize()) >
              (addr / cpu.cacheLineSize())) {
      panic("CPU can't deal with accesses across a cache line boundary.");
    }

    if (!isRead) {
        // Copy the data into this object.
        data = new uint8_t[size];
        memcpy(data, _data, size);
    }
}

MemoryRequest::MemoryRequest(LearningSimpleCPU &cpu, SimpleThread &thread,
                             Addr inst_addr)
    : cpu(cpu), thread(thread), inst(StaticInst::nullStaticInstPtr),
      data(nullptr), res(nullptr),
      isFetch(true), isRead(false), isSplit(false),
      req(nullptr), pkt(nullptr), fault(NoFault)
{
    req =
        new Request(0 /* asid */, inst_addr, sizeof(TheISA::MachInst),
            Request::INST_FETCH, cpu.instMasterId(), thread.instAddr(),
            thread.contextId());

    // Double check to see if access crosses cache line boundary.
    // This should be impossible.
    if (((inst_addr + sizeof(TheISA::MachInst) - 1) / cpu.cacheLineSize()) >
            (inst_addr / cpu.cacheLineSize())) {
        panic("CPU can't deal with fetches across a cache line boundary.");
    }
}

MemoryRequest::~MemoryRequest()
{
    delete req;
    // If this faults before the translation finishes then there will be no pkt
    if (pkt) delete pkt;
}

void
MemoryRequest::translate()
{
    if (isFetch) translateFetch();
    else translateData();
}

void
MemoryRequest::translateFetch()
{
    DPRINTFS(LearningSimpleCPU, (&cpu), "Fetching addr %#x (+%#x)\n",
                               thread.instAddr(),
                               req->getVaddr() - thread.instAddr());

    thread.itb->translateTiming(req, thread.getTC(), this,
                                BaseTLB::Execute);
}

void
MemoryRequest::translateData()
{
    DPRINTFS(LearningSimpleCPU, (&cpu), "%s addr %#x (size: %d)\n",
             isRead ? "Read" : "Write", req->getVaddr(), req->getSize());

    thread.dtb->translateTiming(req, thread.getTC(), this,
                                isRead ? BaseTLB::Read : BaseTLB::Write);
}

void
MemoryRequest::send()
{
    if (isFetch) sendFetch();
    else sendData();
}

void
MemoryRequest::sendFetch()
{
    assert(isFetch);
    DPRINTFS(LearningSimpleCPU, (&cpu), "Sending fetch. Addr %#x(pa: %#x)\n",
            req->getVaddr(), req->getPaddr());

    pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    LearningSimpleCPU::CPUPort *port =
        dynamic_cast<LearningSimpleCPU::CPUPort*>(&cpu.getInstPort());
    assert(port);
    port->sendPacket(this, pkt);
}

void
MemoryRequest::sendData()
{
    assert(!isFetch);
    bool do_access = true; // For supressing access on LLSCs
    if (isRead) {
        DPRINTFS(LearningSimpleCPU, (&cpu), "Sending read for %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());
        pkt = Packet::createRead(req);
        pkt->allocate();

        // For LLSC, we have to register this read with the ISA to mark the
        // address as locked.
        if (req->isLLSC()) {
            TheISA::handleLockedRead(&thread, req);
        }
    } else {
        DPRINTFS(LearningSimpleCPU, (&cpu), "Sending write for %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());

        if (req->isLLSC()) {
            do_access = TheISA::handleLockedWrite(&thread, req,
                                        ~((Addr)cpu.cacheLineSize() - 1));
            DPRINTFS(LearningSimpleCPU, (&cpu), "Doing LLSC. %s access\n",
                        do_access ? "Should" : "Should NOT");
        } else if (req->isCondSwap()) {
            assert(res);
            req->setExtraData(*res);
        }

        pkt = Packet::createWrite(req);
        // Assume we have data if it's a write.
        assert(data);
        pkt->dataDynamic<uint8_t>(data);
    }

    if (do_access) {
        LearningSimpleCPU::CPUPort *port =
            dynamic_cast<LearningSimpleCPU::CPUPort*>(&cpu.getDataPort());
        assert(port);
        port->sendPacket(this, pkt);
    } else {
        cpu.dataResponse(pkt, inst);
    }
}

void
MemoryRequest::finish(const Fault &_fault, RequestPtr req,
                      ThreadContext *tc, BaseTLB::Mode mode)
{
    fault = _fault;
    if (isFetch) cpu.finishFetchTranslate(this);
    else cpu.finishDataTranslate(this);
}

bool
MemoryRequest::recvResponse(PacketPtr pkt)
{
    if (isFetch) cpu.decodeInstruction(pkt);
    else cpu.dataResponse(pkt, inst);

    delete this;
    return true;
}

void
MemoryRequest::trace(Trace::InstRecord *trace)
{
    trace->setMem(req->getVaddr(), req->getSize(), req->getFlags());
}
