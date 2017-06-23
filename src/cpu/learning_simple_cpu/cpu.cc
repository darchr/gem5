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

#include "cpu/learning_simple_cpu/cpu.hh"

#include "arch/locked_mem.hh"
#include "arch/mmapped_ipr.hh"
#include "arch/utility.hh"
#include "cpu/learning_simple_cpu/exec_context.hh"
#include "debug/LearningSimpleCPU.hh"

LearningSimpleCPU::LearningSimpleCPU(LearningSimpleCPUParams *params) :
    BaseCPU(params),
    port(name()+".port", this),
    thread(this, 0, params->system, params->workload[0], params->itb,
           params->dtb, params->isa[0]),
    currentMacroOp(nullptr),
    traceData(nullptr)
{
    fatal_if(FullSystem, "The LearningSimpleCPU doesn't support full system.");

    // Register this thread with the BaseCPU.
    threadContexts.push_back(thread.getTC());
}

void
LearningSimpleCPU::init()
{
    DPRINTF(LearningSimpleCPU, "LearningSimpleCPU init\n");

    BaseCPU::init();

    thread.getTC()->initMemProxies(thread.getTC());
}

void
LearningSimpleCPU::startup()
{
    DPRINTF(LearningSimpleCPU, "LearningSimpleCPU startup\n");

    BaseCPU::startup();

    thread.startup();
}

void
LearningSimpleCPU::wakeup(ThreadID tid)
{
    assert(tid == 0); // This CPU doesn't support more than one thread!

    // Activate the thread contexts
    if (thread.status() == ThreadContext::Suspended) {
        DPRINTF(LearningSimpleCPU,"[tid:%d] Suspended Processor awoke\n", tid);
        thread.activate();
    }
}

void
LearningSimpleCPU::activateContext(ThreadID tid)
{
    DPRINTF(LearningSimpleCPU, "ActivateContext thread: %d\n", tid);
    BaseCPU::activateContext(tid);

    thread.activate();
    schedule(new EventFunctionWrapper([this]{ fetch(); },
                                      name()+".initial_fetch",
                                      true),
             curTick());
}

void
LearningSimpleCPU::CPUPort::sendPacket(MemoryRequest *request, PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // On a retry, the request may be the same as the outstandingRequest
    assert(outstandingRequest == nullptr);
    outstandingRequest = request;

    DPRINTF(LearningSimpleCPU, "Sending packet %s\n", pkt->print());

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
LearningSimpleCPU::CPUPort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(LearningSimpleCPU, "Got response for addr %#x\n",
             pkt->getAddr());
    assert(outstandingRequest);

    MemoryRequest *request = outstandingRequest;
    outstandingRequest = nullptr;

    return request->recvResponse(pkt);
}

void
LearningSimpleCPU::CPUPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);
    assert(outstandingRequest);

    DPRINTF(LearningSimpleCPU, "Got retry request.\n");

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    MemoryRequest *request = outstandingRequest;
    outstandingRequest = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(request, pkt);
}

void
LearningSimpleCPU::fetch(Addr offset)
{
    // Make sure it's aligned to the closest PCMask value
    // NOTE: offset is a multiple of sizeof(TheISA::MachInst)
    Addr inst_addr = (thread.instAddr() & BaseCPU::PCMask);

    MemoryRequest *request = new MemoryRequest(*this, thread,
                                                inst_addr + offset);
    request->translate();
}

void
LearningSimpleCPU::finishFetchTranslate(MemoryRequest *request)
{
    if (request->getFault() != NoFault) {
        DPRINTF(LearningSimpleCPU, "Translation of addr %#x faulted\n",
                                   request->getReq()->getVaddr());
        delete request;
        panic("Currently LearningSimpleCPU doesn't support fetch faults\n");
        // fetch fault: advance directly to next instruction (fault handler)
        // advanceInst(fault);
    } else {
        request->send();
    }
}

void
LearningSimpleCPU::decodeInstruction(PacketPtr pkt)
{
    DPRINTF(LearningSimpleCPU, "Decoding the instruction\n");
    // First, we need to decode
    thread.decoder.moreBytes(thread.pcState(), pkt->req->getVaddr(),
                      *pkt->getConstPtr<TheISA::MachInst>());
    TheISA::PCState next_pc = thread.pcState();
    StaticInstPtr inst = thread.decoder.decode(next_pc);

    if (!inst) {
        DPRINTF(LearningSimpleCPU, "Need to fetch more data\n");
        schedule(new EventFunctionWrapper(
                        [this, pkt]
                        {
                            // This is a very complicated calculation. It's
                            // possible that you need to make > 2 accesses to
                            // get the whole instruction. We need to compute
                            // the offset based on the previous access, not the
                            // original access.
                            Addr base = (thread.instAddr() & BaseCPU::PCMask);
                            Addr cur_offset = pkt->req->getVaddr() - base;
                            fetch(cur_offset + sizeof(TheISA::MachInst));
                        },
                        name()+".extra_fetch", true),
                 curTick());
        // We'll be back here soon when fetchTranslate and fetchSend finish
        return;
    }

    // Update the PC state.
    thread.pcState(next_pc);

    executeInstruction(inst);
}

void
LearningSimpleCPU::executeInstruction(StaticInstPtr inst)
{
    DPRINTF(LearningSimpleCPU, "Executing the instruction\n");

    // This is for the stupid trace below. It is really picky.
    StaticInstPtr macroop = NULL;
    if (inst->isMacroop()) {
        assert(!currentMacroOp);
        macroop = inst;
        DPRINTF(LearningSimpleCPU, "Decomposing a macro-op\n");
        inst = inst->fetchMicroop(thread.pcState().microPC());
        if (!inst->isLastMicroop()) {
            currentMacroOp = macroop;
        }
    }

    // Initialize the trace
#if TRACING_ON
    traceData = getTracer()->getInstRecord(curTick(), thread.getTC(), inst,
                                           thread.pcState(), macroop);
#endif

    // maintain $r0 semantics
    thread.setIntReg(TheISA::ZeroReg, 0);

    // Create an execution context for this instruction's execution.
    LearningSimpleContext exec_context(*this, thread, inst);

    if (inst->isMemRef()) {
        DPRINTF(LearningSimpleCPU, "Found a memory instruciton!\n");
        // Make the memory reference...
        Fault fault = inst->initiateAcc(&exec_context, traceData);
        panic_if(fault != NoFault, "Don't know how to deal with mem faults!");
    } else {
        DPRINTF(LearningSimpleCPU, "Found a non-memory instruciton!\n");
        // Execute the instruction...
        Fault fault = inst->execute(&exec_context, traceData);
        finishExecute(inst, fault);
    }
}

void
LearningSimpleCPU::finishDataTranslate(MemoryRequest *request)
{
    if (traceData) {
        request->trace(traceData);
    }

    if (request->getFault() != NoFault) {
        DPRINTF(LearningSimpleCPU, "Translation of addr %#x faulted\n",
                                   request->getReq()->getVaddr());
        delete request;
        panic("Currently LearningSimpleCPU doesn't support memory faults");
        return;
    } else if (request->getReq()->getFlags().isSet(Request::NO_ACCESS)) {
        panic("Don't know how to deal with Request::NO_ACCESS");
    } else if (request->getReq()->isMmappedIpr()) {
        Cycles delay;
        // Let the ISA handle the request.
        PacketPtr pkt = request->getPkt();
        if (request->getIsRead()) {
            delay = TheISA::handleIprRead(thread.getTC(), pkt);
        } else {
            delay = TheISA::handleIprWrite(thread.getTC(), pkt);
        }

        // Delay the repsone kind of like we're doing a normal request.
        schedule(new EventFunctionWrapper([this, request, pkt]
                                            { request->recvResponse(pkt); },
                                          name()+".ipr_delay",
                                          true),
                 clockEdge(delay));
        // No need to do anything else, now.
        return;
    } else {
        request->send();
    }
}

void
LearningSimpleCPU::dataResponse(PacketPtr pkt, StaticInstPtr inst)
{
    assert(!pkt->isError());

    LearningSimpleContext exec_context(*this, thread, inst);

    Fault fault = inst->completeAcc(pkt, &exec_context, traceData);

    panic_if(fault != NoFault, "Don't know how to handle this fault!");

    finishExecute(inst, fault);
}

void
LearningSimpleCPU::finishExecute(StaticInstPtr inst, const Fault &fault)
{
    DPRINTF(LearningSimpleCPU, "Finishing execute and moving to next inst\n");

    // Dump trace.
    if (traceData) {
        traceData->dump();
        delete traceData;
        traceData = nullptr;
    }

    if (fault != NoFault) {
        DPRINTF(LearningSimpleCPU, "Instruction faulted! Invoking...\n");
        fault->invoke(thread.getTC(), inst);
        thread.decoder.reset();
    } else {
        // update the thread's PC to the nextPC
        TheISA::PCState pc_state = thread.pcState();
        TheISA::advancePC(pc_state, inst);
        thread.pcState(pc_state);
    }

    if (inst->isMicroop() && currentMacroOp) {
        DPRINTF(LearningSimpleCPU, "Still more of the macro op to execute\n");
        inst = currentMacroOp;
        currentMacroOp = nullptr;
        schedule(new EventFunctionWrapper(
                    [this,inst]{ executeInstruction(inst); },
                    name()+".next_uop", true),
                 nextCycle());
    } else {
        DPRINTF(LearningSimpleCPU, "Fetching a new instruction\n");
        // Schedule an instruction fetch for the next cycle.
        schedule(new EventFunctionWrapper([this]{ fetch(); },
                                          name()+".initial_fetch",
                                          true),
                 nextCycle());
    }
}

LearningSimpleCPU*
LearningSimpleCPUParams::create()
{
    return new LearningSimpleCPU(this);
}
