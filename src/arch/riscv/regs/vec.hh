// SPDX-FileCopyrightText: Copyright © 2022 by Rivos Inc.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef __ARCH_RISCV_REGS_VEC_HH__
#define __ARCH_RISCV_REGS_VEC_HH__

#include "arch/generic/vec_reg.hh"
#include "cpu/reg_class.hh"
#include "debug/VecRegs.hh"

namespace gem5
{

namespace RiscvISA
{

namespace vec_reg
{

enum : RegIndex
{
    _V0Idx,    _V1Idx,    _V2Idx,    _V3Idx,
    _V4Idx,    _V5Idx,    _V6Idx,    _V7Idx,
    _V8Idx,    _V9Idx,    _V10Idx,   _V11Idx,
    _V12Idx,   _V13Idx,   _V14Idx,   _V15Idx,
    _V16Idx,   _V17Idx,   _V18Idx,   _V19Idx,
    _V20Idx,   _V21Idx,   _V22Idx,   _V23Idx,
    _V24Idx,   _V25Idx,   _V26Idx,   _V27Idx,
    _V28Idx,   _V29Idx,   _V30Idx,   _V31Idx,

    NumArchRegs,

    NumRegs = NumArchRegs
};

}; // namespace vec_reg

// VLEN: a single vector register length in bits.
// ELEN: the maximum element size in bits (i.e. SEW_MAX).
// SEW: selected element width, the size of each element in the vector reg.
const unsigned MaxVlenInBits = 65536;
const unsigned MinSewInBits = 8;
constexpr unsigned MaxNumElemsPerVecReg = MaxVlenInBits / MinSewInBits;

using VecElem = uint8_t;
using VecRegContainer =
    gem5::VecRegContainer<MaxNumElemsPerVecReg * sizeof(VecElem)>;

static inline TypedRegClassOps<RiscvISA::VecRegContainer> vecRegClassOps;

inline constexpr RegClass vecRegClass =
    RegClass(VecRegClass, VecRegClassName, vec_reg::NumRegs, debug::VecRegs).
        ops(vecRegClassOps).
        regType<VecRegContainer>();

namespace vec_reg
{

inline constexpr RegId
    V0 = vecRegClass[_V0Idx],
    V1 = vecRegClass[_V1Idx],
    V2 = vecRegClass[_V2Idx],
    V3 = vecRegClass[_V3Idx],
    V4 = vecRegClass[_V4Idx],
    V5 = vecRegClass[_V5Idx],
    V6 = vecRegClass[_V6Idx],
    V7 = vecRegClass[_V7Idx],
    V8 = vecRegClass[_V8Idx],
    V9 = vecRegClass[_V9Idx],
    V10 = vecRegClass[_V10Idx],
    V11 = vecRegClass[_V11Idx],
    V12 = vecRegClass[_V12Idx],
    V13 = vecRegClass[_V13Idx],
    V14 = vecRegClass[_V14Idx],
    V15 = vecRegClass[_V15Idx],
    V16 = vecRegClass[_V16Idx],
    V17 = vecRegClass[_V17Idx],
    V18 = vecRegClass[_V18Idx],
    V19 = vecRegClass[_V19Idx],
    V20 = vecRegClass[_V20Idx],
    V21 = vecRegClass[_V21Idx],
    V22 = vecRegClass[_V22Idx],
    V23 = vecRegClass[_V23Idx],
    V24 = vecRegClass[_V24Idx],
    V25 = vecRegClass[_V25Idx],
    V26 = vecRegClass[_V26Idx],
    V27 = vecRegClass[_V27Idx],
    V28 = vecRegClass[_V28Idx],
    V29 = vecRegClass[_V29Idx],
    V30 = vecRegClass[_V30Idx],
    V31 = vecRegClass[_V31Idx];

const std::vector<std::string> VectorRegNames = {
    "v0",   "v1",   "v2",   "v3",   "v4",   "v5",   "v6",   "v7",
    "v8",   "v9",   "v10",  "v11",  "v12",  "v13",  "v14",  "v15",
    "v16",  "v17",  "v18",  "v19",  "v20",  "v21",  "v22",  "v23",
    "v24",  "v25",  "v26",  "v27",  "v28",  "v29",  "v30",  "v31"
};

} // namespace vec_reg

} // namespace RiscvISA
} // namespace gem5

#endif