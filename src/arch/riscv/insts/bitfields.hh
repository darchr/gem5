#ifndef __ARCH_RISCV_BITFIELDS_HH__
#define __ARCH_RISCV_BITFIELDS_HH__

#include "base/bitfield.hh"

#define CSRIMM  bits(machInst, 19, 15)
#define FUNCT12 bits(machInst, 31, 20)
#define IMM5    bits(machInst, 11, 7)
#define IMM7    bits(machInst, 31, 25)
#define IMMSIGN bits(machInst, 31)
#define OPCODE  bits(machInst, 6, 0)

#define AQ      bits(machInst, 26)
#define RD      bits(machInst, 11, 7)
#define RL      bits(machInst, 25)
#define RS1     bits(machInst, 19, 15)
#define RS2     bits(machInst, 24, 20)

// Vector instructions
#define VFUNC_6   bits(machInst, 31,26)
#define VFUNC_5   bits(machInst, 31,27)
#define VFUNC_3   bits(machInst, 27,25)
#define VFUNC_2   bits(machInst, 26,25)

#define VS3       bits(machInst, 11,7)
#define VS2       bits(machInst, 24,20)
#define VS1       bits(machInst, 19,15)
#define VD        bits(machInst, 11,7)

#define NF        bits(machInst, 31,29)
#define MEW       bits(machInst, 28,28)
#define MOP       bits(machInst, 27,26)
#define VM        bits(machInst, 25)
#define LUMOP     bits(machInst, 24,20)
#define SUMOP     bits(machInst, 24,20)
#define WIDTH     bits(machInst, 14,12)

#define SIMM_3    bits(machInst, 17,15)
#define BIT31     bits(machInst, 31)
#define BIT30     bits(machInst, 30)
#define ZIMM11    bits(machInst, 30,20)
#define ZIMM10    bits(machInst, 29,20)
#define UIMM5     bits(machInst, 19,15)

#endif // __ARCH_RISCV_BITFIELDS_HH__
