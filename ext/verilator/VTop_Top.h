// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See VTop.h for the primary calling header

#ifndef _VTop_Top_H_
#define _VTop_Top_H_

#include "verilated_heavy.h"
#include "VTop__Dpi.h"

class VTop__Syms;
class VTop_DualPortedMemory;

//----------

VL_MODULE(VTop_Top) {
  public:
    // CELLS
    VTop_DualPortedMemory*	mem;
    
    // PORTS
    VL_IN8(clock,0,0);
    VL_IN8(reset,0,0);
    VL_OUT8(__PVT__io_success,0,0);
    
    // LOCAL SIGNALS
    VL_SIG8(__PVT__cpu__DOT__control_io_toreg,1,0);
    VL_SIG8(__PVT__cpu__DOT__control_io_immediate,0,0);
    VL_SIG8(__PVT__cpu__DOT__control_io_alusrc1,1,0);
    VL_SIG8(__PVT__cpu__DOT__control_io_jump,1,0);
    VL_SIG8(__PVT__cpu__DOT__registers_io_wen,0,0);
    VL_SIG8(__PVT__cpu__DOT__aluControl_io_operation,3,0);
    VL_SIG(__PVT__cpu__DOT__registers_io_writedata,31,0);
    VL_SIG(__PVT__cpu__DOT__registers_io_readdata1,31,0);
    VL_SIG(__PVT__cpu__DOT__registers_io_readdata2,31,0);
    VL_SIG(__PVT__cpu__DOT__alu_io_inputx,31,0);
    VL_SIG(__PVT__cpu__DOT__alu_io_inputy,31,0);
    VL_SIG(__PVT__cpu__DOT__immGen_io_sextImm,31,0);
    VL_SIG(__PVT__cpu__DOT__pcPlusFour_io_inputx,31,0);
    VL_SIG(__PVT__cpu__DOT__branchAdd_io_result,31,0);
    VL_SIG(__PVT__cpu__DOT__pc,31,0);
    VL_SIG(__PVT__cpu__DOT__value,29,0);
    VL_SIG(__PVT__cpu__DOT___T_2,29,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_0,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_1,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_2,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_3,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_4,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_5,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_6,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_7,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_8,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_9,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_10,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_11,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_12,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_13,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_14,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_15,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_16,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_17,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_18,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_19,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_20,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_21,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_22,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_23,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_24,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_25,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_26,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_27,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_28,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_29,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_30,31,0);
    VL_SIG(__PVT__cpu__DOT__registers__DOT__regs_31,31,0);
    VL_SIG(__PVT__cpu__DOT__alu__DOT___T_3,31,0);
    VL_SIG(__PVT__cpu__DOT__immGen__DOT___T_26,31,0);
    VL_SIG64(__PVT__cpu__DOT__alu__DOT___GEN_10,62,0);
    
    // LOCAL VARIABLES
    
    // INTERNAL VARIABLES
  private:
    VTop__Syms* __VlSymsp;  // Symbol table
  public:
    
    // PARAMETERS
    
    // CONSTRUCTORS
  private:
    VTop_Top& operator= (const VTop_Top&);  ///< Copying not allowed
    VTop_Top(const VTop_Top&);  ///< Copying not allowed
  public:
    VTop_Top(const char* name="TOP");
    ~VTop_Top();
    
    // API METHODS
    
    // INTERNAL METHODS
    void __Vconfigure(VTop__Syms* symsp, bool first);
  private:
    void _ctor_var_reset();
  public:
    static void _sequent__TOP__Top__2(VTop__Syms* __restrict vlSymsp);
    static void _sequent__TOP__Top__3(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__Top__1(VTop__Syms* __restrict vlSymsp);
} VL_ATTR_ALIGNED(128);

#endif // guard
