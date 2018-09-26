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

#include <algorithm>
#include <memory>
#include <string>

#include "base/intmath.hh"
#include "base/trace.hh"
#include "debug/SDCPUBranchPred.hh"
#include "debug/SDCPUBufferDump.hh"
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
                    unsigned branch_pred_max_depth,
                    unsigned fetch_buf_size, bool strict_ser):
    memIface(*this),
    _cpuPtr(cpu_),
    _name(cpu_->name() + ".thread" + to_string(tid_)),
    strictSerialize(strict_ser),
    _committedState(new SimpleThread(cpu_, tid_, system_, itb_, dtb_, isa_,
                                     use_kernel_stats_)),
    isa(isa_),
    fetchBuf(vector<uint8_t>(fetch_buf_size)),
    fetchBufMask(~(static_cast<Addr>(fetch_buf_size) - 1)),
    remainingBranchPredDepth(branch_pred_max_depth ?
                             branch_pred_max_depth : -1)
{
    panic_if(fetch_buf_size % sizeof(TheISA::MachInst) != 0,
             "Fetch buffer size should be multiple of instruction size!");
    panic_if(fetch_buf_size > _cpuPtr->cacheLineSize(),
             "Can't send fetch requests larger than cache lines!");
}

// Non-fullsystem constructor
SDCPUThread::SDCPUThread(SimpleDataflowCPU* cpu_, ThreadID tid_,
                    System* system_, Process* process_, BaseTLB* itb_,
                    BaseTLB* dtb_, TheISA::ISA* isa_,
                    unsigned branch_pred_max_depth,
                    unsigned fetch_buf_size, bool strict_ser):
    memIface(*this),
    _cpuPtr(cpu_),
    _name(cpu_->name() + ".thread" + to_string(tid_)),
    strictSerialize(strict_ser),
    _committedState(new SimpleThread(cpu_, tid_, system_, process_, itb_, dtb_,
                                     isa_)),
    isa(isa_),
    fetchBuf(vector<uint8_t>(fetch_buf_size)),
    fetchBufMask(~(static_cast<Addr>(fetch_buf_size) - 1)),
    remainingBranchPredDepth(branch_pred_max_depth ?
                             branch_pred_max_depth : -1)
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
        advanceInst(getNextPC());
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
SDCPUThread::advanceInst(TheISA::PCState next_pc)
{
    if (status() != ThreadContext::Active) {
        // If this thread isn't currently active, don't do anything
        return;
    }

    fetchedThisCycle++;

    const InstSeqNum seq_num = lastCommittedInstNum + inflightInsts.size() + 1;

    DPRINTF(SDCPUThreadEvent, "advanceInst(seq %d, pc %#x, upc %#x)\n",
                              seq_num,
                              next_pc.pc(),
                              next_pc.upc());
    DPRINTF(SDCPUThreadEvent, "Buffer size increased to %d\n",
                              inflightInsts.size());

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
        callback, [inst]{
            shared_ptr<InflightInst> inst_ptr = inst.lock();
            return !inst_ptr || inst_ptr->isSquashed();
        });
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
    DPRINTF(SDCPUThreadEvent, "Committing complete instructions from head of "
                              "buffer(%d).\n", inflightInsts.size());

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

    if (inflightInsts.empty()) {
        DPRINTF(SDCPUThreadEvent, "Buffer has completely emptied.\n");
    } else {
        DPRINTF(SDCPUThreadEvent,
            "%d commits stopped by (seq %d): %s\n",
            inflightInsts.size(),
            inst_ptr->seqNum(),
            inst_ptr->staticInst() ?
                inst_ptr->staticInst()->disassemble(
                    inst_ptr->pcState().instAddr()).c_str() :
                    "Not yet decoded.");
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

    if (_cpuPtr->hasBranchPredictor() && inst_ptr->staticInst()->isControl()) {
        _cpuPtr->getBranchPredictor()->update(inst_ptr->seqNum(), threadId());
    }

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

    instTypes[inst_ptr->staticInst()->opClass()]++;

    lastCommittedInstNum = inst_ptr->seqNum();

    Trace::InstRecord* const trace_data = inst_ptr->traceData();
    if (trace_data) {
        trace_data->setFetchSeq(lastCommittedInstNum);
        trace_data->setCPSeq(lastCommittedInstNum);
        trace_data->dump();
    }
}

void
SDCPUThread::dumpBuffer()
{
    DPRINTF(SDCPUBufferDump, "%d instructions on the buffer:\n",
                             inflightInsts.size());

    for (shared_ptr<InflightInst>& inst_ptr : inflightInsts) {
        const StaticInstPtr static_inst = inst_ptr->staticInst();

        const char* status;
        switch (inst_ptr->status()) {
          case InflightInst::Waiting:
            status = "Waiting"; break;
          case InflightInst::Executing:
            status = "Executing"; break;
          case InflightInst::EffAddred:
            status = "EffAddred"; break;
          case InflightInst::Memorying:
            status = "Memorying"; break;
          case InflightInst::Complete:
            status = "Complete"; break;
          case InflightInst::Committed:
            status = "Committed"; break;
          default:
            status = "Unknown";
        }

        DPRINTF(SDCPUBufferDump, "(seq %d) Squashed: %s, Status: %s, Ready: "
                                 "%s, MemReady: %s\n    %s\n",
            inst_ptr->seqNum(),
            inst_ptr->isSquashed() ? "yes" : "no",
            status,
            inst_ptr->isReady() ? "yes" : "no",
            inst_ptr->isMemReady() ? "yes" : "no",
            static_inst ?
                static_inst->disassemble(inst_ptr->pcState().pc()).c_str() :
                "Not yet decoded");
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

    inst_ptr->status(InflightInst::Executing);
    _cpuPtr->requestExecution(static_inst,
                              static_pointer_cast<ExecContext>(inst_ptr),
                              inst_ptr->traceData(), callback);
    // For memrefs, this will result in a corresponding call to either
    // MemIFace::initiateMemRead() or MemIFace::writeMem(), depending on
    // whether the access is a write or read.
}

void
SDCPUThread::freeBranchPredDepth()
{
    DPRINTF(SDCPUBranchPred, "Released a branch prediction depth limit\n");

    // If another branch was denied a prediction earlier due to depth limits,
    // we give them this newly released resource immediately.
    while (!unpredictedBranches.empty()) {
        shared_ptr<InflightInst> inst_ptr = unpredictedBranches.front().lock();

        // If the branch who asked before no longer needs a prediction, we
        // remove it from the queue and keep checking.
        if (!inst_ptr || inst_ptr->isCommitted() || inst_ptr->isSquashed()
         || inst_ptr->isComplete()) {
            unpredictedBranches.pop_front();
            continue;
        }

        predictCtrlInst(inst_ptr);
        unpredictedBranches.pop_front();

        return;
    }

    // If no branches need prediction, we just add to the pool of unused
    // resources.
    ++remainingBranchPredDepth;
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

    advanceInst(getNextPC());
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
        advanceInst(getNextPC());
    } else if (_cpuPtr->hasBranchPredictor()) {
        if (!remainingBranchPredDepth) {
            DPRINTF(SDCPUBranchPred, "This control would exceed the branch "
                                      "prediction depth limit, not requesting "
                                      "a prediction immediately.\n");

            unpredictedBranches.push_back(inst);

            return;
        }

        --remainingBranchPredDepth;

        predictCtrlInst(inst_ptr);
    } else {
        DPRINTF(SDCPUThreadEvent, "Delaying fetch until control instruction's "
                                  "execution.\n");
    }
}

void
SDCPUThread::markFault(shared_ptr<InflightInst> inst_ptr, Fault fault)
{
    DPRINTF(SDCPUThreadEvent, "Fault detected (seq %d): %s\n",
                              inst_ptr->seqNum(), fault->name());

    inst_ptr->fault(fault);

    squashUpTo(inst_ptr);

    inst_ptr->notifySquashed();

    commitAllAvailableInstructions();
}

void
SDCPUThread::onBranchPredictorAccessed(std::weak_ptr<InflightInst> inst,
                                       BPredUnit* pred)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr
     || inst_ptr->isCommitted()
     || inst_ptr->isSquashed()
     || inst_ptr->isComplete()) {
        freeBranchPredDepth();
        // No need to do anything for an instruction that has been
        // squashed, or a branch that has already been resolved.
        return;
    }

    // We check before requesting one from the CPU, so this function should
    // only be called with a non-NULL branch predictor.
    panic_if(!pred, "I wasn't programmed to understand nullptr branch "
                    "predictors!");

    TheISA::PCState pc = inst_ptr->pcState();

    const bool taken = pred->predict(inst_ptr->staticInst(),
                                     inst_ptr->seqNum(), pc, threadId());

    // BPredUnit::predict takes pc by reference, and updates it in-place.

    DPRINTF(SDCPUBranchPred, "(seq %d) predicted %s (predicted pc: %#x).\n",
                              inst_ptr->seqNum(),
                              taken ? "taken" : "not taken",
                              pc.instAddr());
    // Count stats using the predict return value?

    StaticInstPtr static_inst = inst_ptr->staticInst();
    if (static_inst->isDirectCtrl() && static_inst->isUncondCtrl()) {
        // If we *know* we've mispredicted, no need to keep fetching
        // new instructions. This optimization improves the simulator's
        // performance, but may underestimate the instruction fetch bandwidth
        if (!taken || static_inst->branchTarget(inst_ptr->pcState()) != pc) {
            DPRINTF(SDCPUBranchPred, "Unconditional branch mispredicted\n");
            freeBranchPredDepth();
            return;
        }
    } else if ((static_inst->isCall() || static_inst->isReturn()) && !taken) {
        // Calls are always taken.
        DPRINTF(SDCPUBranchPred, "Call/return taken misprediction\n");
        freeBranchPredDepth();
        return;
    }

    inst_ptr->addSquashCallback([this, inst] {
        const shared_ptr<InflightInst> inst_ptr = inst.lock();
        if (!inst_ptr || inst_ptr->isCommitted() || inst_ptr->isComplete())
            return;

        freeBranchPredDepth();
    });


    advanceInst(pc);
}

void
SDCPUThread::onDataAddrTranslated(weak_ptr<InflightInst> inst, Fault fault,
                                  const RequestPtr& req, bool write,
                                  shared_ptr<uint8_t> data,
                                  shared_ptr<SplitRequest> sreq, bool high)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    DPRINTF(SDCPUThreadEvent, "onDataAddrTranslated(seq %d%s)\n",
                              inst_ptr->seqNum(),
                              sreq ? (high ? ", high" : ", low") : "");

    if (fault != NoFault) {
        markFault(inst_ptr, fault);

        return;
    }

    // TODO set the effective physical address for the inflightInst
    // TODO add dependencies according to the now known effective address, and
    //      unset this instruction ready if appropriate.

    // IF we have a split request
    if (sreq) {
        if (high) {
            sreq->high = req;
            inst_ptr->effPAddrRangeHigh(AddrRange(req->getPaddr(),
                req->getPaddr() + req->getSize() - 1));
        } else {
            sreq->low = req;
            inst_ptr->effPAddrRange(AddrRange(req->getPaddr(),
                req->getPaddr() + req->getSize() - 1));
        }

        // IF not both requests have been translated successfully
        if (!sreq->high || !sreq->low)
            return; // We need to await the other translation before sending
                    // the request to memory.
    }

    inst_ptr->effPAddrRange(AddrRange(req->getPaddr(),
        req->getPaddr() + req->getSize() - 1));

    inst_ptr->notifyEffAddrCalculated();

    if (inst_ptr->isMemReady()) {
        sendToMemory(inst_ptr, req, write, data, sreq);
    } else { // has remaining dependencies
        inst_ptr->addMemReadyCallback(
            [this, inst, req, write, data, sreq] {
                sendToMemory(inst, req, write, data, sreq);
            }
        );
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

    if (inst_ptr->staticInst()->isControl()) {
        // We must have our next PC now, since this branch has just resolved

        if (_cpuPtr->hasBranchPredictor()) {
            // If a branch predictor has been set, then we need to check if
            // a prediction was made, squash instructions if incorrectly
            // predicted, and notify the predictor accordingly.

            auto it = find(inflightInsts.begin(), inflightInsts.end(),
                           inst_ptr);

            if (it == inflightInsts.end()) {
                // Should be unreachable...
                panic("Did a completion callback on an instruction not in "
                      "our buffer.");
            }

            shared_ptr<InflightInst> following_inst =
                (++it != inflightInsts.end()) ? *it : nullptr;

            if (following_inst) {
                // This condition is true if a branch prediction was made prior
                // to this point in simulation.

                freeBranchPredDepth();

                const TheISA::PCState calculatedPC = inst_ptr->pcState();
                TheISA::PCState correctPC = calculatedPC;
                inst_ptr->staticInst()->advancePC(correctPC);
                const TheISA::PCState predictedPC = following_inst->pcState();

                if (correctPC.pc() == predictedPC.pc()
                 && correctPC.upc() == predictedPC.upc()) {
                    // It was a correct prediction
                    DPRINTF(SDCPUBranchPred,
                            "Branch predicted correctly (seq %d)\n",
                            inst_ptr->seqNum());
                    // Calculate stat correct predictions?
                    // Otherwise do nothing because we guessed correctly.
                    // Just remember to update the branch predictor at commit.
                } else { // It was an incorrect prediction
                    DPRINTF(SDCPUBranchPred,
                            "Branch predicted incorrectly (seq %d)\n",
                            inst_ptr->seqNum());

                    // Squash all mispredicted instructions
                    squashUpTo(inst_ptr, true);

                    // Notify branch predictor of incorrect prediction

                    _cpuPtr->getBranchPredictor()->squash(inst_ptr->seqNum(),
                        correctPC, calculatedPC.branching(), threadId());

                    advanceInst(correctPC);
                }
            } else { // predictor hasn't fired yet, so we can preemptively
                     // place the next inst on the buffer with the known pc.
                advanceInst(getNextPC());
            }
        } else { // No branch predictor
            // If we are a control instruction, then we were unable to begin a
            // fetch during decode, so we should start one now.
            advanceInst(getNextPC());
        }
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
    _cpuPtr->requestInstructionData(req, callback,
        [inst]{
            shared_ptr<InflightInst> inst_ptr = inst.lock();
            return !inst_ptr || inst_ptr->isSquashed();
        });
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

    // BEGIN Explicit rules

    // BEGIN ISA explicit serialization

    if (strictSerialize) {
        auto serializing_inst = lastSerializingInstruction.lock();
        if (serializing_inst) {
            DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                    inst_ptr->seqNum(), serializing_inst->seqNum());
            inst_ptr->addCommitDependency(serializing_inst);
        }

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

            lastSerializingInstruction = inst_ptr;
        }
    } else {
        if (inflightInsts.size() > 1 && static_inst->isSerializeBefore()) {
            const shared_ptr<InflightInst> last_inst =
                *(++inflightInsts.rbegin());

            DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                    inst_ptr->seqNum(), last_inst->seqNum());
            inst_ptr->addCommitDependency(last_inst);
        }

        auto serializing_inst = lastSerializingInstruction.lock();
        if (serializing_inst) {
            DPRINTF(SDCPUDeps, "Dep %d -> %d [serial]\n",
                    inst_ptr->seqNum(), serializing_inst->seqNum());
            inst_ptr->addCommitDependency(serializing_inst);
        }

        if (static_inst->isSerializeAfter()) {
            lastSerializingInstruction = inst_ptr;
        }
    }

    // END ISA explicit serialization

    // BEGIN ISA explicit memory barriers

    // By creating a dependency between any memory requests and the most recent
    // prior memory request, all memory instructions are enforced to execute in
    // program-order.
    if (static_inst->isMemRef()) {
        shared_ptr<InflightInst> last_barrier = lastMemBarrier.lock();
        if (last_barrier) {
            DPRINTF(SDCPUDeps, "Dep %d -> %d [mem ref -> barrier]\n",
                    inst_ptr->seqNum(), last_barrier->seqNum());
            inst_ptr->addMemDependency(last_barrier);
        }
    }

    if (static_inst->isMemBarrier()) {
        lastMemBarrier = inst_ptr;
        for (auto itr = inflightInsts.rbegin();
             itr != inflightInsts.rend();
             ++itr) {
            if (*itr == inst_ptr) continue;
            if ((*itr)->isComplete() || ((*itr)->isSquashed())) continue;

            const StaticInstPtr other_si = (*itr)->staticInst();

            if (other_si->isMemBarrier() || other_si->isMemRef()) {
                DPRINTF(SDCPUDeps, "Dep %d -> %d [mem barrier -> ref/barrier]"
                                   "\n",
                        inst_ptr->seqNum(), (*itr)->seqNum());
                if (static_inst->isMemRef()) {
                    inst_ptr->addMemDependency(*itr);
                } else {
                    inst_ptr->addDependency(*itr);
                }
            }
        }
    }

    // Should we handle static_inst->isWriteBarrier() separately?
    // I'm convinced that it wouldn't change the order of any writes occuring,
    // since stores are already only sent to memory at commit-time, which is a
    // stricter restriction already than that which the write barrier would
    // add.

    // END ISA explicit memory barriers

    if (inflightInsts.size() > 1 && static_inst->isNonSpeculative()) {
        const shared_ptr<InflightInst> last_inst = *(++inflightInsts.rbegin());

        if (!(last_inst->isCommitted() || last_inst->isSquashed())) {
            inst_ptr->addCommitDependency(last_inst);
        }

        // This is a very conservative implementation of the rule, but has been
        // implemented this way since clear explanation of the flag is not
        // available. If we only mean branch speculation, then a commit-time
        // dependency on the last control instruction should be sufficient.
    }

    // END Explicit rules

    // BEGIN Sequential Consistency

    // By creating a dependency between any memory requests and the most recent
    // prior memory request, all memory instructions are enforced to execute in
    // program-order.
    // if (static_inst->isMemRef() || static_inst->isMemBarrier()) {
    //     for (auto itr = inflightInsts.rbegin();
    //          itr != inflightInsts.rend();
    //          ++itr) {
    //         if (*itr == inst_ptr) continue;
    //         if ((*itr)->isComplete() || ((*itr)->isSquashed())) continue;

    //         const StaticInstPtr other_si = (*itr)->staticInst();

    //         if (other_si->isMemRef() || other_si->isMemBarrier()) {
    //             DPRINTF(SDCPUDeps, "Dep %d -> %d [mem barrier]\n",
    //                     inst_ptr->seqNum(), (*itr)->seqNum());
    //             inst_ptr->addMemDependency(*itr);
    //             break;
    //         }
    //     }
    // }

    // END Sequential Consistency

    // BEGIN Conservative Memory dependence ordering

    if (static_inst->isLoad()) {
        weak_ptr<InflightInst> weak_inst = inst_ptr;

        // Checking all instructions older than the inst_ptr. (This loop starts
        // from oldest)
        for (shared_ptr<InflightInst>& other : inflightInsts) {
            if (other == inst_ptr) break;
            if (!other->staticInst()->isStore()) continue;

            if (other->isEffAddred()) {
                // Case 1: other has already calculated an effective address
                weak_ptr<InflightInst> weak_other = other;
                inst_ptr->addEffAddrCalculatedCallback(
                    [this, weak_inst, weak_other] {
                        shared_ptr<InflightInst> other = weak_other.lock();
                        if (!other || other->isSquashed()
                         || other->isComplete()) return;

                        shared_ptr<InflightInst> inst_ptr = weak_inst.lock();
                        // Don't need to check validity because this function
                        // can only be referenced through a valid inst_ptr

                        if (inst_ptr->effAddrOverlap(*other)) {
                            DPRINTF(SDCPUDeps, "Dep %d -> %d [mem]\n",
                                    inst_ptr->seqNum(), other->seqNum());
                            inst_ptr->addMemDependency(other);
                        }
                    }
                );
            } else {
                // Case 2: other has not yet calculated an effective address
                //        Case 2a: we will calculate before other
                //        Case 2b: they will calculate before us
                //                 (similar to case 1)

                weak_ptr<InflightInst> weak_other = other;
                inst_ptr->addEffAddrCalculatedCallback(
                    [this, weak_inst, weak_other] {
                        shared_ptr<InflightInst> other = weak_other.lock();
                        if (!other || other->isSquashed()
                         || other->isComplete()) return;

                        // If the other instruction is still calculating the
                        // effective address, let it create the dependency when
                        // it finishes instead, since we don't know if the
                        // dependency is necessary yet.
                        if (!other->isEffAddred()) return;

                        shared_ptr<InflightInst> inst_ptr = weak_inst.lock();
                        // Don't need to check validity because this function
                        // can only be referenced through a valid inst_ptr

                        if (inst_ptr->effAddrOverlap(*other)) {
                            DPRINTF(SDCPUDeps, "Dep %d -> %d [mem]\n",
                                    inst_ptr->seqNum(), other->seqNum());
                            inst_ptr->addMemDependency(other);
                        }
                    }
                );
                other->addEffAddrCalculatedCallback(
                    [this, weak_inst, weak_other] {
                        shared_ptr<InflightInst> inst_ptr = weak_inst.lock();
                        if (!inst_ptr || inst_ptr->isSquashed()) return;

                        // If the later instruction is still calculating the
                        // effective address, let it create the dependency when
                        // it finishes instead, since we don't know if the
                        // dependency is necessary yet.
                        if (!inst_ptr->isEffAddred()) return;

                        shared_ptr<InflightInst> other = weak_other.lock();
                        // Don't need to check validity because this function
                        // can only be referenced through a valid other

                        if (inst_ptr->effAddrOverlap(*other)) {
                            DPRINTF(SDCPUDeps, "Dep %d -> %d [mem]\n",
                                    inst_ptr->seqNum(), other->seqNum());
                            inst_ptr->addMemDependency(other);
                        }
                    }
                );

                // NOTE: It is important that this dependency is added after
                //       the callback above is added to other, since the
                //       callback needs to resolve first, to make sure that we
                //       don't accidentally notifyMemReady() before we've added
                //       all appropriate memory dependencies.
                DPRINTF(SDCPUDeps, "Dep %d -> %d [mem predicted overlap]\n",
                        inst_ptr->seqNum(), other->seqNum());
                inst_ptr->addMemEffAddrDependency(other);
            }
        }
    }

    // END Conservative Memory dependence ordering

    // BEGIN Only store if instruction is at head of inflightInsts

    if (inflightInsts.size() > 1 && static_inst->isStore()) {
        const shared_ptr<InflightInst> last_inst = *(++inflightInsts.rbegin());
        DPRINTF(SDCPUDeps, "Dep %d -> %d [st @ commit]\n",
                inst_ptr->seqNum(), last_inst->seqNum());
        inst_ptr->addMemCommitDependency(last_inst);
    }

    // END Only store if instruction is at head of inflightInsts

    // BEGIN Data dependencies through registers

    const int8_t num_srcs = static_inst->numSrcRegs();
    for (int8_t src_idx = 0; src_idx < num_srcs; ++src_idx) {
        const RegId& src_reg = flattenRegId(static_inst->srcRegIdx(src_idx));
        if (src_reg.isZeroReg()) continue;

        const InflightInst::DataSource& last_use = lastUses[src_reg];
        const shared_ptr<InflightInst> producer = last_use.producer.lock();

        // Attach the dependency if the instruction is still around
        if (producer) {
            DPRINTF(SDCPUDeps, "Dep %d -> %d [data (%s[%d])]\n",
                    inst_ptr->seqNum(), producer->seqNum(),
                    src_reg.className(),
                    src_reg.index());
            inst_ptr->addDependency(producer);
        }

        inst_ptr->setDataSource(src_idx, last_use);
    }

    // END Data dependencies through registers
}

void
SDCPUThread::populateUses(shared_ptr<InflightInst> inst_ptr)
{
    const StaticInstPtr static_inst = inst_ptr->staticInst();
    const int8_t num_dsts = static_inst->numDestRegs();
    for (int8_t dst_idx = 0; dst_idx < num_dsts; ++dst_idx) {
        const RegId& dst_reg = flattenRegId(static_inst->destRegIdx(dst_idx));

        lastUses[dst_reg] = {inst_ptr, dst_idx};
        DPRINTF(SDCPUDeps, "%d is producer of %s[%d]\n", inst_ptr->seqNum(),
                dst_reg.className(),
                dst_reg.index());
    }
}

void
SDCPUThread::predictCtrlInst(shared_ptr<InflightInst> inst_ptr)
{
    DPRINTF(SDCPUBranchPred, "Requesting branch predictor for control\n");

    const InstSeqNum seqnum = inst_ptr->seqNum();
    inst_ptr->addSquashCallback([this, seqnum] {
        _cpuPtr->getBranchPredictor()->squash(seqnum, threadId());
    });

    const weak_ptr<InflightInst> weak_inst = inst_ptr;
    auto callback = [this, weak_inst] (BPredUnit* pred) {
        onBranchPredictorAccessed(weak_inst, pred);
    };

    _cpuPtr->requestBranchPredictor(callback);
}

void
SDCPUThread::recordCycleStats()
{
    activeInstructions.sample(inflightInsts.size());
    if (squashedThisCycle > 0) {
        squashedPerCycle.sample(squashedThisCycle);
        squashedThisCycle = 0;
    }
    if (fetchedThisCycle > 0) {
        fetchedInstsPerCycle.sample(fetchedThisCycle);
        fetchedThisCycle = 0;
    }
}

void
SDCPUThread::sendToMemory(weak_ptr<InflightInst> inst,
                          const RequestPtr& req, bool write,
                          shared_ptr<uint8_t> data,
                          shared_ptr<SplitRequest> sreq)
{
    const shared_ptr<InflightInst> inst_ptr = inst.lock();
    if (!inst_ptr || inst_ptr->isSquashed()) {
        // No need to do anything for an instruction that has been squashed.
        return;
    }

    sendToMemory(inst_ptr, req, write, data, sreq);
}

void
SDCPUThread::sendToMemory(shared_ptr<InflightInst> inst_ptr,
                          const RequestPtr& req, bool write,
                          shared_ptr<uint8_t> data,
                          shared_ptr<SplitRequest> sreq)
{
    inst_ptr->status(InflightInst::Memorying);

    weak_ptr<InflightInst> weak_inst(inst_ptr);
    auto callback =
        [this, weak_inst] (Fault fault) {
            onExecutionCompleted(weak_inst, fault);
        };

    if (sreq) {
        assert(sreq->high && sreq->low);
        if (write) {
            _cpuPtr->requestSplitMemWrite(sreq->main, sreq->low,
                sreq->high, getThreadContext(),
                inst_ptr->staticInst(),
                static_pointer_cast<ExecContext>(inst_ptr),
                inst_ptr->traceData(), data.get(),
                callback);
        } else { // read
            _cpuPtr->requestSplitMemRead(sreq->main, sreq->low,
                sreq->high, getThreadContext(),
                inst_ptr->staticInst(),
                static_pointer_cast<ExecContext>(inst_ptr),
                inst_ptr->traceData(),
                callback);
        }
    } else {
        if (write) {
            _cpuPtr->requestMemWrite(req, getThreadContext(),
                inst_ptr->staticInst(),
                static_pointer_cast<ExecContext>(inst_ptr),
                inst_ptr->traceData(), data.get(), callback);
        } else { // read
            _cpuPtr->requestMemRead(req, getThreadContext(),
                inst_ptr->staticInst(),
                static_pointer_cast<ExecContext>(inst_ptr),
                inst_ptr->traceData(), callback);
        }
    }
}

void
SDCPUThread::squashUpTo(shared_ptr<InflightInst> inst_ptr, bool rebuild_lasts)
{
    bool last_ser_correct = true;
    bool last_mem_bar_correct = true;

    shared_ptr<InflightInst> ser_inst = lastSerializingInstruction.lock();
    shared_ptr<InflightInst> bar_inst = lastMemBarrier.lock();
    size_t count = 0;
    while (inflightInsts.back() != inst_ptr) {
        inflightInsts.back()->notifySquashed();
        if (inflightInsts.back() == ser_inst) last_ser_correct = false;
        if (inflightInsts.back() == bar_inst) last_mem_bar_correct = false;
        inflightInsts.pop_back();
        ++count;
    }

    numSquashed.sample(count);
    squashedThisCycle += count;

    DPRINTF(SDCPUThreadEvent, "%d instructions squashed.\n", count);

    if (rebuild_lasts) {
        for (shared_ptr<InflightInst>& it : inflightInsts) {
            populateUses(it);
        }

        for (auto itr = inflightInsts.rbegin();
             itr != inflightInsts.rend()
                && !(last_ser_correct && last_mem_bar_correct);
             ++itr) {
            StaticInstPtr static_inst = (*itr)->staticInst();

            if (strictSerialize) {
                if (!last_ser_correct && static_inst->isSerializing()) {
                    lastSerializingInstruction = *itr;
                    last_ser_correct = true;
                }
            } else {
                if (!last_ser_correct && static_inst->isSerializeAfter()) {
                    lastSerializingInstruction = *itr;
                    last_ser_correct = true;
                }
            }

            if (!last_mem_bar_correct && static_inst->isMemBarrier()) {
                lastMemBarrier = *itr;
                last_mem_bar_correct = true;
            }
        }
    }

    decoder.reset();
    curMacroOp = StaticInst::nullStaticInstPtr;
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

        inst_ptr->isSplitMemReq(true);

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
                                                        inst_ptr, callback1);

        sdCPUThread._cpuPtr->requestDataAddrTranslation(req2, tc, false,
                                                        inst_ptr, callback2);

        return NoFault;
    }


    auto callback = [this, inst](Fault fault, const RequestPtr& req) {
        sdCPUThread.onDataAddrTranslated(inst, fault, req, false,
                                         nullptr);
    };

    sdCPUThread._cpuPtr->requestDataAddrTranslation(req, tc, false, inst_ptr,
                                                    callback);

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
    shared_ptr<uint8_t> copy(new uint8_t[size], default_delete<uint8_t[]>());
    memcpy(copy.get(), data, size);

    if (split_addr > addr) { // The request spans two cache lines
        assert(!(req->isLLSC() || req->isMmappedIpr())); // don't deal with
                                                         // weird cases for now

        inst_ptr->isSplitMemReq(true);

        RequestPtr req1, req2;

        req->splitOnVaddr(split_addr, req1, req2);

        shared_ptr<SplitRequest> split_acc = make_shared<SplitRequest>();
        split_acc->main = req;

        auto callback1 = [this, inst, split_acc, copy, size](Fault fault,
            const RequestPtr& req) {
            sdCPUThread.onDataAddrTranslated(inst, fault, req, true,
                                             copy, split_acc, false);
        };

        auto callback2 = [this, inst, split_acc, copy, size](Fault fault,
                                                 const RequestPtr& req) {
            sdCPUThread.onDataAddrTranslated(inst, fault, req, true,
                                             copy, split_acc, true);
        };


        sdCPUThread._cpuPtr->requestDataAddrTranslation(req1, tc, true,
                                                        inst_ptr, callback1);

        sdCPUThread._cpuPtr->requestDataAddrTranslation(req2, tc, true,
                                                        inst_ptr, callback2);

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
    };

    sdCPUThread._cpuPtr->requestDataAddrTranslation(req, tc, true, inst_ptr,
                                                    callback);

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

    numSquashed
        .name(name + ".numSquashed")
        .init(16)
        .desc("Instructions squashed on each squash request")
    ;

    fetchedInstsPerCycle
        .name(name + ".fetchedInstsPerCycle")
        .init(16)
        .desc("Number of instructions \"fetched\" for each cycle")
    ;
    squashedPerCycle
        .name(name + ".squashedPerCycle")
        .init(16)
        .desc("Number of instructions squashed each cycle")
    ;

    activeInstructions
        .name(name + ".activeInstructions")
        .init(32)
        .desc("Number of instructions active each cycle")
    ;

    instTypes
        .name(name + ".instTypes")
        .init(Enums::Num_OpClass)
        .desc("Number of each type of instruction committed")
        ;
    for (int i=0; i < Enums::Num_OpClass; ++i) {
        instTypes.subname(i, Enums::OpClassStrings[i]);
    }
}
