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

#ifndef __CPU_FLEXCPU_INFLIGHT_INST_HH__
#define __CPU_FLEXCPU_INFLIGHT_INST_HH__

#include <memory>
#include <vector>

#include "arch/isa.hh"
#include "base/addr_range.hh"
#include "cpu/exec_context.hh"
#include "cpu/flexcpu/generic_reg.hh"
#include "cpu/reg_class.hh"

class InflightInst : public ExecContext,
                     public std::enable_shared_from_this<InflightInst>
{
  public:
    using IntReg = TheISA::IntReg;
    using PCState = TheISA::PCState;
    using FloatReg = TheISA::FloatReg;
    using FloatRegBits = TheISA::FloatRegBits;
    using MiscReg = TheISA::MiscReg;

    using CCReg = TheISA::CCReg;
    using VecRegContainer = TheISA::VecRegContainer;
    using VecElem = TheISA::VecElem;

    enum Status
    {
        Invalid = 0,
        Waiting, // Has not been executed yet. Likely has dependencies still.
        // Is a dedicated Ready (dependencies satisfied) but not executed state
        // valuable?
        Executing, // Request for execution sent, but waiting for results
        Complete, // Results have been received, but not yet committed.
        Committed,
        Squashed // Results should never be committed.
    };

    struct DataSource
    {
        std::weak_ptr<InflightInst> producer;
        int8_t resultIdx;
    };

    class MemIface
    {
      public:
        virtual Fault readMem(std::shared_ptr<InflightInst> inst, Addr addr,
                              uint8_t *data, unsigned int size,
                              Request::Flags flags)
        { panic("MemIface::readMem() not implemented!"); }

        virtual Fault initiateMemRead(std::shared_ptr<InflightInst> inst,
                                      Addr addr, unsigned int size,
                                      Request::Flags flags)
        { panic("MemIface::initiateMemRead() not implemented!"); }

        virtual Fault writeMem(std::shared_ptr<InflightInst> inst,
                               uint8_t *data, unsigned int size, Addr addr,
                               Request::Flags flags, uint64_t *res)
        { panic("MemIface::writeMem() not implemented!"); }

        static const MemIface unimplemented;
    };

    // Maybe another interface for more nuanced

  protected:
    ThreadContext* backingContext;
    TheISA::ISA* backingISA;
    MemIface* backingMemoryInterface;

    // Where am I in completing this instruction?
    Status _status;

    // What is this instruction I'm executing?
    InstSeqNum _seqNum;
    StaticInstPtr instRef;

    /**
     * A storage for the exact PCState at the time of executing this
     * instruction. Any changes to the PC through the instruction can also be
     * captured in this variable, allowing this variable also to serve as the
     * result storage for changes to the PCState. If this variable is changed
     * in this fashion however, we may need to flush speculative state.
     */
    PCState _pcState;

    std::vector<std::function<void()>> commitCallbacks;

    // Who depends on me?
    std::vector<std::function<void()>> completionCallbacks;

    // How many dependencies have to be resolved before I can run?
    size_t remainingDependencies = 0;

    std::vector<std::function<void()>> readyCallbacks;

    std::vector<DataSource> sources;
    std::vector<GenericReg> results;
    std::vector<bool> resultValid;

    /**
     * For use in storing results of writes to miscRegs that don't go through
     * the operands of the StaticInst.
     */
    std::vector<MiscReg> miscResultVals;
    std::vector<int> miscResultIdxs;

    Fault _fault = NoFault;

    // For use in tracking dependencies through memory
    bool translationComplete = false;
    AddrRange accessedPAddrs;

    bool splitTranslationComplete = false;
    AddrRange accessedSplitPAddrs; // Second variable to store range for second
                                   // request as part of split accesses

    // TODO deal with macroops?
    // TODO deal with memory operations, a la LSQ?

  public:
    InflightInst(ThreadContext* backing_context, TheISA::ISA* backing_isa,
                 MemIface* backing_mem_iface, InstSeqNum seq_num,
                 const TheISA::PCState& pc_,
                 StaticInstPtr inst_ref = StaticInst::nullStaticInstPtr);

    void addCommitCallback(std::function<void()> callback);
    void addCommitDependency(std::shared_ptr<InflightInst> parent);
    void addCompletionCallback(std::function<void()> callback);
    void addDependency(std::shared_ptr<InflightInst> parent);

    void addReadyCallback(std::function<void()> callback);
    // May be useful to add ability to remove a callback. Will be difficult if
    // we use raw function objects, could be resolved by holding pointers?

    /**
     * Takes all state-changing effects that this InflightInst has, and applies
     * them to the backing ThreadContext of this InflightInst.
     */
    void commitToTC();

    inline const Fault& fault() const
    { return _fault; }
    inline const Fault& fault(const Fault& f)
    { return _fault = f; }

    /**
     * Accessor function to retrieve a result produced by this instruction.
     * Non-const variant returns a reference, so that external classes can make
     * modifications if they need to.
     */
    inline const GenericReg& getResult(int8_t result_idx) const
    {
        panic_if(!resultValid[result_idx], "Tried to access invalid result!");
        return results[result_idx];
    }
    inline GenericReg& getResult(int8_t result_idx)
    {
        panic_if(!resultValid[result_idx], "Tried to access invalid result!");
        return results[result_idx];
    }

    inline const StaticInstPtr& staticInst() const
    { return instRef; }
    const StaticInstPtr& staticInst(const StaticInstPtr& inst_ref);

    inline bool isCommitted() const
    { return status() == Committed; }

    inline bool isComplete() const
    { return status() == Complete; }

    inline bool isFaulted() const
    { return fault() != NoFault; }

    inline bool isReady() const
    { return remainingDependencies == 0; }

    inline bool isSquashed() const
    { return status() == Squashed; }

    /**
     * Notify all registered commit callback listeners that this in-flight
     * instruction has been committed.
     *
     * NOTE: The responsibility for calling this function falls with whoever
     *       is managing commit of this instruction.
     */
    void notifyCommitted();

    /**
     * Notify all registered completion callback listeners that this in-flight
     * instruction has completed execution (is ready to commit).
     *
     * NOTE: The responsibility for calling this function falls with whoever
     *       is managing execution of this instruction.
     */
    void notifyComplete();

    /**
     * Notify all registered ready callback listeners that this in-flight
     * instruction is ready (has no more outstanding dependencies).
     *
     * NOTE: The responsibility for calling this function falls with this
     *       class, so I'm actually pretty sure it doesn't need to be public.
     *       Only leaving here for now for consistency with notifyComplete, and
     *       in-case of new responsibilities for managing readiness.
     */
    void notifyReady();

    /**
     * Set this InflightInst to grab data for its execution from a particular
     * producer. Used to attach data dependencies in a way that can feed data
     * from a ROB or equivalent structure. Note that this function will not
     * actually add the dependency, and that guaranteeing that the producer has
     * completed execution when read should be done through a call to
     * addDependency().
     *
     * @param src_idx Which source of this instruction to set
     * @param producer What instruction will produce the value I need
     * @param res_idx Which of the items the producing instruction produces
     *  that I want
     */
    void setDataSource(int8_t src_idx, DataSource source);

    /**
     * Ask the InflightInst what its current sequence number is
     */
    inline InstSeqNum seqNum() const
    { return _seqNum; }

    /**
     * Set the sequence number of the InflightInst
     */
    inline InstSeqNum seqNum(InstSeqNum seqNum)
    { return _seqNum = seqNum; }

    inline void squash()
    { status(Squashed); }

    /**
     * Ask the InflightInst what its current status is
     */
    inline Status status() const
    { return _status; }

    /**
     * Set the status of the InflightInst
     */
    inline Status status(Status status)
    { return _status = status; }


    // BEGIN ExecContext interface functions

    IntReg readIntRegOperand(const StaticInst* si, int op_idx) override;
    void setIntRegOperand(const StaticInst* si, int dst_idx,
                          IntReg val) override;

    FloatReg readFloatRegOperand(const StaticInst* si, int op_idx) override;
    FloatRegBits readFloatRegOperandBits(const StaticInst* si,
                                            int op_idx) override;

    void setFloatRegOperand(const StaticInst* si, int dst_idx,
                            FloatReg val) override;
    void setFloatRegOperandBits(const StaticInst* si, int dst_idx,
                                FloatRegBits val) override;


    const VecRegContainer&
    readVecRegOperand(const StaticInst* si, int op_idx) const override;

    VecRegContainer&
    getWritableVecRegOperand(const StaticInst* si, int op_idx) override;

    void setVecRegOperand(const StaticInst* si, int dst_idx,
                          const VecRegContainer& val) override;


    ConstVecLane8
    readVec8BitLaneOperand(const StaticInst* si, int op_idx) const override;

    ConstVecLane16
    readVec16BitLaneOperand(const StaticInst* si, int op_idx) const override;

    ConstVecLane32
    readVec32BitLaneOperand(const StaticInst* si, int op_idx) const override;

    ConstVecLane64
    readVec64BitLaneOperand(const StaticInst* si, int op_idx) const override;

    void setVecLaneOperand(const StaticInst* si, int dst_idx,
                           const LaneData<LaneSize::Byte>& val) override;
    void setVecLaneOperand(const StaticInst* si, int dst_idx,
                           const LaneData<LaneSize::TwoByte>& val) override;
    void setVecLaneOperand(const StaticInst* si, int dst_idx,
                           const LaneData<LaneSize::FourByte>& val) override;
    void setVecLaneOperand(const StaticInst* si, int dst_idx,
                           const LaneData<LaneSize::EightByte>& val) override;

    VecElem readVecElemOperand(const StaticInst* si,
                               int op_idx) const override;
    void setVecElemOperand(const StaticInst* si, int dst_idx,
                           const VecElem val) override;

    CCReg readCCRegOperand(const StaticInst* si, int op_idx) override;
    void setCCRegOperand(const StaticInst* si, int dst_idx, CCReg val)
                         override;

    MiscReg readMiscRegOperand(const StaticInst* si, int op_idx) override;
    void setMiscRegOperand(const StaticInst* si, int dst_idx,
                           const MiscReg& val) override;

    MiscReg readMiscReg(int misc_reg) override;
    void setMiscReg(int misc_reg, const MiscReg& val) override;

    PCState pcState() const override;
    void pcState(const PCState& val) override;


    Fault readMem(Addr addr, uint8_t *data, unsigned int size,
                  Request::Flags flags) override;

    Fault initiateMemRead(Addr addr, unsigned int size, Request::Flags flags)
                          override;

    Fault writeMem(uint8_t* data, unsigned int size, Addr addr,
                   Request::Flags flags, uint64_t* res) override;


    void setStCondFailures(unsigned int sc_failures) override;

    unsigned int readStCondFailures() const override;

    void syscall(int64_t callnum, Fault* fault) override;

    ThreadContext* tcBase() override;

    /**
     * Alpha-Specific
     */
    Fault hwrei() override;
    bool simPalCheck(int palFunc) override;

    /**
     * ARM-specific
     */
    bool readPredicate() override;
    void setPredicate(bool val) override;

    /**
     * X86-specific
     */
    void demapPage(Addr vaddr, uint64_t asn) override;
    void armMonitor(Addr address) override;
    bool mwait(PacketPtr pkt) override;
    void mwaitAtomic(ThreadContext* tc) override;
    AddressMonitor* getAddrMonitor() override;

    /**
     * MIPS-Specific
     */
#if THE_ISA == MIPS_ISA
    MiscReg readRegOtherThread(const RegId& reg,
                               ThreadID tid = InvalidThreadID) override;
    void setRegOtherThread(const RegId& reg, MiscReg val,
                           ThreadID tid = InvalidThreadID) override;
#endif

    // END ExecContext interface functions
}; // END class InflightInst

#endif // __CPU_FLEXCPU_INFLIGHT_INST_HH__
