// SPDX-FileCopyrightText: Copyright © 2022 by Rivos Inc.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "arch/riscv/insts/vector.hh"

#include <sstream>

#include "arch/riscv/insts/bitfields.hh"
#include "arch/riscv/regs/misc.hh"
#include "arch/riscv/regs/vec.hh"
#include "arch/riscv/utility.hh"
#include "debug/Vsetvl.hh"

namespace gem5
{

namespace RiscvISA
{

float
getVflmul(const uint32_t& vlmul_encoding)
{
    int vlmul = int8_t(vlmul_encoding << 5) >> 5;
    float vflmul = vlmul >= 0 ? 1 << vlmul : 1.0 / (1 << -vlmul);
    return vflmul;
}

uint32_t
getVlmax(const VTYPE& vtype, const int& vlen)
{
    uint32_t sew = getSew(vtype.vsew);
    uint32_t vlmax = (vlen/sew) * getVflmul(vtype.vlmul);
    return vlmax;
}

uint32_t
setVsetvlCSR(ExecContext *xc,
             uint32_t rd_bits,
             uint32_t rs1_bits,
             uint32_t requested_vl,
             uint32_t requested_vtype,
             VTYPE& new_vtype)
{
    new_vtype = requested_vtype;

    int vlen = xc->readMiscReg(MISCREG_VLENB) * 8;
    uint32_t elen = xc->readMiscReg(MISCREG_ELEN);

    uint32_t vlmax = getVlmax(xc->readMiscReg(MISCREG_VTYPE), vlen);

    if (xc->readMiscReg(MISCREG_VTYPE) != new_vtype)
    {
        vlmax = getVlmax(new_vtype, vlen);

        float vflmul = getVflmul(new_vtype.vlmul);

        uint32_t sew = getSew(new_vtype.vsew);

        uint32_t new_vill =
          !(vflmul >= 0.125 && vflmul <= 8) ||
            sew > std::min(vflmul, 1.0f) * elen ||
            bits(requested_vtype, 30, 8) != 0;
        if (new_vill)
        {
            vlmax = 0;
            new_vtype = 0;
            new_vtype.vill = 1;
        }

        xc->setMiscReg(MISCREG_VTYPE, new_vtype);
    }

    // Set vl
    uint32_t current_vl = xc->readMiscReg(MISCREG_VL);
    uint32_t vl = 0;
    if (vlmax == 0)
        vl = 0;
    else if (rd_bits == 0 && rs1_bits == 0)
        vl = current_vl > vlmax ? vlmax : current_vl;
    else if (rd_bits != 0 && rs1_bits == 0)
        vl = vlmax;
    else if (rs1_bits != 0)
        vl = requested_vl > vlmax ? vlmax : requested_vl;

    xc->setMiscReg(MISCREG_VL, vl);

    DPRINTF(Vsetvl, "Setting vl=%d, vtype=%d\n", (uint64_t)vl,
            (uint64_t)new_vtype);
    DPRINTF(Vsetvl, "Misc vl=%d, vtype=%d\n",
            (uint64_t)xc->readMiscReg(MISCREG_VL),
            (uint64_t)xc->readMiscReg(MISCREG_VTYPE));

    return vl;
}

std::string
VectorCfgOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    std::string mnemonic_string(mnemonic);
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << int_reg::RegNames[RD] << ", ";
    if (mnemonic_string == "vsetivli") {
      ss << csprintf("%d, ", UIMM5);
    } else {
      ss << int_reg::RegNames[RS1] << ", ";
    }

    if (mnemonic_string == "vsetvl") {
        ss << vec_reg::VectorRegNames[VS2] ;
    } else {
        VTYPE vtype = 0;
        if (mnemonic_string == "vsetvli")
            vtype = ZIMM11;
        else if (mnemonic_string == "vsetivli")
            vtype = ZIMM10;

        ss << csprintf("e%d", getSew(vtype.vsew));
        ss << ", ";

        if (vtype.vta) {
            ss << "ta";
        } else {
            ss << "tu";
        }

        ss << ", ";

        switch(vtype.vlmul) {
        case VLMUL_MF8:
            ss << "mf8";
            break;
        case VLMUL_MF4:
            ss << "mf4";
            break;
        case VLMUL_MF2:
            ss << "mf2";
            break;
        case VLMUL_M1:
            ss << "m1";
            break;
        case VLMUL_M2:
            ss << "m2";
            break;
        case VLMUL_M4:
            ss << "m4";
            break;
        case VLMUL_M8:
            ss << "m8";
            break;
        }

        ss << ", ";

        if (vtype.vma) {
            ss << "ma";
        } else {
            ss << "mu";
        }
    }

    return ss.str();
}

// op vd, vs2, uimm
std::string
VectorOPIVIMacroOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << vec_reg::VectorRegNames[VD] << ", ";
    ss << vec_reg::VectorRegNames[VS1] << ", ";
    ss << csprintf("%d", UIMM5);
    if (VM==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

std::string
VectorOPIVIMicroOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << vec_reg::VectorRegNames[VD] << ", ";
    ss << vec_reg::VectorRegNames[VS1] << ", ";
    ss << csprintf("%d", UIMM5);
    if (VM==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

std::string
VectorUnitStrideMemLoadMacroOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << vec_reg::VectorRegNames[VD] << ", ";
    ss << csprintf("(%s)", int_reg::RegNames[RS1]);
    if (VM==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

std::string
VectorUnitStrideMemLoadMicroOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << vec_reg::VectorRegNames[VD] << ", ";
    ss << csprintf("(%s)", int_reg::RegNames[RS1]);
    if (VM==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

std::string
MicroNop::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    std::stringstream ss;
    ss << mnemonic;
    return ss.str();
}

}

}