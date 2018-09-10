/*
 * Copyright (c) 2018 The Regents of The University of California
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
 * Authors: Bradley Wang
 */

#include "cpu/flexcpu/simple_dataflow_thread.hh"

#include <string>

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/SDCPUDeps.hh"
#include "debug/SDCPUInstEvent.hh"
#include "debug/SDCPUThreadEvent.hh"
#include "mem/request.hh"
#include "sim/byteswap.hh"

using namespace std;

// Fullsystem mode constructor
SDCPUThread::SDCPUThread(SimpleDataflowCPU* cpu_, ThreadID tid_,
                    System* system_, BaseTLB* itb_, BaseTLB* dtb_,
                    TheISA::ISA* isa_, bool use_kernel_stats_,
                    unsigned fetch_buf_size, bool strict_ser):
    memIface(*this),
    _cpuPtr(cpu_),
    _name(cpu_->name() + ".thread" + to_string(tid_)),
    strictSerialize(strict_ser),
    _committedState(new SimpleThread(cpu_, tid_, system_, itb_, dtb_, isa_,
                                     use_kernel_stats_)),
    isa(isa_),
    fetchBuf(vector<uint8_t>(fetch_buf_size)),
    fetchBufMask(~(static_cast<Addr>(fetch_buf_size) - 1))
{
    panic_if(fetch_buf_size % sizeof(TheISA::MachInst) != 0,
             "Fetch buffer size should be multiple of instruction size!");
    panic_if(fetch_buf_size > _cpuPtr->cacheLineSize(),
             "Can't send fetch requests larger than cache lines!");
}

// Non-fullsystem constructor
SDCPUThread::SDCPUThread(SimpleDataflowCPU* cpu_, ThreadID tid_,
                    System* system_, Process* process_, BaseTLB* itb_,
                    BaseTLB* dtb_, TheISA::ISA* isa_, unsigned fetch_buf_size,
                    bool strict_ser):
    memIface(*this),
    _cpuPtr(cpu_),
    _name(cpu_->name() + ".thread" + to_string(tid_)),
    strictSerialize(strict_ser),
    _committedState(new SimpleThread(cpu_, tid_, system_, process_, itb_, dtb_,
                                     isa_)),
    isa(isa_),
    fetchBuf(vector<uint8_t>(fetch_buf_size)),
    fetchBufMask(~(static_cast<Addr>(fetch_buf_size) - 1))
{
    panic_if(fetch_buf_size % sizeof(TheISA::MachInst) != 0,
             "Fetch buffer size should be multiple of instruction size!");
    panic_if(fetch_buf_size > _cpuPtr->cacheLineSize(),
             "Can't send fetch requests larger than cache lines!");
}

SDCPUThread::~SDCPUThread()
{
    // TODO Properly hold state since THIS IS SUPER UNSAFE AND WILL RESULT IN
    // MULTIPLE DELETION UPON COPY.
    delete _committedState;
}

void
SDCPUThread::activate()
{
    // Assuming everything else has been set up, and I can immediately request
    // my first instruction via a fetch request.

    _committedState->activate();

    auto callback = [this]{
        advanceInst();
    };

    // Schedule the first call to the entry point of thread runtime. We ensure
    // a clean callstack by sending the call through the scheduler as a
    // separate event.
    _cpuPtr->schedule(new EventFunctionWrapper(callback,
                                               name() + ".activateEvent",
                                               true),
                      curTick());
}

void
SDCPUThread::advanceInst()
{
    if (status() != ThreadContext::Active) {
        // If this thread isn't currently active, don't do anything
        return;
    }

    const InstSeqNum seq_num = lastCommittedInstNum + inflightInsts.size() + 1;

    DPRINTF(SDCPUThreadEvent, "advanceInst(seq %d)\n", seq_num);

    TheISA::PCState next_pc = getNextPC();

    if (isRomMicroPC(next_pc.microPC())) {
        const StaticInstPtr static_inst =
            _cpuPtr->microcodeRom.fetchMicroop(next_pc.microPC(), curMacroOp);

        DPRINTF(SDCPUInstEvent,
                "Decoded ROM microop (seq %d) - %#x : %s\n",
                seq_num,
                next_pc.microPC(),
                static_inst->disassemble(next_pc.microPC()).c_str());

        shared_ptr<InflightInst> dynamic_inst_ptr =
            make_shared<InflightInst>(getThreadContext(), isa, &memIface,
                                      seq_num, next_pc, static_inst);

        inflightInsts.push_back(dynamic_inst_ptr);

        issueInstruction(dynamic_inst_ptr);

        return;
    }

    if (curMacroOp) {
        const StaticInstPtr static_inst =
            curMacroOp->fetchMicroop(next_pc.microPC());

        DPRINTF(SDCPUInstEvent,
                "Decoded microop (seq %d) - %#x : %s\n",
                seq_num,
                next_pc.microPC(),
                static_inst->disassemble(next_pc.microPC()).c_str());

        shared_ptr<InflightInst> dynamic_inst_ptr =
            make_shared<InflightInst>(getThreadContext(), isa, &memIface,
                                      seq_num, next_pc, static_inst);

        inflightInsts.push_back(dynamic_inst_ptr);

        issueInstruction(dynamic_inst_ptr);

        return;
    }

    shared_ptr<InflightInst> dynamic_inst_ptr =
        make_shared<InflightInst>(getThreadContext(), isa, &memIface, seq_num,
                                  next_pc);

    inflightInsts.push_back(dynamic_inst_ptr);

    fetchOffset = 0;
    attemptFetch(weak_ptr<InflightInst>(dynamic_inst_ptr));
}

void
SDCPUThread::attemptFetch(weak_ptr<InflightInst> inst)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    DPRINTF(SDCPUThreadEvent, "attemptFetch()\n");

    // TODO check for interrupts
    // NOTE: You cannot take an interrupt in the middle of a macro-op.
    //      (How would you recover the architectural state since you are in a
    //       non-architectural state during a macro-op?)
    //      Thus, we must check instruction->isDelayedCommit() and make sure
    //      that interrupts only happen after an instruction *without* the
    //      isDelayedCommit flag is committed.
    // TODO something something PCEventQueue?

    TheISA::PCState pc_value = inst_ptr->pcState();

    // Retrieve the value of the speculative PC as a raw Addr value, aligned
    // to MachInst size, but offset by the amount of data we've fetched thus
    // far, since multiple fetches may be needed to feed one decode.
    const Addr pc_addr = pc_value.instAddr();
    const Addr req_vaddr = (pc_addr & BaseCPU::PCMask) + fetchOffset;

    // If HIT in fetch buffer, we can skip sending a new request, and decode
    // from the buffer immediately.
    if (fetchBufBase
     && req_vaddr >= fetchBufBase
     && req_vaddr + sizeof(TheISA::MachInst)
        <= fetchBufBase + fetchBuf.size()) {
        DPRINTF(SDCPUThreadEvent, "Grabbing fetch data for vaddr(%#x) from "
                                  "buffer.\n", req_vaddr);

        const TheISA::MachInst inst_data =
            *reinterpret_cast<TheISA::MachInst*>(fetchBuf.data()
                                               + (req_vaddr - fetchBufBase));

        onInstDataFetched(inst, inst_data);
        return;
    } // Else we need to send a request to update the buffer.

    DPRINTF(SDCPUThreadEvent, "Preparing inst translation request at %#x\n",
                              req_vaddr);

    const RequestPtr fetch_req = std::make_shared<Request>();

    // Set Ids for tracing requests through the system
    fetch_req->taskId(_cpuPtr->taskId());
    fetch_req->setContext(_committedState->contextId()); // May need to store
                                                         // speculative variant
                                                         // if we allow out-of-
                                                         // order execution
                                                         // between context
                                                         // changes?

    const Addr new_buf_base = req_vaddr & fetchBufMask;
    const unsigned req_size = fetchBuf.size();

    fetch_req->setVirt(0, new_buf_base, req_size,
                       Request::INST_FETCH, _cpuPtr->instMasterId(), pc_addr);

    auto callback = [this, inst](Fault f, const RequestPtr& r) {
        onPCTranslated(inst, f, r);
    };

    _cpuPtr->requestInstAddrTranslation(fetch_req, getThreadContext(),
                                        callback);
}

void
SDCPUThread::bufferInstructionData(Addr vaddr, uint8_t* data)
{
    DPRINTF(SDCPUThreadEvent, "Updating fetch buffer with data from %#x\n",
                              vaddr);

    fetchBufBase = vaddr;
    if (!vaddr) return;

    memcpy(fetchBuf.data(), data, fetchBuf.size());
}

void
SDCPUThread::commitAllAvailableInstructions()
{
    shared_ptr<InflightInst> inst_ptr;

    while (!inflightInsts.empty()
        && ((inst_ptr = inflightInsts.front())->isComplete()
            || inst_ptr->isSquashed())) {

        if (inst_ptr->fault() != NoFault) {
            handleFault(inst_ptr);

            return;
        }

        if (!inst_ptr->isSquashed()){
            commitInstruction(inst_ptr);
            inst_ptr->notifyCommitted();
        }

        inflightInsts.pop_front();
    }
}

void
SDCPUThread::commitInstruction(std::shared_ptr<InflightInst> inst_ptr)
{
    assert(inst_ptr->isComplete());

    DPRINTF(SDCPUInstEvent,
            "Committing instruction (seq %d)\n", inst_ptr->seqNum());

    System *system = getSystemPtr();

    // Check for any PC events (e.g., breakpoints)
    Addr pc = instAddr();
    Addr M5_VAR_USED oldpc = pc;
    system->pcEventQueue.service(this);
    assert(oldpc == pc); // We currently don't handle the case where we
                         // shouldn't commit this instruction

    inst_ptr->commitToTC();

    if (!inst_ptr->staticInst()->isMicroop() ||
           inst_ptr->staticInst()->isLastMicroop()) {
        numInstsStat++;
        numInsts++;
        system->totalNumInsts++;
        system->instEventQueue.serviceEvents(system->totalNumInsts);
        getCpuPtr()->comInstEventQueue[threadId()]->serviceEvents(numInsts);
    }
    numOpsStat++;
    numOps++;

    lastCommittedInstNum = inst_ptr->seqNum();

    Trace::InstRecord* const trace_data = inst_ptr->traceData();
    if (trace_data) {
        trace_data->setFetchSeq(lastCommittedInstNum);
        trace_data->setCPSeq(lastCommittedInstNum);
        trace_data->dump();
    }
}

void
SDCPUThread::executeInstruction(weak_ptr<InflightInst> inst)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    panic_if(!inst_ptr->isReady(), "Tried to execute an instruction which was "
                                   "not ready to execute!");
    const StaticInstPtr static_inst = inst_ptr->staticInst();

    DPRINTF(SDCPUInstEvent, "Beginning executing instruction (seq %d)\n",
                            inst_ptr->seqNum());

    function<void(Fault fault)> callback;

    if (static_inst->isMemRef()) {
        callback = [this, inst] (Fault fault) {
                shared_ptr<InflightInst> inst_ptr = inst.lock();
                if (!inst_ptr || inst_ptr->isSquashed()) return;

                if (fault != NoFault) markFault(inst_ptr, fault);
            };
    } else {
        callback = [this, inst](Fault fault) {
                onExecutionCompleted(inst, fault);
            };
    }

    _cpuPtr->requestExecution(static_inst,
                              static_pointer_cast<ExecContext>(inst_ptr),
                              inst_ptr->traceData(), callback);
    // For memrefs, this will result in a corresponding call to either
    // MemIFace::initiateMemRead() or MemIFace::writeMem(), depending on
    // whether the access is a write or read.
}

TheISA::PCState
SDCPUThread::getNextPC()
{
    if (inflightInsts.empty()) {
        const TheISA::PCState ret = _committedState->pcState();
        DPRINTF(SDCPUThreadEvent, "Getting PC %#x from committed state\n",
                                  ret.instAddr());
        return ret;
    } else {
        const shared_ptr<InflightInst>& inst_ptr = inflightInsts.back();

        assert(!inst_ptr->staticInst()->isControl() || inst_ptr->isComplete());

        TheISA::PCState ret = inst_ptr->pcState();
        inst_ptr->staticInst()->advancePC(ret);

        DPRINTF(SDCPUThreadEvent, "Getting PC %#x from inst (seq %d)\n",
                                  ret.instAddr(),
                                  inst_ptr->seqNum());

        return ret;
    }
}

ThreadContext*
SDCPUThread::getThreadContext()
{
    return this;
}

void
SDCPUThread::handleFault(std::shared_ptr<InflightInst> inst_ptr)
{
    DPRINTF(SDCPUThreadEvent, "Handling fault (seq %d): %s\n",
                              inst_ptr->seqNum(), inst_ptr->fault()->name());

    inflightInsts.clear();
    // TODO may need to move any uncommitted InflightInsts somewhere else
    //      to guarantee the memory is still valid until all events
    //      involving those objects are handled.

    inst_ptr->fault()->invoke(this, inst_ptr->staticInst());
    decoder.reset();

    advanceInst();
}

void
SDCPUThread::issueInstruction(weak_ptr<InflightInst> inst)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    DPRINTF(SDCPUInstEvent, "Issuing instruction (seq %d)\n",
                            inst_ptr->seqNum());

#if TRACING_ON
    // Calls new, must delete eventually.
    inst_ptr->traceData(
        _cpuPtr->getTracer()->getInstRecord(curTick(), this,
                inst_ptr->staticInst(), inst_ptr->pcState(), curMacroOp));
#else
    inst_ptr->traceData(nullptr);
#endif

    populateDependencies(inst_ptr);
    populateUses(inst_ptr);

    if (inst_ptr->isReady()) { // If no dependencies, execute now
        executeInstruction(inst);
    } else { // Else, add an event callback to execute when ready
        inst_ptr->addReadyCallback([this, inst](){
                executeInstruction(inst);
        });
    }

    if (inst_ptr->staticInst()->isLastMicroop()) {
        DPRINTF(SDCPUThreadEvent, "Releasing MacroOp after decoding final "
                                  "microop\n");
        curMacroOp = StaticInst::nullStaticInstPtr;
    }

    if (hasNextPC()) {
        // If we are still running and know where to fetch, we should
        // immediately send the necessary requests to the CPU to fetch the next
        // instruction
        advanceInst();
    }
}

void
SDCPUThread::markFault(shared_ptr<InflightInst> inst_ptr, Fault fault)
{
    DPRINTF(SDCPUThreadEvent, "Fault detected (seq %d): %s\n",
                              inst_ptr->seqNum(), fault->name());

    inst_ptr->fault(fault);

    while (inflightInsts.back() != inst_ptr) {
        inflightInsts.back()->squash();
        inflightInsts.pop_back();
    }

    inst_ptr->squash();

    commitAllAvailableInstructions();
}

void
SDCPUThread::onDataAddrTranslated(weak_ptr<InflightInst> inst, Fault fault,
                                  const RequestPtr& req, bool write,
                                  uint8_t* data,
                                  shared_ptr<SplitRequest> sreq, bool high)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    DPRINTF(SDCPUThreadEvent, "onDataAddrTranslated()\n");

    if (fault != NoFault) {
        markFault(inst_ptr, fault);

        return;
    }

    // TODO set the effective physical address for the inflightInst
    // TODO add dependencies according to the now known effective address, and
    //      unset this instruction ready if appropriate.

    auto callback = [this, inst](Fault fault) {
            onExecutionCompleted(inst, fault);
        };

    // IF we have a split request
    if (sreq) {
        if (high) {
            sreq->high = req;
        } else {
            sreq->low = req;
        }

        // IF both requests have been translated successfully
        if (sreq->high && sreq->low) {
            if (write) {
                if (!_cpuPtr->requestSplitMemWrite(sreq->main, sreq->low,
                        sreq->high, getThreadContext(), inst_ptr->staticInst(),
                        static_pointer_cast<ExecContext>(inst_ptr),
                        inst_ptr->traceData(), data,
                        callback)) {
                    panic("The CPU rejected my mem access request and I "
                          "haven't been programmed to know how to "
                          "continue!!!");
                }
            } else { // read
                if (!_cpuPtr->requestSplitMemRead(sreq->main, sreq->low,
                        sreq->high, getThreadContext(), inst_ptr->staticInst(),
                        static_pointer_cast<ExecContext>(inst_ptr),
                        inst_ptr->traceData(),
                        callback)) {
                    panic("The CPU rejected my mem access request and I "
                          "haven't been programmed to know how to "
                          "continue!!!");
                }
            }
        }

        return; // finished handling split request
    }

    if (write) {
        if (!_cpuPtr->requestMemWrite(req, getThreadContext(),
                inst_ptr->staticInst(),
                static_pointer_cast<ExecContext>(inst_ptr),
                inst_ptr->traceData(), data, callback)) {
            panic("The CPU rejected my mem access request and I haven't been "
                  "programmed to know how to continue!!!");
        }
    } else { // read
        if (!_cpuPtr->requestMemRead(req, getThreadContext(),
                inst_ptr->staticInst(),
                static_pointer_cast<ExecContext>(inst_ptr),
                inst_ptr->traceData(), callback)) {
            panic("The CPU rejected my mem access request and I haven't been "
                  "programmed to know how to continue!!!");
        }
    }
}

void
SDCPUThread::onExecutionCompleted(weak_ptr<InflightInst> inst, Fault fault)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    if (fault != NoFault) {
        markFault(inst_ptr, fault);

        return;
    }

    // if (!inst->readPredicate()) // TODO ensure this is enforced.
    //     inst->forwardOldRegs();

    DPRINTF(SDCPUThreadEvent, "onExecutionCompleted(seq %d)\n",
                              inst_ptr->seqNum());

    DPRINTF(SDCPUInstEvent, "Marking instruction as complete (seq %d)\n",
                            inst_ptr->seqNum());

    // The InflightInst will match any of its dependencies automatically,
    // and perform ready callbacks if any instructions are made ready as a
    // result of this instruction completed
    inst_ptr->notifyComplete();

    if (inst_ptr->staticInst()->isControl() && hasNextPC()) {
        // If we are a control instruction, then we were unable to begin a
        // fetch during decode, so we should start one now.
        advanceInst();
    }

    commitAllAvailableInstructions();
}

void
SDCPUThread::onInstDataFetched(weak_ptr<InflightInst> inst,
                               const TheISA::MachInst fetch_data)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    // TODO store in fetch buffer, do things with it.

    DPRINTF(SDCPUThreadEvent, "onInstDataFetched(%#x)\n",
                              TheISA::gtoh(fetch_data));

    // Capure the PC state at the point of instruction retrieval.
    TheISA::PCState pc = inst_ptr->pcState();
    Addr fetched_addr = (pc.instAddr() & BaseCPU::PCMask) + fetchOffset;

    decoder.moreBytes(pc, fetched_addr, TheISA::gtoh(fetch_data));
    StaticInstPtr decode_result = decoder.decode(pc);

    inst_ptr->pcState(pc);


    if (decode_result) { // If a complete instruction was decoded
        DPRINTF(SDCPUInstEvent,
                "Decoded instruction (seq %d) - %#x : %s\n",
                inst_ptr->seqNum(),
                pc.instAddr(),
                decode_result->disassemble(pc.instAddr()).c_str());

        if (decode_result->isMacroop()) {
            DPRINTF(SDCPUThreadEvent, "Detected MacroOp, capturing...\n");
            curMacroOp = decode_result;

            decode_result = curMacroOp->fetchMicroop(pc.microPC());

            DPRINTF(SDCPUInstEvent,
                    "Replaced with microop (seq %d) - %#x : %s\n",
                    inst_ptr->seqNum(),
                    pc.microPC(),
                    decode_result->disassemble(pc.microPC()).c_str());
        }

        inst_ptr->staticInst(decode_result);

        issueInstruction(inst);

    } else { // If we still need to fetch more MachInsts.
        fetchOffset += sizeof(TheISA::MachInst);
        attemptFetch(inst);
    }
}

void
SDCPUThread::onPCTranslated(weak_ptr<InflightInst> inst, Fault fault,
                            const RequestPtr& req)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    const TheISA::PCState pc = inst_ptr->pcState();

    DPRINTF(SDCPUThreadEvent, "onPCTranslated(pc: %#x)\n",
                              pc.instAddr());


    if (fault != NoFault) {
        markFault(inst_ptr, fault);

        return;
    }

    const Addr vaddr = req->getVaddr();

    DPRINTF(SDCPUThreadEvent, "Received PC translation (va: %#x, pa: %#x)\n",
            vaddr, req->getPaddr());

    auto callback = [this, inst, vaddr](uint8_t* data) {
        bufferInstructionData(vaddr, data);
        attemptFetch(inst);
    };

    // No changes to the request before asking the CPU to handle it if there is
    // not a fault. Let the CPU generate the packet.
    if (!_cpuPtr->requestInstructionData(req, callback)) {
        panic("The CPU rejected my fetch request and I haven't been programmed"
              " to know how to continue!!!");
    }
}

void
SDCPUThread::populateDependencies(shared_ptr<InflightInst> inst_ptr)
{
    // NOTE: instead of implementing dependency detection all in this one
    //       function, maybe it's better to define a dependency provider
    //       interface, to make these rules swappable?

    // Temporary virtual dependency rule to simulate in-order CPU mode.
    // if (inflightInsts.size() > 1) {
    //     const shared_ptr<InflightInst> last_inst =
    //                                             *(++inflightInsts.rbegin());

    //     if (!(last_inst->isComplete() || last_inst->isSquashed()))
    //         inst_ptr->addDependency(last_inst);
    // }

    /*
     * Note: An assumption is being made that inst_ptr is at the end of the
     *       inflightInsts buffer.
     */

    const StaticInstPtr static_inst = inst_ptr->staticInst();

    // BEGIN ISA explicit serialization

    if (strictSerialize) {
        if (inflightInsts.size() > 1 && static_inst->isSerializing()) {
            const shared_ptr<InflightInst> last_inst =
                *(++inflightInsts.rbegin());

            // TODO don't add the dependency when last is squashed, but find
            //      newest instruction that isn't squashed
            if (!(last_inst->isCommitted() || last_inst->isSquashed())) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                        inst_ptr->seqNum(), last_inst->seqNum());
                inst_ptr->addCommitDependency(last_inst);
            }

            return;
        }

        for (auto& prior : inflightInsts) {
            if (prior == inst_ptr || prior->isCommitted()
             || prior->isSquashed()) {
                continue;
            }

            if (prior->staticInst()->isSerializing()) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                        inst_ptr->seqNum(), prior->seqNum());
                inst_ptr->addCommitDependency(prior);
            }
        }
    } else {
        if (inflightInsts.size() > 1 && static_inst->isSerializeBefore()) {
            const shared_ptr<InflightInst> last_inst =
                *(++inflightInsts.rbegin());

            if (!(last_inst->isCommitted() || last_inst->isSquashed())) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                        inst_ptr->seqNum(), last_inst->seqNum());
                inst_ptr->addCommitDependency(last_inst);
            }

            return;
        }

        for (auto& prior : inflightInsts) {
            if (prior == inst_ptr || prior->isCommitted()
             || prior->isSquashed()) {
                continue;
            }

            if (prior->staticInst()->isSerializeAfter()) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                        inst_ptr->seqNum(), prior->seqNum());
                inst_ptr->addCommitDependency(prior);
            }
        }
    }

    // END ISA explicit serialization

    // BEGIN Sequential Consistency

    // TODO IMPORTANT this still doesn't make it safe to speculate on memory
    //      instructions, since if we produce a fault and need to squash, we
    //      still need a way to inform the cpu to undo stores being sent to
    //      memory.

    // By creating a dependency between any memory requests and the most recent
    // prior memory request, all memory instructions are enforced to execute in
    // program-order.
    if (static_inst->isMemRef() || static_inst->isMemBarrier()) {
        for (auto itr = inflightInsts.rbegin();
             itr != inflightInsts.rend();
             ++itr) {
            if (*itr == inst_ptr) continue;
            if ((*itr)->isComplete() || ((*itr)->isSquashed())) continue;

            const StaticInstPtr other_si = (*itr)->staticInst();

            if (other_si->isMemRef() || other_si->isMemBarrier()) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [mem barrier]\n",
                        inst_ptr->seqNum(), (*itr)->seqNum());
                inst_ptr->addDependency(*itr);
                break;
            }
        }
    }

    // END Sequential Consistency

    // BEGIN Only store if instruction is at head of inflightInsts

    if (inflightInsts.size() > 1 && static_inst->isStore()) {
        const shared_ptr<InflightInst> last_inst = *(++inflightInsts.rbegin());

        if (!(last_inst->isCommitted() || last_inst->isSquashed()))
            inst_ptr->addCommitDependency(last_inst);

        return;
    }

    // END Only store if instruction is at head of inflightInsts

    // BEGIN Data dependencies through registers

    const int8_t num_srcs = static_inst->numSrcRegs();
    for (int8_t src_idx = 0; src_idx < num_srcs; ++src_idx) {
        const RegId& src_reg = static_inst->srcRegIdx(src_idx);
        if (src_reg.isZeroReg()) continue;

        const InflightInst::DataSource& last_use = lastUses[src_reg];
        const shared_ptr<InflightInst> producer = last_use.producer.lock();

        // Attach the dependency if the instruction is still around
        if (producer && !(producer->isComplete() || producer->isSquashed())) {
            DPRINTF(SDCPUDeps, "Dep %d -> %d [data (reg[%d])]\n",
                    inst_ptr->seqNum(), producer->seqNum(),
                    src_reg.index());
            inst_ptr->addDependency(producer);
        }

        inst_ptr->setDataSource(src_idx, last_use);
    }

    // END Data dependencies through registers
}

void
SDCPUThread::populateUses(shared_ptr<InflightInst> inst_ptr) {
    const StaticInstPtr static_inst = inst_ptr->staticInst();
    const int8_t num_dsts = static_inst->numDestRegs();
    for (int8_t dst_idx = 0; dst_idx < num_dsts; ++dst_idx) {
        const RegId& dst_reg = static_inst->destRegIdx(dst_idx);

        InflightInst::DataSource source = {inst_ptr, dst_idx};
        lastUses[dst_reg] = source;
        DPRINTF(SDCPUDeps, "%d is producer of reg[%d]\n", inst_ptr->seqNum(),
                dst_reg.index());
    }
}

bool
SDCPUThread::hasNextPC()
{
    // TODO check if we're still running

    if (inflightInsts.empty()) return true;

    const shared_ptr<InflightInst> inst_ptr = inflightInsts.back();
    return !inst_ptr->staticInst()->isControl() || inst_ptr->isComplete();
}

void
SDCPUThread::startup()
{
    isa->startup(getThreadContext());
    // TODO other things?
}

Fault
SDCPUThread::MemIface::initiateMemRead(shared_ptr<InflightInst> inst_ptr,
                                       Addr addr, unsigned int size,
                                       Request::Flags flags)
{
    const int asid = 0;
    const Addr pc = 0; // TODO extract this from inflightinst, but otherwise
                       //      only used for tracing/debugging.
    const MasterID mid = sdCPUThread._cpuPtr->dataMasterId();
    ThreadContext* const tc = sdCPUThread.getThreadContext();
    const ContextID cid = tc->contextId();

    RequestPtr req = make_shared<Request>(asid, addr, size, flags, mid, pc,
                                          cid);

    // For tracing?
    req->taskId(sdCPUThread._cpuPtr->taskId());

    const unsigned cache_line_size = sdCPUThread._cpuPtr->cacheLineSize();
    const Addr split_addr = roundDown(addr + size - 1, cache_line_size);

    weak_ptr<InflightInst> inst = inst_ptr;

    if (split_addr > addr) { // The request spans two cache lines
        assert(!(req->isLLSC() || req->isMmappedIpr())); // don't deal with
                                                         // weird cases for now

        RequestPtr req1, req2;

        req->splitOnVaddr(split_addr, req1, req2);

        shared_ptr<SplitRequest> split_acc = make_shared<SplitRequest>();
        split_acc->main = req;


        auto callback1 = [this, inst, split_acc](Fault fault,
                                                 const RequestPtr& req) {
            sdCPUThread.onDataAddrTranslated(inst, fault, req, false,
                                             nullptr, split_acc, false);
        };
        auto callback2 = [this, inst, split_acc](Fault fault,
                                                 const RequestPtr& req) {
            sdCPUThread.onDataAddrTranslated(inst, fault, req, false,
                                             nullptr, split_acc, true);
        };

        sdCPUThread._cpuPtr->requestDataAddrTranslation(req1, tc, false,
                                                        callback1);

        sdCPUThread._cpuPtr->requestDataAddrTranslation(req2, tc, false,
                                                        callback2);

        return NoFault;
    }


    auto callback = [this, inst](Fault fault, const RequestPtr& req) {
        sdCPUThread.onDataAddrTranslated(inst, fault, req, false,
                                         nullptr);
    };

    sdCPUThread._cpuPtr->requestDataAddrTranslation(req, tc, false, callback);

    return NoFault;
}

Fault
SDCPUThread::MemIface::writeMem(shared_ptr<InflightInst> inst_ptr,
                                uint8_t *data,
                                unsigned int size, Addr addr,
                                Request::Flags flags, uint64_t *res)
{
    const int asid = 0;
    const Addr pc = 0; // TODO extract this from inflightinst, but otherwise
                       //      only used for tracing/debugging.
    const MasterID mid = sdCPUThread._cpuPtr->dataMasterId();
    ThreadContext* const tc = sdCPUThread.getThreadContext();
    const ContextID cid = tc->contextId();

    RequestPtr req = make_shared<Request>(asid, addr, size, flags, mid, pc,
                                          cid);

    // For tracing?
    req->taskId(sdCPUThread._cpuPtr->taskId());

    const unsigned cache_line_size = sdCPUThread._cpuPtr->cacheLineSize();
    const Addr split_addr = roundDown(addr + size - 1, cache_line_size);

    weak_ptr<InflightInst> inst = inst_ptr;

    // Have to do this, because I can't guarantee that data will still be a
    // valid pointer to the information after writeMem returns. The only other
    // way to do this is to create the packet early, but that would require
    // deleting that packet anyway in more cases, if the translation fails.
    // Note: size + 1 for an extra slot for ref-counting.
    uint8_t* copy = new uint8_t[size + 1];
    memcpy(copy, data, size);

    if (split_addr > addr) { // The request spans two cache lines
        assert(!(req->isLLSC() || req->isMmappedIpr())); // don't deal with
                                                         // weird cases for now

        RequestPtr req1, req2;

        req->splitOnVaddr(split_addr, req1, req2);

        shared_ptr<SplitRequest> split_acc = make_shared<SplitRequest>();
        split_acc->main = req;

        copy[size] = 2;

        auto callback1 = [this, inst, split_acc, copy, size](Fault fault,
            const RequestPtr& req) {
            sdCPUThread.onDataAddrTranslated(inst, fault, req, true,
                                             copy, split_acc, false);
            --copy[size];
            if (!copy[size]) delete [] copy;
        };

        auto callback2 = [this, inst, split_acc, copy, size](Fault fault,
                                                 const RequestPtr& req) {
            sdCPUThread.onDataAddrTranslated(inst, fault, req, true,
                                             copy, split_acc, true);
            --copy[size];
            if (!copy[size]) delete [] copy;
        };


        sdCPUThread._cpuPtr->requestDataAddrTranslation(req1, tc, true,
                                                        callback1);

        sdCPUThread._cpuPtr->requestDataAddrTranslation(req2, tc, true,
                                                        callback2);

        return NoFault;
    }

    // As far as I can tell, this is the only case that @param res is used
    // anywhere in CPU implementations, so I may as well read the value early
    // and store it in the request as it makes its way through the system. Also
    // TimingSimpleCPU's implementation of this seems to imply a request can't
    // be split and a condswap at the same time.
    if (req->isCondSwap()) {
        assert(res);
        req->setExtraData(*res);
    }

    auto callback = [this, inst, copy](Fault fault,
                                            const RequestPtr& req) {
        sdCPUThread.onDataAddrTranslated(inst, fault, req, true, copy);
        delete [] copy;
    };

    sdCPUThread._cpuPtr->requestDataAddrTranslation(req, tc, true, callback);

    return NoFault;
}

void
SDCPUThread::regStats(const std::string &name)
{
    _committedState->regStats(name);

    numInstsStat
        .name(name + ".numInsts")
        .desc("Total number of instructions committed")
        ;

    numOpsStat
        .name(name + ".numOps")
        .desc("Total number of micro-ops committed")
        ;
}
