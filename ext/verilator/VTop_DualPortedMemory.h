// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See VTop.h for the primary calling header

#ifndef _VTop_DualPortedMemory_H_
#define _VTop_DualPortedMemory_H_

#include "verilated_heavy.h"
#include "VTop__Dpi.h"

class VTop__Syms;
class VTop_DualPortedMemoryBlackBox;

//----------

VL_MODULE(VTop_DualPortedMemory) {
  public:
    // CELLS
    VTop_DualPortedMemoryBlackBox*	memory;
    
    // PORTS
    VL_IN8(__PVT__clock,0,0);
    VL_IN8(__PVT__io_dmem_memread,0,0);
    VL_IN8(__PVT__io_dmem_memwrite,0,0);
    VL_IN8(__PVT__io_dmem_maskmode,1,0);
    VL_IN8(__PVT__io_dmem_sext,0,0);
    VL_IN(__PVT__io_imem_address,31,0);
    VL_OUT(__PVT__io_imem_instruction,31,0);
    VL_IN(__PVT__io_dmem_address,31,0);
    VL_OUT(__PVT__io_dmem_readdata,31,0);
    
    // LOCAL SIGNALS
    
    // LOCAL VARIABLES
    
    // INTERNAL VARIABLES
  private:
    VTop__Syms* __VlSymsp;  // Symbol table
  public:
    
    // PARAMETERS
    
    // CONSTRUCTORS
  private:
    VTop_DualPortedMemory& operator= (const VTop_DualPortedMemory&);  ///< Copying not allowed
    VTop_DualPortedMemory(const VTop_DualPortedMemory&);  ///< Copying not allowed
  public:
    VTop_DualPortedMemory(const char* name="TOP");
    ~VTop_DualPortedMemory();
    
    // API METHODS
    
    // INTERNAL METHODS
    void __Vconfigure(VTop__Syms* symsp, bool first);
  private:
    void _ctor_var_reset();
  public:
    static void _sequent__TOP__Top__mem__3(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__Top__mem__1(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__Top__mem__2(VTop__Syms* __restrict vlSymsp);
} VL_ATTR_ALIGNED(128);

#endif // guard
