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
class VTop___024unit;

//----------

VL_MODULE(VTop) {
  public:
    // CELLS
    // Public to allow access to /*verilator_public*/ items;
    // otherwise the application code can consider these internals.
    VTop___024unit*    	__PVT____024unit;
    
    // PORTS
    // The application code writes and reads these signals to
    // propagate new values into/out from the Verilated model.
    VL_IN8(clock,0,0);
    VL_IN8(reset,0,0);
    VL_OUT8(io_success,0,0);
    
    // LOCAL SIGNALS
    // Internals; generally not touched by application code
    VL_SIG8(Top__DOT__tile__DOT__debug_io_dmi_resp_valid,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core_reset,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmstatus_allrunning,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmstatus_allhalted,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__sbcs_sbsingleread,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__sbcs_sbaccess,2,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__sbcs_sbautoincrement,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__sbcs_sbautoread,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__sbcs_sberror,2,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__abstractcs_cmderr,2,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__command_postexec,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__command_transfer,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__command_write,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmcontrol_haltreq,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmcontrol_resumereq,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmcontrol_hartreset,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmcontrol_ndmreset,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__dmcontrol_dmactive,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__memreadfire,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__coreresetval,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_345,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_407,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_415,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT__firstreaddone,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_426,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_432,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_435,0,0);
    VL_SIG8(Top__DOT__tile__DOT__debug__DOT___T_445,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_dmem_req_bits_typ,2,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_pc_sel,2,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_op1_sel,1,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_op2_sel,1,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_alu_fun,3,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_wb_sel,1,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c_io_ctl_csr_cmd,2,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d_io_dat_br_eq,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d_io_dat_br_lt,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d_io_dat_br_ltu,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c__DOT__cs_val_inst,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c__DOT__cs_br_type,3,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c__DOT___T_800,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c__DOT__cs0_3,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c__DOT__cs0_6,2,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__c__DOT___T_984,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mstatus_prv,1,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mstatus_mpie,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mstatus_mie,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mip_mtip,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mip_msip,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mie_mtip,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mie_msip,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_258,5,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_270,5,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_271,6,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_dcsr_ebreakm,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_dcsr_step,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__priv_sufficient,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__wen,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__insn_call,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__insn_break,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__insn_ret,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1058,0,0);
    VL_SIG8(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1063,0,0);
    VL_SIG8(Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_req_valid,0,0);
    VL_SIG8(Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid,0,0);
    VL_SIG8(Top__DOT__tile__DOT__memory__DOT__async_data_dw_mask,3,0);
    VL_SIG8(Top__DOT__tile__DOT__memory__DOT__async_data_dw_en,0,0);
    VL_SIG8(Top__DOT__SimDTM__DOT__r_reset,0,0);
    VL_SIG8(Top__DOT__SimDTM__DOT_____05Fdebug_req_ready,0,0);
    VL_SIG8(Top__DOT__SimDTM__DOT_____05Fdebug_req_valid,0,0);
    VL_SIG16(Top__DOT__tile__DOT__debug__DOT__command_regno,15,0);
    VL_SIG16(Top__DOT__tile__DOT__core__DOT__d__DOT__imm_b,11,0);
    VL_SIG16(Top__DOT__tile__DOT__core__DOT__d__DOT__imm_s,11,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT__data0,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT__data1,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT__data2,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT__sbaddr,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT__sbdata,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT___T_319,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT___T_336,31,0);
    VL_SIG(Top__DOT__tile__DOT__debug__DOT___T_410,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d_io_dmem_req_bits_addr,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__regfile___05FT_188_data,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr_io_rw_wdata,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr_io_evec,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__pc_reg,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__pc_plus4,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__rs1_data,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__imm_i_sext,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__imm_j,19,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__alu_op1,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__rs2_data,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__alu_op2,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT___T_293,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__alu_out,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT___T_370,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mepc,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mcause,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mtval,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_mscratch,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_medeleg,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_dpc,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__reg_dscratch,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT__wdata,31,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_4,31,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_hr_data,31,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr,20,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_dw_data,31,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_addr,20,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_data,31,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_addr,20,0);
    VL_SIG(Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_data,31,0);
    VL_SIG(Top__DOT__SimDTM__DOT_____05Fdebug_req_bits_addr,31,0);
    VL_SIG(Top__DOT__SimDTM__DOT_____05Fdebug_req_bits_op,31,0);
    VL_SIG(Top__DOT__SimDTM__DOT_____05Fdebug_req_bits_data,31,0);
    VL_SIG(Top__DOT__SimDTM__DOT_____05Fexit,31,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT___T_299,32,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT___T_297,32,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT___T_295,32,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT___T_256,32,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_262,57,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_266,57,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_267,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_274,57,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_278,57,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_279,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_282,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_285,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_288,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_291,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_294,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_297,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_300,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_303,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_306,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_309,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_312,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_315,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_318,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_321,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_324,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_327,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_330,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_333,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_336,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_339,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_342,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_345,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_348,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_351,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_354,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_357,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_360,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_363,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_366,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_369,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_372,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_375,39,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1323,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1580,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1583,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1587,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___T_1590,63,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_110,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_111,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_112,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_113,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_114,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_115,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_116,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_117,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_118,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_119,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_120,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_121,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_122,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_123,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_124,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_125,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_126,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_127,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_128,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_129,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_130,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_131,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_132,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_133,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_134,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_135,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_136,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_137,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_138,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_139,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_140,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_141,40,0);
    VL_SIG64(Top__DOT__tile__DOT__core__DOT__d__DOT__csr__DOT___GEN_142,63,0);
    VL_SIG(Top__DOT__tile__DOT__core__DOT__d__DOT__regfile[32],31,0);
    
    // LOCAL VARIABLES
    // Internals; generally not touched by application code
    VL_SIG8(__Vclklast__TOP__clock,0,0);
    
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
  private:
    static QData _change_request(VTop__Syms* __restrict vlSymsp);
  public:
    static void _combo__TOP__3(VTop__Syms* __restrict vlSymsp);
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
    static void _sequent__TOP__2(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__1(VTop__Syms* __restrict vlSymsp);
    static void _settle__TOP__4(VTop__Syms* __restrict vlSymsp);
} VL_ATTR_ALIGNED(128);

#endif // guard
