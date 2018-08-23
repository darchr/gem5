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

#include "cpu/flexcpu/inflight_inst.hh"

using namespace std;
using IntReg = TheISA::IntReg;
using PCState = TheISA::PCState;
using FloatReg = TheISA::FloatReg;
using FloatRegBits = TheISA::FloatRegBits;
using MiscReg = TheISA::MiscReg;

using CCReg = TheISA::CCReg;
using VecElem = TheISA::VecElem;

InflightInst::InflightInst(ThreadContext* backing_context,
                           TheISA::ISA* backing_isa,
                           MemIface* backing_mem_iface,
                           InstSeqNum seq_num,
                           const PCState& pc_,
                           StaticInstPtr inst_ref):
    backingContext(backing_context),
    backingISA(backing_isa),
    backingMemoryInterface(backing_mem_iface),
    _status(Waiting),
    _seqNum(seq_num),
    _pcState(pc_)
{
    staticInst(inst_ref);
}

void
InflightInst::addCompletionCallback(std::function<void()> callback)
{
    completionCallbacks.push_back(callback);
}

void
InflightInst::addDependency(shared_ptr<InflightInst> parent)
{
    ++remainingDependencies;

    weak_ptr<InflightInst> weak_this = shared_from_this();
    parent->addCompletionCallback([weak_this]() {
        shared_ptr<InflightInst> inst_ptr = weak_this.lock();
        if (!inst_ptr) return;

        --inst_ptr->remainingDependencies;

        if (!inst_ptr->remainingDependencies) {
            inst_ptr->notifyReady();
        }
    });
}

void
InflightInst::addReadyCallback(std::function<void()> callback)
{
    readyCallbacks.push_back(callback);
}

void
InflightInst::commitToTC()
{
    assert(isComplete());

    const int8_t num_dsts = instRef->numDestRegs();
    for (int8_t dst_idx = 0; dst_idx < num_dsts; ++dst_idx) {
        const RegId& dst_reg = instRef->destRegIdx(dst_idx);
        const GenericReg& result = getResult(dst_idx);

        switch (dst_reg.classValue()) {
          case IntRegClass:
            backingContext->setIntReg(dst_reg.index(), result.asIntReg());
            break;
          case FloatRegClass:
            backingContext->setFloatReg(dst_reg.index(), result.asFloatReg());
            break;
          case VecRegClass:
            backingContext->setVecReg(dst_reg, result.asVecReg());
            break;
          case VecElemClass:
            backingContext->setVecElem(dst_reg, result.asVecElem());
            break;
          case CCRegClass:
            backingContext->setCCReg(dst_reg.index(), result.asCCReg());
            break;
          case MiscRegClass:
            backingContext->setMiscReg(dst_reg.index(), result.asMiscReg());
            break;
          default:
            break;
        }
    }

    TheISA::PCState pc = pcState();
    instRef->advancePC(pc);
    backingContext->pcState(pc);

    for (size_t i = 0; i < miscResultIdxs.size(); i++)
        backingISA->setMiscReg(
            miscResultIdxs[i], miscResultVals[i], backingContext);
}

void
InflightInst::notifyComplete()
{
    status(Complete);

    for (function<void()>& callback_func : completionCallbacks) {
        callback_func();
    }
}

void
InflightInst::notifyReady()
{
    for (function<void()>& callback_func : readyCallbacks) {
        callback_func();
    }
}

void
InflightInst::setDataSource(int8_t src_idx, DataSource source)
{
    sources[src_idx] = source;
}

const StaticInstPtr&
InflightInst::staticInst(const StaticInstPtr& inst_ref)
{
    instRef = inst_ref;

    if (inst_ref) {
        const int8_t num_dsts = inst_ref->numDestRegs();
        for (int8_t i = 0; i < num_dsts; ++i) {
            // TODO consider setting class to corresponding RegId, and filling
            // values in, in-case of conditional register access.
            // Fill results table with dummy values
            results.push_back(GenericReg(0, IntRegClass));
            resultValid.push_back(false);
        }

        const int8_t num_srcs = inst_ref->numSrcRegs();
        for (int8_t i = 0; i < num_srcs; ++i) {
            sources.push_back({});
        }
    }

    return instRef;
}

// BEGIN ExecContext interface functions

IntReg
InflightInst::readIntRegOperand(const StaticInst* si, int op_idx)
{
    // Sanity check
    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isIntReg());

    if (reg_id.isZeroReg()) return 0;

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asIntReg();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->readIntReg(reg_id.index());
    }
}

void
InflightInst::setIntRegOperand(const StaticInst* si, int dst_idx, IntReg val)
{
    const RegId& reg_id = si->destRegIdx(dst_idx);
    assert(reg_id.isIntReg());

    results[dst_idx].set(reg_id.isZeroReg() ? 0 : val, IntRegClass);
    resultValid[dst_idx] = true;
}

FloatReg
InflightInst::readFloatRegOperand(const StaticInst* si, int op_idx)
{
    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isFloatReg());

    if (reg_id.isZeroReg()) return 0;

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asFloatReg();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->readFloatReg(reg_id.index());
    }
}

FloatRegBits
InflightInst::readFloatRegOperandBits(const StaticInst* si, int op_idx)
{
    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isFloatReg());

    if (reg_id.isZeroReg()) return 0;

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asFloatRegBits();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->readFloatRegBits(reg_id.index());
    }
}

void
InflightInst::setFloatRegOperand(const StaticInst* si, int dst_idx,
                                 FloatReg val)
{
    const RegId& reg_id = si->destRegIdx(dst_idx);
    assert(reg_id.isFloatReg());

    results[dst_idx].set(reg_id.isZeroReg() ? 0 : val, FloatRegClass);
    resultValid[dst_idx] = true;
}

void
InflightInst::setFloatRegOperandBits(const StaticInst* si, int dst_idx,
                                     FloatRegBits val)
{
    const RegId& reg_id = si->destRegIdx(dst_idx);
    assert(reg_id.isFloatReg());

    results[dst_idx].set(reg_id.isZeroReg() ? 0 : val, FloatRegClass);
    resultValid[dst_idx] = true;
}

const TheISA::VecRegContainer&
InflightInst::readVecRegOperand(const StaticInst* si, int op_idx) const
{
    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isVecReg());

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asVecReg();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->readVecReg(reg_id);
    }
}

TheISA::VecRegContainer&
InflightInst::getWritableVecRegOperand(const StaticInst* si, int op_idx)
{
    // TODO this one may produce bad results, need to rethink this, since if
    //      we write to a srcreg, then the dependencies might not be matched
    //      correctly, nor will the changed state necessarily be applied to the
    //      committed state during commit time.

    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isVecReg());

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asVecReg();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->getWritableVecReg(reg_id);
    }
}

void
InflightInst::setVecRegOperand(const StaticInst* si, int dst_idx,
                               const TheISA::VecRegContainer& val)
{
    assert(si->destRegIdx(dst_idx).isVecReg());

    // NOTE This assignment results in two calls to copy the VecRegContainer
    // object, one to construct the temporary GenericReg, the other to
    // perform the copy into the results table. There may be a more efficient
    // way to do this, perhaps with an in-place "set" function.
    results[dst_idx].set(val, VecRegClass);
}

ConstVecLane8
InflightInst::readVec8BitLaneOperand(const StaticInst* si, int op_idx) const
{
    panic("readVec8BitLaneOperand() not implemented!");
    // To make sure compiler doesn't complain
    return backingContext->readVec8BitLaneReg(si->srcRegIdx(op_idx));
    // TODO
}

ConstVecLane16
InflightInst::readVec16BitLaneOperand(const StaticInst* si, int op_idx) const
{
    panic("readVec16BitLaneOperand() not implemented!");
    // To make sure compiler doesn't complain
    return backingContext->readVec16BitLaneReg(si->srcRegIdx(op_idx));
    // TODO
}

ConstVecLane32
InflightInst::readVec32BitLaneOperand(const StaticInst* si, int op_idx) const
{
    panic("readVec32BitLaneOperand() not implemented!");
    // To make sure compiler doesn't complain
    return backingContext->readVec32BitLaneReg(si->srcRegIdx(op_idx));
    // TODO
}

ConstVecLane64
InflightInst::readVec64BitLaneOperand(const StaticInst* si, int op_idx) const
{
    panic("readVec64BitLaneOperand() not implemented!");
    // To make sure compiler doesn't complain
    return backingContext->readVec64BitLaneReg(si->srcRegIdx(op_idx));
    // TODO
}


void
InflightInst::setVecLaneOperand(const StaticInst* si, int dst_idx,
                        const LaneData<LaneSize::Byte>& val)
{
    panic("setVecLaneOperand() not implemented!");
    // TODO
}

void
InflightInst::setVecLaneOperand(const StaticInst* si, int dst_idx,
                        const LaneData<LaneSize::TwoByte>& val)
{
    panic("setVecLaneOperand() not implemented!");
    // TODO
}

void
InflightInst::setVecLaneOperand(const StaticInst* si, int dst_idx,
                        const LaneData<LaneSize::FourByte>& val)
{
    panic("setVecLaneOperand() not implemented!");
    // TODO
}

void
InflightInst::setVecLaneOperand(const StaticInst* si, int dst_idx,
                        const LaneData<LaneSize::EightByte>& val)
{
    panic("setVecLaneOperand() not implemented!");
    // TODO
}

VecElem
InflightInst::readVecElemOperand(const StaticInst* si, int op_idx) const
{
    panic("readVecElemOperand() not implemented!");
    return 0;
    // TODO
}

void
InflightInst::setVecElemOperand(const StaticInst* si, int dst_idx,
                                const VecElem val)
{
    panic("setVecElemOperand() not implemented!");
    // TODO
}

CCReg
InflightInst::readCCRegOperand(const StaticInst* si, int op_idx)
{
    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isCCReg());

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asCCReg();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->readCCReg(reg_id.index());
    }
}

void
InflightInst::setCCRegOperand(const StaticInst* si, int dst_idx, CCReg val)
{
    assert(si->destRegIdx(dst_idx).isCCReg());

    results[dst_idx].set(val, CCRegClass);
    resultValid[dst_idx] = true;
}

MiscReg
InflightInst::readMiscRegOperand(const StaticInst* si, int op_idx)
{
    const RegId& reg_id = si->srcRegIdx(op_idx);
    assert(reg_id.isMiscReg());

    const DataSource& source = sources[op_idx];
    const shared_ptr<InflightInst> producer = source.producer.lock();

    if (producer) {
        // If the producing instruction is still in the buffer, grab the result
        // assuming that it has been produced, and our index is in bounds.
        assert(producer->isComplete());
        assert(source.resultIdx >= 0
            && source.resultIdx < producer->results.size()
            && producer->resultValid[source.resultIdx]);
        return producer->getResult(source.resultIdx).asMiscReg();
    } else {
        // Either the producing instruction has already been committed, or no
        // dependency was in-flight at the time that this instruction was
        // issued.
        return backingContext->readMiscReg(reg_id.index());
    }
}

void
InflightInst::setMiscRegOperand(const StaticInst* si, int dst_idx,
                                const MiscReg& val)
{
    assert(si->destRegIdx(dst_idx).isMiscReg());

    results[dst_idx].set(val, MiscRegClass);
    resultValid[dst_idx] = true;
}

MiscReg
InflightInst::readMiscReg(int misc_reg)
{
    return backingISA->readMiscReg(misc_reg, backingContext);
}

void
InflightInst::setMiscReg(int misc_reg, const MiscReg& val)
{
    // O3 hides multiple writes to the same misc reg during execution, but due
    // to potential side effects of each access, I don't think we should be
    // hiding them. Instead, we will replay each write sequentially during
    // commit time.

    miscResultIdxs.push_back(misc_reg);
    miscResultVals.push_back(val);
    // Values should be applied to the ISA/TC at commit time, when we know the
    // instruction will not be squashed.
}

PCState
InflightInst::pcState() const
{
    return _pcState;
}

void
InflightInst::pcState(const PCState& val)
{
    _pcState = val;
}

Fault
InflightInst::readMem(Addr addr, uint8_t *data, unsigned int size,
                Request::Flags flags)
{
    if (backingMemoryInterface) {
        return backingMemoryInterface->readMem(shared_from_this(), addr, data,
                                               size, flags);
    } else {
        panic("Attempted to readMem() without a memory interface!");
        return NoFault;
    }
}

Fault
InflightInst::initiateMemRead(Addr addr, unsigned int size,
                              Request::Flags flags)
{
    if (backingMemoryInterface) {
        return backingMemoryInterface->initiateMemRead(shared_from_this(),
                                                       addr, size, flags);
    } else {
        panic("Attempted to initiateMemRead() without a memory interface!");
        return NoFault;
    }
}

Fault
InflightInst::writeMem(uint8_t* data, unsigned int size, Addr addr,
                       Request::Flags flags, uint64_t* res)
{
    if (backingMemoryInterface) {
        return backingMemoryInterface->writeMem(shared_from_this(), data, size,
                                                addr, flags, res);
    } else {
        panic("Attempted to writeMem() without a memory interface!");
        return NoFault;
    }
}

void
InflightInst::setStCondFailures(unsigned int sc_failures)
{
    panic("setStCondFailures() not implemented!");
    // TODO
}

unsigned int
InflightInst::readStCondFailures() const
{
    panic("readStCondFailures() not implemented!");
    return 0;
    // TODO
}

void
InflightInst::syscall(int64_t callnum, Fault* fault)
{
    backingContext->syscall(callnum, fault);
}

ThreadContext*
InflightInst::tcBase()
{
    return backingContext;
}

Fault
InflightInst::hwrei()
{
    panic("hwrei() not implemented!");
    return NoFault;
    // TODO
}

bool
InflightInst::simPalCheck(int palFunc)
{
    panic("simPalCheck() not implemented!");
    return false;
    // TODO
}

bool
InflightInst::readPredicate()
{
    panic("readPredicate() not implemented!");
    return false;
    // TODO
}

void
InflightInst::setPredicate(bool val)
{
    panic("setPredicate() not implemented!");
    // TODO
}

void
InflightInst::demapPage(Addr vaddr, uint64_t asn)
{
    panic("demapPage() not implemented!");
    // TODO
}

void
InflightInst::armMonitor(Addr address)
{
    panic("armMonitor() not implemented!");
    // TODO
}

bool
InflightInst::mwait(PacketPtr pkt)
{
    panic("mwait() not implemented!");
    return false;
    // TODO
}

void
InflightInst::mwaitAtomic(ThreadContext* tc)
{
    panic("mwaitAtomic() not implemented!");
    // TODO
}

AddressMonitor*
InflightInst::getAddrMonitor()
{
    panic("getAddrMonitor() not implemented!");
    return nullptr;
    // TODO
}

#if THE_ISA == MIPS_ISA
MiscReg
InflightInst::readRegOtherThread(const RegId& reg,
                                 ThreadID tid = InvalidThreadID)
{
    panic("readRegOtherThread() not implemented!");
    return 0;
    // TODO
}

void
InflightInst::setRegOtherThread(const RegId& reg, MiscReg val,
                                ThreadID tid = InvalidThreadID)
{
    panic("setRegOtherThread() not implemented!");
    // TODO
}
#endif

// END ExecContext interface functions
