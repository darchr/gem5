// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See VTop.h for the primary calling header

#include "VTop___024unit.h"    // For This
#include "VTop__Syms.h"

#include "verilated_dpi.h"


//--------------------
// Internal Methods

VL_INLINE_OPT void VTop___024unit::__Vdpiimwrap_debug_tick_TOP____024unit(CData& debug_req_valid, CData debug_req_ready, IData& debug_req_bits_addr, IData& debug_req_bits_op, IData& debug_req_bits_data, CData debug_resp_valid, CData& debug_resp_ready, IData debug_resp_bits_resp, IData debug_resp_bits_data, IData& debug_tick__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+        VTop___024unit::__Vdpiimwrap_debug_tick_TOP____024unit\n"); );
    // Body
    unsigned char debug_req_valid__Vcvt;
    unsigned char debug_req_ready__Vcvt;
    debug_req_ready__Vcvt = debug_req_ready;
    int debug_req_bits_addr__Vcvt;
    int debug_req_bits_op__Vcvt;
    int debug_req_bits_data__Vcvt;
    unsigned char debug_resp_valid__Vcvt;
    debug_resp_valid__Vcvt = debug_resp_valid;
    unsigned char debug_resp_ready__Vcvt;
    int debug_resp_bits_resp__Vcvt;
    debug_resp_bits_resp__Vcvt = debug_resp_bits_resp;
    int debug_resp_bits_data__Vcvt;
    debug_resp_bits_data__Vcvt = debug_resp_bits_data;
    int debug_tick__Vfuncrtn__Vcvt = debug_tick(&debug_req_valid__Vcvt, debug_req_ready__Vcvt, &debug_req_bits_addr__Vcvt, &debug_req_bits_op__Vcvt, &debug_req_bits_data__Vcvt, debug_resp_valid__Vcvt, &debug_resp_ready__Vcvt, debug_resp_bits_resp__Vcvt, debug_resp_bits_data__Vcvt);
    debug_req_valid = (1U & debug_req_valid__Vcvt);
    debug_req_bits_addr = debug_req_bits_addr__Vcvt;
    debug_req_bits_op = debug_req_bits_op__Vcvt;
    debug_req_bits_data = debug_req_bits_data__Vcvt;
    debug_resp_ready = (1U & debug_resp_ready__Vcvt);
    debug_tick__Vfuncrtn = debug_tick__Vfuncrtn__Vcvt;
}
