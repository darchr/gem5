/*
 * Copyright (c) 2018 Jason Lowe-Power
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

#include "cpu/thread_context.hh"
#include "learning_gem5/part4/register_file.hh"

class LSCThreadContext : public ThreadContext
{
  private:
    LearningSimpleCPU *cpu;

    System *system;

    /// For SE mode when emulating processes
    Process *process;

    /// System-wide context id. Used for marking memory requests with the
    /// originating requestor.
    ContextID contextId;

    /// The id for this context (0 since we don't support multithreading)
    ThreadID threadId;

    /// These are the architectural state
    TheISA::PCState pcState;
    RegisterFile registers;
    TheISA::Decoder decoder;

    /// Pointer to the ISA object which holds things
    TheISA::ISA *isa;

    ThreadContext::Status status;

    /// For special accesses (in FS mode?)
    /** A port proxy outgoing only for functional accesses to physical
     * addresses.*/
    PortProxy *physProxy;

    /** A translating port proxy, outgoing only, for functional
     * accesse to virtual addresses. */
    FSTranslatingPortProxy *virtProxy;
    SETranslatingPortProxy *seMemProxy;

    /// For quiesce magic instruction
    Tick lastActivate;
    Tick lastSuspend;

  public:
    LSCThreadContext(LearningSimpleCPU *cpu);

    BaseCPU *getCpuPtr() override { return cpu; }

    int cpuId() const override { return cpu->cpuId(); }

    uint32_t socketId() const override { return cpu->socketId(); }

    int threadId() const override { return 0; }

    void setThreadId(int id) override {
        panic_if(id != 0, "LSCThreadContext does not support multithreading");
    }

    int contextId() const override { return contextId; }

    void setContextId(int id) override { contextId = id; }

    /// Since the the ITB and DTB are parameters of the CPU object, we'll store
    /// them there
    BaseTLB *getITBPtr() override { return cpu->getITBPtr(); }

    BaseTLB *getDTBPtr() override { return cpu->getITBPtr(); }

    CheckerCPU *getCheckerCpuPtr() override { return nullptr; }

    TheISA::Decoder *getDecoderPtr() override { return &decoder} ;

    System *getSystemPtr() override { return system; }

    TheISA::Kernel::Statistics *getKernelStats() override
    { panic("LearningSimpleCPU does not support kernel stats"); }

    PortProxy &getPhysProxy() override {
        assert(virtProxy != NULL);
        return *virtProxy;
    }

    FSTranslatingPortProxy &getVirtProxy() override {
        assert(physProxy != NULL);
        return *physProxy;
    }

    void initMemProxies(ThreadContext *tc) override;

    SETranslatingPortProxy &getMemProxy() override {
        assert(seMemProxy != NULL);
        return *seMemProxy;
    }

    Process *getProcessPtr() override { return process; }

    void setProcessPtr(Process *p) override { process = p; }

    ThreadContext::Status status() const override { return status; }

    void setStatus(ThreadContext::Status new_status) override {
        status = new_status;
    }

    void activate() override;

    void suspend() override;

    void halt() override;

    void dumpFuncProfile() override
    { panic("LearningSimpleCPU does not support profiling"); }

    void profileClear() override
    { panic("LearningSimpleCPU does not support profiling"); }
    void profileSample() override
    { panic("LearningSimpleCPU does not support profiling"); }

    void takeOverFrom(ThreadContext *old_context) override
    { panic("LearningSimpleCPU does not support take over from"); }

    void regStats(const std::string &name) override
    { /* NOTE: LearningSimpleCPU does not support kernel stats */ }

    EndQuiesceEvent *getQuiesceEvent() override
    { panic("LearningSimpleCPU does not support quiesce events"); }

    /// For quiesce magic instruction
    Tick readLastActivate() override { return lastActivate; }
    Tick readLastSuspend() override { return lastSuspend; }

    void copyArchRegs(ThreadContext *tc) override
    { TheISA::copyRegs(tc, this); }

    void clearArchRegs() override;

    //
    // New accessors for new decoder.
    //
    uint64_t readIntReg(int reg_idx) override;

    FloatReg readFloatReg(int reg_idx) override;

    FloatRegBits readFloatRegBits(int reg_idx) override;

    const VecRegContainer& readVecReg(const RegId& reg) const override;
    VecRegContainer& getWritableVecReg(const RegId& reg) override;

    /** Vector Register Lane Interfaces. */
    /** @{ */
    /** Reads source vector 8bit operand. */
    ConstVecLane8 readVec8BitLaneReg(const RegId& reg) const override;

    /** Reads source vector 16bit operand. */
    ConstVecLane16 readVec16BitLaneReg(const RegId& reg) const override;

    /** Reads source vector 32bit operand. */
    ConstVecLane32 readVec32BitLaneReg(const RegId& reg) const override;

    /** Reads source vector 64bit operand. */
    ConstVecLane64 readVec64BitLaneReg(const RegId& reg) const override;

    /** Write a lane of the destination vector register. */
    void setVecLane(const RegId& reg,
            const LaneData<LaneSize::Byte>& val) override;
    void setVecLane(const RegId& reg,
            const LaneData<LaneSize::TwoByte>& val) override;
    void setVecLane(const RegId& reg,
            const LaneData<LaneSize::FourByte>& val) override;
    void setVecLane(const RegId& reg,
            const LaneData<LaneSize::EightByte>& val) override;
    /** @} */

    const VecElem& readVecElem(const RegId& reg) const override;

    CCReg readCCReg(int reg_idx) override;

    void setIntReg(int reg_idx, uint64_t val) override;

    void setFloatReg(int reg_idx, FloatReg val) override;

    void setFloatRegBits(int reg_idx, FloatRegBits val) override;

    void setVecReg(const RegId& reg, const VecRegContainer& val) override;

    void setVecElem(const RegId& reg, const VecElem& val) override;

    void setCCReg(int reg_idx, CCReg val) override;

    TheISA::PCState pcState() override;

    void pcState(const TheISA::PCState &val) override;

    void pcStateNoRecord(const TheISA::PCState &val) override;

    Addr instAddr() override;

    Addr nextInstAddr() override;

    MicroPC microPC() override;

    MiscReg readMiscRegNoEffect(int misc_reg) const override;

    MiscReg readMiscReg(int misc_reg) override;

    void setMiscRegNoEffect(int misc_reg, const MiscReg &val) override;

    void setMiscReg(int misc_reg, const MiscReg &val) override;

    RegId flattenRegId(const RegId& regId) const override;

    unsigned readStCondFailures() override;

    void setStCondFailures(unsigned sc_failures) override;

    // Same with st cond failures.
    Counter readFuncExeInst() override;

    void syscall(int64_t callnum, Fault *fault) override;

    uint64_t readIntRegFlat(int idx) override;
    void setIntRegFlat(int idx, uint64_t val) override;

    FloatReg readFloatRegFlat(int idx) override;
    void setFloatRegFlat(int idx, FloatReg val) override;

    FloatRegBits readFloatRegBitsFlat(int idx) override;
    void setFloatRegBitsFlat(int idx, FloatRegBits val) override;

    const VecRegContainer& readVecRegFlat(int idx) const override;
    VecRegContainer& getWritableVecRegFlat(int idx) override;
    void setVecRegFlat(int idx, const VecRegContainer& val) override;

    const VecElem& readVecElemFlat(const RegIndex& idx,
                                   const ElemIndex& elemIdx) const override;
    void setVecElemFlat(const RegIndex& idx, const ElemIndex& elemIdx,
                                const VecElem& val) override;

    CCReg readCCRegFlat(int idx) override;
    void setCCRegFlat(int idx, CCReg val) override;
};
