/*
 * Copyright (c) 2020 Barcelona Supercomputing Center
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
 * Author: Cristóbal Ramírez
 */

#ifndef __ARCH_RISCV_VECTOR_STATIC_INSTS_HH__
#define __ARCH_RISCV_VECTOR_STATIC_INSTS_HH__

#include <string>

#include "arch/riscv/types.hh"
#include "cpu/static_inst.hh"

namespace gem5
{

namespace RiscvISA
{

class VectorMicroInst: public RiscvMicroInst
{
  protected:
    uint64_t num_elements_per_reg;
    uint64_t num_non_tail_elements;
    uint64_t mask_policy;
    uint64_t tail_policy;
  public:
    VectorMicroInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t num_elements_per_reg, uint64_t num_non_tail_elements,
      uint64_t mask_policy, uint64_t tail_policy) :
          RiscvMicroInst(mnem, _machInst, __opClass)
    {
        this->num_elements_per_reg = num_elements_per_reg;
        this->num_non_tail_elements = num_non_tail_elements;
        this->mask_policy = mask_policy;
        this->tail_policy = tail_policy;
    }
};

class VectorSameElementWidthMicroInst: public VectorMicroInst
{
  protected:
    uint64_t sew;
  public:
    VectorSameElementWidthMicroInst(
      const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t num_elements_per_reg, uint64_t num_non_tail_elements,
      uint64_t sew, uint64_t mask_policy, uint64_t tail_policy):
          VectorMicroInst(mnem, _machInst, __opClass,
              num_elements_per_reg, num_non_tail_elements, mask_policy,
              tail_policy)
    {
        this->sew = sew;
    }
};

class VectorMacroInst: public RiscvMacroInst
{
  protected:
    RiscvISA::VTYPE vtype;
    uint32_t vl;
    int vlen;
  public:
    //using RiscvMacroInst::RiscvMacroInst;
    VectorMacroInst(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, RiscvISA::VTYPE vtype, uint32_t vl, int vlen)
        : RiscvMacroInst(mnem, _machInst, __opClass)
    {
        this->vtype = vtype;
        this->vl = vl;
        this->vlen = vlen;
    }
};

class VectorStaticInst: public RiscvStaticInst
{
  protected:
    RiscvISA::VTYPE vtype;
    uint32_t vl;
    int vlen;
  public:
    VectorStaticInst(const char *mnem, ExtMachInst _machInst,
        OpClass __opClass, RiscvISA::VTYPE vtype, uint32_t vl, int vlen)
        : RiscvStaticInst(mnem, _machInst, __opClass)
    {
        this->vtype = vtype;
        this->vl = vl;
        this->vlen = vlen;
    }
};

}

}

#endif // __ARCH_RISCV_VECTOR_STATIC_INSTS_HH__