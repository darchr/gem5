// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See VTop.h for the primary calling header

#ifndef _VTop_DualPortedMemoryBlackBox_H_
#define _VTop_DualPortedMemoryBlackBox_H_

#include "verilated_heavy.h"
#include "VTop__Dpi.h"

class VTop__Syms;

//----------

VL_MODULE(VTop_DualPortedMemoryBlackBox) {
  public:
    
    // PORTS
    VL_IN8(__PVT__clk,0,0);
    VL_IN8(dmem_memread,0,0);
    VL_IN8(dmem_memwrite,0,0);
    VL_IN8(dmem_maskmode,1,0);
    VL_IN8(dmem_sext,0,0);
    VL_IN(imem_address,31,0);
    VL_OUT(imem_instruction,31,0);
    VL_IN(dmem_address,31,0);
    VL_IN(dmem_writedata,31,0);
    VL_OUT(dmem_readdata,31,0);
    
    // LOCAL SIGNALS
    VL_SIG(dmem_dataout,31,0);
    VL_SIG(imem_dataout,31,0);
    
    // LOCAL VARIABLES
    
    // INTERNAL VARIABLES
  private:
    VTop__Syms* __VlSymsp;  // Symbol table
  public:
    
    // PARAMETERS
    
    // CONSTRUCTORS
  private:
    VTop_DualPortedMemoryBlackBox& operator= (const VTop_DualPortedMemoryBlackBox&);  ///< Copying not allowed
    VTop_DualPortedMemoryBlackBox(const VTop_DualPortedMemoryBlackBox&);  ///< Copying not allowed
  public:
    VTop_DualPortedMemoryBlackBox(const char* name="TOP");
    ~VTop_DualPortedMemoryBlackBox();
    
    // API METHODS
    
    // INTERNAL METHODS
    void __Vconfigure(VTop__Syms* symsp, bool first);
  private:
    void _ctor_var_reset();
  public:
    static void _sequent__TOP__Top__mem__memory__1(VTop__Syms* __restrict vlSymsp);
} VL_ATTR_ALIGNED(128);

#endif // guard
