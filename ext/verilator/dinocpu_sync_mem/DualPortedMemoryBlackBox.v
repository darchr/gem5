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

module DualPortedMemoryBlackBox(
	input clk,
	input [31:0] imem_address,
	output [31:0] imem_instruction,
	input [31:0] dmem_address, 
	input [31:0] dmem_writedata,
	input dmem_memread,
	input dmem_memwrite,
	input [1:0] dmem_maskmode,
	input dmem_sext,
	output [31:0] dmem_readdata
);
  import "DPI-C" function int ifetch(input int imem_address, chandle handle);
  import "DPI-C" function int datareq(input int dmem_address, 
      input int dmem_writedata,input bit dmem_memread, 
      input bit dmem_memwrite, input bit [1:0] dmem_maskmode, 
      input bit dmem_sext, chandle handle);

  import "DPI-C" function chandle setGem5Handle(); 

  chandle gem5MemBlkBox;
  initial begin
      gem5MemBlkBox = setGem5Handle();
  end

  always_comb imem_instruction = ifetch(imem_address, gem5MemBlkBox);

  always_comb dmem_readdata = datareq(dmem_address, dmem_writedata,
      dmem_memread, dmem_memwrite, dmem_maskmode, dmem_sext, gem5MemBlkBox);

endmodule
