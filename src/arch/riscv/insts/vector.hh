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

inline uint32_t getSew(uint32_t vsew)
{
    assert(vsew <= 3);
    return (8 << vsew);
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

}

}

#endif // __ARCH_RISCV_VECTOR_INSTS_HH__