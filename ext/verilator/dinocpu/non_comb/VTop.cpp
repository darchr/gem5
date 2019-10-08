// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See VTop.h for the primary calling header

#include "VTop.h"              // For This
#include "VTop__Syms.h"

#include "verilated_dpi.h"


//--------------------
// STATIC VARIABLES


//--------------------

VL_CTOR_IMP(VTop) {
    VTop__Syms* __restrict vlSymsp = __VlSymsp = new VTop__Syms(this, name());
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Reset internal values
    
    // Reset structure values
    _ctor_var_reset();
}

void VTop::__Vconfigure(VTop__Syms* vlSymsp, bool first) {
    if (0 && first) {}  // Prevent unused
    this->__VlSymsp = vlSymsp;
}

VTop::~VTop() {
    delete __VlSymsp; __VlSymsp=NULL;
}

//--------------------


void VTop::eval() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate VTop::eval\n"); );
    VTop__Syms* __restrict vlSymsp = this->__VlSymsp;  // Setup global symbol table
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
#ifdef VL_DEBUG
    // Debug assertions
    _eval_debug_assertions();
#endif // VL_DEBUG
    // Initialize
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) _eval_initial_loop(vlSymsp);
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    while (VL_LIKELY(__Vchange)) {
	VL_DEBUG_IF(VL_DBG_MSGF("+ Clock loop\n"););
	_eval(vlSymsp);
	__Vchange = _change_request(vlSymsp);
	if (VL_UNLIKELY(++__VclockLoop > 100)) VL_FATAL_MT(__FILE__,__LINE__,__FILE__,"Verilated model didn't converge");
    }
}

void VTop::_eval_initial_loop(VTop__Syms* __restrict vlSymsp) {
    vlSymsp->__Vm_didInit = true;
    _eval_initial(vlSymsp);
    int __VclockLoop = 0;
    QData __Vchange = 1;
    while (VL_LIKELY(__Vchange)) {
	_eval_settle(vlSymsp);
	_eval(vlSymsp);
	__Vchange = _change_request(vlSymsp);
	if (VL_UNLIKELY(++__VclockLoop > 100)) VL_FATAL_MT(__FILE__,__LINE__,__FILE__,"Verilated model didn't DC converge");
    }
}

//--------------------
// Internal Methods

VL_INLINE_OPT void VTop::__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__ifetch_TOP(CData imem_request_ready, CData imem_request_valid, IData imem_request_bits_address, CData& imem_response_valid, QData handle, IData& ifetch__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__ifetch_TOP\n"); );
    // Body
    unsigned char imem_request_ready__Vcvt;
    imem_request_ready__Vcvt = imem_request_ready;
    unsigned char imem_request_valid__Vcvt;
    imem_request_valid__Vcvt = imem_request_valid;
    int imem_request_bits_address__Vcvt;
    imem_request_bits_address__Vcvt = imem_request_bits_address;
    unsigned char imem_response_valid__Vcvt;
    void* handle__Vcvt;
    handle__Vcvt = VL_CVT_Q_VP(handle);
    int ifetch__Vfuncrtn__Vcvt = ifetch(imem_request_ready__Vcvt, imem_request_valid__Vcvt, imem_request_bits_address__Vcvt, &imem_response_valid__Vcvt, handle__Vcvt);
    imem_response_valid = (1U & imem_response_valid__Vcvt);
    ifetch__Vfuncrtn = ifetch__Vfuncrtn__Vcvt;
}

VL_INLINE_OPT void VTop::__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__datareq_TOP(CData dmem_request_ready, CData dmem_request_valid, IData dmem_request_bits_address, IData dmem_request_bits_writedata, CData dmem_request_bits_operation, CData& dmem_response_valid, QData handle, IData& datareq__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__datareq_TOP\n"); );
    // Body
    unsigned char dmem_request_ready__Vcvt;
    dmem_request_ready__Vcvt = dmem_request_ready;
    unsigned char dmem_request_valid__Vcvt;
    dmem_request_valid__Vcvt = dmem_request_valid;
    int dmem_request_bits_address__Vcvt;
    dmem_request_bits_address__Vcvt = dmem_request_bits_address;
    int dmem_request_bits_writedata__Vcvt;
    dmem_request_bits_writedata__Vcvt = dmem_request_bits_writedata;
    unsigned char dmem_request_bits_operation__Vcvt;
    dmem_request_bits_operation__Vcvt = dmem_request_bits_operation;
    unsigned char dmem_response_valid__Vcvt;
    void* handle__Vcvt;
    handle__Vcvt = VL_CVT_Q_VP(handle);
    int datareq__Vfuncrtn__Vcvt = datareq(dmem_request_ready__Vcvt, dmem_request_valid__Vcvt, dmem_request_bits_address__Vcvt, dmem_request_bits_writedata__Vcvt, dmem_request_bits_operation__Vcvt, &dmem_response_valid__Vcvt, handle__Vcvt);
    dmem_response_valid = (1U & dmem_response_valid__Vcvt);
    datareq__Vfuncrtn = datareq__Vfuncrtn__Vcvt;
}

VL_INLINE_OPT void VTop::__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__setGem5Handle_TOP(QData& setGem5Handle__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__setGem5Handle_TOP\n"); );
    // Body
    void* setGem5Handle__Vfuncrtn__Vcvt = setGem5Handle();
    setGem5Handle__Vfuncrtn = VL_CVT_VP_Q(setGem5Handle__Vfuncrtn__Vcvt);
}

VL_INLINE_OPT void VTop::_combo__TOP__1(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_combo__TOP__1\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->io_dmem_good = vlTOPp->Top__DOT__memory_io_dmem_response_valid;
    vlTOPp->io_imem_good = vlTOPp->io_imem_valid;
    // ALWAYS at DualPortedSyncMemoryBlackBox.v:65
    vlTOPp->__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__ifetch_TOP(vlTOPp->Top__DOT__memory__DOT__memory_imem_request_ready, (IData)(vlTOPp->io_imem_valid), vlTOPp->io_imem_address, vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__imem_response_valid, vlTOPp->Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__Vfuncout);
    vlTOPp->Top__DOT__memory__DOT__memory_imem_response_bits_data 
	= vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__Vfuncout;
    vlTOPp->io_imem_instruction = vlTOPp->Top__DOT__memory__DOT__memory_imem_response_bits_data;
}

void VTop::_settle__TOP__2(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_settle__TOP__2\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Variables
    VL_SIGW(__Vtemp2,95,0,3);
    VL_SIGW(__Vtemp3,95,0,3);
    VL_SIGW(__Vtemp4,95,0,3);
    VL_SIGW(__Vtemp6,95,0,3);
    VL_SIGW(__Vtemp7,95,0,3);
    VL_SIGW(__Vtemp8,95,0,3);
    VL_SIGW(__Vtemp9,95,0,3);
    VL_SIGW(__Vtemp10,95,0,3);
    VL_SIGW(__Vtemp15,95,0,3);
    VL_SIGW(__Vtemp16,95,0,3);
    VL_SIGW(__Vtemp17,95,0,3);
    VL_SIGW(__Vtemp19,95,0,3);
    // Body
    vlTOPp->io_dmem_good = vlTOPp->Top__DOT__memory_io_dmem_response_valid;
    vlTOPp->io_imem_good = vlTOPp->io_imem_valid;
    vlTOPp->Top__DOT__dmem__DOT___T_47 = ((0x1fU >= 
					   (0x18U & 
					    (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
					     << 3U)))
					   ? (vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data 
					      >> (0x18U 
						  & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						     << 3U)))
					   : 0U);
    VL_EXTEND_WI(71,32, __Vtemp2, vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data);
    __Vtemp3[0U] = 0xffU;
    __Vtemp3[1U] = 0U;
    __Vtemp3[2U] = 0U;
    VL_SHIFTL_WWI(71,71,6, __Vtemp4, __Vtemp3, (0x18U 
						& (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						   << 3U)));
    __Vtemp6[0U] = (__Vtemp2[0U] & (~ __Vtemp4[0U]));
    __Vtemp6[1U] = (__Vtemp2[1U] & (~ __Vtemp4[1U]));
    __Vtemp6[2U] = (__Vtemp2[2U] & (~ __Vtemp4[2U]));
    VL_EXTEND_WW(79,71, __Vtemp7, __Vtemp6);
    VL_EXTEND_WI(79,32, __Vtemp8, vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data);
    __Vtemp9[0U] = 0xffffU;
    __Vtemp9[1U] = 0U;
    __Vtemp9[2U] = 0U;
    VL_SHIFTL_WWI(79,79,6, __Vtemp10, __Vtemp9, (0x18U 
						 & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						    << 3U)));
    VL_EXTEND_WI(95,32, __Vtemp15, ((0U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
				     ? __Vtemp7[0U]
				     : (__Vtemp8[0U] 
					& (~ __Vtemp10[0U]))));
    VL_EXTEND_WI(95,32, __Vtemp16, vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_writedata);
    VL_SHIFTL_WWI(95,95,6, __Vtemp17, __Vtemp16, (0x18U 
						  & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						     << 3U)));
    VL_EXTEND_WI(95,32, __Vtemp19, vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_writedata);
    vlTOPp->Top__DOT__dmem__DOT___GEN_17 = ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation)
					     ? ((2U 
						 != (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
						 ? 
						(__Vtemp15[0U] 
						 | __Vtemp17[0U])
						 : 
						__Vtemp19[0U])
					     : 0U);
    vlTOPp->Top__DOT__dmem_io_request_bits_operation 
	= ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid) 
	   & (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation));
    vlTOPp->Top__DOT__dmem__DOT__sending = (((IData)(vlTOPp->io_dmem_valid) 
					     & ((IData)(vlTOPp->io_dmem_memread) 
						| (IData)(vlTOPp->io_dmem_memwrite))) 
					    & ((~ (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid)) 
					       | ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid) 
						  & ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid) 
						     & (~ (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation))))));
    vlTOPp->Top__DOT__dmem__DOT___GEN_12 = ((0U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
					     ? (0xffU 
						& vlTOPp->Top__DOT__dmem__DOT___T_47)
					     : ((1U 
						 == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
						 ? 
						(0xffffU 
						 & vlTOPp->Top__DOT__dmem__DOT___T_47)
						 : vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data));
    vlTOPp->Top__DOT__dmem__DOT___T_65 = ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid) 
					  | (IData)(vlTOPp->Top__DOT__dmem__DOT__sending));
    vlTOPp->Top__DOT__dmem_io_request_valid = ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
					        ? ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation) 
						   | (IData)(vlTOPp->Top__DOT__dmem__DOT__sending))
					        : (IData)(vlTOPp->Top__DOT__dmem__DOT__sending));
    vlTOPp->io_dmem_readdata = ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_sext)
				 ? ((0U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
				     ? ((((0x80U & vlTOPp->Top__DOT__dmem__DOT___GEN_12)
					   ? 0xffffffU
					   : 0U) << 8U) 
					| (0xffU & vlTOPp->Top__DOT__dmem__DOT___GEN_12))
				     : ((1U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
					 ? ((((0x8000U 
					       & vlTOPp->Top__DOT__dmem__DOT___GEN_12)
					       ? 0xffffU
					       : 0U) 
					     << 0x10U) 
					    | (0xffffU 
					       & vlTOPp->Top__DOT__dmem__DOT___GEN_12))
					 : vlTOPp->Top__DOT__dmem__DOT___GEN_12))
				 : vlTOPp->Top__DOT__dmem__DOT___GEN_12);
}

void VTop::_initial__TOP__3(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_initial__TOP__3\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Variables
    VL_SIG64(__Vfunc_Top__DOT__memory__DOT__memory__DOT__setGem5Handle__0__Vfuncout,63,0);
    // Body
    // INITIAL at DualPortedSyncMemoryBlackBox.v:61
    // Function: setGem5Handle at DualPortedSyncMemoryBlackBox.v:62
    vlTOPp->__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__setGem5Handle_TOP(__Vfunc_Top__DOT__memory__DOT__memory__DOT__setGem5Handle__0__Vfuncout);
    vlTOPp->Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox 
	= __Vfunc_Top__DOT__memory__DOT__memory__DOT__setGem5Handle__0__Vfuncout;
}

VL_INLINE_OPT void VTop::_sequent__TOP__4(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_sequent__TOP__4\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // ALWAYS at Top.v:221
    if (VL_UNLIKELY(((IData)(vlTOPp->Top__DOT__dmem__DOT__sending) 
		     & (~ ((~ ((IData)(vlTOPp->io_dmem_memread) 
			       & (IData)(vlTOPp->io_dmem_memwrite))) 
			   | (IData)(vlTOPp->reset)))))) {
	VL_FWRITEF(0x80000002U,"Assertion failed\n    at memory-ports.scala:78 assert (! (io.memread && io.memwrite))\n");
    }
    if (VL_UNLIKELY(((IData)(vlTOPp->Top__DOT__dmem__DOT__sending) 
		     & (~ ((~ ((IData)(vlTOPp->io_dmem_memread) 
			       & (IData)(vlTOPp->io_dmem_memwrite))) 
			   | (IData)(vlTOPp->reset)))))) {
	VL_WRITEF("[%0t] %%Error: Top.v:280: Assertion failed in %NTop.dmem\n",
		  64,VL_TIME_Q(),vlSymsp->name());
	VL_STOP_MT("Top.v",280,"");
    }
    if (VL_UNLIKELY(((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid) 
		     & (~ ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid) 
			   | (IData)(vlTOPp->reset)))))) {
	VL_FWRITEF(0x80000002U,"Assertion failed\n    at memory-ports.scala:107 assert(outstandingReq.valid)\n");
    }
    if (VL_UNLIKELY(((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid) 
		     & (~ ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid) 
			   | (IData)(vlTOPp->reset)))))) {
	VL_WRITEF("[%0t] %%Error: Top.v:302: Assertion failed in %NTop.dmem\n",
		  64,VL_TIME_Q(),vlSymsp->name());
	VL_STOP_MT("Top.v",302,"");
    }
    // ALWAYS at Top.v:221
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_sext = 0U;
    } else {
	if (vlTOPp->Top__DOT__dmem__DOT__sending) {
	    vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_sext 
		= vlTOPp->io_dmem_sext;
	}
    }
    // ALWAYS at Top.v:221
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_writedata = 0U;
    } else {
	if (vlTOPp->Top__DOT__dmem__DOT__sending) {
	    vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_writedata 
		= vlTOPp->io_dmem_writedata;
	}
    }
    // ALWAYS at Top.v:221
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode = 0U;
    } else {
	if (vlTOPp->Top__DOT__dmem__DOT__sending) {
	    vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode 
		= vlTOPp->io_dmem_maskmode;
	}
    }
    // ALWAYS at Top.v:221
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address = 0U;
    } else {
	if (vlTOPp->Top__DOT__dmem__DOT__sending) {
	    vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
		= vlTOPp->io_dmem_address;
	}
    }
    // ALWAYS at Top.v:221
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation = 0U;
    } else {
	if (vlTOPp->Top__DOT__dmem__DOT__sending) {
	    vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation 
		= vlTOPp->io_dmem_memwrite;
	}
    }
    // ALWAYS at Top.v:221
    vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid 
	= ((~ (IData)(vlTOPp->reset)) & ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
					  ? (IData)(vlTOPp->Top__DOT__dmem__DOT__sending)
					  : (IData)(vlTOPp->Top__DOT__dmem__DOT___T_65)));
}

void VTop::_settle__TOP__5(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_settle__TOP__5\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // ALWAYS at DualPortedSyncMemoryBlackBox.v:65
    vlTOPp->__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__ifetch_TOP(vlTOPp->Top__DOT__memory__DOT__memory_imem_request_ready, (IData)(vlTOPp->io_imem_valid), vlTOPp->io_imem_address, vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__imem_response_valid, vlTOPp->Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__Vfuncout);
    vlTOPp->Top__DOT__memory__DOT__memory_imem_response_bits_data 
	= vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__Vfuncout;
    // ALWAYS at DualPortedSyncMemoryBlackBox.v:69
    vlTOPp->__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__datareq_TOP(vlTOPp->Top__DOT__memory__DOT__memory_dmem_request_ready, (IData)(vlTOPp->Top__DOT__dmem_io_request_valid), 
									 ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
									   ? 
									  ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation)
									    ? vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address
									    : vlTOPp->io_dmem_address)
									   : vlTOPp->io_dmem_address), 
									 ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
									   ? vlTOPp->Top__DOT__dmem__DOT___GEN_17
									   : 0U), (IData)(vlTOPp->Top__DOT__dmem_io_request_bits_operation), vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__dmem_response_valid, vlTOPp->Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__Vfuncout);
    vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_valid 
	= vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__dmem_response_valid;
    vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data 
	= vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__Vfuncout;
    vlTOPp->io_imem_instruction = vlTOPp->Top__DOT__memory__DOT__memory_imem_response_bits_data;
    vlTOPp->Top__DOT__memory_io_dmem_response_valid 
	= ((IData)(vlTOPp->Top__DOT__dmem_io_request_valid) 
	   & ((~ (IData)(vlTOPp->Top__DOT__dmem_io_request_bits_operation)) 
	      | ((~ (IData)(vlTOPp->Top__DOT__dmem_io_request_bits_operation)) 
		 & (IData)(vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_valid))));
}

VL_INLINE_OPT void VTop::_combo__TOP__6(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_combo__TOP__6\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Variables
    VL_SIGW(__Vtemp24,95,0,3);
    VL_SIGW(__Vtemp25,95,0,3);
    VL_SIGW(__Vtemp26,95,0,3);
    VL_SIGW(__Vtemp28,95,0,3);
    VL_SIGW(__Vtemp29,95,0,3);
    VL_SIGW(__Vtemp30,95,0,3);
    VL_SIGW(__Vtemp31,95,0,3);
    VL_SIGW(__Vtemp32,95,0,3);
    VL_SIGW(__Vtemp37,95,0,3);
    VL_SIGW(__Vtemp38,95,0,3);
    VL_SIGW(__Vtemp39,95,0,3);
    VL_SIGW(__Vtemp41,95,0,3);
    // Body
    vlTOPp->Top__DOT__dmem__DOT___T_47 = ((0x1fU >= 
					   (0x18U & 
					    (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
					     << 3U)))
					   ? (vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data 
					      >> (0x18U 
						  & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						     << 3U)))
					   : 0U);
    VL_EXTEND_WI(71,32, __Vtemp24, vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data);
    __Vtemp25[0U] = 0xffU;
    __Vtemp25[1U] = 0U;
    __Vtemp25[2U] = 0U;
    VL_SHIFTL_WWI(71,71,6, __Vtemp26, __Vtemp25, (0x18U 
						  & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						     << 3U)));
    __Vtemp28[0U] = (__Vtemp24[0U] & (~ __Vtemp26[0U]));
    __Vtemp28[1U] = (__Vtemp24[1U] & (~ __Vtemp26[1U]));
    __Vtemp28[2U] = (__Vtemp24[2U] & (~ __Vtemp26[2U]));
    VL_EXTEND_WW(79,71, __Vtemp29, __Vtemp28);
    VL_EXTEND_WI(79,32, __Vtemp30, vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data);
    __Vtemp31[0U] = 0xffffU;
    __Vtemp31[1U] = 0U;
    __Vtemp31[2U] = 0U;
    VL_SHIFTL_WWI(79,79,6, __Vtemp32, __Vtemp31, (0x18U 
						  & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						     << 3U)));
    VL_EXTEND_WI(95,32, __Vtemp37, ((0U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
				     ? __Vtemp29[0U]
				     : (__Vtemp30[0U] 
					& (~ __Vtemp32[0U]))));
    VL_EXTEND_WI(95,32, __Vtemp38, vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_writedata);
    VL_SHIFTL_WWI(95,95,6, __Vtemp39, __Vtemp38, (0x18U 
						  & (vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address 
						     << 3U)));
    VL_EXTEND_WI(95,32, __Vtemp41, vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_writedata);
    vlTOPp->Top__DOT__dmem__DOT___GEN_17 = ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation)
					     ? ((2U 
						 != (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
						 ? 
						(__Vtemp37[0U] 
						 | __Vtemp39[0U])
						 : 
						__Vtemp41[0U])
					     : 0U);
    vlTOPp->Top__DOT__dmem_io_request_bits_operation 
	= ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid) 
	   & (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation));
    vlTOPp->Top__DOT__dmem__DOT__sending = (((IData)(vlTOPp->io_dmem_valid) 
					     & ((IData)(vlTOPp->io_dmem_memread) 
						| (IData)(vlTOPp->io_dmem_memwrite))) 
					    & ((~ (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid)) 
					       | ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid) 
						  & ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid) 
						     & (~ (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation))))));
    vlTOPp->Top__DOT__dmem__DOT___GEN_12 = ((0U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
					     ? (0xffU 
						& vlTOPp->Top__DOT__dmem__DOT___T_47)
					     : ((1U 
						 == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
						 ? 
						(0xffffU 
						 & vlTOPp->Top__DOT__dmem__DOT___T_47)
						 : vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data));
    vlTOPp->Top__DOT__dmem__DOT___T_65 = ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_valid) 
					  | (IData)(vlTOPp->Top__DOT__dmem__DOT__sending));
    vlTOPp->Top__DOT__dmem_io_request_valid = ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
					        ? ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation) 
						   | (IData)(vlTOPp->Top__DOT__dmem__DOT__sending))
					        : (IData)(vlTOPp->Top__DOT__dmem__DOT__sending));
    vlTOPp->io_dmem_readdata = ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_sext)
				 ? ((0U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
				     ? ((((0x80U & vlTOPp->Top__DOT__dmem__DOT___GEN_12)
					   ? 0xffffffU
					   : 0U) << 8U) 
					| (0xffU & vlTOPp->Top__DOT__dmem__DOT___GEN_12))
				     : ((1U == (IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_maskmode))
					 ? ((((0x8000U 
					       & vlTOPp->Top__DOT__dmem__DOT___GEN_12)
					       ? 0xffffU
					       : 0U) 
					     << 0x10U) 
					    | (0xffffU 
					       & vlTOPp->Top__DOT__dmem__DOT___GEN_12))
					 : vlTOPp->Top__DOT__dmem__DOT___GEN_12))
				 : vlTOPp->Top__DOT__dmem__DOT___GEN_12);
    // ALWAYS at DualPortedSyncMemoryBlackBox.v:69
    vlTOPp->__Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__datareq_TOP(vlTOPp->Top__DOT__memory__DOT__memory_dmem_request_ready, (IData)(vlTOPp->Top__DOT__dmem_io_request_valid), 
									 ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
									   ? 
									  ((IData)(vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_operation)
									    ? vlTOPp->Top__DOT__dmem__DOT__outstandingReq_bits_address
									    : vlTOPp->io_dmem_address)
									   : vlTOPp->io_dmem_address), 
									 ((IData)(vlTOPp->Top__DOT__memory_io_dmem_response_valid)
									   ? vlTOPp->Top__DOT__dmem__DOT___GEN_17
									   : 0U), (IData)(vlTOPp->Top__DOT__dmem_io_request_bits_operation), vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__dmem_response_valid, vlTOPp->Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__Vfuncout);
    vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_valid 
	= vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__dmem_response_valid;
    vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data 
	= vlTOPp->__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__Vfuncout;
    vlTOPp->Top__DOT__memory_io_dmem_response_valid 
	= ((IData)(vlTOPp->Top__DOT__dmem_io_request_valid) 
	   & ((~ (IData)(vlTOPp->Top__DOT__dmem_io_request_bits_operation)) 
	      | ((~ (IData)(vlTOPp->Top__DOT__dmem_io_request_bits_operation)) 
		 & (IData)(vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_valid))));
}

void VTop::_eval(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_eval\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_combo__TOP__1(vlSymsp);
    if (((IData)(vlTOPp->clock) & (~ (IData)(vlTOPp->__Vclklast__TOP__clock)))) {
	vlTOPp->_sequent__TOP__4(vlSymsp);
    }
    vlTOPp->_combo__TOP__6(vlSymsp);
    // Final
    vlTOPp->__Vclklast__TOP__clock = vlTOPp->clock;
}

void VTop::_eval_initial(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_eval_initial\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_initial__TOP__3(vlSymsp);
}

void VTop::final() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::final\n"); );
    // Variables
    VTop__Syms* __restrict vlSymsp = this->__VlSymsp;
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
}

void VTop::_eval_settle(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_eval_settle\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_settle__TOP__2(vlSymsp);
    vlTOPp->_settle__TOP__5(vlSymsp);
}

VL_INLINE_OPT QData VTop::_change_request(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_change_request\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // Change detection
    QData __req = false;  // Logically a bool
    __req |= ((vlTOPp->Top__DOT__memory_io_dmem_response_valid ^ vlTOPp->__Vchglast__TOP__Top__DOT__memory_io_dmem_response_valid)
	 | (vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data ^ vlTOPp->__Vchglast__TOP__Top__DOT__memory__DOT__memory_dmem_response_bits_data));
    VL_DEBUG_IF( if(__req && ((vlTOPp->Top__DOT__memory_io_dmem_response_valid ^ vlTOPp->__Vchglast__TOP__Top__DOT__memory_io_dmem_response_valid))) VL_DBG_MSGF("        CHANGE: Top.v:421: Top.memory_io_dmem_response_valid\n"); );
    VL_DEBUG_IF( if(__req && ((vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data ^ vlTOPp->__Vchglast__TOP__Top__DOT__memory__DOT__memory_dmem_response_bits_data))) VL_DBG_MSGF("        CHANGE: Top.v:335: Top.memory.memory_dmem_response_bits_data\n"); );
    // Final
    vlTOPp->__Vchglast__TOP__Top__DOT__memory_io_dmem_response_valid 
	= vlTOPp->Top__DOT__memory_io_dmem_response_valid;
    vlTOPp->__Vchglast__TOP__Top__DOT__memory__DOT__memory_dmem_response_bits_data 
	= vlTOPp->Top__DOT__memory__DOT__memory_dmem_response_bits_data;
    return __req;
}

#ifdef VL_DEBUG
void VTop::_eval_debug_assertions() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((clock & 0xfeU))) {
	Verilated::overWidthError("clock");}
    if (VL_UNLIKELY((reset & 0xfeU))) {
	Verilated::overWidthError("reset");}
    if (VL_UNLIKELY((io_imem_valid & 0xfeU))) {
	Verilated::overWidthError("io_imem_valid");}
    if (VL_UNLIKELY((io_dmem_valid & 0xfeU))) {
	Verilated::overWidthError("io_dmem_valid");}
    if (VL_UNLIKELY((io_dmem_memread & 0xfeU))) {
	Verilated::overWidthError("io_dmem_memread");}
    if (VL_UNLIKELY((io_dmem_memwrite & 0xfeU))) {
	Verilated::overWidthError("io_dmem_memwrite");}
    if (VL_UNLIKELY((io_dmem_maskmode & 0xfcU))) {
	Verilated::overWidthError("io_dmem_maskmode");}
    if (VL_UNLIKELY((io_dmem_sext & 0xfeU))) {
	Verilated::overWidthError("io_dmem_sext");}
}
#endif // VL_DEBUG

void VTop::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_ctor_var_reset\n"); );
    // Body
    clock = VL_RAND_RESET_I(1);
    reset = VL_RAND_RESET_I(1);
    io_imem_address = VL_RAND_RESET_I(32);
    io_imem_valid = VL_RAND_RESET_I(1);
    io_imem_instruction = VL_RAND_RESET_I(32);
    io_imem_good = VL_RAND_RESET_I(1);
    io_dmem_address = VL_RAND_RESET_I(32);
    io_dmem_valid = VL_RAND_RESET_I(1);
    io_dmem_writedata = VL_RAND_RESET_I(32);
    io_dmem_memread = VL_RAND_RESET_I(1);
    io_dmem_memwrite = VL_RAND_RESET_I(1);
    io_dmem_maskmode = VL_RAND_RESET_I(2);
    io_dmem_sext = VL_RAND_RESET_I(1);
    io_dmem_readdata = VL_RAND_RESET_I(32);
    io_dmem_good = VL_RAND_RESET_I(1);
    Top__DOT__dmem_io_request_valid = VL_RAND_RESET_I(1);
    Top__DOT__dmem_io_request_bits_operation = VL_RAND_RESET_I(1);
    Top__DOT__memory_io_dmem_response_valid = VL_RAND_RESET_I(1);
    Top__DOT__dmem__DOT__outstandingReq_valid = VL_RAND_RESET_I(1);
    Top__DOT__dmem__DOT__outstandingReq_bits_address = VL_RAND_RESET_I(32);
    Top__DOT__dmem__DOT__outstandingReq_bits_writedata = VL_RAND_RESET_I(32);
    Top__DOT__dmem__DOT__outstandingReq_bits_maskmode = VL_RAND_RESET_I(2);
    Top__DOT__dmem__DOT__outstandingReq_bits_operation = VL_RAND_RESET_I(1);
    Top__DOT__dmem__DOT__outstandingReq_bits_sext = VL_RAND_RESET_I(1);
    Top__DOT__dmem__DOT__sending = VL_RAND_RESET_I(1);
    Top__DOT__dmem__DOT___T_47 = VL_RAND_RESET_I(32);
    Top__DOT__dmem__DOT___GEN_12 = VL_RAND_RESET_I(32);
    Top__DOT__dmem__DOT___GEN_17 = VL_RAND_RESET_I(32);
    Top__DOT__dmem__DOT___T_65 = VL_RAND_RESET_I(1);
    Top__DOT__memory__DOT__memory_imem_request_ready = VL_RAND_RESET_I(1);
    Top__DOT__memory__DOT__memory_imem_response_bits_data = VL_RAND_RESET_I(32);
    Top__DOT__memory__DOT__memory_dmem_request_ready = VL_RAND_RESET_I(1);
    Top__DOT__memory__DOT__memory_dmem_response_valid = VL_RAND_RESET_I(1);
    Top__DOT__memory__DOT__memory_dmem_response_bits_data = VL_RAND_RESET_I(32);
    Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox = 0;
    __Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__Vfuncout = 0;
    __Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__imem_response_valid = 0;
    __Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__Vfuncout = 0;
    __Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__dmem_response_valid = 0;
    __Vclklast__TOP__clock = VL_RAND_RESET_I(1);
    __Vchglast__TOP__Top__DOT__memory_io_dmem_response_valid = VL_RAND_RESET_I(1);
    __Vchglast__TOP__Top__DOT__memory__DOT__memory_dmem_response_bits_data = VL_RAND_RESET_I(32);
}
