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
	reg [31:0] dmem_dataout /*verilator public*/;
	reg [31:0] imem_dataout /*verilator public*/;

	always @( posedge clk) begin
		dmem_readdata = dmem_dataout;
	end
	
	always @(posedge clk) begin
		imem_instruction = imem_dataout;
	end

endmodule
