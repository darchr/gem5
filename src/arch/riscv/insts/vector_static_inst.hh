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
#include "arch/riscv/vecregs.hh"
#include "cpu/static_inst.hh"

namespace gem5
{

namespace RiscvISA
{

class VectorMicroInst: public RiscvMicroInst
{
  private:
    uint64_t num_elements_per_regs;
    uint64_t num_non_tail_elements;
    uint64_t mask_policy;
    uint64_t tail_policy;
  public:
    VectorMicroInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t num_elements_per_regs, uint64_t num_non_tail_elements,
      uint64_t mask_policy, uint64_t tail_policy)
      : RiscvMicroInst(mnem, _machInst, __opClass)
    {
        this->num_elements_per_regs = num_elements_per_regs;
        this->num_non_tail_elements = num_non_tail_elements;
        this->mask_policy = mask_policy;
        this->tail_policy = tail_policy;
    }
};

class VectorSameWidthMicroInst: public VectorMicroInst
{
  private:
    uint64_t sew;
  public:
    VectorSameWidthMicroInst(
      const char *mnem, ExtMachInst _machInst, OpClass __opClass,
      uint64_t num_elements_per_regs, uint64_t num_non_tail_elements,
      uint64_t sew, uint64_t mask_policy, uint64_t tail_policy)
      : VectorMicroInst(mnem, _machInst, __opClass,
          num_elements_per_regs, num_non_tail_elements, mask_policy,
          tail_policy)
    {
        this->sew = sew;
    }
};

/* VectorStaticInst holds the info of all vector instructions */
class VectorStaticInst : public StaticInst
{
protected:
    using StaticInst::StaticInst;
    //std::bitset<NumVecFlags> vecflags;

public:
      void advancePC(PCStateBase &pc_state) const { pc_state.advance(); }
      /* vector instruction name*/
      virtual std::string getName() const = 0;

      /* the PC of the instruction*/
      uint64_t getPC() { return pc; }
      void setPC(uint64_t program_counter) { pc = program_counter; }

private:
  /* the PC of the instruction*/
  uint64_t pc;
};


class VectorInsn : public VectorStaticInst
{
public:
  VectorInsn(const char *mnem, MachInst _machInst, OpClass __opClass):
      VectorStaticInst(mnem, __opClass),
      machInst(_machInst),
      mnemo(mnem){}
  ~VectorInsn() {}

  std::string getName() const  override { return mnemo; }

  std::string regName(RegIndex reg) const;

  RegIndex rs1()             const { return (RegIndex)bits(machInst, 19, 15); }
  RegIndex rs2()             const { return (RegIndex)bits(machInst, 24, 20); }
  RegIndex rd()              const { return (RegIndex)bits(machInst, 11,  7); }

  RegIndex vs1()             const { return (RegIndex)bits(machInst, 19, 15); }
  RegIndex vs2()             const { return (RegIndex)bits(machInst, 24, 20); }
  RegIndex vs3()             const { return (RegIndex)bits(machInst, 11, 7); }
  RegIndex vd()              const { return (RegIndex)bits(machInst, 11, 7); }

  bool vm()                  const { return bits(machInst, 25, 25); }

  uint32_t vtypei()          const { return (bits(machInst, 31, 31) == 0) ? bits(machInst, 30, 20) : bits(machInst, 29, 20); }

  uint8_t uimm5()            const { return bits(machInst, 19, 15); }

  const RiscvISA::ExtMachInst machInst;

private:
  const char *mnemo;
};

class VectorMemInst : public VectorInsn
{
  protected:
    Request::Flags memAccessFlags;

    VectorMemInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass)
        : VectorInsn(mnem, _machInst, __opClass)
    {}
};

}

}

#endif // __ARCH_RISCV_VECTOR_STATIC_INSTS_HH__
