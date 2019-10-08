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

VL_INLINE_OPT void VTop::__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__ifetch_TOP(IData imem_address, QData handle, IData& ifetch__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__ifetch_TOP\n"); );
    // Body
    int imem_address__Vcvt;
    imem_address__Vcvt = imem_address;
    void* handle__Vcvt;
    handle__Vcvt = VL_CVT_Q_VP(handle);
    int ifetch__Vfuncrtn__Vcvt = ifetch(imem_address__Vcvt, handle__Vcvt);
    ifetch__Vfuncrtn = ifetch__Vfuncrtn__Vcvt;
}

VL_INLINE_OPT void VTop::__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__datareq_TOP(IData dmem_address, IData dmem_writedata, CData dmem_memread, CData dmem_memwrite, CData dmem_maskmode, CData dmem_sext, QData handle, IData& datareq__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__datareq_TOP\n"); );
    // Body
    int dmem_address__Vcvt;
    dmem_address__Vcvt = dmem_address;
    int dmem_writedata__Vcvt;
    dmem_writedata__Vcvt = dmem_writedata;
    unsigned char dmem_memread__Vcvt;
    dmem_memread__Vcvt = dmem_memread;
    unsigned char dmem_memwrite__Vcvt;
    dmem_memwrite__Vcvt = dmem_memwrite;
    svBitVecVal dmem_maskmode__Vcvt;
    dmem_maskmode__Vcvt = dmem_maskmode;
    unsigned char dmem_sext__Vcvt;
    dmem_sext__Vcvt = dmem_sext;
    void* handle__Vcvt;
    handle__Vcvt = VL_CVT_Q_VP(handle);
    int datareq__Vfuncrtn__Vcvt = datareq(dmem_address__Vcvt, dmem_writedata__Vcvt, dmem_memread__Vcvt, dmem_memwrite__Vcvt, &dmem_maskmode__Vcvt, dmem_sext__Vcvt, handle__Vcvt);
    datareq__Vfuncrtn = datareq__Vfuncrtn__Vcvt;
}

VL_INLINE_OPT void VTop::__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__setGem5Handle_TOP(QData& setGem5Handle__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__setGem5Handle_TOP\n"); );
    // Body
    void* setGem5Handle__Vfuncrtn__Vcvt = setGem5Handle();
    setGem5Handle__Vfuncrtn = VL_CVT_VP_Q(setGem5Handle__Vfuncrtn__Vcvt);
}

void VTop::_initial__TOP__1(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_initial__TOP__1\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Variables
    VL_SIG64(__Vfunc_Top__DOT__mem__DOT__memory__DOT__setGem5Handle__0__Vfuncout,63,0);
    // Body
    // INITIAL at Top.v:3260
    vlTOPp->io_success = 0U;
    // INITIAL at DualPortedMemoryBlackBox.v:51
    // Function: setGem5Handle at DualPortedMemoryBlackBox.v:52
    vlTOPp->__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__setGem5Handle_TOP(__Vfunc_Top__DOT__mem__DOT__memory__DOT__setGem5Handle__0__Vfuncout);
    vlTOPp->Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox 
	= __Vfunc_Top__DOT__mem__DOT__memory__DOT__setGem5Handle__0__Vfuncout;
}

VL_INLINE_OPT void VTop::_sequent__TOP__2(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_sequent__TOP__2\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // ALWAYS at Top.v:2974
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"DASM(%x)\n",32,vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"CYCLE=%10#\n",30,vlTOPp->Top__DOT__cpu__DOT__value);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"pc: %x\n",32,vlTOPp->Top__DOT__cpu__DOT__pc);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"control: AnonymousBundle(opcode -> %3#, validinst -> %1#, branch -> %1#, memread -> %1#, toreg -> %1#, add -> %1#, memwrite -> %1#, regwrite -> %1#, immediate -> %1#, alusrc1 -> %1#, jump -> %1#)\n",
		   7,(0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction),
		   1,(IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_validinst),
		   1,((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & ((3U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    & ((0x23U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			       & (0x63U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))),
		   1,((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & (3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))),
		   2,(3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)),
		   1,((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			       | ((0x63U != (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				  & ((0x37U == (0x7fU 
						& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				     | (0x17U == (0x7fU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))),
		   1,((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & ((3U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    & (0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))),
		   1,((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      | ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    | ((0x23U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			       & ((0x63U != (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				  & ((0x37U == (0x7fU 
						& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				     | ((0x17U == (0x7fU 
						   & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
					| ((0x6fU == 
					    (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
					   | (0x67U 
					      == (0x7fU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))))),
		   1,(IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_immediate),
		   2,vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1,
		   2,(IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_jump));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"registers: AnonymousBundle(readreg1 -> %2#, readreg2 -> %2#, writereg -> %2#, writedata -> %10#, wen -> %1#, readdata1 -> %10#, readdata2 -> %10#)\n",
		   5,(0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 0xfU)),5,(0x1fU & 
					    (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					     >> 0x14U)),
		   5,(0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)),32,vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata,
		   1,(IData)(vlTOPp->Top__DOT__cpu__DOT__registers_io_wen),
		   32,vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1,
		   32,vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"csr: AnonymousBundle(illegal_inst -> %1#, retire_inst -> 1, pc -> %10#, read_data -> %10#, inst -> %10#, immid -> %10#, read_illegal -> %1#, write_illegal -> %1#, system_illegal -> 0, csr_stall -> 0, eret -> %1#, evec -> %10#, write_data -> %10#, reg_write -> %1#, status -> MStatus(sd -> 0, wpri1 ->   0, tsr -> 0, tw -> 0, tvm -> 0, mxr -> 0, sum -> 0, mprv -> 0, xs -> 0, fs -> 0, mpp -> 3, wpri2 -> 0, spp -> 0, mpie -> %1#, wpri3 -> 0, spie -> 0, upie -> 0, mie -> %1#, wpri4 -> 0, sie -> 0, uie -> 0), time -> %10#)\n",
		   1,vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst,
		   32,vlTOPp->Top__DOT__cpu__DOT__pc,
		   32,vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1,
		   32,vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction,
		   32,vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm,
		   1,(1U & (~ (((((((((((((((((((0x320U 
						 == 
						 (0xfffU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U))) 
						| (0xb00U 
						   == 
						   (0xfffU 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0x14U)))) 
					       | (0xb02U 
						  == 
						  (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))) 
					      | (0xf13U 
						 == 
						 (0xfffU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))) 
					     | (0xf12U 
						== 
						(0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))) 
					    | (0xf11U 
					       == (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))) 
					   | (0x301U 
					      == (0xfffU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))) 
					  | (0x300U 
					     == (0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))) 
					 | (0x305U 
					    == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
					| (0x344U == 
					   (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))) 
				       | (0x304U == 
					  (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))) 
				      | (0x340U == 
					 (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))) 
				     | (0x341U == (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))) 
				    | (0x343U == (0xfffU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))) 
				   | (0x342U == (0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))) 
				  | (0xf14U == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
				 | (0x302U == (0xfffU 
					       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) 
				| (0xb80U == (0xfffU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))) 
			       | (0xb82U == (0xfffU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))))),
		   1,(3U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				   >> 0x1eU))),1,(((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call) 
						   | (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)) 
						  | (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_ret)),
		   32,((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)
		        ? 0x80000008U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call)
					  ? (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtvec_base 
					     << 2U)
					  : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334)
					      ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc
					      : 0x80000000U))),
		   32,(IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374),
		   1,((0x73U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((3U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				       >> 0xcU))) | 
			 ((7U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					>> 0xcU))) 
			  | ((2U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0xcU))) 
			     | ((6U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0xcU))) 
				| ((1U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0xcU))) 
				   | (5U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xcU))))))))),
		   1,(IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie),
		   1,vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie,
		   32,(IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"aluControl: AnonymousBundle(add -> %1#, immediate -> %1#, funct7 -> %3#, funct3 -> %1#, operation -> %2#)\n",
		   1,((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			       | ((0x63U != (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				  & ((0x37U == (0x7fU 
						& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				     | (0x17U == (0x7fU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))),
		   1,(IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_immediate),
		   7,(0x7fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 0x19U)),3,(7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xcU)),
		   4,(IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"alu: AnonymousBundle(operation -> %2#, inputx -> %10#, inputy -> %10#, result -> %10#)\n",
		   4,vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation,
		   32,vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx,
		   32,vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy,
		   32,(IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"immGen: AnonymousBundle(instruction -> %10#, sextImm -> %10#)\n",
		   32,vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction,
		   32,vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"branchCtrl: AnonymousBundle(branch -> %1#, funct3 -> %1#, inputx -> %10#, inputy -> %10#, taken -> %1#)\n",
		   1,((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		      & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & ((3U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    & ((0x23U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			       & (0x63U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))),
		   3,(7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			    >> 0xcU)),32,vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1,
		   32,vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2,
		   1,(((0U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0xcU))) ? (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
						   == vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
		        : ((1U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					 >> 0xcU)))
			    ? (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
			       != vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
			    : ((4U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					     >> 0xcU)))
			        ? VL_LTS_III(1,32,32, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
			        : ((5U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0xcU)))
				    ? VL_GTES_III(1,32,32, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
				    : ((6U == (7U & 
					       (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0xcU)))
				        ? (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
					   < vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
				        : (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
					   >= vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)))))) 
		      & ((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			 & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			    & ((3U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			       & ((0x23U != (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				  & (0x63U == (0x7fU 
					       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"pcPlusFour: AnonymousBundle(inputx -> %10#, inputy ->          4, result -> %10#)\n",
		   32,vlTOPp->Top__DOT__cpu__DOT__pc,
		   32,((IData)(4U) + vlTOPp->Top__DOT__cpu__DOT__pc));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"branchAdd: AnonymousBundle(inputx -> %10#, inputy -> %10#, result -> %10#)\n",
		   32,vlTOPp->Top__DOT__cpu__DOT__pc,
		   32,vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm,
		   32,(vlTOPp->Top__DOT__cpu__DOT__pc 
		       + vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm));
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"\n");
    }
    // ALWAYS at Top.v:523
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 0: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_0);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 1: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_1);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 2: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_2);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 3: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_3);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 4: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_4);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 5: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_5);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 6: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_6);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 7: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_7);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 8: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_8);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 9: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_9);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 10: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_10);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 11: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_11);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 12: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_12);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 13: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_13);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 14: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_14);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 15: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_15);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 16: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_16);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 17: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_17);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 18: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_18);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 19: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_19);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 20: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_20);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 21: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_21);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 22: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_22);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 23: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_23);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 24: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_24);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 25: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_25);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 26: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_26);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 27: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_27);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 28: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_28);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 29: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_29);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 30: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_30);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"reg 31: %x ",32,vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_31);
    }
    if (VL_UNLIKELY((1U & (~ (IData)(vlTOPp->reset))))) {
	VL_FWRITEF(0x80000002U,"\n");
    }
    // ALWAYS at Top.v:1901
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_mtix 
	= (1U & (~ (IData)(vlTOPp->reset)));
    // ALWAYS at Top.v:1901
    if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	if ((0x343U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				  >> 0x14U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtval 
		= vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata;
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	if ((0x340U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				  >> 0x14U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mscratch 
		= vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata;
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	if ((0x302U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				  >> 0x14U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_medeleg 
		= vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata;
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_msix = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x344U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_msix 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 3U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_meix = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x304U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_meix 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xbU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_mtix = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x304U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_mtix 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 7U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_msix = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x304U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_msix 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 3U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm31 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm31 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x1fU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm30 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm30 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x1eU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm29 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm29 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x1dU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm28 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm28 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x1cU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm27 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm27 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x1bU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm26 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm26 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x1aU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm25 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm25 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x19U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm24 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm24 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x18U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm23 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm23 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x17U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm22 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm22 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x16U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm21 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm21 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x15U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm20 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm20 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x14U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm19 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm19 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x13U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm18 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm18 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x12U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm17 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm17 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x11U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm16 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm16 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0x10U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm15 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm15 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xfU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm14 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm14 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xeU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm13 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm13 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xdU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm12 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm12 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xcU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm11 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm11 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xbU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm10 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm10 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 0xaU));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm9 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm9 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 9U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm8 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm8 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 8U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm7 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm7 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 7U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm6 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm6 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 6U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm5 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm5 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 5U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm4 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm4 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 4U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm3 = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm3 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 3U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_tmzero = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_tmzero 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 1U));
	    }
	}
    }
    // ALWAYS at Top.v:1901
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt 
	= (1U & ((~ (IData)(vlTOPp->reset)) & ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
					        ? (
						   (0x342U 
						    == 
						    (0xfffU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x14U)))
						    ? 
						   (0x80000000U 
						    & vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)
						    : vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_30)
					        : vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_30)));
    // ALWAYS at Top.v:1901
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode 
	= (0x7fffffffU & ((IData)(vlTOPp->reset) ? 0U
			   : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
			       ? ((0x342U == (0xfffU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))
				   ? (0x1fU & vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)
				   : vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_31)
			       : vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_31)));
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 = VL_ULL(0);
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy) {
		    if ((0x40U & ((IData)(1U) + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)))) {
			vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
			    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_66;
		    }
		} else {
		    if ((0xb00U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U)))) {
			vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
			    = (VL_ULL(0x3ffffffffffffff) 
			       & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_475 
				  >> 6U));
		    } else {
			if ((0xb80U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) {
			    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
				= (VL_ULL(0x3ffffffffffffff) 
				   & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_472 
				      >> 6U));
			} else {
			    if ((0x40U & ((IData)(1U) 
					  + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)))) {
				vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
				    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_66;
			    }
			}
		    }
		}
	    } else {
		if ((0x40U & ((IData)(1U) + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)))) {
		    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
			= vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_66;
		}
	    }
	} else {
	    if ((0x40U & ((IData)(1U) + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
		    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_66;
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 = VL_ULL(0);
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir) {
		    if ((0x40U & ((IData)(1U) + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)))) {
			vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
			    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_73;
		    }
		} else {
		    if ((0xb02U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U)))) {
			vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
			    = (VL_ULL(0x3ffffffffffffff) 
			       & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_483 
				  >> 6U));
		    } else {
			if ((0xb82U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) {
			    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
				= (VL_ULL(0x3ffffffffffffff) 
				   & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_480 
				      >> 6U));
			} else {
			    if ((0x40U & ((IData)(1U) 
					  + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)))) {
				vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
				    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_73;
			    }
			}
		    }
		}
	    } else {
		if ((0x40U & ((IData)(1U) + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)))) {
		    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
			= vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_73;
		}
	    }
	} else {
	    if ((0x40U & ((IData)(1U) + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
		    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_73;
	    }
	}
    }
    // ALWAYS at Top.v:2974
    vlTOPp->Top__DOT__cpu__DOT__value = ((IData)(vlTOPp->reset)
					  ? 0U : vlTOPp->Top__DOT__cpu__DOT___T_2);
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie = 1U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x300U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 3U));
	    } else {
		if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334) {
		    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie 
			= vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie;
		}
	    }
	} else {
	    if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie 
		    = vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie;
	    }
	}
    }
    // ALWAYS at Top.v:2974
    vlTOPp->Top__DOT__cpu__DOT__pc = ((IData)(vlTOPp->reset)
				       ? 0U : (((((0U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU)))
						   ? 
						  (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
						   == vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
						   : 
						  ((1U 
						    == 
						    (7U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xcU)))
						    ? 
						   (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
						    != vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
						    : 
						   ((4U 
						     == 
						     (7U 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xcU)))
						     ? 
						    VL_LTS_III(1,32,32, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
						     : 
						    ((5U 
						      == 
						      (7U 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xcU)))
						      ? 
						     VL_GTES_III(1,32,32, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1, vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
						      : 
						     ((6U 
						       == 
						       (7U 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0xcU)))
						       ? 
						      (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
						       < vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)
						       : 
						      (vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
						       >= vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2)))))) 
						 & ((0x33U 
						     != 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
						    & ((0x13U 
							!= 
							(0x7fU 
							 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
						       & ((3U 
							   != 
							   (0x7fU 
							    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
							  & ((0x23U 
							      != 
							      (0x7fU 
							       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
							     & (0x63U 
								== 
								(0x7fU 
								 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))) 
						| (2U 
						   == (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_jump)))
					        ? vlTOPp->Top__DOT__cpu__DOT__branchAdd_io_result
					        : (
						   (3U 
						    == (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_jump))
						    ? 
						   (0xfffffffeU 
						    & (IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10))
						    : 
						   ((1U 
						     & ((((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call) 
							  | (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)) 
							 | (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_ret)) 
							| (~ (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_validinst))))
						     ? 
						    ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)
						      ? 0x80000008U
						      : 
						     ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call)
						       ? 
						      (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtvec_base 
						       << 2U)
						       : 
						      ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334)
						        ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc
						        : 0x80000000U)))
						     : 
						    ((IData)(4U) 
						     + vlTOPp->Top__DOT__cpu__DOT__pcPlusFour_io_inputx)))));
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_0 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((1U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_1 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((2U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_2 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((3U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_3 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((4U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_4 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((5U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_5 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((6U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_6 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((7U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_7 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((8U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_8 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((9U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			     >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_9 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0xaU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_10 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0xbU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_11 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0xcU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_12 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0xdU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_13 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0xeU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_14 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0xfU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_15 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x10U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_16 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x11U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_17 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x12U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_18 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x13U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_19 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x14U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_20 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x15U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_21 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x16U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_22 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x17U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_23 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x18U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_24 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x19U == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_25 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x1aU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_26 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x1bU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_27 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x1cU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_28 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x1dU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_29 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x1eU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_30 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    // ALWAYS at Top.v:523
    if (vlTOPp->Top__DOT__cpu__DOT__registers_io_wen) {
	if ((0x1fU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				>> 7U)))) {
	    vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_31 
		= vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata;
	}
    }
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_66 = (VL_ULL(0x3ffffffffffffff) 
						   & (VL_ULL(1) 
						      + vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63));
    // ALWAYS at Top.v:1901
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61 = (0x3fU 
						   & ((IData)(vlTOPp->reset)
						       ? 0U
						       : (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_176)));
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy 
		    = (1U & vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata);
	    }
	}
    }
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_73 = (VL_ULL(0x3ffffffffffffff) 
						   & (VL_ULL(1) 
						      + vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70));
    // ALWAYS at Top.v:1901
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68 = (0x3fU 
						   & ((IData)(vlTOPp->reset)
						       ? 0U
						       : (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_178)));
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 2U));
	    }
	}
    }
    vlTOPp->Top__DOT__cpu__DOT___T_2 = (0x3fffffffU 
					& ((IData)(1U) 
					   + vlTOPp->Top__DOT__cpu__DOT__value));
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x300U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie 
		    = (1U & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
			     >> 7U));
	    } else {
		if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334) {
		    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie = 1U;
		}
	    }
	} else {
	    if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie = 1U;
	    }
	}
    }
    // ALWAYS at Top.v:1901
    if (vlTOPp->reset) {
	vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtvec_base = 0U;
    } else {
	if (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen) {
	    if ((0x305U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0x14U)))) {
		vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtvec_base 
		    = (0x3fffffffU & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
				      >> 2U));
	    }
	}
    }
    vlTOPp->Top__DOT__cpu__DOT__pcPlusFour_io_inputx 
	= vlTOPp->Top__DOT__cpu__DOT__pc;
    // ALWAYS at Top.v:1901
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc 
	= (IData)(((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
		    ? ((0x341U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					     >> 0x14U)))
		        ? ((QData)((IData)((0x3fffffffU 
					    & (vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata 
					       >> 2U)))) 
			   << 2U) : (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_21)))
		    : (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_21))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67 = (
						   (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
						    << 6U) 
						   | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74 = (
						   (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
						    << 6U) 
						   | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)));
    // ALWAYS at DualPortedMemoryBlackBox.v:55
    vlTOPp->__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__ifetch_TOP(vlTOPp->Top__DOT__cpu__DOT__pc, vlTOPp->Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__ifetch__1__Vfuncout);
    vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
	= vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__ifetch__1__Vfuncout;
    vlTOPp->Top__DOT__cpu__DOT__registers_io_wen = 
	((((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	   | ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	      | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		 | ((0x23U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		    & ((0x63U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		       & ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			  | ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			     | ((0x6fU == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				| (0x67U == (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))))) 
	  | ((0x73U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	     & ((3U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			      >> 0xcU))) | ((7U == 
					     (7U & 
					      (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0xcU))) 
					    | ((2U 
						== 
						(7U 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0xcU))) 
					       | ((6U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU))) 
						  | ((1U 
						      == 
						      (7U 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xcU))) 
						     | (5U 
							== 
							(7U 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0xcU)))))))))) 
	 & (0U != (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			    >> 7U))));
    vlTOPp->Top__DOT__cpu__DOT__control_io_jump = (
						   (0x33U 
						    == 
						    (0x7fU 
						     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						    ? 0U
						    : 
						   ((0x13U 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						     ? 0U
						     : 
						    ((3U 
						      == 
						      (0x7fU 
						       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						      ? 0U
						      : 
						     ((0x23U 
						       == 
						       (0x7fU 
							& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						       ? 0U
						       : 
						      ((0x63U 
							== 
							(0x7fU 
							 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						        ? 0U
						        : 
						       ((0x37U 
							 == 
							 (0x7fU 
							  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							 ? 0U
							 : 
							((0x17U 
							  == 
							  (0x7fU 
							   & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							  ? 0U
							  : 
							 ((0x6fU 
							   == 
							   (0x7fU 
							    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							   ? 2U
							   : 
							  ((0x67U 
							    == 
							    (0x7fU 
							     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							    ? 3U
							    : 0U)))))))));
    vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3 
	= ((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			      ? 1U : ((0x23U == (0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						    ? 0U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						     ? 0U
						     : 
						    ((0x6fU 
						      == 
						      (0x7fU 
						       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						      ? 2U
						      : 
						     ((0x67U 
						       == 
						       (0x7fU 
							& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						       ? 2U
						       : 
						      ((0x73U 
							== 
							(0x7fU 
							 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						        ? 3U
						        : 4U))))))))));
    vlTOPp->Top__DOT__cpu__DOT__control_io_validinst 
	= ((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	   | ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	      | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		 | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		    | ((0x63U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		       | ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			  | ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			     | ((0x6fU == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				| ((0x67U == (0x7fU 
					      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				   | ((0x73U == (0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				      | (0xfU == (0x7fU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374 = 
	(((((((((((((QData)((IData)(((0x320U == (0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))
				      ? (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm31) 
					  << 0x1fU) 
					 | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm30) 
					     << 0x1eU) 
					    | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm29) 
						<< 0x1dU) 
					       | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm28) 
						   << 0x1cU) 
						  | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm27) 
						      << 0x1bU) 
						     | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm26) 
							 << 0x1aU) 
							| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm25) 
							    << 0x19U) 
							   | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm24) 
							       << 0x18U) 
							      | ((((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm23) 
								   << 0x17U) 
								  | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm22) 
								      << 0x16U) 
								     | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm21) 
									 << 0x15U) 
									| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm20) 
									    << 0x14U) 
									   | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm19) 
									       << 0x13U) 
									      | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm18) 
										<< 0x12U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm17) 
										<< 0x11U) 
										| ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm16) 
										<< 0x10U)))))))) 
								 | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm15) 
								     << 0xfU) 
								    | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm14) 
									<< 0xeU) 
								       | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm13) 
									   << 0xdU) 
									  | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm12) 
									      << 0xcU) 
									     | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm11) 
										<< 0xbU) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm10) 
										<< 0xaU) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm9) 
										<< 9U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm8) 
										<< 8U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm7) 
										<< 7U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm6) 
										<< 6U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm5) 
										<< 5U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm4) 
										<< 4U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm3) 
										<< 3U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir) 
										<< 2U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_tmzero) 
										<< 1U) 
										| (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy)))))))))))))))))))))))))
				      : 0U))) | ((0xb00U 
						  == 
						  (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))
						  ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67
						  : VL_ULL(0))) 
		   | ((0xb02U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))
		       ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74
		       : VL_ULL(0))) | (QData)((IData)(
						       ((0x301U 
							 == 
							 (0xfffU 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0x14U)))
							 ? 0x10U
							 : 0U)))) 
		 | (QData)((IData)(((0x300U == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))
				     ? (0x1800U | (
						   ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie) 
						    << 7U) 
						   | ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie) 
						      << 3U)))
				     : 0U)))) | (QData)((IData)(
								((0x305U 
								  == 
								  (0xfffU 
								   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								      >> 0x14U)))
								  ? 0x80000000U
								  : 0U)))) 
	       | (QData)((IData)(((0x344U == (0xfffU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))
				   ? (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_mtix) 
				       << 7U) | ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_msix) 
						 << 3U))
				   : 0U)))) | (QData)((IData)(
							      ((0x304U 
								== 
								(0xfffU 
								 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								    >> 0x14U)))
							        ? 
							       (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_meix) 
								 << 0xbU) 
								| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_mtix) 
								    << 7U) 
								   | ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_msix) 
								      << 3U)))
							        : 0U)))) 
	     | (QData)((IData)(((0x340U == (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))
				 ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mscratch
				 : 0U)))) | (QData)((IData)(
							    ((0x341U 
							      == 
							      (0xfffU 
							       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								  >> 0x14U)))
							      ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc
							      : 0U)))) 
	   | (QData)((IData)(((0x343U == (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))
			       ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtval
			       : 0U)))) | (QData)((IData)(
							  ((0x342U 
							    == 
							    (0xfffU 
							     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								>> 0x14U)))
							    ? 
							   (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt) 
							     << 0x1fU) 
							    | vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode)
							    : 0U)))) 
	 | (QData)((IData)(((0x302U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))
			     ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_medeleg
			     : 0U))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd = ((0x73U 
						  == 
						  (0x7fU 
						   & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						  ? 
						 ((3U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU)))
						   ? 3U
						   : 
						  ((7U 
						    == 
						    (7U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xcU)))
						    ? 3U
						    : 
						   ((2U 
						     == 
						     (7U 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xcU)))
						     ? 2U
						     : 
						    ((6U 
						      == 
						      (7U 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xcU)))
						      ? 2U
						      : 
						     ((1U 
						       == 
						       (7U 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0xcU)))
						       ? 1U
						       : 
						      ((5U 
							== 
							(7U 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0xcU)))
						        ? 1U
						        : 
						       ((0U 
							 == 
							 (7U 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0xcU)))
							 ? 4U
							 : 3U)))))))
						  : 0U);
    vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1 
	= ((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			      ? 0U : ((0x23U == (0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						    ? 1U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						     ? 2U
						     : 
						    (0x6fU 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))));
    vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2 
	= ((0x1fU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 0x14U))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				   >> 0x14U))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				       >> 0x14U))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))
		        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))
			    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))
			        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))
				    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))
				        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))
					    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))
					        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x14U)))
						    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0x14U)))
						     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0x14U)))
						      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0x14U)))
						       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0x14U)))
						        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0x14U)))
							 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							      >> 0x14U)))
							  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							       >> 0x14U)))
							   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								>> 0x14U)))
							    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								 >> 0x14U)))
							     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								  >> 0x14U)))
							      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								   >> 0x14U)))
							       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								    >> 0x14U)))
							        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								     >> 0x14U)))
								 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								      >> 0x14U)))
								  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								       >> 0x14U)))
								   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									>> 0x14U)))
								    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									 >> 0x14U)))
								     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									  >> 0x14U)))
								      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									   >> 0x14U)))
								       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									    >> 0x14U)))
								        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_1
								        : vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__control_io_immediate 
	= ((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	   & ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	      | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		 | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		    | ((0x63U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		       & ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			  | ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			     | ((0x6fU != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				& (0x67U == (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))))));
    vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
	= ((0x1fU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 0xfU))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				   >> 0xfU))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				       >> 0xfU))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0xfU)))
		        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0xfU)))
			    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xfU)))
			        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0xfU)))
				    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0xfU)))
				        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0xfU)))
					    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0xfU)))
					        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xfU)))
						    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xfU)))
						     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xfU)))
						      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0xfU)))
						       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0xfU)))
						        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0xfU)))
							 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							      >> 0xfU)))
							  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							       >> 0xfU)))
							   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								>> 0xfU)))
							    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								 >> 0xfU)))
							     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								  >> 0xfU)))
							      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								   >> 0xfU)))
							       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								    >> 0xfU)))
							        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								     >> 0xfU)))
								 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								      >> 0xfU)))
								  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								       >> 0xfU)))
								   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									>> 0xfU)))
								    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									 >> 0xfU)))
								     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									  >> 0xfU)))
								      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									   >> 0xfU)))
								       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									    >> 0xfU)))
								        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_1
								        : vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26 
	= ((((0x80000000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	      ? 0xfffffU : 0U) << 0xcU) | (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)));
    vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst 
	= (1U & (((~ (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_validinst)) 
		  | (~ (((((((((((((((((((0x320U == 
					  (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U))) 
					 | (0xb00U 
					    == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
					| (0xb02U == 
					   (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))) 
				       | (0xf13U == 
					  (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))) 
				      | (0xf12U == 
					 (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))) 
				     | (0xf11U == (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))) 
				    | (0x301U == (0xfffU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))) 
				   | (0x300U == (0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))) 
				  | (0x305U == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
				 | (0x344U == (0xfffU 
					       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) 
				| (0x304U == (0xfffU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))) 
			       | (0x340U == (0xfffU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))) 
			      | (0x341U == (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))) 
			     | (0x343U == (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))) 
			    | (0x342U == (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))) 
			   | (0xf14U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
			  | (0x302U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) 
			 | (0xb80U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))) 
			| (0xb82U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))))) 
		 | (3U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				 >> 0x1eU)))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_ret 
	= (0x3fU & ((4U == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
		    & (((IData)(1U) << (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U))) 
		       >> 2U)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call 
	= (0xffU & ((4U == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
		    & ((IData)(1U) << (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					     >> 0x14U)))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break 
	= (0x7fU & ((4U == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
		    & (((IData)(1U) << (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U))) 
		       >> 1U)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen = (((
						   (0U 
						    != (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
						   & (4U 
						      != (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd))) 
						  & (5U 
						     != (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd))) 
						 & (3U 
						    != 
						    (3U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x1eU))));
    vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation 
	= (((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	    & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	       & ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		  | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		     | ((0x63U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			& ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			   | (0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))))
	    ? 2U : ((0U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				  >> 0xcU))) ? (((IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_immediate) 
						 | (0U 
						    == 
						    (0x7fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x19U))))
						 ? 2U
						 : 3U)
		     : ((1U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0xcU))) ? 6U
			 : ((2U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					  >> 0xcU)))
			     ? 4U : ((3U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xcU)))
				      ? 5U : ((4U == 
					       (7U 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xcU)))
					       ? 9U
					       : ((5U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU)))
						   ? 
						  ((0U 
						    == 
						    (0x7fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x19U)))
						    ? 7U
						    : 8U)
						   : 
						  ((6U 
						    == 
						    (7U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xcU)))
						    ? 1U
						    : 
						   ((7U 
						     == 
						     (7U 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xcU)))
						     ? 0U
						     : 0xfU)))))))));
    vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx = ((0U 
						  == (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1))
						  ? vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1
						  : 
						 ((1U 
						   == (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1))
						   ? 0U
						   : vlTOPp->Top__DOT__cpu__DOT__pc));
    vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm = 
	((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	  ? (0xfffff000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	  : ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	      ? (0xfffff000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	      : ((0x6fU == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		  ? ((((0x80000000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
		        ? 0x7ffU : 0U) << 0x15U) | 
		     ((0x100000U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				    >> 0xbU)) | ((0xff000U 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction) 
						 | ((0x800U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 9U)) 
						    | (0x7feU 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0x14U))))))
		  : ((0x67U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		      ? vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26
		      : ((0x63U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			  ? ((((0x80000000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
			        ? 0x7ffffU : 0U) << 0xdU) 
			     | ((0x1000U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x13U)) 
				| ((0x800U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      << 4U)) 
				   | ((0x7e0U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)) 
				      | (0x1eU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 7U))))))
			  : ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			      ? vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26
			      : ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				  ? ((((0x80000000U 
					& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
				        ? 0xfffffU : 0U) 
				      << 0xcU) | ((0xfe0U 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)) 
						  | (0x1fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 7U))))
				  : ((0x13U == (0x7fU 
						& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				      ? vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26
				      : ((0x73U == 
					  (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
					  ? (0x1fU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0xfU))
					  : 0U)))))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_21 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst)
	  ? vlTOPp->Top__DOT__cpu__DOT__pc : vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_ret) 
	 & (~ (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
	       >> 0x1eU)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_30 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)
	  ? 0U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call)
		   ? 0U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst)
			    ? 0U : (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_31 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)
	  ? 3U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call)
		   ? 0xbU : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst)
			      ? 2U : vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode)));
    vlTOPp->Top__DOT__cpu__DOT__branchAdd_io_result 
	= (vlTOPp->Top__DOT__cpu__DOT__pc + vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_266 = 
	((0x4000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	  ? vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm
	  : vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1);
    vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy = ((IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_immediate)
						  ? vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm
						  : vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata = (
						   ((((2U 
						       == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
						      | (3U 
							 == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)))
						      ? (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374)
						      : 0U) 
						    | vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_266) 
						   & (~ 
						      ((3U 
							== (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd))
						        ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_266
						        : 0U)));
    vlTOPp->Top__DOT__cpu__DOT__alu__DOT___T_3 = (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
						  | vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_472 = 
	(((QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)) 
	  << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_475 = 
	(((QData)((IData)((vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67 
			   >> 0x20U))) << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_480 = 
	(((QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)) 
	  << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_483 = 
	(((QData)((IData)((vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74 
			   >> 0x20U))) << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)));
    vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10 = 
	(VL_ULL(0x7fffffffffffffff) & ((0U == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
				        ? (QData)((IData)(
							  (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
							   & vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
				        : ((1U == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
					    ? (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___T_3))
					    : ((2U 
						== (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
					        ? (QData)((IData)(
								  (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
								   + vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
					        : (
						   (3U 
						    == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						    ? (QData)((IData)(
								      (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
								       - vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
						    : 
						   ((4U 
						     == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						     ? (QData)((IData)(
								       VL_LTS_III(1,32,32, vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx, vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
						     : 
						    ((5U 
						      == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						      ? (QData)((IData)(
									(vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
									 < vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
						      : 
						     ((6U 
						       == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						       ? 
						      ((QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx)) 
						       << 
						       (0x1fU 
							& vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy))
						       : (QData)((IData)(
									 ((7U 
									   == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									   ? 
									  (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
									   >> 
									   (0x1fU 
									    & vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy))
									   : 
									  ((8U 
									    == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									    ? 
									   VL_SHIFTRS_III(32,32,5, vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx, 
										(0x1fU 
										& vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy))
									    : 
									   ((9U 
									     == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									     ? 
									    (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
									     ^ vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)
									     : 
									    ((0xaU 
									      == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									      ? 
									     (~ vlTOPp->Top__DOT__cpu__DOT__alu__DOT___T_3)
									      : 0U))))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_176 
	= ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
	    ? ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0x14U))) ? 
	       ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy)
		 ? (QData)((IData)((0x7fU & ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)))))
		 : ((0xb00U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					  >> 0x14U)))
		     ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_475
		     : ((0xb80U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U)))
			 ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_472
			 : (QData)((IData)((0x7fU & 
					    ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61))))))))
	        : (QData)((IData)((0x7fU & ((IData)(1U) 
					    + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61))))))
	    : (QData)((IData)((0x7fU & ((IData)(1U) 
					+ (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_178 
	= ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
	    ? ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0x14U))) ? 
	       ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir)
		 ? (QData)((IData)((0x7fU & ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)))))
		 : ((0xb02U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					  >> 0x14U)))
		     ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_483
		     : ((0xb82U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U)))
			 ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_480
			 : (QData)((IData)((0x7fU & 
					    ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68))))))))
	        : (QData)((IData)((0x7fU & ((IData)(1U) 
					    + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68))))))
	    : (QData)((IData)((0x7fU & ((IData)(1U) 
					+ (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68))))));
    // ALWAYS at DualPortedMemoryBlackBox.v:57
    vlTOPp->__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__datareq_TOP((IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10), vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2, 
								      ((0x33U 
									!= 
									(0x7fU 
									 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
								       & ((0x13U 
									   != 
									   (0x7fU 
									    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
									  & (3U 
									     == 
									     (0x7fU 
									      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))), 
								      ((0x33U 
									!= 
									(0x7fU 
									 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
								       & ((0x13U 
									   != 
									   (0x7fU 
									    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
									  & ((3U 
									      != 
									      (0x7fU 
									       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
									     & (0x23U 
										== 
										(0x7fU 
										& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))), 
								      (3U 
								       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									  >> 0xcU)), 
								      (1U 
								       & (~ 
									  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									   >> 0xeU))), vlTOPp->Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__datareq__2__Vfuncout);
    vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata 
	= vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__datareq__2__Vfuncout;
    vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata 
	= ((1U == (3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)))
	    ? ((0x4000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	        ? vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata
	        : ((0U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				 >> 0xcU))) ? ((((0x80U 
						  & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata)
						  ? 0xffffffU
						  : 0U) 
						<< 8U) 
					       | (0xffU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata))
		    : ((1U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0xcU))) ? (
						   (((0x8000U 
						      & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata)
						      ? 0xffffU
						      : 0U) 
						    << 0x10U) 
						   | (0xffffU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata))
		        : vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata)))
	    : ((2U == (3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)))
	        ? ((IData)(4U) + vlTOPp->Top__DOT__cpu__DOT__pc)
	        : ((3U == (3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)))
		    ? (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374)
		    : (IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10))));
}

void VTop::_settle__TOP__3(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_settle__TOP__3\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_66 = (VL_ULL(0x3ffffffffffffff) 
						   & (VL_ULL(1) 
						      + vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_73 = (VL_ULL(0x3ffffffffffffff) 
						   & (VL_ULL(1) 
						      + vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70));
    vlTOPp->Top__DOT__cpu__DOT___T_2 = (0x3fffffffU 
					& ((IData)(1U) 
					   + vlTOPp->Top__DOT__cpu__DOT__value));
    vlTOPp->Top__DOT__cpu__DOT__pcPlusFour_io_inputx 
	= vlTOPp->Top__DOT__cpu__DOT__pc;
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67 = (
						   (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_63 
						    << 6U) 
						   | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74 = (
						   (vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_70 
						    << 6U) 
						   | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)));
    // ALWAYS at DualPortedMemoryBlackBox.v:55
    vlTOPp->__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__ifetch_TOP(vlTOPp->Top__DOT__cpu__DOT__pc, vlTOPp->Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__ifetch__1__Vfuncout);
    vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
	= vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__ifetch__1__Vfuncout;
    vlTOPp->Top__DOT__cpu__DOT__registers_io_wen = 
	((((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	   | ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	      | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		 | ((0x23U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		    & ((0x63U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		       & ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			  | ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			     | ((0x6fU == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				| (0x67U == (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))))) 
	  | ((0x73U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	     & ((3U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			      >> 0xcU))) | ((7U == 
					     (7U & 
					      (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0xcU))) 
					    | ((2U 
						== 
						(7U 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0xcU))) 
					       | ((6U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU))) 
						  | ((1U 
						      == 
						      (7U 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xcU))) 
						     | (5U 
							== 
							(7U 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0xcU)))))))))) 
	 & (0U != (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			    >> 7U))));
    vlTOPp->Top__DOT__cpu__DOT__control_io_jump = (
						   (0x33U 
						    == 
						    (0x7fU 
						     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						    ? 0U
						    : 
						   ((0x13U 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						     ? 0U
						     : 
						    ((3U 
						      == 
						      (0x7fU 
						       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						      ? 0U
						      : 
						     ((0x23U 
						       == 
						       (0x7fU 
							& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						       ? 0U
						       : 
						      ((0x63U 
							== 
							(0x7fU 
							 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						        ? 0U
						        : 
						       ((0x37U 
							 == 
							 (0x7fU 
							  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							 ? 0U
							 : 
							((0x17U 
							  == 
							  (0x7fU 
							   & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							  ? 0U
							  : 
							 ((0x6fU 
							   == 
							   (0x7fU 
							    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							   ? 2U
							   : 
							  ((0x67U 
							    == 
							    (0x7fU 
							     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
							    ? 3U
							    : 0U)))))))));
    vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3 
	= ((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			      ? 1U : ((0x23U == (0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						    ? 0U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						     ? 0U
						     : 
						    ((0x6fU 
						      == 
						      (0x7fU 
						       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						      ? 2U
						      : 
						     ((0x67U 
						       == 
						       (0x7fU 
							& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						       ? 2U
						       : 
						      ((0x73U 
							== 
							(0x7fU 
							 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						        ? 3U
						        : 4U))))))))));
    vlTOPp->Top__DOT__cpu__DOT__control_io_validinst 
	= ((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	   | ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	      | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		 | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		    | ((0x63U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		       | ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			  | ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			     | ((0x6fU == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				| ((0x67U == (0x7fU 
					      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				   | ((0x73U == (0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				      | (0xfU == (0x7fU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374 = 
	(((((((((((((QData)((IData)(((0x320U == (0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))
				      ? (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm31) 
					  << 0x1fU) 
					 | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm30) 
					     << 0x1eU) 
					    | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm29) 
						<< 0x1dU) 
					       | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm28) 
						   << 0x1cU) 
						  | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm27) 
						      << 0x1bU) 
						     | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm26) 
							 << 0x1aU) 
							| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm25) 
							    << 0x19U) 
							   | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm24) 
							       << 0x18U) 
							      | ((((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm23) 
								   << 0x17U) 
								  | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm22) 
								      << 0x16U) 
								     | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm21) 
									 << 0x15U) 
									| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm20) 
									    << 0x14U) 
									   | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm19) 
									       << 0x13U) 
									      | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm18) 
										<< 0x12U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm17) 
										<< 0x11U) 
										| ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm16) 
										<< 0x10U)))))))) 
								 | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm15) 
								     << 0xfU) 
								    | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm14) 
									<< 0xeU) 
								       | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm13) 
									   << 0xdU) 
									  | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm12) 
									      << 0xcU) 
									     | (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm11) 
										<< 0xbU) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm10) 
										<< 0xaU) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm9) 
										<< 9U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm8) 
										<< 8U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm7) 
										<< 7U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm6) 
										<< 6U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm5) 
										<< 5U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm4) 
										<< 4U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm3) 
										<< 3U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir) 
										<< 2U) 
										| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_tmzero) 
										<< 1U) 
										| (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy)))))))))))))))))))))))))
				      : 0U))) | ((0xb00U 
						  == 
						  (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))
						  ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67
						  : VL_ULL(0))) 
		   | ((0xb02U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))
		       ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74
		       : VL_ULL(0))) | (QData)((IData)(
						       ((0x301U 
							 == 
							 (0xfffU 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0x14U)))
							 ? 0x10U
							 : 0U)))) 
		 | (QData)((IData)(((0x300U == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))
				     ? (0x1800U | (
						   ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie) 
						    << 7U) 
						   | ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie) 
						      << 3U)))
				     : 0U)))) | (QData)((IData)(
								((0x305U 
								  == 
								  (0xfffU 
								   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								      >> 0x14U)))
								  ? 0x80000000U
								  : 0U)))) 
	       | (QData)((IData)(((0x344U == (0xfffU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))
				   ? (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_mtix) 
				       << 7U) | ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mip_msix) 
						 << 3U))
				   : 0U)))) | (QData)((IData)(
							      ((0x304U 
								== 
								(0xfffU 
								 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								    >> 0x14U)))
							        ? 
							       (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_meix) 
								 << 0xbU) 
								| (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_mtix) 
								    << 7U) 
								   | ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mie_msix) 
								      << 3U)))
							        : 0U)))) 
	     | (QData)((IData)(((0x340U == (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))
				 ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mscratch
				 : 0U)))) | (QData)((IData)(
							    ((0x341U 
							      == 
							      (0xfffU 
							       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								  >> 0x14U)))
							      ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc
							      : 0U)))) 
	   | (QData)((IData)(((0x343U == (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))
			       ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mtval
			       : 0U)))) | (QData)((IData)(
							  ((0x342U 
							    == 
							    (0xfffU 
							     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								>> 0x14U)))
							    ? 
							   (((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt) 
							     << 0x1fU) 
							    | vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode)
							    : 0U)))) 
	 | (QData)((IData)(((0x302U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))
			     ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_medeleg
			     : 0U))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd = ((0x73U 
						  == 
						  (0x7fU 
						   & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						  ? 
						 ((3U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU)))
						   ? 3U
						   : 
						  ((7U 
						    == 
						    (7U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xcU)))
						    ? 3U
						    : 
						   ((2U 
						     == 
						     (7U 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xcU)))
						     ? 2U
						     : 
						    ((6U 
						      == 
						      (7U 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xcU)))
						      ? 2U
						      : 
						     ((1U 
						       == 
						       (7U 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0xcU)))
						       ? 1U
						       : 
						      ((5U 
							== 
							(7U 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0xcU)))
						        ? 1U
						        : 
						       ((0U 
							 == 
							 (7U 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0xcU)))
							 ? 4U
							 : 3U)))))))
						  : 0U);
    vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1 
	= ((0x33U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	    ? 0U : ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		     ? 0U : ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			      ? 0U : ((0x23U == (0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				       ? 0U : ((0x63U 
						== 
						(0x7fU 
						 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
					        ? 0U
					        : (
						   (0x37U 
						    == 
						    (0x7fU 
						     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						    ? 1U
						    : 
						   ((0x17U 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
						     ? 2U
						     : 
						    (0x6fU 
						     == 
						     (0x7fU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))))))));
    vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2 
	= ((0x1fU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 0x14U))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				   >> 0x14U))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				       >> 0x14U))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))
		        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))
			    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))
			        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))
				    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))
				        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))
					    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))
					        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x14U)))
						    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0x14U)))
						     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0x14U)))
						      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0x14U)))
						       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0x14U)))
						        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0x14U)))
							 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							      >> 0x14U)))
							  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							       >> 0x14U)))
							   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								>> 0x14U)))
							    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								 >> 0x14U)))
							     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								  >> 0x14U)))
							      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								   >> 0x14U)))
							       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								    >> 0x14U)))
							        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								     >> 0x14U)))
								 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								      >> 0x14U)))
								  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								       >> 0x14U)))
								   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									>> 0x14U)))
								    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									 >> 0x14U)))
								     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									  >> 0x14U)))
								      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									   >> 0x14U)))
								       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									    >> 0x14U)))
								        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_1
								        : vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__control_io_immediate 
	= ((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	   & ((0x13U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	      | ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		 | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		    | ((0x63U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		       & ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			  | ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			     | ((0x6fU != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
				& (0x67U == (0x7fU 
					     & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))))));
    vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1 
	= ((0x1fU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
			       >> 0xfU))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_31
	    : ((0x1eU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				   >> 0xfU))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_30
	        : ((0x1dU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				       >> 0xfU))) ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_29
		    : ((0x1cU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0xfU)))
		        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_28
		        : ((0x1bU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0xfU)))
			    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_27
			    : ((0x1aU == (0x1fU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xfU)))
			        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_26
			        : ((0x19U == (0x1fU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0xfU)))
				    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_25
				    : ((0x18U == (0x1fU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0xfU)))
				        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_24
				        : ((0x17U == 
					    (0x1fU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0xfU)))
					    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_23
					    : ((0x16U 
						== 
						(0x1fU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0xfU)))
					        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_22
					        : (
						   (0x15U 
						    == 
						    (0x1fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xfU)))
						    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_21
						    : 
						   ((0x14U 
						     == 
						     (0x1fU 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xfU)))
						     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_20
						     : 
						    ((0x13U 
						      == 
						      (0x1fU 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0xfU)))
						      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_19
						      : 
						     ((0x12U 
						       == 
						       (0x1fU 
							& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							   >> 0xfU)))
						       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_18
						       : 
						      ((0x11U 
							== 
							(0x1fU 
							 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							    >> 0xfU)))
						        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_17
						        : 
						       ((0x10U 
							 == 
							 (0x1fU 
							  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							     >> 0xfU)))
							 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_16
							 : 
							((0xfU 
							  == 
							  (0x1fU 
							   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							      >> 0xfU)))
							  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_15
							  : 
							 ((0xeU 
							   == 
							   (0x1fU 
							    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							       >> 0xfU)))
							   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_14
							   : 
							  ((0xdU 
							    == 
							    (0x1fU 
							     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								>> 0xfU)))
							    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_13
							    : 
							   ((0xcU 
							     == 
							     (0x1fU 
							      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								 >> 0xfU)))
							     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_12
							     : 
							    ((0xbU 
							      == 
							      (0x1fU 
							       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								  >> 0xfU)))
							      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_11
							      : 
							     ((0xaU 
							       == 
							       (0x1fU 
								& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								   >> 0xfU)))
							       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_10
							       : 
							      ((9U 
								== 
								(0x1fU 
								 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								    >> 0xfU)))
							        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_9
							        : 
							       ((8U 
								 == 
								 (0x1fU 
								  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								     >> 0xfU)))
								 ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_8
								 : 
								((7U 
								  == 
								  (0x1fU 
								   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								      >> 0xfU)))
								  ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_7
								  : 
								 ((6U 
								   == 
								   (0x1fU 
								    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
								       >> 0xfU)))
								   ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_6
								   : 
								  ((5U 
								    == 
								    (0x1fU 
								     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									>> 0xfU)))
								    ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_5
								    : 
								   ((4U 
								     == 
								     (0x1fU 
								      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									 >> 0xfU)))
								     ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_4
								     : 
								    ((3U 
								      == 
								      (0x1fU 
								       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									  >> 0xfU)))
								      ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_3
								      : 
								     ((2U 
								       == 
								       (0x1fU 
									& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									   >> 0xfU)))
								       ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_2
								       : 
								      ((1U 
									== 
									(0x1fU 
									 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									    >> 0xfU)))
								        ? vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_1
								        : vlTOPp->Top__DOT__cpu__DOT__registers__DOT__regs_0)))))))))))))))))))))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26 
	= ((((0x80000000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	      ? 0xfffffU : 0U) << 0xcU) | (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)));
    vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst 
	= (1U & (((~ (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_validinst)) 
		  | (~ (((((((((((((((((((0x320U == 
					  (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U))) 
					 | (0xb00U 
					    == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
					| (0xb02U == 
					   (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))) 
				       | (0xf13U == 
					  (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))) 
				      | (0xf12U == 
					 (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))) 
				     | (0xf11U == (0xfffU 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)))) 
				    | (0x301U == (0xfffU 
						  & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						     >> 0x14U)))) 
				   | (0x300U == (0xfffU 
						 & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						    >> 0x14U)))) 
				  | (0x305U == (0xfffU 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
				 | (0x344U == (0xfffU 
					       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) 
				| (0x304U == (0xfffU 
					      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))) 
			       | (0x340U == (0xfffU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))) 
			      | (0x341U == (0xfffU 
					    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					       >> 0x14U)))) 
			     | (0x343U == (0xfffU & 
					   (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x14U)))) 
			    | (0x342U == (0xfffU & 
					  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					   >> 0x14U)))) 
			   | (0xf14U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0x14U)))) 
			  | (0x302U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 0x14U)))) 
			 | (0xb80U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)))) 
			| (0xb82U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0x14U)))))) 
		 | (3U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				 >> 0x1eU)))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_ret 
	= (0x3fU & ((4U == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
		    & (((IData)(1U) << (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U))) 
		       >> 2U)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call 
	= (0xffU & ((4U == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
		    & ((IData)(1U) << (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					     >> 0x14U)))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break 
	= (0x7fU & ((4U == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
		    & (((IData)(1U) << (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U))) 
		       >> 1U)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen = (((
						   (0U 
						    != (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
						   & (4U 
						      != (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd))) 
						  & (5U 
						     != (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd))) 
						 & (3U 
						    != 
						    (3U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x1eU))));
    vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation 
	= (((0x33U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	    & ((0x13U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
	       & ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		  | ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
		     | ((0x63U != (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			& ((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
			   | (0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))))))
	    ? 2U : ((0U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				  >> 0xcU))) ? (((IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_immediate) 
						 | (0U 
						    == 
						    (0x7fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x19U))))
						 ? 2U
						 : 3U)
		     : ((1U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				      >> 0xcU))) ? 6U
			 : ((2U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					  >> 0xcU)))
			     ? 4U : ((3U == (7U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xcU)))
				      ? 5U : ((4U == 
					       (7U 
						& (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						   >> 0xcU)))
					       ? 9U
					       : ((5U 
						   == 
						   (7U 
						    & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						       >> 0xcU)))
						   ? 
						  ((0U 
						    == 
						    (0x7fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0x19U)))
						    ? 7U
						    : 8U)
						   : 
						  ((6U 
						    == 
						    (7U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 0xcU)))
						    ? 1U
						    : 
						   ((7U 
						     == 
						     (7U 
						      & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							 >> 0xcU)))
						     ? 0U
						     : 0xfU)))))))));
    vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx = ((0U 
						  == (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1))
						  ? vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1
						  : 
						 ((1U 
						   == (IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_alusrc1))
						   ? 0U
						   : vlTOPp->Top__DOT__cpu__DOT__pc));
    vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm = 
	((0x37U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	  ? (0xfffff000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	  : ((0x17U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
	      ? (0xfffff000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	      : ((0x6fU == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		  ? ((((0x80000000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
		        ? 0x7ffU : 0U) << 0x15U) | 
		     ((0x100000U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				    >> 0xbU)) | ((0xff000U 
						  & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction) 
						 | ((0x800U 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 9U)) 
						    | (0x7feU 
						       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							  >> 0x14U))))))
		  : ((0x67U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
		      ? vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26
		      : ((0x63U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			  ? ((((0x80000000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
			        ? 0x7ffffU : 0U) << 0xdU) 
			     | ((0x1000U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					    >> 0x13U)) 
				| ((0x800U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      << 4U)) 
				   | ((0x7e0U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						 >> 0x14U)) 
				      | (0x1eU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						  >> 7U))))))
			  : ((3U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
			      ? vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26
			      : ((0x23U == (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				  ? ((((0x80000000U 
					& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
				        ? 0xfffffU : 0U) 
				      << 0xcU) | ((0xfe0U 
						   & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						      >> 0x14U)) 
						  | (0x1fU 
						     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
							>> 7U))))
				  : ((0x13U == (0x7fU 
						& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
				      ? vlTOPp->Top__DOT__cpu__DOT__immGen__DOT___T_26
				      : ((0x73U == 
					  (0x7fU & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))
					  ? (0x1fU 
					     & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
						>> 0xfU))
					  : 0U)))))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_21 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst)
	  ? vlTOPp->Top__DOT__cpu__DOT__pc : vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mepc);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_334 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_ret) 
	 & (~ (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
	       >> 0x1eU)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_30 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)
	  ? 0U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call)
		   ? 0U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst)
			    ? 0U : (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_31 = 
	((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_break)
	  ? 3U : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__insn_call)
		   ? 0xbU : ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr_io_illegal_inst)
			      ? 2U : vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode)));
    vlTOPp->Top__DOT__cpu__DOT__branchAdd_io_result 
	= (vlTOPp->Top__DOT__cpu__DOT__pc + vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_266 = 
	((0x4000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	  ? vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm
	  : vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata1);
    vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy = ((IData)(vlTOPp->Top__DOT__cpu__DOT__control_io_immediate)
						  ? vlTOPp->Top__DOT__cpu__DOT__immGen_io_sextImm
						  : vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata = (
						   ((((2U 
						       == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)) 
						      | (3U 
							 == (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd)))
						      ? (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374)
						      : 0U) 
						    | vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_266) 
						   & (~ 
						      ((3U 
							== (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__cmd))
						        ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_266
						        : 0U)));
    vlTOPp->Top__DOT__cpu__DOT__alu__DOT___T_3 = (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
						  | vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy);
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_472 = 
	(((QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)) 
	  << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_475 = 
	(((QData)((IData)((vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_67 
			   >> 0x20U))) << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_480 = 
	(((QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)) 
	  << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74)));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_483 = 
	(((QData)((IData)((vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_74 
			   >> 0x20U))) << 0x20U) | (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wdata)));
    vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10 = 
	(VL_ULL(0x7fffffffffffffff) & ((0U == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
				        ? (QData)((IData)(
							  (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
							   & vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
				        : ((1U == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
					    ? (QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___T_3))
					    : ((2U 
						== (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
					        ? (QData)((IData)(
								  (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
								   + vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
					        : (
						   (3U 
						    == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						    ? (QData)((IData)(
								      (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
								       - vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
						    : 
						   ((4U 
						     == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						     ? (QData)((IData)(
								       VL_LTS_III(1,32,32, vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx, vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
						     : 
						    ((5U 
						      == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						      ? (QData)((IData)(
									(vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
									 < vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)))
						      : 
						     ((6U 
						       == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
						       ? 
						      ((QData)((IData)(vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx)) 
						       << 
						       (0x1fU 
							& vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy))
						       : (QData)((IData)(
									 ((7U 
									   == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									   ? 
									  (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
									   >> 
									   (0x1fU 
									    & vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy))
									   : 
									  ((8U 
									    == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									    ? 
									   VL_SHIFTRS_III(32,32,5, vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx, 
										(0x1fU 
										& vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy))
									    : 
									   ((9U 
									     == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									     ? 
									    (vlTOPp->Top__DOT__cpu__DOT__alu_io_inputx 
									     ^ vlTOPp->Top__DOT__cpu__DOT__alu_io_inputy)
									     : 
									    ((0xaU 
									      == (IData)(vlTOPp->Top__DOT__cpu__DOT__aluControl_io_operation))
									      ? 
									     (~ vlTOPp->Top__DOT__cpu__DOT__alu__DOT___T_3)
									      : 0U))))))))))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_176 
	= ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
	    ? ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0x14U))) ? 
	       ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy)
		 ? (QData)((IData)((0x7fU & ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61)))))
		 : ((0xb00U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					  >> 0x14U)))
		     ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_475
		     : ((0xb80U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U)))
			 ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_472
			 : (QData)((IData)((0x7fU & 
					    ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61))))))))
	        : (QData)((IData)((0x7fU & ((IData)(1U) 
					    + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61))))))
	    : (QData)((IData)((0x7fU & ((IData)(1U) 
					+ (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_61))))));
    vlTOPp->Top__DOT__cpu__DOT__csr__DOT___GEN_178 
	= ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__wen)
	    ? ((0x320U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0x14U))) ? 
	       ((IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir)
		 ? (QData)((IData)((0x7fU & ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68)))))
		 : ((0xb02U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					  >> 0x14U)))
		     ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_483
		     : ((0xb82U == (0xfffU & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
					      >> 0x14U)))
			 ? vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_480
			 : (QData)((IData)((0x7fU & 
					    ((IData)(1U) 
					     + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68))))))))
	        : (QData)((IData)((0x7fU & ((IData)(1U) 
					    + (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68))))))
	    : (QData)((IData)((0x7fU & ((IData)(1U) 
					+ (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_68))))));
    // ALWAYS at DualPortedMemoryBlackBox.v:57
    vlTOPp->__Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__datareq_TOP((IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10), vlTOPp->Top__DOT__cpu__DOT__registers_io_readdata2, 
								      ((0x33U 
									!= 
									(0x7fU 
									 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
								       & ((0x13U 
									   != 
									   (0x7fU 
									    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
									  & (3U 
									     == 
									     (0x7fU 
									      & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)))), 
								      ((0x33U 
									!= 
									(0x7fU 
									 & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
								       & ((0x13U 
									   != 
									   (0x7fU 
									    & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
									  & ((3U 
									      != 
									      (0x7fU 
									       & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)) 
									     & (0x23U 
										== 
										(0x7fU 
										& vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction))))), 
								      (3U 
								       & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									  >> 0xcU)), 
								      (1U 
								       & (~ 
									  (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
									   >> 0xeU))), vlTOPp->Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox, vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__datareq__2__Vfuncout);
    vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata 
	= vlTOPp->__Vfunc_Top__DOT__mem__DOT__memory__DOT__datareq__2__Vfuncout;
    vlTOPp->Top__DOT__cpu__DOT__registers_io_writedata 
	= ((1U == (3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)))
	    ? ((0x4000U & vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction)
	        ? vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata
	        : ((0U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				 >> 0xcU))) ? ((((0x80U 
						  & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata)
						  ? 0xffffffU
						  : 0U) 
						<< 8U) 
					       | (0xffU 
						  & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata))
		    : ((1U == (3U & (vlTOPp->Top__DOT__mem__DOT__memory_imem_instruction 
				     >> 0xcU))) ? (
						   (((0x8000U 
						      & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata)
						      ? 0xffffU
						      : 0U) 
						    << 0x10U) 
						   | (0xffffU 
						      & vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata))
		        : vlTOPp->Top__DOT__mem__DOT__memory_dmem_readdata)))
	    : ((2U == (3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)))
	        ? ((IData)(4U) + vlTOPp->Top__DOT__cpu__DOT__pc)
	        : ((3U == (3U & (IData)(vlTOPp->Top__DOT__cpu__DOT__control__DOT__signals_3)))
		    ? (IData)(vlTOPp->Top__DOT__cpu__DOT__csr__DOT___T_374)
		    : (IData)(vlTOPp->Top__DOT__cpu__DOT__alu__DOT___GEN_10))));
}

void VTop::_eval(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_eval\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    if (((IData)(vlTOPp->clock) & (~ (IData)(vlTOPp->__Vclklast__TOP__clock)))) {
	vlTOPp->_sequent__TOP__2(vlSymsp);
    }
    // Final
    vlTOPp->__Vclklast__TOP__clock = vlTOPp->clock;
}

void VTop::_eval_initial(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_eval_initial\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_initial__TOP__1(vlSymsp);
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
    vlTOPp->_settle__TOP__3(vlSymsp);
}

VL_INLINE_OPT QData VTop::_change_request(VTop__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_change_request\n"); );
    VTop* __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // Change detection
    QData __req = false;  // Logically a bool
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
}
#endif // VL_DEBUG

void VTop::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    VTop::_ctor_var_reset\n"); );
    // Body
    clock = VL_RAND_RESET_I(1);
    reset = VL_RAND_RESET_I(1);
    io_success = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__control_io_validinst = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__control_io_immediate = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__control_io_alusrc1 = VL_RAND_RESET_I(2);
    Top__DOT__cpu__DOT__control_io_jump = VL_RAND_RESET_I(2);
    Top__DOT__cpu__DOT__registers_io_writedata = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers_io_wen = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__registers_io_readdata1 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers_io_readdata2 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr_io_illegal_inst = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__aluControl_io_operation = VL_RAND_RESET_I(4);
    Top__DOT__cpu__DOT__alu_io_inputx = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__alu_io_inputy = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__immGen_io_sextImm = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__pcPlusFour_io_inputx = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__branchAdd_io_result = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__pc = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__value = VL_RAND_RESET_I(30);
    Top__DOT__cpu__DOT___T_2 = VL_RAND_RESET_I(30);
    Top__DOT__cpu__DOT__control__DOT__signals_3 = VL_RAND_RESET_I(3);
    Top__DOT__cpu__DOT__registers__DOT__regs_0 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_1 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_2 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_3 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_4 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_5 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_6 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_7 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_8 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_9 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_10 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_11 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_12 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_13 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_14 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_15 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_16 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_17 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_18 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_19 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_20 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_21 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_22 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_23 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_24 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_25 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_26 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_27 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_28 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_29 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_30 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__registers__DOT__regs_31 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mepc = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode = VL_RAND_RESET_I(31);
    Top__DOT__cpu__DOT__csr__DOT__reg_mtval = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__reg_mscratch = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__reg_medeleg = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__reg_mip_mtix = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mip_msix = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mie_meix = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mie_mtix = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mie_msix = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mtvec_base = VL_RAND_RESET_I(30);
    Top__DOT__cpu__DOT__csr__DOT___T_61 = VL_RAND_RESET_I(6);
    Top__DOT__cpu__DOT__csr__DOT___T_63 = VL_RAND_RESET_Q(58);
    Top__DOT__cpu__DOT__csr__DOT___T_66 = VL_RAND_RESET_Q(58);
    Top__DOT__cpu__DOT__csr__DOT___T_67 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___T_68 = VL_RAND_RESET_I(6);
    Top__DOT__cpu__DOT__csr__DOT___T_70 = VL_RAND_RESET_Q(58);
    Top__DOT__cpu__DOT__csr__DOT___T_73 = VL_RAND_RESET_Q(58);
    Top__DOT__cpu__DOT__csr__DOT___T_74 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm31 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm30 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm29 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm28 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm27 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm26 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm25 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm24 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm23 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm22 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm21 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm20 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm19 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm18 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm17 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm16 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm15 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm14 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm13 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm12 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm11 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm10 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm9 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm8 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm7 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm6 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm5 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm4 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm3 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_tmzero = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__cmd = VL_RAND_RESET_I(3);
    Top__DOT__cpu__DOT__csr__DOT__wen = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT___T_266 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__wdata = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT__insn_call = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__insn_break = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT__insn_ret = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT___GEN_21 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT___T_334 = VL_RAND_RESET_I(1);
    Top__DOT__cpu__DOT__csr__DOT___GEN_30 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__csr__DOT___GEN_31 = VL_RAND_RESET_I(31);
    Top__DOT__cpu__DOT__csr__DOT___T_374 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___T_472 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___T_475 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___T_480 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___T_483 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___GEN_176 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__csr__DOT___GEN_178 = VL_RAND_RESET_Q(64);
    Top__DOT__cpu__DOT__alu__DOT___T_3 = VL_RAND_RESET_I(32);
    Top__DOT__cpu__DOT__alu__DOT___GEN_10 = VL_RAND_RESET_Q(63);
    Top__DOT__cpu__DOT__immGen__DOT___T_26 = VL_RAND_RESET_I(32);
    Top__DOT__mem__DOT__memory_imem_instruction = VL_RAND_RESET_I(32);
    Top__DOT__mem__DOT__memory_dmem_readdata = VL_RAND_RESET_I(32);
    Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox = 0;
    __Vfunc_Top__DOT__mem__DOT__memory__DOT__ifetch__1__Vfuncout = 0;
    __Vfunc_Top__DOT__mem__DOT__memory__DOT__datareq__2__Vfuncout = 0;
    __Vclklast__TOP__clock = VL_RAND_RESET_I(1);
}
