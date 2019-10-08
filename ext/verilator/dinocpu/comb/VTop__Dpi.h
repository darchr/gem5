// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Prototypes for DPI import and export functions.
//
// Verilator includes this file in all generated .cpp files that use DPI functions.
// Manually include this file where DPI .c import functions are declared to insure
// the C functions match the expectations of the DPI imports.

#include "svdpi.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    
    // DPI IMPORTS
    // DPI Import at DualPortedMemoryBlackBox.v:43
    extern int datareq (unsigned int dmem_address, unsigned int dmem_writedata, unsigned char dmem_memread, unsigned char dmem_memwrite, const svBitVecVal* dmem_maskmode, unsigned char dmem_sext, void* handle);
    // DPI Import at DualPortedMemoryBlackBox.v:42
    extern int ifetch (unsigned int imem_address, void* handle);
    // DPI Import at DualPortedMemoryBlackBox.v:48
    extern void* setGem5Handle ();
    
#ifdef __cplusplus
}
#endif
