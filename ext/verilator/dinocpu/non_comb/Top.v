module IMemPort(
  input  [31:0] io_address,
  input         io_valid,
  output        io_good,
  input         io_response_valid,
  input  [31:0] io_response_bits_data,
  output        io_request_valid,
  output [31:0] io_request_bits_address,
  output [31:0] io_instruction
);
  assign io_good = io_response_valid; // @[memory-ports.scala 46:11]
  assign io_request_valid = io_valid; // @[memory-ports.scala 40:22 memory-ports.scala 42:22]
  assign io_request_bits_address = io_address; // @[memory-ports.scala 39:22]
  assign io_instruction = io_response_bits_data; // @[memory-ports.scala 48:20]
endmodule
module DMemPort(
  input         clock,
  input         reset,
  input  [31:0] io_address,
  input         io_valid,
  output        io_good,
  input         io_response_valid,
  input  [31:0] io_response_bits_data,
  output        io_request_valid,
  output [31:0] io_request_bits_address,
  output [31:0] io_request_bits_writedata,
  output        io_request_bits_operation,
  input  [31:0] io_writedata,
  input         io_memread,
  input         io_memwrite,
  input  [1:0]  io_maskmode,
  input         io_sext,
  output [31:0] io_readdata
);
  reg  outstandingReq_valid; // @[memory-ports.scala 64:31]
  reg [31:0] _RAND_0;
  reg [31:0] outstandingReq_bits_address; // @[memory-ports.scala 64:31]
  reg [31:0] _RAND_1;
  reg [31:0] outstandingReq_bits_writedata; // @[memory-ports.scala 64:31]
  reg [31:0] _RAND_2;
  reg [1:0] outstandingReq_bits_maskmode; // @[memory-ports.scala 64:31]
  reg [31:0] _RAND_3;
  reg  outstandingReq_bits_operation; // @[memory-ports.scala 64:31]
  reg [31:0] _RAND_4;
  reg  outstandingReq_bits_sext; // @[memory-ports.scala 64:31]
  reg [31:0] _RAND_5;
  wire  _T_8; // @[memory-ports.scala 75:15]
  wire  _T_9; // @[memory-ports.scala 75:117]
  wire  _T_10; // @[memory-ports.scala 75:84]
  wire  _T_11; // @[memory-ports.scala 75:59]
  wire  ready; // @[memory-ports.scala 75:37]
  wire  _T_12; // @[memory-ports.scala 76:33]
  wire  _T_13; // @[memory-ports.scala 76:18]
  wire  sending; // @[memory-ports.scala 76:49]
  wire  _T_15; // @[memory-ports.scala 78:27]
  wire  _T_16; // @[memory-ports.scala 78:13]
  wire  _T_18; // @[memory-ports.scala 78:12]
  wire  _T_19; // @[memory-ports.scala 78:12]
  wire  _T_21; // @[memory-ports.scala 107:11]
  wire  _T_22; // @[memory-ports.scala 107:11]
  wire  _T_25; // @[memory-ports.scala 112:42]
  wire [1:0] _T_26; // @[memory-ports.scala 114:50]
  wire  _T_29; // @[memory-ports.scala 119:44]
  wire [3:0] _GEN_27; // @[memory-ports.scala 120:63]
  wire [5:0] _T_30; // @[memory-ports.scala 120:63]
  wire [70:0] _T_31; // @[memory-ports.scala 120:52]
  wire [70:0] _T_32; // @[memory-ports.scala 120:43]
  wire [70:0] _GEN_28; // @[memory-ports.scala 120:41]
  wire [70:0] _T_33; // @[memory-ports.scala 120:41]
  wire [78:0] _T_35; // @[memory-ports.scala 122:54]
  wire [78:0] _T_36; // @[memory-ports.scala 122:43]
  wire [78:0] _GEN_30; // @[memory-ports.scala 122:41]
  wire [78:0] _T_37; // @[memory-ports.scala 122:41]
  wire [78:0] _GEN_9; // @[memory-ports.scala 119:53]
  wire [94:0] _GEN_32; // @[memory-ports.scala 124:60]
  wire [94:0] _T_39; // @[memory-ports.scala 124:60]
  wire [31:0] _T_28; // @[memory-ports.scala 117:25 memory-ports.scala 120:16 memory-ports.scala 122:16]
  wire [94:0] _GEN_33; // @[memory-ports.scala 124:27]
  wire [94:0] _T_40; // @[memory-ports.scala 124:27]
  wire [94:0] _GEN_10; // @[memory-ports.scala 112:51]
  wire [31:0] _T_47; // @[memory-ports.scala 146:49]
  wire [31:0] _T_48; // @[memory-ports.scala 146:68]
  wire  _T_49; // @[memory-ports.scala 147:49]
  wire [31:0] _T_52; // @[memory-ports.scala 149:68]
  wire [31:0] _GEN_11; // @[memory-ports.scala 147:58]
  wire [31:0] _GEN_12; // @[memory-ports.scala 144:51]
  wire  _T_54; // @[memory-ports.scala 157:59]
  wire [23:0] _T_56; // @[Bitwise.scala 72:12]
  wire [7:0] _T_57; // @[memory-ports.scala 157:79]
  wire [31:0] _T_58; // @[Cat.scala 30:58]
  wire  _T_60; // @[memory-ports.scala 160:59]
  wire [15:0] _T_62; // @[Bitwise.scala 72:12]
  wire [15:0] _T_63; // @[memory-ports.scala 160:79]
  wire [31:0] _T_64; // @[Cat.scala 30:58]
  wire [31:0] _GEN_13; // @[memory-ports.scala 158:60]
  wire [31:0] _GEN_14; // @[memory-ports.scala 155:53]
  wire [31:0] _T_24; // @[memory-ports.scala 109:28 memory-ports.scala 124:19 memory-ports.scala 127:19]
  wire [31:0] _GEN_17; // @[memory-ports.scala 108:52]
  wire [31:0] _GEN_18; // @[memory-ports.scala 108:52]
  wire  _GEN_19; // @[memory-ports.scala 108:52]
  wire  _T_65; // @[memory-ports.scala 175:50]
  assign _T_8 = outstandingReq_valid == 1'h0; // @[memory-ports.scala 75:15]
  assign _T_9 = outstandingReq_bits_operation == 1'h0; // @[memory-ports.scala 75:117]
  assign _T_10 = outstandingReq_valid & _T_9; // @[memory-ports.scala 75:84]
  assign _T_11 = io_response_valid & _T_10; // @[memory-ports.scala 75:59]
  assign ready = _T_8 | _T_11; // @[memory-ports.scala 75:37]
  assign _T_12 = io_memread | io_memwrite; // @[memory-ports.scala 76:33]
  assign _T_13 = io_valid & _T_12; // @[memory-ports.scala 76:18]
  assign sending = _T_13 & ready; // @[memory-ports.scala 76:49]
  assign _T_15 = io_memread & io_memwrite; // @[memory-ports.scala 78:27]
  assign _T_16 = _T_15 == 1'h0; // @[memory-ports.scala 78:13]
  assign _T_18 = _T_16 | reset; // @[memory-ports.scala 78:12]
  assign _T_19 = _T_18 == 1'h0; // @[memory-ports.scala 78:12]
  assign _T_21 = outstandingReq_valid | reset; // @[memory-ports.scala 107:11]
  assign _T_22 = _T_21 == 1'h0; // @[memory-ports.scala 107:11]
  assign _T_25 = outstandingReq_bits_maskmode != 2'h2; // @[memory-ports.scala 112:42]
  assign _T_26 = outstandingReq_bits_address[1:0]; // @[memory-ports.scala 114:50]
  assign _T_29 = outstandingReq_bits_maskmode == 2'h0; // @[memory-ports.scala 119:44]
  assign _GEN_27 = {{2'd0}, _T_26}; // @[memory-ports.scala 120:63]
  assign _T_30 = _GEN_27 * 4'h8; // @[memory-ports.scala 120:63]
  assign _T_31 = 71'hff << _T_30; // @[memory-ports.scala 120:52]
  assign _T_32 = ~ _T_31; // @[memory-ports.scala 120:43]
  assign _GEN_28 = {{39'd0}, io_response_bits_data}; // @[memory-ports.scala 120:41]
  assign _T_33 = _GEN_28 & _T_32; // @[memory-ports.scala 120:41]
  assign _T_35 = 79'hffff << _T_30; // @[memory-ports.scala 122:54]
  assign _T_36 = ~ _T_35; // @[memory-ports.scala 122:43]
  assign _GEN_30 = {{47'd0}, io_response_bits_data}; // @[memory-ports.scala 122:41]
  assign _T_37 = _GEN_30 & _T_36; // @[memory-ports.scala 122:41]
  assign _GEN_9 = _T_29 ? {{8'd0}, _T_33} : _T_37; // @[memory-ports.scala 119:53]
  assign _GEN_32 = {{63'd0}, outstandingReq_bits_writedata}; // @[memory-ports.scala 124:60]
  assign _T_39 = _GEN_32 << _T_30; // @[memory-ports.scala 124:60]
  assign _T_28 = _GEN_9[31:0]; // @[memory-ports.scala 117:25 memory-ports.scala 120:16 memory-ports.scala 122:16]
  assign _GEN_33 = {{63'd0}, _T_28}; // @[memory-ports.scala 124:27]
  assign _T_40 = _GEN_33 | _T_39; // @[memory-ports.scala 124:27]
  assign _GEN_10 = _T_25 ? _T_40 : {{63'd0}, outstandingReq_bits_writedata}; // @[memory-ports.scala 112:51]
  assign _T_47 = io_response_bits_data >> _T_30; // @[memory-ports.scala 146:49]
  assign _T_48 = _T_47 & 32'hff; // @[memory-ports.scala 146:68]
  assign _T_49 = outstandingReq_bits_maskmode == 2'h1; // @[memory-ports.scala 147:49]
  assign _T_52 = _T_47 & 32'hffff; // @[memory-ports.scala 149:68]
  assign _GEN_11 = _T_49 ? _T_52 : io_response_bits_data; // @[memory-ports.scala 147:58]
  assign _GEN_12 = _T_29 ? _T_48 : _GEN_11; // @[memory-ports.scala 144:51]
  assign _T_54 = _GEN_12[7]; // @[memory-ports.scala 157:59]
  assign _T_56 = _T_54 ? 24'hffffff : 24'h0; // @[Bitwise.scala 72:12]
  assign _T_57 = _GEN_12[7:0]; // @[memory-ports.scala 157:79]
  assign _T_58 = {_T_56,_T_57}; // @[Cat.scala 30:58]
  assign _T_60 = _GEN_12[15]; // @[memory-ports.scala 160:59]
  assign _T_62 = _T_60 ? 16'hffff : 16'h0; // @[Bitwise.scala 72:12]
  assign _T_63 = _GEN_12[15:0]; // @[memory-ports.scala 160:79]
  assign _T_64 = {_T_62,_T_63}; // @[Cat.scala 30:58]
  assign _GEN_13 = _T_49 ? _T_64 : _GEN_12; // @[memory-ports.scala 158:60]
  assign _GEN_14 = _T_29 ? _T_58 : _GEN_13; // @[memory-ports.scala 155:53]
  assign _T_24 = _GEN_10[31:0]; // @[memory-ports.scala 109:28 memory-ports.scala 124:19 memory-ports.scala 127:19]
  assign _GEN_17 = outstandingReq_bits_operation ? _T_24 : 32'h0; // @[memory-ports.scala 108:52]
  assign _GEN_18 = outstandingReq_bits_operation ? outstandingReq_bits_address : io_address; // @[memory-ports.scala 108:52]
  assign _GEN_19 = outstandingReq_bits_operation ? 1'h1 : sending; // @[memory-ports.scala 108:52]
  assign _T_65 = outstandingReq_valid | sending; // @[memory-ports.scala 175:50]
  assign io_good = io_response_valid; // @[memory-ports.scala 60:11]
  assign io_request_valid = io_response_valid ? _GEN_19 : sending; // @[memory-ports.scala 98:31 memory-ports.scala 101:22 memory-ports.scala 136:24]
  assign io_request_bits_address = io_response_valid ? _GEN_18 : io_address; // @[memory-ports.scala 95:31 memory-ports.scala 135:24]
  assign io_request_bits_writedata = io_response_valid ? _GEN_17 : 32'h0; // @[memory-ports.scala 96:31 memory-ports.scala 135:24]
  assign io_request_bits_operation = io_response_valid ? outstandingReq_bits_operation : 1'h0; // @[memory-ports.scala 97:31 memory-ports.scala 135:24]
  assign io_readdata = outstandingReq_bits_sext ? _GEN_14 : _GEN_12; // @[memory-ports.scala 169:19]
`ifdef RANDOMIZE_GARBAGE_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_INVALID_ASSIGN
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_REG_INIT
`define RANDOMIZE
`endif
`ifdef RANDOMIZE_MEM_INIT
`define RANDOMIZE
`endif
`ifndef RANDOM
`define RANDOM $random
`endif
`ifdef RANDOMIZE_MEM_INIT
  integer initvar;
`endif
initial begin
  `ifdef RANDOMIZE
    `ifdef INIT_RANDOM
      `INIT_RANDOM
    `endif
    `ifndef VERILATOR
      `ifdef RANDOMIZE_DELAY
        #`RANDOMIZE_DELAY begin end
      `else
        #0.002 begin end
      `endif
    `endif
  `ifdef RANDOMIZE_REG_INIT
  _RAND_0 = {1{`RANDOM}};
  outstandingReq_valid = _RAND_0[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  outstandingReq_bits_address = _RAND_1[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  outstandingReq_bits_writedata = _RAND_2[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  outstandingReq_bits_maskmode = _RAND_3[1:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  outstandingReq_bits_operation = _RAND_4[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  outstandingReq_bits_sext = _RAND_5[0:0];
  `endif // RANDOMIZE_REG_INIT
  `endif // RANDOMIZE
end
  always @(posedge clock) begin
    if (reset) begin
      outstandingReq_valid <= 1'h0;
    end else begin
      if (io_response_valid) begin
        outstandingReq_valid <= sending;
      end else begin
        outstandingReq_valid <= _T_65;
      end
    end
    if (reset) begin
      outstandingReq_bits_address <= 32'h0;
    end else begin
      if (sending) begin
        outstandingReq_bits_address <= io_address;
      end
    end
    if (reset) begin
      outstandingReq_bits_writedata <= 32'h0;
    end else begin
      if (sending) begin
        outstandingReq_bits_writedata <= io_writedata;
      end
    end
    if (reset) begin
      outstandingReq_bits_maskmode <= 2'h0;
    end else begin
      if (sending) begin
        outstandingReq_bits_maskmode <= io_maskmode;
      end
    end
    if (reset) begin
      outstandingReq_bits_operation <= 1'h0;
    end else begin
      if (sending) begin
        outstandingReq_bits_operation <= io_memwrite;
      end
    end
    if (reset) begin
      outstandingReq_bits_sext <= 1'h0;
    end else begin
      if (sending) begin
        outstandingReq_bits_sext <= io_sext;
      end
    end
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (sending & _T_19) begin
          $fwrite(32'h80000002,"Assertion failed\n    at memory-ports.scala:78 assert (! (io.memread && io.memwrite))\n"); // @[memory-ports.scala 78:12]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef STOP_COND
      if (`STOP_COND) begin
    `endif
        if (sending & _T_19) begin
          $fatal; // @[memory-ports.scala 78:12]
        end
    `ifdef STOP_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (io_response_valid & _T_22) begin
          $fwrite(32'h80000002,"Assertion failed\n    at memory-ports.scala:107 assert(outstandingReq.valid)\n"); // @[memory-ports.scala 107:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef STOP_COND
      if (`STOP_COND) begin
    `endif
        if (io_response_valid & _T_22) begin
          $fatal; // @[memory-ports.scala 107:11]
        end
    `ifdef STOP_COND
      end
    `endif
    `endif // SYNTHESIS
  end
endmodule
module DualPortedSyncMemory(
  input         io_imem_request_valid,
  input  [31:0] io_imem_request_bits_address,
  output        io_imem_response_valid,
  output [31:0] io_imem_response_bits_data,
  input         io_dmem_request_valid,
  input  [31:0] io_dmem_request_bits_address,
  input  [31:0] io_dmem_request_bits_writedata,
  input         io_dmem_request_bits_operation,
  output        io_dmem_response_valid,
  output [31:0] io_dmem_response_bits_data
);
  wire  memory_imem_request_ready; // @[memory-async.scala 88:26]
  wire  memory_imem_request_valid; // @[memory-async.scala 88:26]
  wire [31:0] memory_imem_request_bits_address; // @[memory-async.scala 88:26]
  wire [31:0] memory_imem_request_bits_writedata; // @[memory-async.scala 88:26]
  wire  memory_imem_request_bits_operation; // @[memory-async.scala 88:26]
  wire  memory_imem_response_valid; // @[memory-async.scala 88:26]
  wire [31:0] memory_imem_response_bits_data; // @[memory-async.scala 88:26]
  wire  memory_dmem_request_ready; // @[memory-async.scala 88:26]
  wire  memory_dmem_request_valid; // @[memory-async.scala 88:26]
  wire [31:0] memory_dmem_request_bits_address; // @[memory-async.scala 88:26]
  wire [31:0] memory_dmem_request_bits_writedata; // @[memory-async.scala 88:26]
  wire  memory_dmem_request_bits_operation; // @[memory-async.scala 88:26]
  wire  memory_dmem_response_valid; // @[memory-async.scala 88:26]
  wire [31:0] memory_dmem_response_bits_data; // @[memory-async.scala 88:26]
  wire  _T_4; // @[memory-async.scala 104:42]
  wire  _GEN_2; // @[memory-async.scala 108:61]
  wire  _GEN_4; // @[memory-async.scala 104:51]
  DualPortedSyncMemoryBlackBox memory ( // @[memory-async.scala 88:26]
    .imem_request_ready(memory_imem_request_ready),
    .imem_request_valid(memory_imem_request_valid),
    .imem_request_bits_address(memory_imem_request_bits_address),
    .imem_request_bits_writedata(memory_imem_request_bits_writedata),
    .imem_request_bits_operation(memory_imem_request_bits_operation),
    .imem_response_valid(memory_imem_response_valid),
    .imem_response_bits_data(memory_imem_response_bits_data),
    .dmem_request_ready(memory_dmem_request_ready),
    .dmem_request_valid(memory_dmem_request_valid),
    .dmem_request_bits_address(memory_dmem_request_bits_address),
    .dmem_request_bits_writedata(memory_dmem_request_bits_writedata),
    .dmem_request_bits_operation(memory_dmem_request_bits_operation),
    .dmem_response_valid(memory_dmem_response_valid),
    .dmem_response_bits_data(memory_dmem_response_bits_data)
  );
  assign _T_4 = io_dmem_request_bits_operation == 1'h0; // @[memory-async.scala 104:42]
  assign _GEN_2 = io_dmem_request_bits_operation ? 1'h0 : memory_dmem_response_valid; // @[memory-async.scala 108:61]
  assign _GEN_4 = _T_4 ? 1'h1 : _GEN_2; // @[memory-async.scala 104:51]
  assign io_imem_response_valid = io_imem_request_valid; // @[memory-async.scala 89:19 memory-async.scala 94:28 memory-async.scala 98:28]
  assign io_imem_response_bits_data = memory_imem_response_bits_data; // @[memory-async.scala 89:19 memory-async.scala 96:32]
  assign io_dmem_response_valid = io_dmem_request_valid ? _GEN_4 : 1'h0; // @[memory-async.scala 90:19 memory-async.scala 105:31 memory-async.scala 109:31 memory-async.scala 114:28]
  assign io_dmem_response_bits_data = memory_dmem_response_bits_data; // @[memory-async.scala 90:19]
  assign memory_imem_request_valid = io_imem_request_valid; // @[memory-async.scala 89:19]
  assign memory_imem_request_bits_address = io_imem_request_bits_address; // @[memory-async.scala 89:19]
  assign memory_imem_request_bits_writedata = 32'h0; // @[memory-async.scala 89:19]
  assign memory_imem_request_bits_operation = 1'h0; // @[memory-async.scala 89:19]
  assign memory_dmem_request_valid = io_dmem_request_valid; // @[memory-async.scala 90:19]
  assign memory_dmem_request_bits_address = io_dmem_request_bits_address; // @[memory-async.scala 90:19]
  assign memory_dmem_request_bits_writedata = io_dmem_request_bits_writedata; // @[memory-async.scala 90:19 memory-async.scala 111:45]
  assign memory_dmem_request_bits_operation = io_dmem_request_bits_operation; // @[memory-async.scala 90:19]
endmodule
module Top(
  input         clock,
  input         reset,
  input  [31:0] io_imem_address,
  input         io_imem_valid,
  output [31:0] io_imem_instruction,
  output        io_imem_good,
  input  [31:0] io_dmem_address,
  input         io_dmem_valid,
  input  [31:0] io_dmem_writedata,
  input         io_dmem_memread,
  input         io_dmem_memwrite,
  input  [1:0]  io_dmem_maskmode,
  input         io_dmem_sext,
  output [31:0] io_dmem_readdata,
  output        io_dmem_good
);
  wire [31:0] imem_io_address; // @[SyncMemoryTestModule.scala 34:20]
  wire  imem_io_valid; // @[SyncMemoryTestModule.scala 34:20]
  wire  imem_io_good; // @[SyncMemoryTestModule.scala 34:20]
  wire  imem_io_response_valid; // @[SyncMemoryTestModule.scala 34:20]
  wire [31:0] imem_io_response_bits_data; // @[SyncMemoryTestModule.scala 34:20]
  wire  imem_io_request_valid; // @[SyncMemoryTestModule.scala 34:20]
  wire [31:0] imem_io_request_bits_address; // @[SyncMemoryTestModule.scala 34:20]
  wire [31:0] imem_io_instruction; // @[SyncMemoryTestModule.scala 34:20]
  wire  dmem_clock; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_reset; // @[SyncMemoryTestModule.scala 36:20]
  wire [31:0] dmem_io_address; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_valid; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_good; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_response_valid; // @[SyncMemoryTestModule.scala 36:20]
  wire [31:0] dmem_io_response_bits_data; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_request_valid; // @[SyncMemoryTestModule.scala 36:20]
  wire [31:0] dmem_io_request_bits_address; // @[SyncMemoryTestModule.scala 36:20]
  wire [31:0] dmem_io_request_bits_writedata; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_request_bits_operation; // @[SyncMemoryTestModule.scala 36:20]
  wire [31:0] dmem_io_writedata; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_memread; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_memwrite; // @[SyncMemoryTestModule.scala 36:20]
  wire [1:0] dmem_io_maskmode; // @[SyncMemoryTestModule.scala 36:20]
  wire  dmem_io_sext; // @[SyncMemoryTestModule.scala 36:20]
  wire [31:0] dmem_io_readdata; // @[SyncMemoryTestModule.scala 36:20]
  wire  memory_io_imem_request_valid; // @[SyncMemoryTestModule.scala 37:22]
  wire [31:0] memory_io_imem_request_bits_address; // @[SyncMemoryTestModule.scala 37:22]
  wire  memory_io_imem_response_valid; // @[SyncMemoryTestModule.scala 37:22]
  wire [31:0] memory_io_imem_response_bits_data; // @[SyncMemoryTestModule.scala 37:22]
  wire  memory_io_dmem_request_valid; // @[SyncMemoryTestModule.scala 37:22]
  wire [31:0] memory_io_dmem_request_bits_address; // @[SyncMemoryTestModule.scala 37:22]
  wire [31:0] memory_io_dmem_request_bits_writedata; // @[SyncMemoryTestModule.scala 37:22]
  wire  memory_io_dmem_request_bits_operation; // @[SyncMemoryTestModule.scala 37:22]
  wire  memory_io_dmem_response_valid; // @[SyncMemoryTestModule.scala 37:22]
  wire [31:0] memory_io_dmem_response_bits_data; // @[SyncMemoryTestModule.scala 37:22]
  IMemPort imem ( // @[SyncMemoryTestModule.scala 34:20]
    .io_address(imem_io_address),
    .io_valid(imem_io_valid),
    .io_good(imem_io_good),
    .io_response_valid(imem_io_response_valid),
    .io_response_bits_data(imem_io_response_bits_data),
    .io_request_valid(imem_io_request_valid),
    .io_request_bits_address(imem_io_request_bits_address),
    .io_instruction(imem_io_instruction)
  );
  DMemPort dmem ( // @[SyncMemoryTestModule.scala 36:20]
    .clock(dmem_clock),
    .reset(dmem_reset),
    .io_address(dmem_io_address),
    .io_valid(dmem_io_valid),
    .io_good(dmem_io_good),
    .io_response_valid(dmem_io_response_valid),
    .io_response_bits_data(dmem_io_response_bits_data),
    .io_request_valid(dmem_io_request_valid),
    .io_request_bits_address(dmem_io_request_bits_address),
    .io_request_bits_writedata(dmem_io_request_bits_writedata),
    .io_request_bits_operation(dmem_io_request_bits_operation),
    .io_writedata(dmem_io_writedata),
    .io_memread(dmem_io_memread),
    .io_memwrite(dmem_io_memwrite),
    .io_maskmode(dmem_io_maskmode),
    .io_sext(dmem_io_sext),
    .io_readdata(dmem_io_readdata)
  );
  DualPortedSyncMemory memory ( // @[SyncMemoryTestModule.scala 37:22]
    .io_imem_request_valid(memory_io_imem_request_valid),
    .io_imem_request_bits_address(memory_io_imem_request_bits_address),
    .io_imem_response_valid(memory_io_imem_response_valid),
    .io_imem_response_bits_data(memory_io_imem_response_bits_data),
    .io_dmem_request_valid(memory_io_dmem_request_valid),
    .io_dmem_request_bits_address(memory_io_dmem_request_bits_address),
    .io_dmem_request_bits_writedata(memory_io_dmem_request_bits_writedata),
    .io_dmem_request_bits_operation(memory_io_dmem_request_bits_operation),
    .io_dmem_response_valid(memory_io_dmem_response_valid),
    .io_dmem_response_bits_data(memory_io_dmem_response_bits_data)
  );
  assign io_imem_instruction = imem_io_instruction; // @[SyncMemoryTestModule.scala 43:23]
  assign io_imem_good = imem_io_good; // @[SyncMemoryTestModule.scala 44:23]
  assign io_dmem_readdata = dmem_io_readdata; // @[SyncMemoryTestModule.scala 52:23]
  assign io_dmem_good = dmem_io_good; // @[SyncMemoryTestModule.scala 53:23]
  assign imem_io_address = io_imem_address; // @[SyncMemoryTestModule.scala 41:23]
  assign imem_io_valid = io_imem_valid; // @[SyncMemoryTestModule.scala 42:23]
  assign imem_io_response_valid = memory_io_imem_response_valid; // @[SyncMemoryTestModule.scala 58:27]
  assign imem_io_response_bits_data = memory_io_imem_response_bits_data; // @[SyncMemoryTestModule.scala 58:27]
  assign dmem_clock = clock;
  assign dmem_reset = reset;
  assign dmem_io_address = io_dmem_address; // @[SyncMemoryTestModule.scala 45:23]
  assign dmem_io_valid = io_dmem_valid; // @[SyncMemoryTestModule.scala 46:23]
  assign dmem_io_response_valid = memory_io_dmem_response_valid; // @[SyncMemoryTestModule.scala 61:27]
  assign dmem_io_response_bits_data = memory_io_dmem_response_bits_data; // @[SyncMemoryTestModule.scala 61:27]
  assign dmem_io_writedata = io_dmem_writedata; // @[SyncMemoryTestModule.scala 47:23]
  assign dmem_io_memread = io_dmem_memread; // @[SyncMemoryTestModule.scala 48:23]
  assign dmem_io_memwrite = io_dmem_memwrite; // @[SyncMemoryTestModule.scala 49:23]
  assign dmem_io_maskmode = io_dmem_maskmode; // @[SyncMemoryTestModule.scala 50:23]
  assign dmem_io_sext = io_dmem_sext; // @[SyncMemoryTestModule.scala 51:23]
  assign memory_io_imem_request_valid = imem_io_request_valid; // @[SyncMemoryTestModule.scala 57:27]
  assign memory_io_imem_request_bits_address = imem_io_request_bits_address; // @[SyncMemoryTestModule.scala 57:27]
  assign memory_io_dmem_request_valid = dmem_io_request_valid; // @[SyncMemoryTestModule.scala 60:27]
  assign memory_io_dmem_request_bits_address = dmem_io_request_bits_address; // @[SyncMemoryTestModule.scala 60:27]
  assign memory_io_dmem_request_bits_writedata = dmem_io_request_bits_writedata; // @[SyncMemoryTestModule.scala 60:27]
  assign memory_io_dmem_request_bits_operation = dmem_io_request_bits_operation; // @[SyncMemoryTestModule.scala 60:27]
endmodule
