// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See VTop.h for the primary calling header

#ifndef _VTop___024unit_H_
#define _VTop___024unit_H_

#include "verilated_heavy.h"
#include "VTop__Dpi.h"

class VTop__Syms;

//----------

VL_MODULE(VTop___024unit) {
  public:
    
    // PORTS
    
    // LOCAL SIGNALS
    
    // LOCAL VARIABLES
    
    // INTERNAL VARIABLES
  private:
    VTop__Syms* __VlSymsp;  // Symbol table
  public:
    
    // PARAMETERS
    
    // CONSTRUCTORS
  private:
    VTop___024unit& operator= (const VTop___024unit&);  ///< Copying not allowed
    VTop___024unit(const VTop___024unit&);  ///< Copying not allowed
  public:
    VTop___024unit(const char* name="TOP");
    ~VTop___024unit();
    
    // API METHODS
    
    // INTERNAL METHODS
    void __Vconfigure(VTop__Syms* symsp, bool first);
    void __Vdpiimwrap_debug_tick_TOP____024unit(CData& debug_req_valid, CData debug_req_ready, IData& debug_req_bits_addr, IData& debug_req_bits_op, IData& debug_req_bits_data, CData debug_resp_valid, CData& debug_resp_ready, IData debug_resp_bits_resp, IData debug_resp_bits_data, IData& debug_tick__Vfuncrtn);
  private:
    void _ctor_var_reset();
} VL_ATTR_ALIGNED(128);

#endif // guard
