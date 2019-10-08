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

module DualPortedSyncMemoryBlackBox(
	input imem_request_ready,
    input imem_request_valid,
    input [31:0] imem_request_bits_address,
    input [31:0] imem_request_bits_writedata,
    input imem_request_bits_operation,
    output imem_response_valid,
    output [31:0] imem_response_bits_data,
    input dmem_request_ready,
    input dmem_request_valid,
    input [31:0] dmem_request_bits_address,
    input [31:0] dmem_request_bits_writedata,
    input dmem_request_bits_operation,
    output dmem_response_valid,
    output [31:0] dmem_response_bits_data
);
  import "DPI-C" function int ifetch(input bit imem_request_ready,
    input bit imem_request_valid,
    input int imem_request_bits_address,
    output bit imem_response_valid, input chandle handle);

  import "DPI-C" function int datareq( input bit dmem_request_ready,
    input bit dmem_request_valid,
    input int dmem_request_bits_address,
    input int dmem_request_bits_writedata,
    input bit dmem_request_bits_operation,
    output bit dmem_response_valid, input chandle handle);

  import "DPI-C" function chandle setGem5Handle(); 

  chandle gem5MemBlkBox;
  initial begin
      gem5MemBlkBox = setGem5Handle();
  end

  always_comb imem_response_bits_data = ifetch(imem_request_ready,
    imem_request_valid, imem_request_bits_address, imem_response_valid,
    gem5MemBlkBox);

  always_comb dmem_response_bits_data = datareq(dmem_request_ready, dmem_request_valid, 
    dmem_request_bits_address, dmem_request_bits_writedata, 
    dmem_request_bits_operation, dmem_response_valid, gem5MemBlkBox);

endmodule
