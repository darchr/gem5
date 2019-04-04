// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table implementation internals

#include "VTop__Syms.h"
#include "VTop.h"
#include "VTop_Top.h"
#include "VTop_DualPortedMemory.h"
#include "VTop_DualPortedMemoryBlackBox.h"

// FUNCTIONS
VTop__Syms::VTop__Syms(VTop* topp, const char* namep)
	// Setup locals
	: __Vm_namep(namep)
	, __Vm_didInit(false)
	// Setup submodule names
	, TOP__Top                       (Verilated::catName(topp->name(),"Top"))
	, TOP__Top__mem                  (Verilated::catName(topp->name(),"Top.mem"))
	, TOP__Top__mem__memory          (Verilated::catName(topp->name(),"Top.mem.memory"))
{
    // Pointer to top level
    TOPp = topp;
    // Setup each module's pointers to their submodules
    TOPp->Top                       = &TOP__Top;
    TOPp->Top->mem                  = &TOP__Top__mem;
    TOPp->Top->mem->memory          = &TOP__Top__mem__memory;
    // Setup each module's pointer back to symbol table (for public functions)
    TOPp->__Vconfigure(this, true);
    TOP__Top.__Vconfigure(this, true);
    TOP__Top__mem.__Vconfigure(this, true);
    TOP__Top__mem__memory.__Vconfigure(this, true);
    // Setup scope names
    __Vscope_Top__mem__memory.configure(this,name(),"Top.mem.memory");
    // Setup export functions
    for (int __Vfinal=0; __Vfinal<2; __Vfinal++) {
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_address", &(TOP__Top__mem__memory.dmem_address), VLVT_UINT32,VLVD_IN|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_dataout", &(TOP__Top__mem__memory.dmem_dataout), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_maskmode", &(TOP__Top__mem__memory.dmem_maskmode), VLVT_UINT8,VLVD_IN|VLVF_PUB_RW,1 ,1,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_memread", &(TOP__Top__mem__memory.dmem_memread), VLVT_UINT8,VLVD_IN|VLVF_PUB_RW,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_memwrite", &(TOP__Top__mem__memory.dmem_memwrite), VLVT_UINT8,VLVD_IN|VLVF_PUB_RW,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_readdata", &(TOP__Top__mem__memory.dmem_readdata), VLVT_UINT32,VLVD_OUT|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_sext", &(TOP__Top__mem__memory.dmem_sext), VLVT_UINT8,VLVD_IN|VLVF_PUB_RW,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"dmem_writedata", &(TOP__Top__mem__memory.dmem_writedata), VLVT_UINT32,VLVD_IN|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"imem_address", &(TOP__Top__mem__memory.imem_address), VLVT_UINT32,VLVD_IN|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"imem_dataout", &(TOP__Top__mem__memory.imem_dataout), VLVT_UINT32,VLVD_NODIR|VLVF_PUB_RW,1 ,31,0);
	__Vscope_Top__mem__memory.varInsert(__Vfinal,"imem_instruction", &(TOP__Top__mem__memory.imem_instruction), VLVT_UINT32,VLVD_OUT|VLVF_PUB_RW,1 ,31,0);
    }
}
