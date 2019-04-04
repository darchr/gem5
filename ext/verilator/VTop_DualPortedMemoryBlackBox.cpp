// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See VTop.h for the primary calling header

#include "VTop_DualPortedMemoryBlackBox.h" // For This
#include "VTop__Syms.h"

#include "verilated_dpi.h"


//--------------------
// STATIC VARIABLES


//--------------------

VL_CTOR_IMP(VTop_DualPortedMemoryBlackBox) {
    // Reset internal values
    // Reset structure values
    _ctor_var_reset();
}

void VTop_DualPortedMemoryBlackBox::__Vconfigure(VTop__Syms* vlSymsp, bool first) {
    if (0 && first) {}  // Prevent unused
    this->__VlSymsp = vlSymsp;
}

VTop_DualPortedMemoryBlackBox::~VTop_DualPortedMemoryBlackBox() {
}

//--------------------
// Internal Methods

VL_INLINE_OPT void VTop_DualPortedMemoryBlackBox::_sequent__TOP__Top__mem__memory__1(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          VTop_DualPortedMemoryBlackBox::_sequent__TOP__Top__mem__memory__1\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // ALWAYS at DualPortedMemoryBlackBox.v:16
    vlSymsp->TOP__Top__mem__memory.dmem_readdata = vlSymsp->TOP__Top__mem__memory.dmem_dataout;
    // ALWAYS at DualPortedMemoryBlackBox.v:20
    vlSymsp->TOP__Top__mem__memory.imem_instruction 
	= vlSymsp->TOP__Top__mem__memory.imem_dataout;
}

void VTop_DualPortedMemoryBlackBox::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+          VTop_DualPortedMemoryBlackBox::_ctor_var_reset\n"); );
    // Body
    __PVT__clk = VL_RAND_RESET_I(1);
    imem_address = VL_RAND_RESET_I(32);
    imem_instruction = VL_RAND_RESET_I(32);
    dmem_address = VL_RAND_RESET_I(32);
    dmem_writedata = VL_RAND_RESET_I(32);
    dmem_memread = VL_RAND_RESET_I(1);
    dmem_memwrite = VL_RAND_RESET_I(1);
    dmem_maskmode = VL_RAND_RESET_I(2);
    dmem_sext = VL_RAND_RESET_I(1);
    dmem_readdata = VL_RAND_RESET_I(32);
    dmem_dataout = VL_RAND_RESET_I(32);
    imem_dataout = VL_RAND_RESET_I(32);
}
