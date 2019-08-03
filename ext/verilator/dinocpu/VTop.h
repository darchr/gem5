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
    VL_IN8(io_imem_valid,0,0);
    VL_OUT8(io_imem_good,0,0);
    VL_IN8(io_dmem_valid,0,0);
    VL_IN8(io_dmem_memread,0,0);
    VL_IN8(io_dmem_memwrite,0,0);
    VL_IN8(io_dmem_maskmode,1,0);
    VL_IN8(io_dmem_sext,0,0);
    VL_OUT8(io_dmem_good,0,0);
    VL_IN(io_imem_address,31,0);
    VL_OUT(io_imem_instruction,31,0);
    VL_IN(io_dmem_address,31,0);
    VL_IN(io_dmem_writedata,31,0);
    VL_OUT(io_dmem_readdata,31,0);
    
    // LOCAL SIGNALS
    // Internals; generally not touched by application code
    VL_SIG8(Top__DOT__dmem_io_request_valid,0,0);
    VL_SIG8(Top__DOT__dmem_io_request_bits_operation,0,0);
    VL_SIG8(Top__DOT__memory_io_dmem_response_valid,0,0);
    VL_SIG8(Top__DOT__dmem__DOT__outstandingReq_valid,0,0);
    VL_SIG8(Top__DOT__dmem__DOT__outstandingReq_bits_maskmode,1,0);
    VL_SIG8(Top__DOT__dmem__DOT__outstandingReq_bits_operation,0,0);
    VL_SIG8(Top__DOT__dmem__DOT__outstandingReq_bits_sext,0,0);
    VL_SIG8(Top__DOT__dmem__DOT__sending,0,0);
    VL_SIG8(Top__DOT__dmem__DOT___T_65,0,0);
    VL_SIG8(Top__DOT__memory__DOT__memory_imem_request_ready,0,0);
    VL_SIG8(Top__DOT__memory__DOT__memory_dmem_request_ready,0,0);
    VL_SIG8(Top__DOT__memory__DOT__memory_dmem_response_valid,0,0);
    VL_SIG(Top__DOT__dmem__DOT__outstandingReq_bits_address,31,0);
    VL_SIG(Top__DOT__dmem__DOT__outstandingReq_bits_writedata,31,0);
    VL_SIG(Top__DOT__dmem__DOT___T_47,31,0);
    VL_SIG(Top__DOT__dmem__DOT___GEN_12,31,0);
    VL_SIG(Top__DOT__dmem__DOT___GEN_17,31,0);
    VL_SIG(Top__DOT__memory__DOT__memory_imem_response_bits_data,31,0);
    VL_SIG(Top__DOT__memory__DOT__memory_dmem_response_bits_data,31,0);
    VL_SIG64(Top__DOT__memory__DOT__memory__DOT__gem5MemBlkBox,63,0);
    
    // LOCAL VARIABLES
    // Internals; generally not touched by application code
    VL_SIG8(__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__imem_response_valid,0,0);
    VL_SIG8(__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__dmem_response_valid,0,0);
    VL_SIG8(__Vclklast__TOP__clock,0,0);
    VL_SIG8(__Vchglast__TOP__Top__DOT__memory_io_dmem_response_valid,0,0);
    VL_SIG(__Vfunc_Top__DOT__memory__DOT__memory__DOT__ifetch__1__Vfuncout,31,0);
    VL_SIG(__Vfunc_Top__DOT__memory__DOT__memory__DOT__datareq__2__Vfuncout,31,0);
    VL_SIG(__Vchglast__TOP__Top__DOT__memory__DOT__memory_dmem_response_bits_data,31,0);
    
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
    void __Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__datareq_TOP(CData dmem_request_ready, CData dmem_request_valid, IData dmem_request_bits_address, IData dmem_request_bits_writedata, CData dmem_request_bits_operation, CData& dmem_response_valid, QData handle, IData& datareq__Vfuncrtn);
    void __Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__ifetch_TOP(CData imem_request_ready, CData imem_request_valid, IData imem_request_bits_address, CData& imem_response_valid, QData handle, IData& ifetch__Vfuncrtn);
    void __Vdpiimwrap_Top__DOT__memory__DOT__memory__DOT__setGem5Handle_TOP(QData& setGem5Handle__Vfuncrtn);
  private:
    static QData _change_request(VTop__Syms* __restrict vlSymsp);
  public:
    static void _combo__TOP__1(VTop__Syms* __restrict vlSymsp);
    static void _combo__TOP__6(VTop__Syms* __restrict vlSymsp);
  private:
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
    static void _initial__TOP__3(VTop__Syms* __restrict vlSymsp);
    static void _sequent__TOP__4(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__2(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__5(VTop__Syms* __restrict vlSymsp);
} VL_ATTR_ALIGNED(128);

#endif // guard
