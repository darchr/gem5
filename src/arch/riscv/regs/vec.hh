// SPDX-FileCopyrightText: Copyright © 2022 by Rivos Inc.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef __ARCH_RISCV_REGS_VEC_HH__
#define __ARCH_RISCV_REGS_VEC_HH__

#include "arch/generic/vec_pred_reg.hh"
#include "arch/generic/vec_reg.hh"
#include "arch/riscv/types.hh"
#include "arch/riscv/vec_params.hh"
#include "base/bitunion.hh"

namespace gem5
{

namespace RiscvISA
{

constexpr uint32_t VLENB = RISCV_VLEN / 8;
constexpr uint32_t ELEN = RISCV_ELEN;

constexpr size_t VecRegSizeBytes = VLENB;
using VecElem = uint8_t;
constexpr unsigned NumVecElemPerVecReg = VLENB / sizeof(VecElem);

using VecRegContainer =
    gem5::VecRegContainer<NumVecElemPerVecReg * sizeof(VecElem)>;

// Not applicable to RISC-V
using VecPredRegContainer = ::gem5::DummyVecPredRegContainer;

const int NumVecRegs = 32;

const std::vector<std::string> VectorRegNames ={
    "v0",   "v1",   "v2",   "v3",   "v4",   "v5",   "v6",   "v7",
    "v8",   "v9",   "v10",  "v11",  "v12",  "v13",  "v14",  "v15",
    "v16",  "v17",  "v18",  "v19",  "v20",  "v21",  "v22",  "v23",
    "v24",  "v25",  "v26",  "v27",  "v28",  "v29",  "v30",  "v31"
};

/**
 * These fields are specified in the RISC-V Vector Extension Manual.
 */
BitUnion32(VTYPE)
    Bitfield<31> vill;
    Bitfield<7> vma;
    Bitfield<6> vta;
    Bitfield<5, 3> vsew;
    Bitfield<2, 0> vlmul;
EndBitUnion(VTYPE)

} // namespace RiscvISA
} // namespace gem5

#endif

