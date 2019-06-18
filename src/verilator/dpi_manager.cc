/*# Copyright (c) 2019 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nima Ganjehloo
*/

//verilator inlcudes
#include "VTop__Dpi.h"
#include "svdpi.h"

//gem5 includes
#include "base/logging.hh"
#include "debug/Verilator.hh"

//gem5 mdeol includes
#include "verilator/dinocpu_sync_mem/verilator_sync_mem_black_box.hh"
#include "verilator/verilator_dino_cpu.hh"

VerilatorSyncMemBlackBox * memBlkBox = nullptr;
VerilatorDinoCPU * dinoCPU = nullptr;

//will run a doFetch from within blackbox wrapper
int ifetch (unsigned int imem_address, void* handle){
  DPRINTF(Verilator, "DPI INST FETCH MADE\n");
  VerilatorSyncMemBlackBox* hndl =
    static_cast<VerilatorSyncMemBlackBox *>(handle);
  //lazy exception handeling
/*  if (imem_address == 0x80000000 ){
    DPRINTF(Verilator, "DPI INST FETCH ILLEGAL INST EXCEPTION\n");
    return 0x00c0006f;//jal x0, 12; effectively halt the processor
  }else if ( imem_address == 0x80000004 ){
    DPRINTF(Verilator, "DPI INST FETCH TRAPPING FOR ECALL\n");
    return 0x0080006f;//jal x0, 8; effectively halt the processor
  }else if ( imem_address == 0x80000008 ){
    DPRINTF(Verilator, "DPI INST FETCH TRAPPING FOR EBREAK\n");
    return 0x0040006f;//jal x0, 4; effectively halt the processor
  }else if ( imem_address > 0x80000008 ){
    //if for w/e reason we end up here then our exception could not be handled
    //either due to a lack of a handler or maybe a stray cosmic ray
    DPRINTF(Verilator, "UNHANDLED EXCEPTION, INDEFINITELY STALLING CPU\n");
    return 0x13; //do a nop for now
  }else{*/
    //normal instruction fetch
    hndl->doFetch(imem_address);

    return hndl->getImemResp();
  //}
}
//will run a doMem from within blackbox wrapper
int datareq (unsigned int dmem_address, unsigned int dmem_writedata,
        unsigned char dmem_memread, unsigned char dmem_memwrite,
        const svBitVecVal* dmem_maskmode, unsigned char dmem_sext,
        void* handle)
{
  DPRINTF(Verilator, "DPI DATA REQ MADE\n");

  VerilatorSyncMemBlackBox* hndl =
    static_cast<VerilatorSyncMemBlackBox *>(handle);
  hndl->doMem(dmem_address,  dmem_writedata, dmem_memread,
        dmem_memwrite, dmem_maskmode, dmem_sext);

  return hndl->getDmemResp();

}

//gives Dulaportedmemoryblackbox handle to verilatormemblkbox class
void* setGem5Handle (){
  DPRINTF(Verilator, "DPI GIVING MEM HANDLE TO VERILOG\n");

  memBlkBox = VerilatorSyncMemBlackBox::getSingleton();
  panic_if( memBlkBox == nullptr,
          "Verilog should not try to access null gem5 model!");

  return (void*) memBlkBox;
}

