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

#include "arch/generic/types.hh"
#include "arch/isa_traits.hh"
#include "arch/registers.hh"

/**
 * The register file for the CPU.
 */
class RegisterFile
{
  private:

    using VecRegContainer = TheISA::VecRegContainer;
    using VecElem = TheISA::VecElem;
    using FloatReg = TheISA::FloatReg;
    using FloatRegBits = TheISA::FloatRegBits;
    using IntReg = TheISA::IntReg;
    using CCReg = TheISA::CCReg;

    typedef union {
        FloatReg f;
        FloatRegBits b;
    } FloatRegUnion;

    std::vector<IntReg> intRegs;
    std::vector<FloatRegUnion> floatRegs;
    std::vector<VecRegContainer> vecRegs;
    std::vector<CCReg> ccRegs;

    template <typename T>
    VecLaneT<T, true> readVecLane(RegIndex idx, ElemIndex lane_id) const
    {
        return vecRegs[idx].laneView<T>(lane_id);
    }

    template <typename LD>
    void setVecLaneT(RegIndex idx, ElemIndex lane_id, const LD& val)
    {
        vecRegs[idx].laneView<typename LD::UnderlyingType>(lane_id) = val;
    }

  public:
    /**
     * Constuct the register file allocating space for all of the registers
     *
     * @param number of integer registers (flattened)
     * @param number of floating point registers (flattened)
     * @param number of vector registers (flattened)
     * @param number of condition code registers (flattened), zero if using
     *        this with an ISA that doesn't have condition codes.
     */
    RegisterFile() :
        intRegs(TheISA::NumIntRegs), floatRegs(TheISA::NumFloatRegs),
        vecRegs(TheISA::NumVecRegs), ccRegs(TheISA::NumCCRegs)
    { clearRegisters(); }

    /**
     * Sets all register to have 0 as their stored value.
     */
    void clearRegisters() {
        FloatRegUnion fzero;
        fzero.b = 0;
        std::fill(floatRegs.begin(), floatRegs.end(), fzero);
        std::fill(intRegs.begin(), intRegs.end(), 0);
        std::fill(ccRegs.begin(), ccRegs.end(), 0);
        for (auto& vreg : vecRegs) {
            vreg.zero();
        }
    }

    /**
     * Read a value out of an integer register
     *
     * @param architectural flat register number
     * @return current register value
     */
    uint64_t readIntReg(RegIndex idx) { return intRegs[idx]; }

    /**
     * Write a value to an integer register
     *
     * @param architectural flat register number
     * @param value to write
     */
    void setIntReg(RegIndex idx, uint64_t val) { intRegs[idx] = val; }

    /**
     * Read a value out of a floating point register
     *
     * @param architectural flat register number
     * @return current register value
     */
    FloatReg readFloatReg(RegIndex idx) { return floatRegs[idx].f; }

    /**
     * Write a value out to a floating point register
     *
     * @param architectural flat register number
     * @param value to write
     */
    void setFloatReg(RegIndex idx, FloatReg val) { floatRegs[idx].f = val; }

    /**
     * Read a value out of a floating point register
     *
     * @param architectural flat register number
     * @return current register value
     */
    FloatRegBits readFloatRegBits(RegIndex idx) { return floatRegs[idx].b; }

    /**
     * Write a value out to a floating point register
     *
     * @param architectural flat register number
     * @param value to write
     */
    void setFloatRegBits(RegIndex idx, FloatRegBits val) {
        floatRegs[idx].b = val;
    }

    /**
     * Get a read-only reference to a whole vector register.
     *
     * @param architectural flat register number
     * @return a reference to the current register container
     */
    const VecRegContainer& readVecReg(RegIndex idx) const {
        return vecRegs[idx];
    }

    /**
     * Get a writable reference to a whole vector register.
     *
     * @param architectural flat register number
     * @return a reference to the register container
     */
    VecRegContainer& getWritableVecReg(RegIndex idx) {
        return vecRegs[idx];
    }

    /**
     * Write a whole vector register.
     *
     * @param architectural flat register number
     * @return the container of values to write
     */
    void setVecReg(RegIndex idx, const VecRegContainer& val) {
        vecRegs[idx] = val;
    }

    /** Vector Register Lane Interfaces. */
    /** @{ */
    /** Reads source vector 8bit operand. */
    ConstVecLane8 readVec8BitLaneReg(RegIndex idx, ElemIndex el_idx) const
    { return readVecLane<uint8_t>(idx, el_idx); }

    /** Reads source vector 16bit operand. */
    ConstVecLane16 readVec16BitLaneReg(RegIndex idx, ElemIndex el_idx) const
    { return readVecLane<uint16_t>(idx, el_idx); }

    /** Reads source vector 32bit operand. */
    ConstVecLane32 readVec32BitLaneReg(RegIndex idx, ElemIndex el_idx) const
    { return readVecLane<uint32_t>(idx, el_idx); }

    /** Reads source vector 64bit operand. */
    ConstVecLane64 readVec64BitLaneReg(RegIndex idx, ElemIndex el_idx) const
    { return readVecLane<uint64_t>(idx, el_idx); }

    /** Write a lane of the destination vector register. */
    void setVecLane(RegIndex idx, ElemIndex el_idx,
                    const LaneData<LaneSize::Byte>& val)
    { setVecLaneT(idx, el_idx, val); }
    void setVecLane(RegIndex idx, ElemIndex el_idx,
                    const LaneData<LaneSize::TwoByte>& val)
    { setVecLaneT(idx, el_idx, val); }
    void setVecLane(RegIndex idx, ElemIndex el_idx,
                    const LaneData<LaneSize::FourByte>& val)
    { setVecLaneT(idx, el_idx, val); }
    void setVecLane(RegIndex idx, ElemIndex el_idx,
                    const LaneData<LaneSize::EightByte>& val)
    { setVecLaneT(idx, el_idx, val); }
    /** @} */

    /**
     * Read one element of a vector register
     *
     * @param architectural flat register number
     * @param element index
     * @return read-only reference to the data in the register lane
     */
    const VecElem& readVecElem(RegIndex idx, ElemIndex el_idx) const
    {
        return vecRegs[idx].as<VecElem>()[el_idx];
    }

    /**
     * Read one element of a vector register
     *
     * @param architectural flat register number
     * @param element index
     * @return read-only reference to the data in the register lane
     */
    void setVecElem(RegIndex idx, ElemIndex el_idx, const VecElem val) {
        vecRegs[idx].as<VecElem>()[el_idx] = val;
    }

    /**
     * Read a condition code register
     *
     * @param architectural flat register number
     * @return current register value
     */
    CCReg readCCReg(RegIndex idx) { return ccRegs[idx]; }

    /**
     * Write a condition code register
     *
     * @param architectural flat register number
     * @param the value to write
     */
    void setCCReg(RegIndex idx, CCReg val) { ccRegs[idx] = val; }

};
