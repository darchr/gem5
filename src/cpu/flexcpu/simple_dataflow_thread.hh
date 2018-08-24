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

#ifndef __CPU_FLEXCPU_SIMPLE_DATAFLOW_THREAD_HH__
#define __CPU_FLEXCPU_SIMPLE_DATAFLOW_THREAD_HH__

#include <list>
#include <memory>
#include <set>
#include <unordered_map>

#include "cpu/exec_context.hh"
#include "cpu/flexcpu/generic_reg.hh"
#include "cpu/flexcpu/inflight_inst.hh"
#include "cpu/flexcpu/simple_dataflow_cpu.hh"
#include "cpu/inst_seq.hh"
#include "cpu/reg_class.hh"
#include "cpu/simple_thread.hh"
#include "mem/request.hh"

class SimpleDataflowCPU;

/**
 * Instances of this class represent a logical "Hyperthread" within the
 * SimpleDataflowCPU. A thread contains an independent complete architectural
 * state, and in this case, also the logical control flow.
 */
class SDCPUThread : public ThreadContext
{
  protected:
    // BEGIN SDCPUThread Internal definitions

    class MemIface : public InflightInst::MemIface
    {
      protected:
        SDCPUThread& sdCPUThread;
      public:
        MemIface(SDCPUThread& sdCPUThread_):
            sdCPUThread(sdCPUThread_)
        { }

        Fault initiateMemRead(std::shared_ptr<InflightInst> inst, Addr addr,
                             unsigned int size, Request::Flags flags) override;

        Fault writeMem(std::shared_ptr<InflightInst> inst, uint8_t *data,
                       unsigned int size, Addr addr, Request::Flags flags,
                       uint64_t *res) override;
    };

    struct SplitRequest {
        RequestPtr main = nullptr;
        RequestPtr low = nullptr;
        RequestPtr high = nullptr;
    };

    // END SDCPUThread Internal definitions


    // BEGIN SDCPUThread Internal variables

    /**
     * We implement the memory interface of the InflightInst so that we can
     * capture calls to request memory access via our CPU.
     */
    MemIface memIface;


    // BEGIN Internal parameters

    /**
     * We hold a pointer to the CPU which owns this logical thread.
     */
    SimpleDataflowCPU* _cpuPtr;

    /**
     * We store a name for use with tracing
     */
    std::string _name;

    /**
     * Controls behavior of StaticInsts marked with the various serializing
     * flags
     */
    bool strictSerialize;

    // END Internal parameters


    // BEGIN Solid architectural state

    /**
     * The SimpleThread class already contains all internal state necessary to
     * hold onto the state of a basic CPU, demonstrated by its use in many
     * other CPU models. Until we find a significant need to hold particular
     * permanent committed state different from a regular CPU in this library,
     * there is no reason to redefine the wheel.
     *
     * The committed state represents the state resulting from the complete
     * correct execution of all instructions up to the point represented by
     * numInstsCommitted.
     */
    SimpleThread* _committedState;

    /**
     * We hold an ISA pointer of our own because _committedState doesn't expose
     * its.
     */
    TheISA::ISA* isa;

    /**
     * We hold onto the InstSeqNum of the last instruction successfully
     * committed, for ease of checking if older instruction dependencies have
     * already been satisfied.
     */
    InstSeqNum lastCommittedInstNum = 0;

    // END Solid architectural state


    // BEGIN Speculative state

    /**
     * For use when we need to fetch multiple MachInsts for one decode.
     */
    Addr fetchOffset = 0;

    /**
     * Number of fetch actions we have begun and not finished at this time
     */
    size_t outstandingFetches = 0;

    /**
     * We hold onto any macroop StaticInstPtrs we detect, to serve as providers
     * of microops instead of fetching from memory for the duration of the
     * macroop.
     */
    StaticInstPtr curMacroOp = StaticInst::nullStaticInstPtr;

    /**
     * We hold a decoder outside of _committedState even though SimpleThread
     * has a decoder object inside, because we are decoding instructions that
     * have not been committed yet, and we want to keep committed and
     * speculative state separate.
     */
    TheISA::Decoder decoder;

    /**
     * We hold a list of all in-flight instructions, added during issue and
     * removed after commit. If we always issue in order, this list should
     * always be sorted, in the order that the instructions should be committed
     * so replacing this with a queue might be a better idea hmm.
     */
    std::list<std::shared_ptr<InflightInst>> inflightInsts;

    /**
     * For quick dependency extraction, we keep track of the most recent
     * write accesses to each unique register.
     */
    std::unordered_map<RegId, InflightInst::DataSource> lastUses;


    // END Speculative state
    // END SDCPU Internal variables


    // BEGIN Internal functions

    /**
     * Entry point to begin work on a new instruction. Will also begin any
     * related events, such as onBeginFetch to kick off any upcoming tasks
     * necessary to complete that instruction. May issue an instruction
     * immediately if no new information is needed to set up a new instruction.
     */
    void advanceInst();

    /**
     * Utility function for iterating through the in-flight instruction table
     * and committing all instructions at once (removing them from the table).
     * Will also begin fault handling if a faulting instruction is found,
     * instead of committing the faulting instruction.
     *
     * After this function is called, the inflightInsts table should be either
     * empty, or its head should be a non-squashed, incomplete instruction.
     */
    void commitAllAvailableInstructions();

    /**
     * Utility function for committing the results of a previously in-flight
     * instruction. This function should update the _committedState using the
     * in-flight instruction's stored results, increment numInstsCommitted, and
     * make no further changes to the thread state.
     */
    void commitInstruction(std::shared_ptr<InflightInst> inst);

    /**
     * Utility function for making a request for the CPU to execute an
     * instruction. This function should be called for all instructions once
     * and only once, at the earliest time after issue that all dependencies
     * are satisfied.
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  request execution.
     */
    void executeInstruction(std::weak_ptr<InflightInst> inst);

    /**
     * Retrieves the PC value from either the last instruction on the in-flight
     * queue, or from the committed state if the queue is empty. Should not be
     * used while the latest upcoming PC value is not yet knowable.
     *
     * @return The PC value that should be used for any upcoming fetch/decode
     *  actions
     */
    TheISA::PCState getNextPC();

    /**
     * Utility function for handling the fault generated by a particular
     * instruction, instead of committing the value to the _committedState.
     */
    void handleFault(std::shared_ptr<InflightInst> inst_ptr);

    /**
     * This function has the job of populating an instruction's fields with
     * dependencies, state, and behavior. It will ensure that all the state
     * internal to the InflightInst is ready, such that when no dependencies
     * exist, the instruction can be executed.
     *
     * After this stage, after the instruction has effectively been "scheduled"
     * internally. If all of the instruction's dependencies are available at
     * issue time, then the instruction's execution should immediately be
     * requested from the CPU.
     *
     * (Note: Assumes in-order issue. This implies that the weak_ptr should
     *        match the pointer at the tail of the inflightInsts table.)
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  request issue.
     */
    void issueInstruction(std::weak_ptr<InflightInst> inst);

    /**
     * Utility function for taking note of a fault, and preemptively squashing
     * instructions that we know will not be committed as a result of the
     * fault. If no instructions prior to inst produce a fault, then this fault
     * can be expected to be handled at commit time.
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  mark a fault.
     * @param fault The fault that should be associated with the instruction.
     */
    void markFault(std::shared_ptr<InflightInst> inst, Fault fault);

    /**
     * Entry point to fetch functionality. May be called more than once for a
     * particular instruction, due to instructions spanning across fetch
     * boundaries.
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  request fetch data.
     */
    void onBeginFetch(std::weak_ptr<InflightInst> inst);

    /**
     * This function serves as the event handler for when the CPU has completed
     * the translation we requested for an instruction that does memory access.
     *
     * This is an extra step which will only be used for memory accessing
     * instructions.
     *
     * This function should be called after a call to executeInstruction() for
     * a corresponding memory accessing InflightInst, which should make a call
     * to either initiateMemRead() or writeMem().
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  handle the given translation.
     * @param req The request object given to the DTB, which should now hold
     *  both a valid virtual and physical address.
     * @param write Whether the translation was requested for a write. The only
     *  other possibility in this implementation is a read.
     * @param data For writes, we also need to carry the data given by the
     *  producing instruction through to be able to send a write packet.
     * @param sreq If we have a split request, this object reference will help
     *  to keep track of the components of the split request.
     * @param high For split requests, we will have two calls to this function
     *  for the two translations. This parameter distinguishes which
     *  translation this callback is for.
     */
    void onDataAddrTranslated(std::weak_ptr<InflightInst> inst, Fault fault,
                              const RequestPtr& req, bool write,
                              uint8_t* data,
                              std::shared_ptr<SplitRequest> sreq = nullptr,
                              bool high = false);

    /**
     * This function serves as the event handler for when the CPU completes the
     * execution of an instruction for which we previously requested execution.
     *
     * This function should be called after a call to executeInstruction() for
     * the corresponding InflightInst, which should make a call to
     * requestExecution() on the CPU. The order in which these results come in
     * may not be obvious, since their execution may be dependent on scheduling
     * or structural hazards, depending on the CPU implementation. They are
     * guaranteed to occur AFTER the request is sent, at least.
     *
     * This function will also assume that any necessary calls to
     * StaticInst::execute() or StaticInst::completeAcc() have already been
     * made by the time this function is called.
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  handle the completion of execution.
     * @param fault If execution of the instruction produced a fault, it will
     *  be passed to us here.
     */
    void onExecutionCompleted(std::weak_ptr<InflightInst> inst, Fault fault);

    /**
     * This function serves as the event handler for when the CPU has provided
     * us with the data we requested to be fetched.
     *
     * This function should be called after onPCTranslated().
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  handle new instruction data.
     * @param fetch_data The chunk of data that was fetched from memory.
     */
    void onInstDataFetched(std::weak_ptr<InflightInst> inst,
                              const TheISA::MachInst fetch_data);

    /**
     * This function serves as the event handler for when the CPU has completed
     * the translation we requested for fetching purposes.
     *
     * This function should be called after onBeginFetch().
     *
     * @param inst A reference to the in-flight instruction object for which to
     *  handle the given translation.
     * @param fault If the translation request generated a fault, it should be
     *  returned through this parameter.
     * @param req A reference to a request object which should hold the
     *  translated physical address.
     */
    void onPCTranslated(std::weak_ptr<InflightInst> inst, Fault fault,
                        const RequestPtr& req);

    /**
     * This function detects any dependencies that a new instruction has, if it
     * were to be issued at the current moment, considering the state of all
     * currently active InflightInsts. The dependencies should be added to this
     * instruction via its addDependency() interface function.
     *
     * @param inst The instruction for which we want to detect dependencies.
     */
    void populateDependencies(std::shared_ptr<InflightInst> inst);

    /**
     * This function iterates through the registers that this instruction
     * writes and associates the registers with the corresponding instructions
     * in order to streamline dependency tracking. Like a scoreboard.
     *
     * @param inst The instruction for which we want to track uses.
     */
    void populateUses(std::shared_ptr<InflightInst> inst);

    /**
     * This function checks if the thread should be active at this moment, and
     * whether or not we have a reasonable PC value ready to be able to
     * initiate another fetch at this time.
     *
     * @return Whether we can start a new fetch at this point in time.
     */
    bool hasNextPC();

    // END Internal functions

    // BEGIN statistics

    Counter numInsts = 0;
    Counter numOps = 0;
    Stats::Scalar numInstsStat;
    Stats::Scalar numOpsStat;

    // END Statistics

  public:

    // Fullsystem mode constructor
    SDCPUThread(SimpleDataflowCPU* cpu_, ThreadID tid_, System* system_,
                     BaseTLB* itb_, BaseTLB* dtb_, TheISA::ISA* isa_,
                     bool use_kernel_stats_, bool strict_ser);

    // Non-fullsystem constructor
    SDCPUThread(SimpleDataflowCPU* cpu_, ThreadID tid_, System* system_,
                     Process* process_, BaseTLB* itb_, BaseTLB* dtb_,
                     TheISA::ISA* isa_, bool strict_ser);

    // May need to define move constructor, due to how SimpleThread is defined,
    // if we want to hold instances of these in a vector instead of pointers
    // gotten from new.

    virtual ~SDCPUThread();

    ThreadContext* getThreadContext();

    const std::string& name() const
    { return _name; }

    void startup();

    // BEGIN ThreadContext functions

    BaseCPU *getCpuPtr() override { return _committedState->getCpuPtr(); }

    int cpuId() const override { return _committedState->cpuId(); }

    uint32_t socketId() const override { return _committedState->socketId(); }

    int threadId() const override { return _committedState->threadId(); }

    void setThreadId(int id) override { _committedState->setThreadId(id); }

    int contextId() const override { return _committedState->contextId(); }

    void setContextId(int id) override { _committedState->setContextId(id); }

    BaseTLB *getITBPtr() override { return _committedState->getITBPtr(); }

    BaseTLB *getDTBPtr() override { return _committedState->getDTBPtr(); }

    CheckerCPU *getCheckerCpuPtr() override
    { return _committedState->getCheckerCpuPtr(); }

    TheISA::Decoder *getDecoderPtr() override
    { return &decoder; }

    System *getSystemPtr() override { return _committedState->getSystemPtr(); }

    TheISA::Kernel::Statistics *getKernelStats() override
    { return _committedState->getKernelStats(); }

    PortProxy &getPhysProxy() override
    { return _committedState->getPhysProxy(); }

    FSTranslatingPortProxy &getVirtProxy() override
    { return _committedState->getVirtProxy(); }

    void initMemProxies(ThreadContext *tc) override
    { _committedState->initMemProxies(tc); }

    SETranslatingPortProxy &getMemProxy() override
    { return _committedState->getMemProxy(); }

    Process *getProcessPtr() override
    { return _committedState->getProcessPtr(); }

    void setProcessPtr(Process *p) override
    { _committedState->setProcessPtr(p); }

    Status status() const override { return _committedState->status(); }

    void setStatus(Status new_status) override
    { _committedState->setStatus(new_status); }

    /// Set the status to Active.
    void activate() override;

    /// Set the status to Suspended.
    void suspend() override { _committedState->suspend(); }

    /// Set the status to Halted.
    void halt() override { _committedState->halt(); }

    void dumpFuncProfile() override { _committedState->dumpFuncProfile(); }

    void takeOverFrom(ThreadContext *oldContext) override
    { _committedState->takeOverFrom(oldContext); }

    /**
     * Register thread-specific stats.
     * The BaseCPU calls this function in its regStats with the thread-
     * specific name
     *
     * @param name from the CPU for this thread
     */
    void regStats(const std::string &name) override;

    EndQuiesceEvent *getQuiesceEvent() override
    { return _committedState->getQuiesceEvent(); }

    Tick readLastActivate() override
    { return _committedState->readLastActivate(); }
    Tick readLastSuspend() override
    { return _committedState->readLastSuspend(); }

    void profileClear() override { return _committedState->profileClear(); }
    void profileSample() override { return _committedState->profileSample(); }

    // @todo: Do I need this?
    void copyArchRegs(ThreadContext *tc) override
    { _committedState->copyArchRegs(tc); }

    void clearArchRegs() override { _committedState->clearArchRegs(); }

    //
    // New accessors for new decoder.
    //
    uint64_t readIntReg(int reg_idx) override
    { return _committedState->readIntReg(reg_idx); }

    FloatReg readFloatReg(int reg_idx) override
    { return _committedState->readFloatReg(reg_idx); }

    FloatRegBits readFloatRegBits(int reg_idx) override
    { return _committedState->readFloatRegBits(reg_idx); }

    const VecRegContainer& readVecReg(const RegId& reg) const override
    { return _committedState->readVecReg(reg); }

    VecRegContainer& getWritableVecReg(const RegId& reg) override
    { return _committedState->getWritableVecReg(reg); }

    /** Vector Register Lane Interfaces. */
    /** @{ */
    /** Reads source vector 8bit operand. */
    ConstVecLane8
    readVec8BitLaneReg(const RegId& reg) const override
    { return _committedState->readVec8BitLaneReg(reg); }

    /** Reads source vector 16bit operand. */
    ConstVecLane16
    readVec16BitLaneReg(const RegId& reg) const override
    { return _committedState->readVec16BitLaneReg(reg); }

    /** Reads source vector 32bit operand. */
    ConstVecLane32
    readVec32BitLaneReg(const RegId& reg) const override
    { return _committedState->readVec32BitLaneReg(reg); }

    /** Reads source vector 64bit operand. */
    ConstVecLane64
    readVec64BitLaneReg(const RegId& reg) const override
    { return _committedState->readVec64BitLaneReg(reg); }

    /** Write a lane of the destination vector register. */
    virtual void setVecLane(const RegId& reg,
            const LaneData<LaneSize::Byte>& val) override
    { return _committedState->setVecLane(reg, val); }
    virtual void setVecLane(const RegId& reg,
            const LaneData<LaneSize::TwoByte>& val) override
    { return _committedState->setVecLane(reg, val); }
    virtual void setVecLane(const RegId& reg,
            const LaneData<LaneSize::FourByte>& val) override
    { return _committedState->setVecLane(reg, val); }
    virtual void setVecLane(const RegId& reg,
            const LaneData<LaneSize::EightByte>& val) override
    { return _committedState->setVecLane(reg, val); }
    /** @} */

    const VecElem& readVecElem(const RegId& reg) const override
    { return _committedState->readVecElem(reg); }

    CCReg readCCReg(int reg_idx) override
    { return _committedState->readCCReg(reg_idx); }

    void setIntReg(int reg_idx, uint64_t val) override
    { _committedState->setIntReg(reg_idx, val); }

    void setFloatReg(int reg_idx, FloatReg val) override
    { _committedState->setFloatReg(reg_idx, val); }

    void setFloatRegBits(int reg_idx, FloatRegBits val) override
    { _committedState->setFloatRegBits(reg_idx, val); }

    void setVecReg(const RegId& reg, const VecRegContainer& val) override
    { _committedState->setVecReg(reg, val); }

    void setVecElem(const RegId& reg, const VecElem& val) override
    { _committedState->setVecElem(reg, val); }

    void setCCReg(int reg_idx, CCReg val) override
    { _committedState->setCCReg(reg_idx, val); }

    TheISA::PCState pcState() override { return _committedState->pcState(); }

    void pcState(const TheISA::PCState &val) override
    { _committedState->pcState(val); }

    void pcStateNoRecord(const TheISA::PCState &val) override
    { _committedState->pcState(val); }

    Addr instAddr() override { return _committedState->instAddr(); }
    Addr nextInstAddr() override { return _committedState->nextInstAddr(); }
    MicroPC microPC() override { return _committedState->microPC(); }

    MiscReg readMiscRegNoEffect(int misc_reg) const override
    { return _committedState->readMiscRegNoEffect(misc_reg); }

    MiscReg readMiscReg(int misc_reg) override
    { return _committedState->readMiscReg(misc_reg); }

    void setMiscRegNoEffect(int misc_reg, const MiscReg &val) override
    { return _committedState->setMiscRegNoEffect(misc_reg, val); }

    void setMiscReg(int misc_reg, const MiscReg &val) override
    { return _committedState->setMiscReg(misc_reg, val); }

    RegId flattenRegId(const RegId& regId) const override
    { return _committedState->flattenRegId(regId); }

    unsigned readStCondFailures() override
    { return _committedState->readStCondFailures(); }

    void setStCondFailures(unsigned sc_failures) override
    { _committedState->setStCondFailures(sc_failures); }

    void syscall(int64_t callnum, Fault *fault) override
    { _committedState->syscall(callnum, fault); }

    Counter readFuncExeInst() override
    { return _committedState->readFuncExeInst(); }

    uint64_t readIntRegFlat(int idx) override
    { return _committedState->readIntRegFlat(idx); }

    void setIntRegFlat(int idx, uint64_t val) override
    { _committedState->setIntRegFlat(idx, val); }

    FloatReg readFloatRegFlat(int idx) override
    { return _committedState->readFloatRegFlat(idx); }

    void setFloatRegFlat(int idx, FloatReg val) override
    { _committedState->setFloatRegFlat(idx, val); }

    FloatRegBits readFloatRegBitsFlat(int idx) override
    { return _committedState->readFloatRegBitsFlat(idx); }

    void setFloatRegBitsFlat(int idx, FloatRegBits val) override
    { _committedState->setFloatRegBitsFlat(idx, val); }

    const VecRegContainer& readVecRegFlat(int id) const override
    { return _committedState->readVecRegFlat(id); }

    VecRegContainer& getWritableVecRegFlat(int id) override
    { return _committedState->getWritableVecRegFlat(id); }

    void setVecRegFlat(int idx, const VecRegContainer& val) override
    { _committedState->setVecRegFlat(idx, val); }

    const VecElem& readVecElemFlat(const RegIndex& id,
                                   const ElemIndex& elemIndex) const override
    { return _committedState->readVecElemFlat(id, elemIndex); }

    void setVecElemFlat(const RegIndex& id, const ElemIndex& elemIndex,
                        const VecElem& val) override
    { _committedState->setVecElemFlat(id, elemIndex, val); }

    CCReg readCCRegFlat(int idx) override
    { return _committedState->readCCRegFlat(idx); }

    void setCCRegFlat(int idx, CCReg val) override
    { _committedState->setCCRegFlat(idx, val); }

    // END ThreadContext functions
};

#endif // __CPU_FLEXCPU_SIMPLE_DATAFLOW_THREAD_HH__
