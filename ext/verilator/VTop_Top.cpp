// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See VTop.h for the primary calling header

#include "VTop_Top.h"          // For This
#include "VTop__Syms.h"

#include "verilated_dpi.h"


//--------------------
// STATIC VARIABLES


//--------------------

VL_CTOR_IMP(VTop_Top) {
    VL_CELL (mem, VTop_DualPortedMemory);
    // Reset internal values
    // Reset structure values
    _ctor_var_reset();
}

void VTop_Top::__Vconfigure(VTop__Syms* vlSymsp, bool first) {
    if (0 && first) {}  // Prevent unused
    this->__VlSymsp = vlSymsp;
}

VTop_Top::~VTop_Top() {
}

//--------------------
// Internal Methods

void VTop_Top::_settle__TOP__Top__1(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+      VTop_Top::_settle__TOP__Top__1\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlSymsp->TOP__Top.__PVT__cpu__DOT___T_2 = (0x3fffffffU 
					       & ((IData)(1U) 
						  + vlSymsp->TOP__Top.__PVT__cpu__DOT__value));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__pcPlusFour_io_inputx 
	= vlSymsp->TOP__Top.__PVT__cpu__DOT__pc;
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen 
	= (((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	    | ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	       | ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		  | ((0x23U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		     & ((0x63U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			& ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			   | ((0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			      | ((0x6fU == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				 | (0x67U == (0x7fU 
					      & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))))) 
	   & (0U != (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			      >> 7U))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_jump 
	= ((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			      ? 0U : ((0x23U == (0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						    ? 0U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						     ? 0U
						     : 
						    ((0x6fU 
						      == 
						      (0x7fU 
						       & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						      ? 2U
						      : 
						     ((0x67U 
						       == 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction))
						       ? 3U
						       : 0U)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg 
	= ((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			      ? 1U : ((0x23U == (0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						    ? 0U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						     ? 0U
						     : 
						    ((0x6fU 
						      == 
						      (0x7fU 
						       & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						      ? 2U
						      : 
						     ((0x67U 
						       == 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction))
						       ? 2U
						       : 3U)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1 
	= ((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			      ? 0U : ((0x23U == (0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						    ? 1U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						     ? 2U
						     : 
						    (0x6fU 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
	= ((0x1fU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 0xfU))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				   >> 0xfU))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				       >> 0xfU))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					   >> 0xfU)))
		        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					       >> 0xfU)))
			    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xfU)))
			        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						 >> 0xfU)))
				    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						     >> 0xfU)))
				        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0xfU)))
					    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						    >> 0xfU)))
					        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0xfU)))
						    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0xfU)))
						     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							  >> 0xfU)))
						      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							   >> 0xfU)))
						       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							    >> 0xfU)))
						        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							     >> 0xfU)))
							 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							      >> 0xfU)))
							  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							       >> 0xfU)))
							   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								>> 0xfU)))
							    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								 >> 0xfU)))
							     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								  >> 0xfU)))
							      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								   >> 0xfU)))
							       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								    >> 0xfU)))
							        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								     >> 0xfU)))
								 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								      >> 0xfU)))
								  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								       >> 0xfU)))
								   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									>> 0xfU)))
								    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									 >> 0xfU)))
								     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									  >> 0xfU)))
								      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									   >> 0xfU)))
								       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									    >> 0xfU)))
								        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_1
								        : vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2 
	= ((0x1fU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 0x14U))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				   >> 0x14U))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				       >> 0x14U))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					   >> 0x14U)))
		        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					       >> 0x14U)))
			    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0x14U)))
			        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						 >> 0x14U)))
				    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						     >> 0x14U)))
				        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0x14U)))
					    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						    >> 0x14U)))
					        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0x14U)))
						    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0x14U)))
						     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							  >> 0x14U)))
						      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							   >> 0x14U)))
						       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							    >> 0x14U)))
						        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							     >> 0x14U)))
							 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							      >> 0x14U)))
							  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							       >> 0x14U)))
							   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								>> 0x14U)))
							    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								 >> 0x14U)))
							     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								  >> 0x14U)))
							      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								   >> 0x14U)))
							       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								    >> 0x14U)))
							        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								     >> 0x14U)))
								 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								      >> 0x14U)))
								  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								       >> 0x14U)))
								   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									>> 0x14U)))
								    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									 >> 0x14U)))
								     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									  >> 0x14U)))
								      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									   >> 0x14U)))
								       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									    >> 0x14U)))
								        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_1
								        : vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate 
	= ((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	   & ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	      | ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		 | ((0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		    | ((0x63U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		       & ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			  | ((0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			     | ((0x6fU != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				& (0x67U == (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26 
	= ((((0x80000000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	      ? 0xfffffU : 0U) << 0xcU) | (0xfffU & 
					   (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					    >> 0x14U)));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
	= ((0U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1))
	    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1
	    : ((1U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1))
	        ? 0U : vlSymsp->TOP__Top.__PVT__cpu__DOT__pc));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation 
	= (((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	    & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	       & ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		  | ((0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		     | ((0x63U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			& ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			   | (0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))))))))
	    ? 2U : ((0U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				  >> 0xcU))) ? (((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate) 
						 | (0U 
						    == 
						    (0x7fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0x19U))))
						 ? 2U
						 : 3U)
		     : ((1U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				      >> 0xcU))) ? 6U
			 : ((2U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					  >> 0xcU)))
			     ? 4U : ((3U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xcU)))
				      ? 5U : ((4U == 
					       (7U 
						& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xcU)))
					       ? 9U
					       : ((5U 
						   == 
						   (7U 
						    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						       >> 0xcU)))
						   ? 
						  ((0U 
						    == 
						    (0x7fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0x19U)))
						    ? 7U
						    : 8U)
						   : 
						  ((6U 
						    == 
						    (7U 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0xcU)))
						    ? 1U
						    : 
						   ((7U 
						     == 
						     (7U 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0xcU)))
						     ? 0U
						     : 0xfU)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm 
	= ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? (0xfffff000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	    : ((0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	        ? (0xfffff000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	        : ((0x6fU == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		    ? ((((0x80000000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
			  ? 0x7ffU : 0U) << 0x15U) 
		       | ((0x100000U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					>> 0xbU)) | 
			  ((0xff000U & vlSymsp->TOP__Top__mem__memory.imem_instruction) 
			   | ((0x800U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					 >> 9U)) | 
			      (0x7feU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					 >> 0x14U))))))
		    : ((0x67U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26
		        : ((0x63U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			    ? ((((0x80000000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
				  ? 0x7ffffU : 0U) 
				<< 0xdU) | ((0x1000U 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0x13U)) 
					    | ((0x800U 
						& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   << 4U)) 
					       | ((0x7e0U 
						   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						      >> 0x14U)) 
						  | (0x1eU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 7U))))))
			    : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26
			        : ((0x23U == (0x7fU 
					      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				    ? ((((0x80000000U 
					  & vlSymsp->TOP__Top__mem__memory.imem_instruction)
					  ? 0xfffffU
					  : 0U) << 0xcU) 
				       | ((0xfe0U & 
					   (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					    >> 0x14U)) 
					  | (0x1fU 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 7U))))
				    : ((0x13U == (0x7fU 
						  & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26
				        : ((0x73U == 
					    (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					    ? (0x1fU 
					       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						  >> 0xfU))
					    : 0U)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__branchAdd_io_result 
	= (vlSymsp->TOP__Top.__PVT__cpu__DOT__pc + vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm);
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy 
	= ((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate)
	    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm
	    : vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2);
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___T_3 
	= (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
	   | vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy);
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10 
	= (VL_ULL(0x7fffffffffffffff) & ((0U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
					  ? (QData)((IData)(
							    (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
							     & vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
					  : ((1U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
					      ? (QData)((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___T_3))
					      : ((2U 
						  == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						  ? (QData)((IData)(
								    (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
								     + vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						  : 
						 ((3U 
						   == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						   ? (QData)((IData)(
								     (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
								      - vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						   : 
						  ((4U 
						    == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						    ? (QData)((IData)(
								      VL_LTS_III(1,32,32, vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx, vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						    : 
						   ((5U 
						     == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						     ? (QData)((IData)(
								       (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
									< vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						     : 
						    ((6U 
						      == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						      ? 
						     ((QData)((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx)) 
						      << 
						      (0x1fU 
						       & vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy))
						      : (QData)((IData)(
									((7U 
									  == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									  ? 
									 (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
									  >> 
									  (0x1fU 
									   & vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy))
									  : 
									 ((8U 
									   == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									   ? 
									  VL_SHIFTRS_III(32,32,5, vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx, 
										(0x1fU 
										& vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy))
									   : 
									  ((9U 
									    == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									    ? 
									   (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
									    ^ vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)
									    : 
									   ((0xaU 
									     == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									     ? 
									    (~ vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___T_3)
									     : 0U))))))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata 
	= ((1U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg))
	    ? ((0x4000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	        ? vlSymsp->TOP__Top__mem__memory.dmem_readdata
	        : ((0U == (3U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				 >> 0xcU))) ? ((((0x80U 
						  & vlSymsp->TOP__Top__mem__memory.dmem_readdata)
						  ? 0xffffffU
						  : 0U) 
						<< 8U) 
					       | (0xffU 
						  & vlSymsp->TOP__Top__mem__memory.dmem_readdata))
		    : ((((0x8000U & vlSymsp->TOP__Top__mem__memory.dmem_readdata)
			  ? 0xffffU : 0U) << 0x10U) 
		       | (0xffffU & vlSymsp->TOP__Top__mem__memory.dmem_readdata))))
	    : ((2U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg))
	        ? ((IData)(4U) + vlSymsp->TOP__Top.__PVT__cpu__DOT__pc)
	        : (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10)));
}

VL_INLINE_OPT void VTop_Top::_sequent__TOP__Top__2(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+      VTop_Top::_sequent__TOP__Top__2\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // ALWAYS at Top.v:1152
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"DASM(%x)\n",32,vlSymsp->TOP__Top__mem__memory.imem_instruction);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"CYCLE=%10#\n",30,vlSymsp->TOP__Top.__PVT__cpu__DOT__value);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"pc: %10#\n",32,vlSymsp->TOP__Top.__PVT__cpu__DOT__pc);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"control: Bundle(opcode -> %3#, branch -> %1#, memread -> %1#, toreg -> %1#, add -> %1#, memwrite -> %1#, regwrite -> %1#, immediate -> %1#, alusrc1 -> %1#, jump -> %1#)\n",
		   7,(0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction),
		   1,((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & ((3U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    & ((0x23U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			       & ((0x63U == (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				  | ((0x37U != (0x7fU 
						& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				     & ((0x17U != (0x7fU 
						   & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
					& (0x6fU == 
					   (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))))))))),
		   1,((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & (3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)))),
		   2,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg),
		   1,((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    | ((0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			       | ((0x63U != (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				  & ((0x37U == (0x7fU 
						& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				     | (0x17U == (0x7fU 
						  & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))),
		   1,((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & ((3U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    & (0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))))),
		   1,((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      | ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 | ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    | ((0x23U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			       & ((0x63U != (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				  & ((0x37U == (0x7fU 
						& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				     | ((0x17U == (0x7fU 
						   & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
					| ((0x6fU == 
					    (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
					   | (0x67U 
					      == (0x7fU 
						  & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))))),
		   1,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate),
		   2,vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1,
		   2,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_jump));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"registers: Bundle(readreg1 -> %2#, readreg2 -> %2#, writereg -> %2#, writedata -> %10#, wen -> %1#, readdata1 -> %10#, readdata2 -> %10#)\n",
		   5,(0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 0xfU)),5,(0x1fU & 
					    (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					     >> 0x14U)),
		   5,(0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)),32,vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata,
		   1,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen),
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1,
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"aluControl: Bundle(add -> %1#, immediate -> %1#, funct7 -> %3#, funct3 -> %1#, operation -> %2#)\n",
		   1,((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    | ((0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			       | ((0x63U != (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				  & ((0x37U == (0x7fU 
						& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				     | (0x17U == (0x7fU 
						  & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))),
		   1,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate),
		   7,(0x7fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 0x19U)),3,(7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xcU)),
		   4,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"alu: Bundle(operation -> %2#, inputx -> %10#, inputy -> %10#, result -> %10#)\n",
		   4,vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation,
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx,
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy,
		   32,(IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"immGen: Bundle(instruction -> %10#, sextImm -> %10#)\n",
		   32,vlSymsp->TOP__Top__mem__memory.imem_instruction,
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"branchCtrl: Bundle(branch -> %1#, funct3 -> %1#, inputx -> %10#, inputy -> %10#, taken -> %1#)\n",
		   1,((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		      & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & ((3U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    & ((0x23U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			       & ((0x63U == (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				  | ((0x37U != (0x7fU 
						& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				     & ((0x17U != (0x7fU 
						   & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
					& (0x6fU == 
					   (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))))))))),
		   3,(7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			    >> 0xcU)),32,vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1,
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2,
		   1,(((0U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				     >> 0xcU))) ? (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
						   == vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
		        : ((1U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					 >> 0xcU)))
			    ? (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
			       != vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
			    : ((4U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					     >> 0xcU)))
			        ? VL_LTS_III(1,32,32, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
			        : ((5U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						 >> 0xcU)))
				    ? VL_GTES_III(1,32,32, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
				    : ((6U == (7U & 
					       (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0xcU)))
				        ? (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
					   < vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
				        : (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
					   >= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)))))) 
		      & ((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			 & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			    & ((3U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			       & ((0x23U != (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				  & ((0x63U == (0x7fU 
						& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				     | ((0x37U != (0x7fU 
						   & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
					& ((0x17U != 
					    (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
					   & (0x6fU 
					      == (0x7fU 
						  & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))))));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"pcPlusFour: Bundle(inputx -> %10#, inputy ->          4, result -> %10#)\n",
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__pc,
		   32,((IData)(4U) + vlSymsp->TOP__Top.__PVT__cpu__DOT__pc));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"branchAdd: Bundle(inputx -> %10#, inputy -> %10#, result -> %10#)\n",
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__pc,
		   32,vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm,
		   32,(vlSymsp->TOP__Top.__PVT__cpu__DOT__pc 
		       + vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"\n");
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_0 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((1U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_1 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((2U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_2 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((3U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_3 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((4U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_4 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((5U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_5 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((6U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_6 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((7U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_7 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((8U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_8 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((9U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			     >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_9 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0xaU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_10 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0xbU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_11 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0xcU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_12 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0xdU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_13 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0xeU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_14 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0xfU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_15 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x10U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_16 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x11U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_17 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x12U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_18 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x13U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_19 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x14U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_20 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x15U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_21 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x16U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_22 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x17U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_23 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x18U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_24 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x19U == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_25 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x1aU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_26 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x1bU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_27 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x1cU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_28 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x1dU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_29 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x1eU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_30 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:496
    if (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen) {
	if ((0x1fU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				>> 7U)))) {
	    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_31 
		= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:1152
    vlSymsp->TOP__Top.__PVT__cpu__DOT__value = ((IData)(vlTOPp->reset)
						 ? 0U
						 : vlSymsp->TOP__Top.__PVT__cpu__DOT___T_2);
    // ALWAYS at Top.v:1152
    vlSymsp->TOP__Top.__PVT__cpu__DOT__pc = ((IData)(vlTOPp->reset)
					      ? 0U : 
					     (((((0U 
						  == 
						  (7U 
						   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						      >> 0xcU)))
						  ? 
						 (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
						  == vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
						  : 
						 ((1U 
						   == 
						   (7U 
						    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						       >> 0xcU)))
						   ? 
						  (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
						   != vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
						   : 
						  ((4U 
						    == 
						    (7U 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0xcU)))
						    ? 
						   VL_LTS_III(1,32,32, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
						    : 
						   ((5U 
						     == 
						     (7U 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0xcU)))
						     ? 
						    VL_GTES_III(1,32,32, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1, vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
						     : 
						    ((6U 
						      == 
						      (7U 
						       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							  >> 0xcU)))
						      ? 
						     (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
						      < vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)
						      : 
						     (vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
						      >= vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2)))))) 
						& ((0x33U 
						    != 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
						   & ((0x13U 
						       != 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
						      & ((3U 
							  != 
							  (0x7fU 
							   & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
							 & ((0x23U 
							     != 
							     (0x7fU 
							      & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
							    & ((0x63U 
								== 
								(0x7fU 
								 & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
							       | ((0x37U 
								   != 
								   (0x7fU 
								    & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
								  & ((0x17U 
								      != 
								      (0x7fU 
								       & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
								     & (0x6fU 
									== 
									(0x7fU 
									 & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))))) 
					       | (2U 
						  == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_jump)))
					       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__branchAdd_io_result
					       : ((3U 
						   == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_jump))
						   ? 
						  (0xfffffffeU 
						   & (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10))
						   : 
						  ((IData)(4U) 
						   + vlSymsp->TOP__Top.__PVT__cpu__DOT__pcPlusFour_io_inputx))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT___T_2 = (0x3fffffffU 
					       & ((IData)(1U) 
						  + vlSymsp->TOP__Top.__PVT__cpu__DOT__value));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__pcPlusFour_io_inputx 
	= vlSymsp->TOP__Top.__PVT__cpu__DOT__pc;
}

VL_INLINE_OPT void VTop_Top::_sequent__TOP__Top__3(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+      VTop_Top::_sequent__TOP__Top__3\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_wen 
	= (((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	    | ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	       | ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		  | ((0x23U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		     & ((0x63U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			& ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			   | ((0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			      | ((0x6fU == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				 | (0x67U == (0x7fU 
					      & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))))) 
	   & (0U != (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			      >> 7U))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_jump 
	= ((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			      ? 0U : ((0x23U == (0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						    ? 0U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						     ? 0U
						     : 
						    ((0x6fU 
						      == 
						      (0x7fU 
						       & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						      ? 2U
						      : 
						     ((0x67U 
						       == 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction))
						       ? 3U
						       : 0U)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg 
	= ((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			      ? 1U : ((0x23U == (0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						    ? 0U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						     ? 0U
						     : 
						    ((0x6fU 
						      == 
						      (0x7fU 
						       & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						      ? 2U
						      : 
						     ((0x67U 
						       == 
						       (0x7fU 
							& vlSymsp->TOP__Top__mem__memory.imem_instruction))
						       ? 2U
						       : 3U)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1 
	= ((0x33U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			      ? 0U : ((0x23U == (0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						    ? 1U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
						     ? 2U
						     : 
						    (0x6fU 
						     == 
						     (0x7fU 
						      & vlSymsp->TOP__Top__mem__memory.imem_instruction)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1 
	= ((0x1fU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 0xfU))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				   >> 0xfU))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				       >> 0xfU))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					   >> 0xfU)))
		        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					       >> 0xfU)))
			    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xfU)))
			        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						 >> 0xfU)))
				    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						     >> 0xfU)))
				        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0xfU)))
					    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						    >> 0xfU)))
					        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0xfU)))
						    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0xfU)))
						     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							  >> 0xfU)))
						      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							   >> 0xfU)))
						       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							    >> 0xfU)))
						        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							     >> 0xfU)))
							 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							      >> 0xfU)))
							  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							       >> 0xfU)))
							   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								>> 0xfU)))
							    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								 >> 0xfU)))
							     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								  >> 0xfU)))
							      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								   >> 0xfU)))
							       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								    >> 0xfU)))
							        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								     >> 0xfU)))
								 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								      >> 0xfU)))
								  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								       >> 0xfU)))
								   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									>> 0xfU)))
								    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									 >> 0xfU)))
								     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									  >> 0xfU)))
								      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									   >> 0xfU)))
								       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									    >> 0xfU)))
								        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_1
								        : vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2 
	= ((0x1fU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
			       >> 0x14U))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				   >> 0x14U))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				       >> 0x14U))) ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					   >> 0x14U)))
		        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					       >> 0x14U)))
			    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0x14U)))
			        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						 >> 0x14U)))
				    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						     >> 0x14U)))
				        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0x14U)))
					    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						    >> 0x14U)))
					        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0x14U)))
						    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0x14U)))
						     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							  >> 0x14U)))
						      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							   >> 0x14U)))
						       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							    >> 0x14U)))
						        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							     >> 0x14U)))
							 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							      >> 0x14U)))
							  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							       >> 0x14U)))
							   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								>> 0x14U)))
							    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								 >> 0x14U)))
							     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								  >> 0x14U)))
							      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								   >> 0x14U)))
							       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								    >> 0x14U)))
							        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								     >> 0x14U)))
								 ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								      >> 0x14U)))
								  ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
								       >> 0x14U)))
								   ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									>> 0x14U)))
								    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									 >> 0x14U)))
								     ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									  >> 0x14U)))
								      ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									   >> 0x14U)))
								       ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
									    >> 0x14U)))
								        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_1
								        : vlSymsp->TOP__Top.__PVT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate 
	= ((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	   & ((0x13U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	      | ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		 | ((0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		    | ((0x63U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		       & ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			  | ((0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			     | ((0x6fU != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
				& (0x67U == (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26 
	= ((((0x80000000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	      ? 0xfffffU : 0U) << 0xcU) | (0xfffU & 
					   (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					    >> 0x14U)));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
	= ((0U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1))
	    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata1
	    : ((1U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_alusrc1))
	        ? 0U : vlSymsp->TOP__Top.__PVT__cpu__DOT__pc));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation 
	= (((0x33U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	    & ((0x13U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
	       & ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		  | ((0x23U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
		     | ((0x63U != (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			& ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction)) 
			   | (0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))))))))
	    ? 2U : ((0U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				  >> 0xcU))) ? (((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate) 
						 | (0U 
						    == 
						    (0x7fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0x19U))))
						 ? 2U
						 : 3U)
		     : ((1U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				      >> 0xcU))) ? 6U
			 : ((2U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					  >> 0xcU)))
			     ? 4U : ((3U == (7U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xcU)))
				      ? 5U : ((4U == 
					       (7U 
						& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   >> 0xcU)))
					       ? 9U
					       : ((5U 
						   == 
						   (7U 
						    & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						       >> 0xcU)))
						   ? 
						  ((0U 
						    == 
						    (0x7fU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0x19U)))
						    ? 7U
						    : 8U)
						   : 
						  ((6U 
						    == 
						    (7U 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 0xcU)))
						    ? 1U
						    : 
						   ((7U 
						     == 
						     (7U 
						      & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							 >> 0xcU)))
						     ? 0U
						     : 0xfU)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm 
	= ((0x37U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	    ? (0xfffff000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	    : ((0x17U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
	        ? (0xfffff000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	        : ((0x6fU == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		    ? ((((0x80000000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
			  ? 0x7ffU : 0U) << 0x15U) 
		       | ((0x100000U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					>> 0xbU)) | 
			  ((0xff000U & vlSymsp->TOP__Top__mem__memory.imem_instruction) 
			   | ((0x800U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					 >> 9U)) | 
			      (0x7feU & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					 >> 0x14U))))))
		    : ((0x67U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
		        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26
		        : ((0x63U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			    ? ((((0x80000000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
				  ? 0x7ffffU : 0U) 
				<< 0xdU) | ((0x1000U 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 0x13U)) 
					    | ((0x800U 
						& (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						   << 4U)) 
					       | ((0x7e0U 
						   & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						      >> 0x14U)) 
						  | (0x1eU 
						     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
							>> 7U))))))
			    : ((3U == (0x7fU & vlSymsp->TOP__Top__mem__memory.imem_instruction))
			        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26
			        : ((0x23U == (0x7fU 
					      & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				    ? ((((0x80000000U 
					  & vlSymsp->TOP__Top__mem__memory.imem_instruction)
					  ? 0xfffffU
					  : 0U) << 0xcU) 
				       | ((0xfe0U & 
					   (vlSymsp->TOP__Top__mem__memory.imem_instruction 
					    >> 0x14U)) 
					  | (0x1fU 
					     & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						>> 7U))))
				    : ((0x13U == (0x7fU 
						  & vlSymsp->TOP__Top__mem__memory.imem_instruction))
				        ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen__DOT___T_26
				        : ((0x73U == 
					    (0x7fU 
					     & vlSymsp->TOP__Top__mem__memory.imem_instruction))
					    ? (0x1fU 
					       & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
						  >> 0xfU))
					    : 0U)))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__branchAdd_io_result 
	= (vlSymsp->TOP__Top.__PVT__cpu__DOT__pc + vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm);
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy 
	= ((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_immediate)
	    ? vlSymsp->TOP__Top.__PVT__cpu__DOT__immGen_io_sextImm
	    : vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_readdata2);
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___T_3 
	= (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
	   | vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy);
    vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10 
	= (VL_ULL(0x7fffffffffffffff) & ((0U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
					  ? (QData)((IData)(
							    (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
							     & vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
					  : ((1U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
					      ? (QData)((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___T_3))
					      : ((2U 
						  == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						  ? (QData)((IData)(
								    (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
								     + vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						  : 
						 ((3U 
						   == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						   ? (QData)((IData)(
								     (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
								      - vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						   : 
						  ((4U 
						    == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						    ? (QData)((IData)(
								      VL_LTS_III(1,32,32, vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx, vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						    : 
						   ((5U 
						     == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						     ? (QData)((IData)(
								       (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
									< vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)))
						     : 
						    ((6U 
						      == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
						      ? 
						     ((QData)((IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx)) 
						      << 
						      (0x1fU 
						       & vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy))
						      : (QData)((IData)(
									((7U 
									  == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									  ? 
									 (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
									  >> 
									  (0x1fU 
									   & vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy))
									  : 
									 ((8U 
									   == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									   ? 
									  VL_SHIFTRS_III(32,32,5, vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx, 
										(0x1fU 
										& vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy))
									   : 
									  ((9U 
									    == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									    ? 
									   (vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputx 
									    ^ vlSymsp->TOP__Top.__PVT__cpu__DOT__alu_io_inputy)
									    : 
									   ((0xaU 
									     == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__aluControl_io_operation))
									     ? 
									    (~ vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___T_3)
									     : 0U))))))))))))));
    vlSymsp->TOP__Top.__PVT__cpu__DOT__registers_io_writedata 
	= ((1U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg))
	    ? ((0x4000U & vlSymsp->TOP__Top__mem__memory.imem_instruction)
	        ? vlSymsp->TOP__Top__mem__memory.dmem_readdata
	        : ((0U == (3U & (vlSymsp->TOP__Top__mem__memory.imem_instruction 
				 >> 0xcU))) ? ((((0x80U 
						  & vlSymsp->TOP__Top__mem__memory.dmem_readdata)
						  ? 0xffffffU
						  : 0U) 
						<< 8U) 
					       | (0xffU 
						  & vlSymsp->TOP__Top__mem__memory.dmem_readdata))
		    : ((((0x8000U & vlSymsp->TOP__Top__mem__memory.dmem_readdata)
			  ? 0xffffU : 0U) << 0x10U) 
		       | (0xffffU & vlSymsp->TOP__Top__mem__memory.dmem_readdata))))
	    : ((2U == (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__control_io_toreg))
	        ? ((IData)(4U) + vlSymsp->TOP__Top.__PVT__cpu__DOT__pc)
	        : (IData)(vlSymsp->TOP__Top.__PVT__cpu__DOT__alu__DOT___GEN_10)));
}

void VTop_Top::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+      VTop_Top::_ctor_var_reset\n"); );
    // Body
    clock = VL_RAND_RESET_I(1);
    reset = VL_RAND_RESET_I(1);
    __PVT__io_success = VL_RAND_RESET_I(1);
    __PVT__cpu__DOT__control_io_toreg = VL_RAND_RESET_I(2);
    __PVT__cpu__DOT__control_io_immediate = VL_RAND_RESET_I(1);
    __PVT__cpu__DOT__control_io_alusrc1 = VL_RAND_RESET_I(2);
    __PVT__cpu__DOT__control_io_jump = VL_RAND_RESET_I(2);
    __PVT__cpu__DOT__registers_io_writedata = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers_io_wen = VL_RAND_RESET_I(1);
    __PVT__cpu__DOT__registers_io_readdata1 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers_io_readdata2 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__aluControl_io_operation = VL_RAND_RESET_I(4);
    __PVT__cpu__DOT__alu_io_inputx = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__alu_io_inputy = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__immGen_io_sextImm = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__pcPlusFour_io_inputx = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__branchAdd_io_result = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__pc = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__value = VL_RAND_RESET_I(30);
    __PVT__cpu__DOT___T_2 = VL_RAND_RESET_I(30);
    __PVT__cpu__DOT__registers__DOT__regs_0 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_1 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_2 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_3 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_4 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_5 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_6 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_7 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_8 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_9 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_10 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_11 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_12 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_13 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_14 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_15 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_16 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_17 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_18 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_19 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_20 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_21 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_22 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_23 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_24 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_25 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_26 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_27 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_28 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_29 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_30 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__registers__DOT__regs_31 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__alu__DOT___T_3 = VL_RAND_RESET_I(32);
    __PVT__cpu__DOT__alu__DOT___GEN_10 = VL_RAND_RESET_Q(63);
    __PVT__cpu__DOT__immGen__DOT___T_26 = VL_RAND_RESET_I(32);
}
