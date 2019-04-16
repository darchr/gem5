module DualPortedMemoryBlackBox(
	input clk,
	input [31:0] imem_address /*verilator public*/,
	output [31:0] imem_instruction /*verilator public*/,
	input [31:0] dmem_address /*verilator public*/, 
	input [31:0] dmem_writedata /*verilator public*/,
	input dmem_memread /*verilator public*/,
	input dmem_memwrite /*verilator public*/,
	input [1:0] dmem_maskmode /*verilator public*/,
	input dmem_sext /*verilator public*/,
	output [31:0] dmem_readdata /*verilator public*/
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

	wire [31:0] dmem_dataout /*verilator public*/;
	wire [31:0] imem_dataout /*verilator public*/;
  always_comb imem_instruction = ifetch(imem_address, gem5MemBlkBox);

  always_comb dmem_readdata = datareq(dmem_address, dmem_writedata,
      dmem_memread, dmem_memwrite, dmem_maskmode, dmem_sext, gem5MemBlkBox);

endmodule
