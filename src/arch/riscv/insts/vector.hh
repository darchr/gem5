// SPDX-FileCopyrightText: Copyright © 2022 by Rivos Inc.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef __ARCH_RISCV_VECTOR_INSTS_HH__
#define __ARCH_RISCV_VECTOR_INSTS_HH__

#include <string>

#include "arch/riscv/insts/static_inst.hh"
#include "arch/riscv/insts/vector_static_inst.hh"
#include "cpu/exec_context.hh"
#include "cpu/static_inst.hh"

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

}

}

#endif // __ARCH_RISCV_VECTOR_INSTS_HH__