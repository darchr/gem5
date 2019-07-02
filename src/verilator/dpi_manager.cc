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
#include "verilator/dinocpu_async_mem/async_mem_black_box.hh"
#include "verilator/driven_object.hh"

AsyncMemBlackBox * memBlkBox = nullptr;
DrivenObject * drivenObj = nullptr;

//will run a doFetch from within blackbox wrapper
int ifetch(unsigned char imem_request_ready,
      unsigned char imem_request_valid, unsigned int imem_request_bits_address,
      unsigned char* imem_response_valid, void** handle)
{
  DPRINTF(Verilator, "DPI INST FETCH MADE\n");
  AsyncMemBlackBox* hndl =
    static_cast<AsyncMemBlackBox *>(*handle);
    hndl->doFetch(imem_request_ready, imem_request_valid,
      imem_request_bits_address, imem_response_valid);
    return hndl->getImemResp();
}

//will run a doMem from within blackbox wrapper
int datareq(unsigned char dmem_request_ready,
      unsigned char dmem_request_valid, int dmem_request_bits_address,
      int dmem_request_bits_writedata,
      unsigned char dmem_request_bits_operation,
      unsigned char* dmem_response_valid, void** handle)
{
  DPRINTF(Verilator, "DPI DATA REQ MADE\n");

  AsyncMemBlackBox* hndl =
    static_cast<AsyncMemBlackBox *>(*handle);
  hndl->doMem(dmem_request_ready, dmem_request_valid,dmem_request_bits_address,
        dmem_request_bits_writedata, dmem_request_bits_operation,
        dmem_response_valid);

  return hndl->getDmemResp();

}

//gives Dulaportedmemoryblackbox handle to verilatormemblkbox class
void* setGem5Handle (){
  DPRINTF(Verilator, "DPI GIVING MEM HANDLE TO VERILOG\n");

  memBlkBox = AsyncMemBlackBox::getSingleton();
  panic_if( memBlkBox == nullptr,
          "Verilog should not try to access null gem5 model!");

  return (void*) memBlkBox;
}

