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
#include "sim/core.hh"
#include "sim/insttracer.hh"

/**
 * Dynamic instruction class for FlexCPU. Note that an InflightInst may only
 * refer to a microop for microcoded instructions (similarly to StaticInst),
 * so multiple InflightInsts (one per op) will make up one instruction in the
 * trace for microcoded instructions.
 *
 * An InflightInst can also be "empty" to serve as a slot on the buffer prior
 * to instruction decode.
 */
class InflightInst : public ExecContext,
                     public std::enable_shared_from_this<InflightInst>
{
  public:
    using PCState = TheISA::PCState;

    using VecRegContainer = TheISA::VecRegContainer;
    using VecElem = TheISA::VecElem;
    using VecPredRegContainer = TheISA::VecPredRegContainer;

    // Mutually exclusive states through which InflightInsts will transition.
    // The ordering allows the use of >= comparison operators for early states,
    // since later states imply that earlier states are either fulfilled or
    // were unnecessary.
    enum Status
    {
        Invalid = 0,
        Empty, // Dynamic instruction slot has been created but not yet filled.
        Decoded, // A StaticInst has been provided.
        // Perhaps an independent rename stage may be useful. Most
        // functionality conventionally called rename is packaged with issue
        // right now.
        Issued, // Dependencies and results have been identified and recorded.
        Executing, // Request for execution sent, but waiting for results.
        EffAddred, // Effective address calculated, but not yet sent to memory.
        Memorying, // Request for memory sent, but waiting for results.
        Complete, // Results have been received, but not yet committed.
        Committed, // Values have been committed, but this object might be
                   // alive for a little longer due to the shared_ptr being in
                   // the call-stack.
        NumInstStatus
    };

    struct TimingRecord
    {
        // When was this InflightInst object created?
        Tick creationTick;
        // When was this InflightInst object provided with a StaticInst?
        Tick decodeTick;
        // NOTE: may want to consider a rename stage.
        // When was this InflightInst object placed into consideration for
        // execution?
        Tick issueTick;
        // When did this InflightInst object get execution requested for it?
        Tick beginExecuteTick;
        // (For memory only) When did this InflightInst object have its
        // physical effective address(es) calculated?
        Tick effAddredTick;
        // (For memory only) When did this InflightInst object get memory
        // requested for it?
        Tick beginMemoryTick;
        // When were all steps for computing the state changes that this
        // InflightInst object will apply to committed state completed?
        Tick completionTick;
        // When was this InflightInst finally applied to committed state?
        Tick commitTick;
        // If squashed, when was this InflightInst squashed?
        Tick squashTick;
    };

    // Plain old container for storing where this instruction should retrieve
    // data from, during execution. If the producer weak_ptr expires, it
    // implies that the data was already committed and should be retrieved from
    // the backing ThreadContext. Otherwise it might have been squashed, in
    // which case the consuming instruction should not be executed anyway.
    struct DataSource
    {
        std::weak_ptr<InflightInst> producer;
        int8_t resultIdx;
    };

    // In order to keep InflightInst well-encapsulated and cpu-agnostic, we
    // can't implement functions which depend on structures like memory ports
    // internally, so we expect that users of this class provide
    // implementations of these ExecContext functions for memory instructions,
    // via this polymorphic interface.
    class MemIface
    {
      public:
        virtual Fault readMem(std::shared_ptr<InflightInst> inst, Addr addr,
                              uint8_t *data, unsigned int size,
                              Request::Flags flags,
                              const std::vector<bool>& byte_enable)
        { panic("MemIface::readMem() not implemented!"); }

        virtual Fault initiateMemRead(std::shared_ptr<InflightInst> inst,
                                      Addr addr, unsigned int size,
                                      Request::Flags flags,
                                      const std::vector<bool>& byte_enable)
        { panic("MemIface::initiateMemRead() not implemented!"); }

        virtual Fault writeMem(std::shared_ptr<InflightInst> inst,
                               uint8_t *data, unsigned int size, Addr addr,
                               Request::Flags flags, uint64_t *res,
                               const std::vector<bool>& byte_enable)
        { panic("MemIface::writeMem() not implemented!"); }

        static const MemIface unimplemented;
    };

    class X86Iface
    {
      public:
        virtual void demapPage(Addr vaddr, uint64_t asn)
        { panic("X86Iface::demapPage() not implemented!"); }
        virtual void armMonitor(Addr address)
        { panic("X86Iface::armMonitor() not implemented!"); }
        virtual bool mwait(PacketPtr pkt)
        { panic("X86Iface::mwait() not implemented!"); }
        virtual void mwaitAtomic(ThreadContext *tc)
        { panic("X86Iface::mwaitAtomic() not implemented!"); }
        virtual AddressMonitor *getAddrMonitor()
        { panic("X86Iface::getAddrMonitor() not implemented!"); }

        static const X86Iface unimplemented;
    };

    // Templatizing these interfaces will allow functions to be linked by the
    // compiler statically instead of requiring virtual function tables to be
    // traversed at runtime. Should TODO later.

  protected:
    ThreadContext* backingContext;
    TheISA::ISA* backingISA;
    MemIface* backingMemoryInterface;
    X86Iface* backingX86Interface;

    // Where am I in completing this instruction?
    Status _status;
    bool _squashed = false;

    // What is this instruction I'm executing?
    // _seqNum: Commit sequence number, consistent across deterministic
    //          executions.
    InstSeqNum _seqNum;
    // _issueSeqNum: Issue sequence number, assigned to all instructions that
    //               reach the issue stage, allows for unique numbers for
    //               potentially squashed instructions as well.
    InstSeqNum _issueSeqNum;
    StaticInstPtr instRef;

    /**
     * A storage for the exact PCState at the time of executing this
     * instruction. Any changes to the PC through the instruction can also be
     * captured in this variable, allowing this variable also to serve as the
     * result storage for changes to the PCState. If this variable is changed
     * in this fashion however, we may need to flush speculative state.
     */
    PCState _pcState;

    Trace::InstRecord* _traceData = nullptr;

    TimingRecord _timingRecord;

    // Callbacks made during a state transition.
    std::vector<std::function<void()>> beginExecCallbacks;
    std::vector<std::function<void()>> commitCallbacks;
    std::vector<std::function<void()>> completionCallbacks;

    std::vector<std::function<void()>> effAddrCalculatedCallbacks;

    // How many dependencies have to be resolved before I can execute?
    size_t remainingDependencies = 0;

    // How many dependencies have to be resolved before I can go to memory?
    size_t remainingMemDependencies = 0;

    std::vector<std::function<void()>> readyCallbacks;
    std::vector<std::function<void()>> memReadyCallbacks;

    std::vector<std::function<void()>> retireCallbacks;

    std::vector<std::function<void()>> squashCallbacks;

    std::vector<DataSource> sources;
    std::vector<GenericReg> results;
    std::vector<bool> resultValid;

    /**
     * For use in storing results of writes to miscRegs that don't go through
     * the operands of the StaticInst.
     */
    std::vector<RegVal> miscResultVals;
    std::vector<int> miscResultIdxs;

    Fault _fault = NoFault;

    // For use in tracking dependencies through memory
    bool accessedPAddrsValid = false;
    AddrRange accessedPAddrs;

    bool _isSplitMemReq = false;
    bool accessedSplitPAddrsValid = false;
    AddrRange accessedSplitPAddrs; // Second variable to store range for second
                                   // request as part of split accesses

    // Predicate flags, used by ARM ISA
    bool predicate;
    bool memAccPredicate;
  public:
    InflightInst(ThreadContext* backing_context, TheISA::ISA* backing_isa,
                 MemIface* backing_mem_iface, X86Iface* backing_x86_iface,
                 InstSeqNum seq_num, const TheISA::PCState& pc_,
                 StaticInstPtr inst_ref = StaticInst::nullStaticInstPtr);

    // Unimplemented copy due to presence of raw pointers.
    InflightInst(const InflightInst& other) = delete;

    virtual ~InflightInst();

    void addBeginExecCallback(std::function<void()> callback);
    void addBeginExecDependency(InflightInst& parent);
    /**
     * Tells this InflightInst to call a particular function when it has been
     * made aware of a commit.
     *
     * NOTE: The order in which callbacks are added will determine the order in
     *       which the callbacks are called upon notify. This is a hard promise
     *       made by InflightInst, in order to provide some control on ordering
     *       for any functionality where ordering is important.
     *
     * @param callback The function to call.
     */
    void addCommitCallback(std::function<void()> callback);
    // Adds a dependency between the commit stage of the parent and the
    // execution stage of this dependency.
    void addCommitDependency(InflightInst& parent);
    void addCompletionCallback(std::function<void()> callback);

    /**
     * If parent has not yet completed and is not squashed, then this will
     * increment an internal counter, which will be decremented when parent
     * completes. When all dependencies have been satisfied (counter reaches
     * 0), notifyReady() will be called.
     *
     * NOTE: This function uses the callback system to implement dependencies,
     *       so if callback order is important, pay attention to the order in
     *       which you add callbacks AND dependencies.
     *
     * All dependencies we support are on a stage completion->stage begin
     * basis, where the younger instruction has a dependency counter on the
     * appropriate stage, and the older instruction has a stage-callback for
     * the driving stage, which decreases this counter, and launches the stage
     * if the counter reaches 0.
     *
     * @param parent The other instruction whose completion this instruction is
     *  dependent on.
     */
    void addDependency(InflightInst& parent);

    void addEffAddrCalculatedCallback(std::function<void()> callback);
    void addMemReadyCallback(std::function<void()> callback);
    void addMemCommitDependency(InflightInst& parent);
    void addMemDependency(InflightInst& parent);
    void addMemEffAddrDependency(InflightInst& parent);
    void addReadyCallback(std::function<void()> callback);
    /**
     * Retire callbacks occur on either commit or squash (Their purpose) is to
     * allow for use of callbacks when the instruction is removed from a
     * primary buffer, regardless of cause. They can be expected to be called
     * only once, since commit and squash are mutually exclusive.
     *
     * @param callback The function to call.
     */
    void addRetireCallback(std::function<void()> callback);
    void addSquashCallback(std::function<void()> callback);
    // May be useful to add ability to remove a callback. Will be difficult if
    // we use raw function objects, could be resolved by holding pointers?

    /**
     * Takes all state-changing effects that this InflightInst has, and applies
     * them to the backing ThreadContext of this InflightInst.
     *
     * Called during commit, to make the effects of this instruction permanent.
     */
    void commitToTC();

    /**
     * For memory instructions only, checks if the addresses accessed by this
     * instruction or other have any intersection. This operation should be
     * commutative.
     *
     * @param other the instruction to check against.
     */
    bool effAddrOverlap(const InflightInst& other) const;

    inline void effPAddrRange(AddrRange range)
    {
        assert(range.valid());
        accessedPAddrsValid = true;
        accessedPAddrs = range;
    }
    inline void effPAddrRangeHigh(AddrRange range)
    {
        assert(_isSplitMemReq && range.valid());
        accessedSplitPAddrsValid = true;
        accessedSplitPAddrs = range;
    }

    /**
     * Checks whether this instruction's accessed physical addresses are a
     * superset of the other instruction's accessed physical addresses. This
     * function should only be called if both instructions have had their
     * effective physical addresses calculated.
     *
     * @param other The other instruction to compare to
     * @return Whether this instruction's accessed effective addresses are a
     *         superset of the other's.
     */
    bool effPAddrSuperset(const InflightInst& other) const;

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

    inline const TimingRecord& getTimingRecord() const
    { return _timingRecord; }

    // Note: due to the sequential nature of the status items, a later status
    //       naturally implies earlier statuses have been met. (e.g. A complete
    //       memory instruction must have already had its effective address
    //       calculated)
    inline bool isCommitted() const
    { return status() >= Committed; }

    inline bool isComplete() const
    { return status() >= Complete; }

    inline bool isEffAddred() const
    { return status() >= EffAddred; }

    inline bool isExecuting() const
    { return status() >= Executing; }

    inline bool isFaulted() const
    { return fault() != NoFault; }

    inline bool isMemorying() const
    { return status() >= Memorying; }

    // For sending through memory port. Memory instructions only.
    inline bool isMemReady()
    { return remainingMemDependencies == 0; }

    // For execution.
    inline bool isReady() const
    { return remainingDependencies == 0; }

    inline bool isSplitMemReq() const
    { return _isSplitMemReq; }
    inline bool isSplitMemReq(bool split)
    { return _isSplitMemReq = split; }

    inline bool isSquashed() const
    { return _squashed; }

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
     * Notify all registered effective address callback listeners that this
     * in-flight instruction has had its effective address(es) calculated.
     *
     * NOTE: The responsibility for calling this function falls with whoever
     *       is calculating the effective address of this MEMORY instruction.
     */
    void notifyEffAddrCalculated();

    /**
     * Notify this InflightInst that it has been decoded.
     */
    void notifyDecoded();

    /**
     * Notify this InflightInst that it has been sent for execution.
     */
    void notifyExecuting();

    /**
     * Notify this InflightInst that it has been issued.
     */
    void notifyIssued();

    /**
     * Notify this InflightInst that it has been sent to memory, and is in the
     * process of awaiting a response.
     */
    void notifyMemorying();

    /**
     * Notify all registered memory ready callback listeners that this
     * in-flight instruction is ready to send to memory.
     *
     * NOTE: The responsibility for calling this function falls with this
     *       class' internal dependency system.
     */
    void notifyMemReady();

    /**
     * Notify all registered ready callback listeners that this in-flight
     * instruction is ready to execute (has no more outstanding dependencies).
     *
     * NOTE: The responsibility for calling this function falls with this
     *       class' internal dependency system.
     */
    void notifyReady();

    /**
     * Notify all registered squash callback listeners that this in-flight
     * instruction has been squashed.
     *
     * NOTE: The responsibility for calling this function falls with whoever
     *       is managing squashing of this instruction.
     */
    void notifySquashed();

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
     * Ask the InflightInst what its current (committed) sequence number is
     */
    inline InstSeqNum seqNum() const
    { return _seqNum; }

    /**
     * Ask the InflightInst what its current (issued) sequence number is
     */
    inline InstSeqNum issueSeqNum() const
    { return _issueSeqNum; }

    /**
     * Set the sequence number of the InflightInst
     */
    inline InstSeqNum seqNum(InstSeqNum seqNum)
    { return _seqNum = seqNum; }

    /**
     * Set the sequence number of the InflightInst
     */
    inline InstSeqNum issueSeqNum(InstSeqNum seqNum)
    { return _issueSeqNum = seqNum; }

    inline const StaticInstPtr& staticInst() const
    { return instRef; }
    const StaticInstPtr& staticInst(const StaticInstPtr& inst_ref);

    /**
     * Ask the InflightInst what its current status is
     */
    inline Status status() const
    { return _status; }

    /**
     * Set the status of the InflightInst
     */
    inline Status status(Status status)
    {
        return _status = status;
    }

    inline Trace::InstRecord* traceData() const
    { return _traceData; }
    inline Trace::InstRecord* traceData(Trace::InstRecord* const trace_data)
    { return _traceData = trace_data; }


    // BEGIN ExecContext interface functions

    RegVal readIntRegOperand(const StaticInst* si, int op_idx) override;
    void setIntRegOperand(const StaticInst* si, int dst_idx,
                          RegVal val) override;

    RegVal readFloatRegOperandBits(const StaticInst* si, int op_idx) override;
    void setFloatRegOperandBits(const StaticInst* si, int dst_idx,
                                RegVal val) override;


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

    const VecPredRegContainer&
    readVecPredRegOperand(const StaticInst *si, int idx) const override;

    VecPredRegContainer&
    getWritableVecPredRegOperand(const StaticInst *si, int idx) override;

    void
    setVecPredRegOperand(const StaticInst *si, int idx,
                         const VecPredRegContainer& val) override;

    RegVal readCCRegOperand(const StaticInst* si, int op_idx) override;
    void setCCRegOperand(const StaticInst* si, int dst_idx, RegVal val)
                         override;

    RegVal readMiscRegOperand(const StaticInst* si, int op_idx) override;
    void setMiscRegOperand(const StaticInst* si, int dst_idx,
                           RegVal val) override;

    RegVal readMiscReg(int misc_reg) override;
    void setMiscReg(int misc_reg, RegVal val) override;

    PCState pcState() const override;
    void pcState(const PCState& val) override;


    Fault readMem(Addr addr, uint8_t *data, unsigned int size,
                  Request::Flags flags,
                  const std::vector<bool>& byte_enable = {}) override;

    Fault initiateMemRead(Addr addr, unsigned int size, Request::Flags flags,
                          const std::vector<bool>& byte_enable = {}) override;

    Fault writeMem(uint8_t* data, unsigned int size, Addr addr,
                   Request::Flags flags, uint64_t* res,
                   const std::vector<bool>& byte_enable = {}) override;


    void setStCondFailures(unsigned int sc_failures) override;

    unsigned int readStCondFailures() const override;

    void syscall(int64_t callnum, Fault* fault) override;

    ThreadContext* tcBase() override;

    /**
     * ARM-specific
     */
    bool readPredicate() const override;
    void setPredicate(bool val) override;
    bool readMemAccPredicate() const override;
    void setMemAccPredicate(bool val) override;

    /**
     * X86-specific
     */
    void demapPage(Addr vaddr, uint64_t asn) override;
    void armMonitor(Addr address) override;
    bool mwait(PacketPtr pkt) override;
    void mwaitAtomic(ThreadContext* tc) override;
    AddressMonitor* getAddrMonitor() override;

    // END ExecContext interface functions
}; // END class InflightInst

#endif // __CPU_FLEXCPU_INFLIGHT_INST_HH__
