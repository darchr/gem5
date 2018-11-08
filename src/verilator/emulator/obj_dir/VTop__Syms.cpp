// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table implementation internals

#include "VTop__Syms.h"
#include "VTop.h"
#include "VTop___024unit.h"

// FUNCTIONS
VTop__Syms::VTop__Syms(VTop* topp, const char* namep)
	// Setup locals
	: __Vm_namep(namep)
	, __Vm_didInit(false)
	// Setup submodule names
	, TOP____024unit                 (Verilated::catName(topp->name(),"$unit"))
{
    // Pointer to top level
    TOPp = topp;
    // Setup each module's pointers to their submodules
    TOPp->__PVT____024unit          = &TOP____024unit;
    // Setup each module's pointer back to symbol table (for public functions)
    TOPp->__Vconfigure(this, true);
    TOP____024unit.__Vconfigure(this, true);
    // Setup scope names
    __Vscope_Top__tile__core.configure(this,name(),"Top.tile.core");
    __Vscope_Top__tile__core__d__csr.configure(this,name(),"Top.tile.core.d.csr");
    __Vscope_Top__tile__memory.configure(this,name(),"Top.tile.memory");
    // Setup export functions
    for (int __Vfinal=0; __Vfinal<2; __Vfinal++) {
	__Vscope_Top__tile__core.varInsert(__Vfinal,"c_io_ctl_stall", &(TOPp->Top__DOT__tile__DOT__core__DOT__c_io_ctl_stall), VLVT_UINT8,VLVD_NODIR|VLVF_PUB_RW,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dataInstr_0_addr", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_addr), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,20,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dataInstr_0_data", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_0_data), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dataInstr_1_addr", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_addr), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,20,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dataInstr_1_data", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dataInstr_1_data), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dw_addr", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dw_addr), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,20,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dw_data", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dw_data), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dw_en", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dw_en), VLVT_UINT8,VLVD_NODIR|VLVF_PUB_RW,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"async_data_dw_mask", &(TOPp->Top__DOT__tile__DOT__memory__DOT__async_data_dw_mask), VLVT_UINT8,VLVD_NODIR|VLVF_PUB_RW,1 ,3,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"io_core_ports_0_req_valid", &(TOPp->Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_req_valid), VLVT_UINT8,VLVD_NODIR|VLVF_PUB_RW,0);
	__Vscope_Top__tile__memory.varInsert(__Vfinal,"io_core_ports_0_resp_valid", &(TOPp->Top__DOT__tile__DOT__memory__DOT__io_core_ports_0_resp_valid), VLVT_UINT8,VLVD_NODIR|VLVF_PUB_RW,0);
    }
}
