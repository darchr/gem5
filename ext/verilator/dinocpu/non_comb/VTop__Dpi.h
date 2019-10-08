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
    // DPI Import at DualPortedSyncMemoryBlackBox.v:51
    extern int datareq (unsigned char dmem_request_ready, unsigned char dmem_request_valid, int dmem_request_bits_address, int dmem_request_bits_writedata, unsigned char dmem_request_bits_operation, unsigned char* dmem_response_valid, void* handle);
    // DPI Import at DualPortedSyncMemoryBlackBox.v:46
    extern int ifetch (unsigned char imem_request_ready, unsigned char imem_request_valid, int imem_request_bits_address, unsigned char* imem_response_valid, void* handle);
    // DPI Import at DualPortedSyncMemoryBlackBox.v:58
    extern void* setGem5Handle ();
    
#ifdef __cplusplus
}
#endif
