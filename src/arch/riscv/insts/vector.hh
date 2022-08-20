// SPDX-FileCopyrightText: Copyright © 2022 by Rivos Inc.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef __ARCH_RISCV_VECTOR_INSTS_HH__
#define __ARCH_RISCV_VECTOR_INSTS_HH__

#include <string>

#include "arch/riscv/insts/standard.hh"
#include "arch/riscv/insts/static_inst.hh"
#include "arch/riscv/insts/vector_static_inst.hh"
#include "cpu/exec_context.hh"
#include "cpu/static_inst.hh"
#include "debug/Vsetvl.hh"

namespace gem5
{

namespace RiscvISA
{

enum VlmulEncoding
{
    VLMUL_MF8 = 0x5,
    VLMUL_MF4 = 0x6,
    VLMUL_MF2 = 0x7,
    VLMUL_M1 = 0x0,
    VLMUL_M2 = 0x1,
    VLMUL_M4 = 0x2,
    VLMUL_M8 = 0x3
};

inline uint32_t
getSew(const uint32_t& vsew)
{
    assert(vsew <= 3);
    return (8 << vsew);
}

inline uint32_t
vlmulToNumRegs(const uint32_t& vlmul)
{
    switch (vlmul)
    {
        case VlmulEncoding::VLMUL_M2:
            return 2;
        case VlmulEncoding::VLMUL_M4:
            return 4;
        case VlmulEncoding::VLMUL_M8:
            return 8;
        case VlmulEncoding::VLMUL_M1:
        case VlmulEncoding::VLMUL_MF2:
        case VlmulEncoding::VLMUL_MF4:
        case VlmulEncoding::VLMUL_MF8:
            return 1;
        default:
            panic("Wrong Vlmul detected. Got %d\n", vlmul);
            break;
    }
}

uint32_t
setVsetvlCSR(ExecContext *xc,
             uint32_t rd_bits,
             uint32_t rs1_bits,
             uint32_t requested_vl,
             uint32_t requested_vtype,
             VTYPE& new_vtype);

/*
 * Vector Configuration Instructions
 */
class VectorCfgOp : public VectorStaticInst
{
  protected:
    uint32_t imm;
    uint32_t rs1_bits;
    uint32_t rd_bits;

    VectorCfgOp(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, RiscvISA::VTYPE vtype, uint32_t vl, int vlen) :
        VectorStaticInst(mnem, _machInst, __opClass, vtype, vl, vlen),
            imm(0), rs1_bits(0), rd_bits(0)
    {}

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorOPIVIMacroOp : public VectorMacroInst
{
  public:
    VectorOPIVIMacroOp(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, uint32_t machVtype, uint32_t machVl, int vlen) :
        VectorMacroInst(mnem, _machInst, __opClass, machVtype, machVl, vlen)
    {
    }

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorOPIVIMicroOp: public VectorSameElementWidthMicroInst
{
  protected:
    uint64_t vdRegID;
    uint64_t vs2RegID;
    uint64_t vmRegID;
    uint64_t mask_offset;
  public:
    VectorOPIVIMicroOp(
      const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t vdRegID, uint64_t vs2RegID, uint64_t vmRegID,
      uint64_t mask_offset, uint64_t num_elements_per_reg,
      uint64_t num_non_tail_elements, uint64_t sew,
      uint64_t mask_policy, uint64_t tail_policy)
        : VectorSameElementWidthMicroInst(mnem, _machInst, __opClass,
            num_elements_per_reg, num_non_tail_elements, sew, mask_policy,
            tail_policy)
    {
        this->vdRegID = vdRegID;
        this->vs2RegID = vs2RegID;
        this->vmRegID = vmRegID;
        this->mask_offset = mask_offset;
    }
    std::string generateDisassembly(
        Addr pc, const loader::SymbolTable *symtab) const override;
};

class VectorVdVs2Vs1MacroOp : public VectorMacroInst
{
  public:
    //using VectorMacroInst::VectorMacroInst;
    VectorVdVs2Vs1MacroOp(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, uint32_t machVtype, uint32_t machVl, int vlen) :
        VectorMacroInst(mnem, _machInst, __opClass, machVtype, machVl, vlen)
    {
        DPRINTF(Vsetvl,
                "Decoding VectorVdVs2Vs1MacroOp with vl=%d, vtype=%d\n",
                machVl, (uint64_t)machVtype);
    }

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorVdVs2Vs1MicroOp: public VectorSameElementWidthMicroInst
{
  protected:
    uint64_t vdRegID;
    uint64_t vs1RegID;
    uint64_t vs2RegID;
    uint64_t vmRegID;
    uint64_t mask_offset;
  public:
    VectorVdVs2Vs1MicroOp(
      const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t vdRegID, uint64_t vs1RegID, uint64_t vs2RegID, uint64_t vmRegID,
      uint64_t mask_offset,
      uint64_t num_elements_per_reg, uint64_t num_non_tail_elements,
      uint64_t sew, uint64_t mask_policy, uint64_t tail_policy)
        : VectorSameElementWidthMicroInst(mnem, _machInst, __opClass,
            num_elements_per_reg, num_non_tail_elements, sew, mask_policy,
            tail_policy)
    {
        this->vdRegID = vdRegID;
        this->vs1RegID = vs1RegID;
        this->vs2RegID = vs2RegID;
        this->vmRegID = vmRegID;
        this->mask_offset = mask_offset;
    }
    std::string generateDisassembly(
        Addr pc, const loader::SymbolTable *symtab) const override;
};

class VectorVRXUNARY0Op : public VectorStaticInst
{
  public:
    VectorVRXUNARY0Op(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, RiscvISA::VTYPE vtype, uint32_t vl, int vlen) :
        VectorStaticInst(mnem, _machInst, __opClass, vtype, vl, vlen)
    {
        DPRINTF(Vsetvl,
            "Decoding VectorVRXUNARY0Op with vl=%d, vtype=%d\n",
            vl, (uint64_t)vtype);
    }

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorVWXUNARY0Op : public VectorStaticInst
{
  public:
    VectorVWXUNARY0Op(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, RiscvISA::VTYPE vtype, uint32_t vl, int vlen) :
        VectorStaticInst(mnem, _machInst, __opClass, vtype, vl, vlen)
    {
        DPRINTF(Vsetvl,
            "Decoding VectorVWXUNARY0Op with vl=%d, vtype=%d\n",
            vl, (uint64_t)vtype);
    }

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorVMUNARY0MacroOp : public VectorMacroInst
{
  public:
    VectorVMUNARY0MacroOp(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, uint32_t machVtype, uint32_t machVl, int vlen) :
        VectorMacroInst(mnem, _machInst, __opClass, machVtype, machVl, vlen)
    {
        DPRINTF(Vsetvl,
            "Decoding VectorVMUNARY0Op with vl=%d, vtype=%d\n",
            vl, (uint64_t)vtype);
    }

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorVMUNARY0MicroOp: public VectorSameElementWidthMicroInst
{
  protected:
    uint64_t vdRegID;
    uint64_t vmRegID;
    uint64_t mask_offset;
  public:
    VectorVMUNARY0MicroOp(
      const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t vdRegID, uint64_t vmRegID, uint64_t mask_offset,
      uint64_t num_elements_per_reg, uint64_t num_non_tail_elements,
      uint64_t sew, uint64_t mask_policy, uint64_t tail_policy)
        : VectorSameElementWidthMicroInst(mnem, _machInst, __opClass,
            num_elements_per_reg, num_non_tail_elements, sew, mask_policy,
            tail_policy)
    {
        this->vdRegID = vdRegID;
        this->vmRegID = vmRegID;
        this->mask_offset = mask_offset;
    }
    std::string generateDisassembly(
        Addr pc, const loader::SymbolTable *symtab) const override;
};

class VectorUnitStrideMemLoadMacroOp : public VectorMacroInst
{
  public:
    VectorUnitStrideMemLoadMacroOp(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, RiscvISA::VTYPE vtype, uint32_t vl, int vlen) :
        VectorMacroInst(mnem, _machInst, __opClass, vtype, vl, vlen)
    {
        DPRINTF(Vsetvl,
            "Decoding VectorUnitStrideMemLoadMacroOp with vl=%d, vtype=%d\n",
            vl, (uint64_t)vtype);
    }
    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;
};

class VectorUnitStrideMemLoadMicroOp : public VectorMemMicroInst
{
  protected:
    uint64_t vdRegID;
    uint64_t vmRegID;
    uint64_t mask_offset;
  public:
    VectorUnitStrideMemLoadMicroOp(
      const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t vdRegID, uint64_t vmRegID, uint64_t mask_offset,
      uint64_t num_elements_per_reg, uint64_t num_non_tail_elements,
      uint64_t eew, uint64_t mask_policy, uint64_t tail_policy) :
        VectorMemMicroInst(mnem, _machInst, __opClass,
            num_elements_per_reg, num_non_tail_elements,
            eew, mask_policy, tail_policy)
    {
        this->vdRegID = vdRegID;
        this->vmRegID = vmRegID;
        this->mask_offset = mask_offset;
    }
    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;

    virtual Fault execute(ExecContext *, Trace::InstRecord *) const = 0;
    virtual Fault initiateAcc(ExecContext *, Trace::InstRecord *) const = 0;
    virtual Fault completeAcc(Packet *, ExecContext *, Trace::InstRecord *)
        const = 0;
};

class MicroNop : public ImmOp<int64_t>
{
  public:
    MicroNop()
      : ImmOp<int64_t>("MicroNop", 0x13, IntAluOp)
    {
        flags[IsMicroop] = true;
    }

    std::string generateDisassembly(Addr pc,
        const loader::SymbolTable *symtab) const;

    Fault
    execute(ExecContext *xc, Trace::InstRecord *traceData) const override
    {
        return NoFault;
    }
};

}

}

#endif // __ARCH_RISCV_VECTOR_INSTS_HH__