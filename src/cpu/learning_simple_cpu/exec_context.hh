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

 #ifndef __CPU_LEARNING_SIMPLE_CPU_EXEC_CONTEXT_HH__
 #define __CPU_LEARNING_SIMPLE_CPU_EXEC_CONTEXT_HH__

 #include "cpu/exec_context.hh"
 #include "cpu/base.hh"
 #include "cpu/learning_simple_cpu/cpu.hh"
 #include "cpu/simple_thread.hh"
 #include "mem/request.hh"

/**
 * The ExecContext is an abstract base class the provides the
 * interface used by the ISA to manipulate the state of the CPU model.
 *
 * Register accessor methods in this class typically provide the index
 * of the instruction's operand (e.g., 0 or 1), not the architectural
 * register index, to simplify the implementation of register
 * renaming.  The architectural register index can be found by
 * indexing into the instruction's own operand index table.
 *
 * @note The methods in this class typically take a raw pointer to the
 * StaticInst is provided instead of a ref-counted StaticInstPtr to
 * reduce overhead as an argument. This is fine as long as the
 * implementation doesn't copy the pointer into any long-term storage
 * (which is pretty hard to imagine they would have reason to do).
 */
class LearningSimpleContext : public ExecContext {
  private:
    LearningSimpleCPU &cpu;
    SimpleThread &thread;
    StaticInstPtr inst;

  public:

    LearningSimpleContext(LearningSimpleCPU &cpu, SimpleThread &thread,
                          StaticInstPtr inst) :
        cpu(cpu), thread(thread), inst(inst)
    { }
    /**
     * @{
     * @name Integer Register Interfaces
     *
     */

    /** Reads an integer register. */
    IntReg readIntRegOperand(const StaticInst *si, int idx) override
    { return thread.readIntReg(si->srcRegIdx(idx)); }

    /** Sets an integer register to a value. */
    void setIntRegOperand(const StaticInst *si, int idx, IntReg val) override
    { thread.setIntReg(si->destRegIdx(idx), val); }

    /** @} */


    /**
     * @{
     * @name Floating Point Register Interfaces
     */

    /** Reads a floating point register of single register width. */
    FloatReg readFloatRegOperand(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::FP_Reg_Base;
        return thread.readFloatReg(reg_idx);
    }

    /** Reads a floating point register in its binary format, instead
     * of by value. */
    FloatRegBits readFloatRegOperandBits(const StaticInst *si, int idx)
        override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::FP_Reg_Base;
        return thread.readFloatRegBits(reg_idx);
    }

    /** Sets a floating point register of single width to a value. */
    void setFloatRegOperand(const StaticInst *si, int idx, FloatReg val)
        override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::FP_Reg_Base;
        thread.setFloatReg(reg_idx, val);
    }

    /** Sets the bits of a floating point register of single width
     * to a binary value. */
    void setFloatRegOperandBits(const StaticInst *si, int idx,
                                FloatRegBits val) override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::FP_Reg_Base;
        thread.setFloatRegBits(reg_idx, val);
    }

    /** @} */

    /**
     * @{
     * @name Condition Code Registers
     */
    CCReg readCCRegOperand(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::CC_Reg_Base;
        return thread.readCCReg(reg_idx);
    }
    void setCCRegOperand(const StaticInst *si, int idx, CCReg val) override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::CC_Reg_Base;
        thread.setCCReg(reg_idx, val);
    }
    /** @} */

    /**
     * @{
     * @name Misc Register Interfaces
     */
    MiscReg readMiscRegOperand(const StaticInst *si, int idx) override
    {
        int reg_idx = si->srcRegIdx(idx) - TheISA::Misc_Reg_Base;
        return thread.readMiscReg(reg_idx);
    }
    void setMiscRegOperand(const StaticInst *si, int idx, const MiscReg &val)
        override
    {
        int reg_idx = si->destRegIdx(idx) - TheISA::Misc_Reg_Base;
        return thread.setMiscReg(reg_idx, val);
    }

    /**
     * Reads a miscellaneous register, handling any architectural
     * side effects due to reading that register.
     */
    MiscReg readMiscReg(int misc_reg) override
    { return thread.readMiscReg(misc_reg); }

    /**
     * Sets a miscellaneous register, handling any architectural
     * side effects due to writing that register.
     */
    void setMiscReg(int misc_reg, const MiscReg &val) override
    { thread.setMiscReg(misc_reg, val); }

    /** @} */

    /**
     * @{
     * @name PC Control
     */
    PCState pcState() const override
    { return thread.pcState(); }
    void pcState(const PCState &val) override
    { thread.pcState(val); }
    /** @} */

    /**
     * @{
     * @name Memory Interface
     */
    /**
     * Record the effective address of the instruction.
     *
     * @note Only valid for memory ops.
     */
    void setEA(Addr EA) override
    { panic("LearningSimpleCPU::setEA() not implemented\n"); }
    /**
     * Get the effective address of the instruction.
     *
     * @note Only valid for memory ops.
     */
    Addr getEA() const override
    { panic("LearningSimpleCPU::setEA() not implemented\n"); }

    /**
     * Perform an atomic memory read operation.  Must be overridden
     * for exec contexts that support atomic memory mode.  Not pure
     * since exec contexts that only support timing memory
     * mode need not override (though in that case this function
     * should never be called).
     */
    Fault readMem(Addr addr, uint8_t *data, unsigned int size,
                  Request::Flags flags) override
    { panic("LearningSimpleCPU doesn't support atomic accesses."); }

    /**
     * Initiate a timing memory read operation.  Must be overridden
     * for exec contexts that support timing memory mode.  Not pure
     * since exec contexts that only support atomic memory
     * mode need not override (though in that case this function
     * should never be called).
     */
    Fault initiateMemRead(Addr addr, unsigned int size, Request::Flags flags)
        override
    {
        MemoryRequest *request = new MemoryRequest(cpu, thread, inst,
                                                   addr, size, flags);
        request->translate();
        return NoFault;
    }

    /**
     * For atomic-mode contexts, perform an atomic memory write operation.
     * For timing-mode contexts, initiate a timing memory write operation.
     */
    Fault writeMem(uint8_t *data, unsigned int size, Addr addr,
                   Request::Flags flags, uint64_t *res) override
    {
        MemoryRequest *request = new MemoryRequest(cpu, thread, inst,
                                                   addr, size, flags,
                                                   data, res);
        request->translate();
        return NoFault;
    }

    /**
     * Sets the number of consecutive store conditional failures.
     */
    void setStCondFailures(unsigned int sc_failures) override
    { thread.setStCondFailures(sc_failures); }

    /**
     * Returns the number of consecutive store conditional failures.
     */
    unsigned int readStCondFailures() const override
    { return thread.readStCondFailures(); }

    /** @} */

    /**
     * @{
     * @name SysCall Emulation Interfaces
     */

    /**
     * Executes a syscall specified by the callnum.
     */
    void syscall(int64_t callnum, Fault *fault) override
    { thread.syscall(callnum, fault); }

    /** @} */

    /** Returns a pointer to the ThreadContext. */
    ThreadContext *tcBase() override
    { return thread.getTC(); }

    /**
     * @{
     * @name Alpha-Specific Interfaces
     */

    /**
     * Somewhat Alpha-specific function that handles returning from an
     * error or interrupt.
     */
    Fault hwrei() override
    { return thread.hwrei(); }

    /**
     * Check for special simulator handling of specific PAL calls.  If
     * return value is false, actual PAL call will be suppressed.
     */
    bool simPalCheck(int palFunc) override
    { return thread.simPalCheck(palFunc); }

    /** @} */

    /**
     * @{
     * @name ARM-Specific Interfaces
     */

    bool readPredicate() override
    { return thread.readPredicate(); }
    void setPredicate(bool val) override
    { thread.setPredicate(val); }

    /** @} */

    /**
     * @{
     * @name X86-Specific Interfaces
     */

    /**
     * Invalidate a page in the DTLB <i>and</i> ITLB.
     */
    void demapPage(Addr vaddr, uint64_t asn) override
    { thread.demapPage(vaddr, asn); }
    void armMonitor(Addr address) override
    { panic("ARM monitor not implemented for LearningSimpleCPU"); }
    bool mwait(PacketPtr pkt) override
    { panic("mwait not implemented for LearningSimpleCPU"); }
    void mwaitAtomic(ThreadContext *tc) override
    { panic("mwaitAtomic not implemented for LearningSimpleCPU"); }
    AddressMonitor *getAddrMonitor() override
    { panic("getAddrMonitor not implemented for LearningSimpleCPU"); }

    /** @} */

    /**
     * @{
     * @name MIPS-Specific Interfaces
     */

#if THE_ISA == MIPS_ISA
    MiscReg readRegOtherThread(int regIdx, ThreadID tid = InvalidThreadID)
        override
    { panic("No support for multithreaded register access."); }
    void setRegOtherThread(int regIdx, MiscReg val,
                           ThreadID tid = InvalidThreadID) override
    { panic("No support for multithreaded register access."); }
#endif

    /** @} */
};

#endif
