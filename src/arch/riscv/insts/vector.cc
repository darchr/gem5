// SPDX-FileCopyrightText: Copyright © 2022 by Rivos Inc.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <sstream>

#include "arch/riscv/insts/vector.hh"
#include "arch/riscv/regs/misc.hh"
#include "arch/riscv/utility.hh"

using namespace std;

namespace gem5
{

namespace RiscvISA
{

float
getVflmul(uint32_t vlmul_encoding) {
  int vlmul = int8_t(vlmul_encoding << 5) >> 5;
  float vflmul = vlmul >= 0 ? 1 << vlmul : 1.0 / (1 << -vlmul);
  return vflmul;
}

uint32_t
getVlmax(VTYPE vtype, int vlen) {
  uint32_t sew = getSew(vtype.vsew);
  uint32_t vlmax = (vlen/sew) * getVflmul(vtype.vlmul);
  return vlmax;
}

void roundUnsignedInteger(__uint128_t &result, uint32_t xrm, int gb) {
  do {
    const uint64_t lsb = 1UL << (gb);
    const uint64_t lsb_half = lsb >> 1;
    switch (xrm) {
      case VectorRoundingMode::RoundToNearestUp:
        result += lsb_half;
        break;
      case VectorRoundingMode::RoundToNearestEven:
        if ((result & lsb_half) && ((result & (lsb_half - 1)) || (result & lsb))) {
          result += lsb;
        }
        break;
      case VectorRoundingMode::RoundDown:
        break;
      case VectorRoundingMode::RoundToOdd:
        if (result & (lsb - 1)) {
          result |= lsb;
        }
        break;
      default:
        printf("error: unknown vector rounding mode\n");
        exit(1);
    }
  } while (0);
}

void roundSignedInteger(__int128_t &result, uint32_t xrm, int gb) {
  do {
    const uint64_t lsb = 1UL << (gb);
    const uint64_t lsb_half = lsb >> 1;
    switch (xrm) {
      case VectorRoundingMode::RoundToNearestUp:
        result += lsb_half;
        break;
      case VectorRoundingMode::RoundToNearestEven:
        if ((result & lsb_half) && ((result & (lsb_half - 1)) || (result & lsb))) {
          result += lsb;
        }
        break;
      case VectorRoundingMode::RoundDown:
        break;
      case VectorRoundingMode::RoundToOdd:
        if (result & (lsb - 1)) {
          result |= lsb;
        }
        break;
      default:
        printf("error: unknown vector rounding mode\n");
        exit(1);
    }
  } while (0);
}

uint32_t
setVsetvlCSR(ExecContext *xc,
             uint32_t rd_bits,
             uint32_t rs1_bits,
             uint32_t requested_vl,
             uint32_t requested_vtype) {
  VTYPE new_vtype = requested_vtype;

  int vlen = xc->readMiscReg(MISCREG_VLENB) * 8;
  uint32_t elen = xc->readMiscReg(MISCREG_ELEN);

  uint32_t vlmax = getVlmax(xc->readMiscReg(MISCREG_VTYPE), vlen);

  if (xc->readMiscReg(MISCREG_VTYPE) != new_vtype) {
    vlmax = getVlmax(new_vtype, vlen);

    float vflmul = getVflmul(new_vtype.vlmul);

    uint32_t sew = getSew(new_vtype.vsew);

    uint32_t new_vill =
      !(vflmul >= 0.125 && vflmul <= 8) ||
        sew > std::min(vflmul, 1.0f) * elen ||
        bits(requested_vtype, 30, 8) != 0;
    if (new_vill) {
      vlmax = 0;
      new_vtype = 0;
      new_vtype.vill = 1;
    }

    xc->setMiscReg(MISCREG_VTYPE, new_vtype);
  }

  // Set vl
  uint32_t current_vl = xc->readMiscReg(MISCREG_VL);
  uint32_t vl = 0;
  if (vlmax == 0) {
    vl = 0;
  } else if (rd_bits == 0 && rs1_bits == 0) {
    vl = current_vl > vlmax ? vlmax : current_vl;
  } else if (rd_bits != 0 && rs1_bits == 0) {
    vl = vlmax;
  } else if (rs1_bits != 0) {
    vl = requested_vl > vlmax ? vlmax : requested_vl;
  }

  xc->setMiscReg(MISCREG_VL, vl);

  return vl;
}

string
VectorVdVs2Vs1Op::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << VectorRegNames[vs2()] << ", ";
    ss << VectorRegNames[vs1()];
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

string
VectorVdVs2Vs1MacroOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << VectorRegNames[vs2()] << ", ";
    ss << VectorRegNames[vs1()];
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

string
VectorVdVs2Vs1MicroOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << VectorRegNames[vs2()] << ", ";
    ss << VectorRegNames[vs1()];
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

string
VectorUnitStrideMemLoadOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << csprintf("(%s)", IntRegNames[rs1()]);
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

string
VectorUnitStrideMemStoreOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << csprintf("(%s)", IntRegNames[rs1()]);
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

// op vd, vs2, uimm
string
VectorOPIVIOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << VectorRegNames[vs1()] << ", ";
    ss << csprintf("%d", uimm5());
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

// vd[0] = rs1
string
VectorVRXUNARY0Op::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << VectorRegNames[vd()] << ", ";
    ss << IntRegNames[rs1()] << ", ";
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}


// rd = vs2[0]
string
VectorVWXUNARY0Op::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << IntRegNames[rd()] << ", ";
    ss << VectorRegNames[vs2()] << ", ";
    if (vm()==0) {
        ss << ", " << "v0";
    }
    return ss.str();
}

string
VectorCfgOp::generateDisassembly(Addr pc,
    const loader::SymbolTable *symtab) const
{
    stringstream ss;
    ss << csprintf("0x%08x", machInst) << " " << mnemonic << " ";
    ss << IntRegNames[rd()] << ", ";
    if (getName() == "vsetivli") {
      ss << csprintf("%d, ", uimm5());
    } else {
      ss << IntRegNames[rs1()] << ", ";
    }

    if (getName() == "vsetvl") {
        ss << IntRegNames[vs2()] ;
    } else {
        VTYPE vtype = vtypei();
        ss << csprintf("e%d, ", getSew(vtype.vsew));

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

        if (vtype.vma) {
          ss << ", ma";
        } else {
          ss << ", mu";
        }

        if (vtype.vta) {
          ss << ", ta";
        } else {
          ss << ", tu";
        }
    }

    return ss.str();
}

}

}
