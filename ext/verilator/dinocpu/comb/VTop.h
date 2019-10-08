// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Primary design header
//
// This header should be included by all source files instantiating the design.
// The class here is then constructed to instantiate the design.
// See the Verilator manual for examples.

#ifndef _VTop_H_
#define _VTop_H_

#include "verilated_heavy.h"
#include "VTop__Dpi.h"

class VTop__Syms;

//----------

VL_MODULE(VTop) {
  public:
    
    // PORTS
    // The application code writes and reads these signals to
    // propagate new values into/out from the Verilated model.
    VL_IN8(clock,0,0);
    VL_IN8(reset,0,0);
    VL_OUT8(io_success,0,0);
    
    // LOCAL SIGNALS
    // Internals; generally not touched by application code
    VL_SIG8(Top__DOT__cpu__DOT__control_io_validinst,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__control_io_immediate,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__control_io_alusrc1,1,0);
    VL_SIG8(Top__DOT__cpu__DOT__control_io_jump,1,0);
    VL_SIG8(Top__DOT__cpu__DOT__registers_io_wen,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr_io_illegal_inst,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__aluControl_io_operation,3,0);
    VL_SIG8(Top__DOT__cpu__DOT__control__DOT__signals_3,2,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mpie,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mstatus_mie,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcause_interrupt,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mip_mtix,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mip_msix,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mie_meix,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mie_mtix,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mie_msix,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT___T_61,5,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT___T_68,5,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm31,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm30,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm29,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm28,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm27,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm26,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm25,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm24,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm23,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm22,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm21,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm20,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm19,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm18,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm17,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm16,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm15,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm14,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm13,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm12,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm11,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm10,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm9,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm8,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm7,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm6,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm5,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm4,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_hpm3,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_ir,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_tmzero,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__reg_mcounterinhibit_cy,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__cmd,2,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__wen,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__insn_call,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__insn_break,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT__insn_ret,0,0);
    VL_SIG8(Top__DOT__cpu__DOT__csr__DOT___T_334,0,0);
    VL_SIG(Top__DOT__cpu__DOT__registers_io_writedata,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers_io_readdata1,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers_io_readdata2,31,0);
    VL_SIG(Top__DOT__cpu__DOT__alu_io_inputx,31,0);
    VL_SIG(Top__DOT__cpu__DOT__alu_io_inputy,31,0);
    VL_SIG(Top__DOT__cpu__DOT__immGen_io_sextImm,31,0);
    VL_SIG(Top__DOT__cpu__DOT__pcPlusFour_io_inputx,31,0);
    VL_SIG(Top__DOT__cpu__DOT__branchAdd_io_result,31,0);
    VL_SIG(Top__DOT__cpu__DOT__pc,31,0);
    VL_SIG(Top__DOT__cpu__DOT__value,29,0);
    VL_SIG(Top__DOT__cpu__DOT___T_2,29,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_0,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_1,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_2,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_3,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_4,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_5,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_6,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_7,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_8,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_9,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_10,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_11,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_12,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_13,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_14,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_15,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_16,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_17,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_18,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_19,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_20,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_21,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_22,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_23,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_24,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_25,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_26,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_27,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_28,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_29,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_30,31,0);
    VL_SIG(Top__DOT__cpu__DOT__registers__DOT__regs_31,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__reg_mepc,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__reg_mcause_exceptioncode,30,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__reg_mtval,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__reg_mscratch,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__reg_medeleg,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__reg_mtvec_base,29,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT___T_266,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT__wdata,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT___GEN_21,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT___GEN_30,31,0);
    VL_SIG(Top__DOT__cpu__DOT__csr__DOT___GEN_31,30,0);
    VL_SIG(Top__DOT__cpu__DOT__alu__DOT___T_3,31,0);
    VL_SIG(Top__DOT__cpu__DOT__immGen__DOT___T_26,31,0);
    VL_SIG(Top__DOT__mem__DOT__memory_imem_instruction,31,0);
    VL_SIG(Top__DOT__mem__DOT__memory_dmem_readdata,31,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_63,57,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_66,57,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_67,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_70,57,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_73,57,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_74,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_374,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_472,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_475,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_480,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___T_483,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___GEN_176,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__csr__DOT___GEN_178,63,0);
    VL_SIG64(Top__DOT__cpu__DOT__alu__DOT___GEN_10,62,0);
    VL_SIG64(Top__DOT__mem__DOT__memory__DOT__gem5MemBlkBox,63,0);
    
    // LOCAL VARIABLES
    // Internals; generally not touched by application code
    VL_SIG8(__Vclklast__TOP__clock,0,0);
    VL_SIG(__Vfunc_Top__DOT__mem__DOT__memory__DOT__ifetch__1__Vfuncout,31,0);
    VL_SIG(__Vfunc_Top__DOT__mem__DOT__memory__DOT__datareq__2__Vfuncout,31,0);
    
    // INTERNAL VARIABLES
    // Internals; generally not touched by application code
    VTop__Syms* __VlSymsp;  // Symbol table
    
    // PARAMETERS
    // Parameters marked /*verilator public*/ for use by application code
    
    // CONSTRUCTORS
  private:
    VTop& operator= (const VTop&);  ///< Copying not allowed
    VTop(const VTop&);  ///< Copying not allowed
  public:
    /// Construct the model; called by application code
    /// The special name  may be used to make a wrapper with a
    /// single model invisible WRT DPI scope names.
    VTop(const char* name="TOP");
    /// Destroy the model; called (often implicitly) by application code
    ~VTop();
    
    // API METHODS
    /// Evaluate the model.  Application must call when inputs change.
    void eval();
    /// Simulation complete, run final blocks.  Application must call on completion.
    void final();
    
    // INTERNAL METHODS
  private:
    static void _eval_initial_loop(VTop__Syms* __restrict vlSymsp);
  public:
    void __Vconfigure(VTop__Syms* symsp, bool first);
    void __Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__datareq_TOP(IData dmem_address, IData dmem_writedata, CData dmem_memread, CData dmem_memwrite, CData dmem_maskmode, CData dmem_sext, QData handle, IData& datareq__Vfuncrtn);
    void __Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__ifetch_TOP(IData imem_address, QData handle, IData& ifetch__Vfuncrtn);
    void __Vdpiimwrap_Top__DOT__mem__DOT__memory__DOT__setGem5Handle_TOP(QData& setGem5Handle__Vfuncrtn);
  private:
    static QData _change_request(VTop__Syms* __restrict vlSymsp);
    void _ctor_var_reset();
  public:
    static void _eval(VTop__Syms* __restrict vlSymsp);
  private:
#ifdef VL_DEBUG
    void _eval_debug_assertions();
#endif // VL_DEBUG
  public:
    static void _eval_initial(VTop__Syms* __restrict vlSymsp);
    static void _eval_settle(VTop__Syms* __restrict vlSymsp);
    static void _initial__TOP__1(VTop__Syms* __restrict vlSymsp);
    static void _sequent__TOP__2(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__3(VTop__Syms* __restrict vlSymsp);
} VL_ATTR_ALIGNED(128);

#endif // guard
