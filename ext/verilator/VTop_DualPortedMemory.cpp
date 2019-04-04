// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See VTop.h for the primary calling header

#include "VTop_DualPortedMemory.h" // For This
#include "VTop__Syms.h"

#include "verilated_dpi.h"


//--------------------
// STATIC VARIABLES


//--------------------

VL_CTOR_IMP(VTop_DualPortedMemory) {
    VL_CELL (memory, VTop_DualPortedMemoryBlackBox);
    // Reset internal values
    // Reset structure values
    _ctor_var_reset();
}

void VTop_DualPortedMemory::__Vconfigure(VTop__Syms* vlSymsp, bool first) {
    if (0 && first) {}  // Prevent unused
    this->__VlSymsp = vlSymsp;
}

VTop_DualPortedMemory::~VTop_DualPortedMemory() {
}

//--------------------
// Internal Methods

void VTop_DualPortedMemory::_settle__TOP__Top__mem__1(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+        VTop_DualPortedMemory::_settle__TOP__Top__mem__1\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlSymsp->TOP__Top__mem__memory.dmem_writedata = 0U;
    vlSymsp->TOP__Top__mem__memory.imem_address = vlSymsp->TOP__Top.__PVT__cpu__DOT__pc;
    vlSymsp->TOP__Top__mem__memory.dmem_memread = (
						   (0x33U 
						    != 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
						   & ((0x13U 
						       != 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
						      & (3U 
							 == 
							 (0x7fU 
							  & vlSymsp->TOP__Top__mem__memory.imem_instruction))));
    vlSymsp->TOP__Top__mem__memory.dmem_memwrite = 
	((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	 & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	    & ((3U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	       & (0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)))));
    vlSymsp->TOP__Top__mem__memory.dmem_maskmode = 
	(3U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
	       >> 0xcU));
    vlSymsp->TOP__Top__mem__memory.dmem_sext = (1U 
						& (~ 
						   (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						    >> 0xeU)));
}

VL_INLINE_OPT void VTop_DualPortedMemory::_settle__TOP__Top__mem__2(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+        VTop_DualPortedMemory::_settle__TOP__Top__mem__2\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlSymsp->TOP__Top__mem__memory.dmem_address = (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10);
}

VL_INLINE_OPT void VTop_DualPortedMemory::_sequent__TOP__Top__mem__3(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+        VTop_DualPortedMemory::_sequent__TOP__Top__mem__3\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlSymsp->TOP__Top__mem__memory.imem_address = vlSymsp->TOP__Top.__PVT__cpu__DOT__pc;
    vlSymsp->TOP__Top__mem__memory.dmem_memread = (
						   (0x33U 
						    != 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
						   & ((0x13U 
						       != 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
						      & (3U 
							 == 
							 (0x7fU 
							  & vlSymsp->TOP__Top__mem__memory.imem_instruction))));
    vlSymsp->TOP__Top__mem__memory.dmem_memwrite = 
	((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	 & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	    & ((3U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	       & (0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)))));
    vlSymsp->TOP__Top__mem__memory.dmem_maskmode = 
	(3U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
	       >> 0xcU));
    vlSymsp->TOP__Top__mem__memory.dmem_sext = (1U 
						& (~ 
						   (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						    >> 0xeU)));
}

void VTop_DualPortedMemory::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+        VTop_DualPortedMemory::_ctor_var_reset\n"); );
    // Body
    __PVT__clock = VL_RAND_RESET_I(1);
    __PVT__io_imem_address = VL_RAND_RESET_I(32);
    __PVT__io_imem_instruction = VL_RAND_RESET_I(32);
    __PVT__io_dmem_address = VL_RAND_RESET_I(32);
    __PVT__io_dmem_memread = VL_RAND_RESET_I(1);
    __PVT__io_dmem_memwrite = VL_RAND_RESET_I(1);
    __PVT__io_dmem_maskmode = VL_RAND_RESET_I(2);
    __PVT__io_dmem_sext = VL_RAND_RESET_I(1);
    __PVT__io_dmem_readdata = VL_RAND_RESET_I(32);
}
