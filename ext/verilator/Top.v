module Control(
  input  [6:0] io_opcode,
  output       io_branch,
  output       io_memread,
  output [1:0] io_toreg,
  output       io_add,
  output       io_memwrite,
  output       io_regwrite,
  output       io_immediate,
  output [1:0] io_alusrc1,
  output [1:0] io_jump
);
  wire  _T_1; // @[Lookup.scala 9:38]
  wire  _T_3; // @[Lookup.scala 9:38]
  wire  _T_5; // @[Lookup.scala 9:38]
  wire  _T_7; // @[Lookup.scala 9:38]
  wire  _T_9; // @[Lookup.scala 9:38]
  wire  _T_11; // @[Lookup.scala 9:38]
  wire  _T_13; // @[Lookup.scala 9:38]
  wire  _T_15; // @[Lookup.scala 9:38]
  wire  _T_17; // @[Lookup.scala 9:38]
  wire  _T_20; // @[Lookup.scala 11:37]
  wire  _T_21; // @[Lookup.scala 11:37]
  wire  _T_22; // @[Lookup.scala 11:37]
  wire  _T_23; // @[Lookup.scala 11:37]
  wire  _T_24; // @[Lookup.scala 11:37]
  wire  _T_25; // @[Lookup.scala 11:37]
  wire  _T_33; // @[Lookup.scala 11:37]
  wire [1:0] _T_34; // @[Lookup.scala 11:37]
  wire [1:0] _T_35; // @[Lookup.scala 11:37]
  wire [1:0] _T_36; // @[Lookup.scala 11:37]
  wire [1:0] _T_37; // @[Lookup.scala 11:37]
  wire [1:0] _T_38; // @[Lookup.scala 11:37]
  wire [1:0] _T_39; // @[Lookup.scala 11:37]
  wire [1:0] _T_40; // @[Lookup.scala 11:37]
  wire [1:0] _T_41; // @[Lookup.scala 11:37]
  wire  _T_45; // @[Lookup.scala 11:37]
  wire  _T_46; // @[Lookup.scala 11:37]
  wire  _T_47; // @[Lookup.scala 11:37]
  wire  _T_48; // @[Lookup.scala 11:37]
  wire  _T_49; // @[Lookup.scala 11:37]
  wire  _T_56; // @[Lookup.scala 11:37]
  wire  _T_57; // @[Lookup.scala 11:37]
  wire  _T_59; // @[Lookup.scala 11:37]
  wire  _T_60; // @[Lookup.scala 11:37]
  wire  _T_61; // @[Lookup.scala 11:37]
  wire  _T_62; // @[Lookup.scala 11:37]
  wire  _T_63; // @[Lookup.scala 11:37]
  wire  _T_64; // @[Lookup.scala 11:37]
  wire  _T_65; // @[Lookup.scala 11:37]
  wire  _T_67; // @[Lookup.scala 11:37]
  wire  _T_68; // @[Lookup.scala 11:37]
  wire  _T_69; // @[Lookup.scala 11:37]
  wire  _T_70; // @[Lookup.scala 11:37]
  wire  _T_71; // @[Lookup.scala 11:37]
  wire  _T_72; // @[Lookup.scala 11:37]
  wire  _T_73; // @[Lookup.scala 11:37]
  wire [1:0] _T_76; // @[Lookup.scala 11:37]
  wire [1:0] _T_77; // @[Lookup.scala 11:37]
  wire [1:0] _T_78; // @[Lookup.scala 11:37]
  wire [1:0] _T_79; // @[Lookup.scala 11:37]
  wire [1:0] _T_80; // @[Lookup.scala 11:37]
  wire [1:0] _T_81; // @[Lookup.scala 11:37]
  wire [1:0] _T_82; // @[Lookup.scala 11:37]
  wire [1:0] _T_83; // @[Lookup.scala 11:37]
  wire [1:0] _T_84; // @[Lookup.scala 11:37]
  wire [1:0] _T_85; // @[Lookup.scala 11:37]
  wire [1:0] _T_86; // @[Lookup.scala 11:37]
  wire [1:0] _T_87; // @[Lookup.scala 11:37]
  wire [1:0] _T_88; // @[Lookup.scala 11:37]
  wire [1:0] _T_89; // @[Lookup.scala 11:37]
  assign _T_1 = 7'h33 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_3 = 7'h13 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_5 = 7'h3 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_7 = 7'h23 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_9 = 7'h63 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_11 = 7'h37 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_13 = 7'h17 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_15 = 7'h6f == io_opcode; // @[Lookup.scala 9:38]
  assign _T_17 = 7'h67 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_20 = _T_13 ? 1'h0 : _T_15; // @[Lookup.scala 11:37]
  assign _T_21 = _T_11 ? 1'h0 : _T_20; // @[Lookup.scala 11:37]
  assign _T_22 = _T_9 ? 1'h1 : _T_21; // @[Lookup.scala 11:37]
  assign _T_23 = _T_7 ? 1'h0 : _T_22; // @[Lookup.scala 11:37]
  assign _T_24 = _T_5 ? 1'h0 : _T_23; // @[Lookup.scala 11:37]
  assign _T_25 = _T_3 ? 1'h0 : _T_24; // @[Lookup.scala 11:37]
  assign _T_33 = _T_3 ? 1'h0 : _T_5; // @[Lookup.scala 11:37]
  assign _T_34 = _T_17 ? 2'h2 : 2'h3; // @[Lookup.scala 11:37]
  assign _T_35 = _T_15 ? 2'h2 : _T_34; // @[Lookup.scala 11:37]
  assign _T_36 = _T_13 ? 2'h0 : _T_35; // @[Lookup.scala 11:37]
  assign _T_37 = _T_11 ? 2'h0 : _T_36; // @[Lookup.scala 11:37]
  assign _T_38 = _T_9 ? 2'h0 : _T_37; // @[Lookup.scala 11:37]
  assign _T_39 = _T_7 ? 2'h0 : _T_38; // @[Lookup.scala 11:37]
  assign _T_40 = _T_5 ? 2'h1 : _T_39; // @[Lookup.scala 11:37]
  assign _T_41 = _T_3 ? 2'h0 : _T_40; // @[Lookup.scala 11:37]
  assign _T_45 = _T_11 ? 1'h1 : _T_13; // @[Lookup.scala 11:37]
  assign _T_46 = _T_9 ? 1'h0 : _T_45; // @[Lookup.scala 11:37]
  assign _T_47 = _T_7 ? 1'h1 : _T_46; // @[Lookup.scala 11:37]
  assign _T_48 = _T_5 ? 1'h1 : _T_47; // @[Lookup.scala 11:37]
  assign _T_49 = _T_3 ? 1'h0 : _T_48; // @[Lookup.scala 11:37]
  assign _T_56 = _T_5 ? 1'h0 : _T_7; // @[Lookup.scala 11:37]
  assign _T_57 = _T_3 ? 1'h0 : _T_56; // @[Lookup.scala 11:37]
  assign _T_59 = _T_15 ? 1'h0 : _T_17; // @[Lookup.scala 11:37]
  assign _T_60 = _T_13 ? 1'h1 : _T_59; // @[Lookup.scala 11:37]
  assign _T_61 = _T_11 ? 1'h1 : _T_60; // @[Lookup.scala 11:37]
  assign _T_62 = _T_9 ? 1'h0 : _T_61; // @[Lookup.scala 11:37]
  assign _T_63 = _T_7 ? 1'h1 : _T_62; // @[Lookup.scala 11:37]
  assign _T_64 = _T_5 ? 1'h1 : _T_63; // @[Lookup.scala 11:37]
  assign _T_65 = _T_3 ? 1'h1 : _T_64; // @[Lookup.scala 11:37]
  assign _T_67 = _T_15 ? 1'h1 : _T_17; // @[Lookup.scala 11:37]
  assign _T_68 = _T_13 ? 1'h1 : _T_67; // @[Lookup.scala 11:37]
  assign _T_69 = _T_11 ? 1'h1 : _T_68; // @[Lookup.scala 11:37]
  assign _T_70 = _T_9 ? 1'h0 : _T_69; // @[Lookup.scala 11:37]
  assign _T_71 = _T_7 ? 1'h0 : _T_70; // @[Lookup.scala 11:37]
  assign _T_72 = _T_5 ? 1'h1 : _T_71; // @[Lookup.scala 11:37]
  assign _T_73 = _T_3 ? 1'h1 : _T_72; // @[Lookup.scala 11:37]
  assign _T_76 = _T_13 ? 2'h2 : {{1'd0}, _T_15}; // @[Lookup.scala 11:37]
  assign _T_77 = _T_11 ? 2'h1 : _T_76; // @[Lookup.scala 11:37]
  assign _T_78 = _T_9 ? 2'h0 : _T_77; // @[Lookup.scala 11:37]
  assign _T_79 = _T_7 ? 2'h0 : _T_78; // @[Lookup.scala 11:37]
  assign _T_80 = _T_5 ? 2'h0 : _T_79; // @[Lookup.scala 11:37]
  assign _T_81 = _T_3 ? 2'h0 : _T_80; // @[Lookup.scala 11:37]
  assign _T_82 = _T_17 ? 2'h3 : 2'h0; // @[Lookup.scala 11:37]
  assign _T_83 = _T_15 ? 2'h2 : _T_82; // @[Lookup.scala 11:37]
  assign _T_84 = _T_13 ? 2'h0 : _T_83; // @[Lookup.scala 11:37]
  assign _T_85 = _T_11 ? 2'h0 : _T_84; // @[Lookup.scala 11:37]
  assign _T_86 = _T_9 ? 2'h0 : _T_85; // @[Lookup.scala 11:37]
  assign _T_87 = _T_7 ? 2'h0 : _T_86; // @[Lookup.scala 11:37]
  assign _T_88 = _T_5 ? 2'h0 : _T_87; // @[Lookup.scala 11:37]
  assign _T_89 = _T_3 ? 2'h0 : _T_88; // @[Lookup.scala 11:37]
  assign io_branch = _T_1 ? 1'h0 : _T_25; // @[control.scala 65:13]
  assign io_memread = _T_1 ? 1'h0 : _T_33; // @[control.scala 66:14]
  assign io_toreg = _T_1 ? 2'h0 : _T_41; // @[control.scala 67:12]
  assign io_add = _T_1 ? 1'h0 : _T_49; // @[control.scala 68:10]
  assign io_memwrite = _T_1 ? 1'h0 : _T_57; // @[control.scala 69:15]
  assign io_regwrite = _T_1 ? 1'h1 : _T_73; // @[control.scala 71:15]
  assign io_immediate = _T_1 ? 1'h0 : _T_65; // @[control.scala 70:16]
  assign io_alusrc1 = _T_1 ? 2'h0 : _T_81; // @[control.scala 72:14]
  assign io_jump = _T_1 ? 2'h0 : _T_89; // @[control.scala 73:11]
endmodule
module RegisterFile(
  input         clock,
  input  [4:0]  io_readreg1,
  input  [4:0]  io_readreg2,
  input  [4:0]  io_writereg,
  input  [31:0] io_writedata,
  input         io_wen,
  output [31:0] io_readdata1,
  output [31:0] io_readdata2
);
  reg [31:0] regs_0; // @[register-file.scala 50:17]
  reg [31:0] _RAND_0;
  reg [31:0] regs_1; // @[register-file.scala 50:17]
  reg [31:0] _RAND_1;
  reg [31:0] regs_2; // @[register-file.scala 50:17]
  reg [31:0] _RAND_2;
  reg [31:0] regs_3; // @[register-file.scala 50:17]
  reg [31:0] _RAND_3;
  reg [31:0] regs_4; // @[register-file.scala 50:17]
  reg [31:0] _RAND_4;
  reg [31:0] regs_5; // @[register-file.scala 50:17]
  reg [31:0] _RAND_5;
  reg [31:0] regs_6; // @[register-file.scala 50:17]
  reg [31:0] _RAND_6;
  reg [31:0] regs_7; // @[register-file.scala 50:17]
  reg [31:0] _RAND_7;
  reg [31:0] regs_8; // @[register-file.scala 50:17]
  reg [31:0] _RAND_8;
  reg [31:0] regs_9; // @[register-file.scala 50:17]
  reg [31:0] _RAND_9;
  reg [31:0] regs_10; // @[register-file.scala 50:17]
  reg [31:0] _RAND_10;
  reg [31:0] regs_11; // @[register-file.scala 50:17]
  reg [31:0] _RAND_11;
  reg [31:0] regs_12; // @[register-file.scala 50:17]
  reg [31:0] _RAND_12;
  reg [31:0] regs_13; // @[register-file.scala 50:17]
  reg [31:0] _RAND_13;
  reg [31:0] regs_14; // @[register-file.scala 50:17]
  reg [31:0] _RAND_14;
  reg [31:0] regs_15; // @[register-file.scala 50:17]
  reg [31:0] _RAND_15;
  reg [31:0] regs_16; // @[register-file.scala 50:17]
  reg [31:0] _RAND_16;
  reg [31:0] regs_17; // @[register-file.scala 50:17]
  reg [31:0] _RAND_17;
  reg [31:0] regs_18; // @[register-file.scala 50:17]
  reg [31:0] _RAND_18;
  reg [31:0] regs_19; // @[register-file.scala 50:17]
  reg [31:0] _RAND_19;
  reg [31:0] regs_20; // @[register-file.scala 50:17]
  reg [31:0] _RAND_20;
  reg [31:0] regs_21; // @[register-file.scala 50:17]
  reg [31:0] _RAND_21;
  reg [31:0] regs_22; // @[register-file.scala 50:17]
  reg [31:0] _RAND_22;
  reg [31:0] regs_23; // @[register-file.scala 50:17]
  reg [31:0] _RAND_23;
  reg [31:0] regs_24; // @[register-file.scala 50:17]
  reg [31:0] _RAND_24;
  reg [31:0] regs_25; // @[register-file.scala 50:17]
  reg [31:0] _RAND_25;
  reg [31:0] regs_26; // @[register-file.scala 50:17]
  reg [31:0] _RAND_26;
  reg [31:0] regs_27; // @[register-file.scala 50:17]
  reg [31:0] _RAND_27;
  reg [31:0] regs_28; // @[register-file.scala 50:17]
  reg [31:0] _RAND_28;
  reg [31:0] regs_29; // @[register-file.scala 50:17]
  reg [31:0] _RAND_29;
  reg [31:0] regs_30; // @[register-file.scala 50:17]
  reg [31:0] _RAND_30;
  reg [31:0] regs_31; // @[register-file.scala 50:17]
  reg [31:0] _RAND_31;
  wire [31:0] _GEN_65; // @[register-file.scala 59:16]
  wire [31:0] _GEN_66; // @[register-file.scala 59:16]
  wire [31:0] _GEN_67; // @[register-file.scala 59:16]
  wire [31:0] _GEN_68; // @[register-file.scala 59:16]
  wire [31:0] _GEN_69; // @[register-file.scala 59:16]
  wire [31:0] _GEN_70; // @[register-file.scala 59:16]
  wire [31:0] _GEN_71; // @[register-file.scala 59:16]
  wire [31:0] _GEN_72; // @[register-file.scala 59:16]
  wire [31:0] _GEN_73; // @[register-file.scala 59:16]
  wire [31:0] _GEN_74; // @[register-file.scala 59:16]
  wire [31:0] _GEN_75; // @[register-file.scala 59:16]
  wire [31:0] _GEN_76; // @[register-file.scala 59:16]
  wire [31:0] _GEN_77; // @[register-file.scala 59:16]
  wire [31:0] _GEN_78; // @[register-file.scala 59:16]
  wire [31:0] _GEN_79; // @[register-file.scala 59:16]
  wire [31:0] _GEN_80; // @[register-file.scala 59:16]
  wire [31:0] _GEN_81; // @[register-file.scala 59:16]
  wire [31:0] _GEN_82; // @[register-file.scala 59:16]
  wire [31:0] _GEN_83; // @[register-file.scala 59:16]
  wire [31:0] _GEN_84; // @[register-file.scala 59:16]
  wire [31:0] _GEN_85; // @[register-file.scala 59:16]
  wire [31:0] _GEN_86; // @[register-file.scala 59:16]
  wire [31:0] _GEN_87; // @[register-file.scala 59:16]
  wire [31:0] _GEN_88; // @[register-file.scala 59:16]
  wire [31:0] _GEN_89; // @[register-file.scala 59:16]
  wire [31:0] _GEN_90; // @[register-file.scala 59:16]
  wire [31:0] _GEN_91; // @[register-file.scala 59:16]
  wire [31:0] _GEN_92; // @[register-file.scala 59:16]
  wire [31:0] _GEN_93; // @[register-file.scala 59:16]
  wire [31:0] _GEN_94; // @[register-file.scala 59:16]
  wire [31:0] _GEN_97; // @[register-file.scala 60:16]
  wire [31:0] _GEN_98; // @[register-file.scala 60:16]
  wire [31:0] _GEN_99; // @[register-file.scala 60:16]
  wire [31:0] _GEN_100; // @[register-file.scala 60:16]
  wire [31:0] _GEN_101; // @[register-file.scala 60:16]
  wire [31:0] _GEN_102; // @[register-file.scala 60:16]
  wire [31:0] _GEN_103; // @[register-file.scala 60:16]
  wire [31:0] _GEN_104; // @[register-file.scala 60:16]
  wire [31:0] _GEN_105; // @[register-file.scala 60:16]
  wire [31:0] _GEN_106; // @[register-file.scala 60:16]
  wire [31:0] _GEN_107; // @[register-file.scala 60:16]
  wire [31:0] _GEN_108; // @[register-file.scala 60:16]
  wire [31:0] _GEN_109; // @[register-file.scala 60:16]
  wire [31:0] _GEN_110; // @[register-file.scala 60:16]
  wire [31:0] _GEN_111; // @[register-file.scala 60:16]
  wire [31:0] _GEN_112; // @[register-file.scala 60:16]
  wire [31:0] _GEN_113; // @[register-file.scala 60:16]
  wire [31:0] _GEN_114; // @[register-file.scala 60:16]
  wire [31:0] _GEN_115; // @[register-file.scala 60:16]
  wire [31:0] _GEN_116; // @[register-file.scala 60:16]
  wire [31:0] _GEN_117; // @[register-file.scala 60:16]
  wire [31:0] _GEN_118; // @[register-file.scala 60:16]
  wire [31:0] _GEN_119; // @[register-file.scala 60:16]
  wire [31:0] _GEN_120; // @[register-file.scala 60:16]
  wire [31:0] _GEN_121; // @[register-file.scala 60:16]
  wire [31:0] _GEN_122; // @[register-file.scala 60:16]
  wire [31:0] _GEN_123; // @[register-file.scala 60:16]
  wire [31:0] _GEN_124; // @[register-file.scala 60:16]
  wire [31:0] _GEN_125; // @[register-file.scala 60:16]
  wire [31:0] _GEN_126; // @[register-file.scala 60:16]
  assign _GEN_65 = 5'h1 == io_readreg1 ? regs_1 : regs_0; // @[register-file.scala 59:16]
  assign _GEN_66 = 5'h2 == io_readreg1 ? regs_2 : _GEN_65; // @[register-file.scala 59:16]
  assign _GEN_67 = 5'h3 == io_readreg1 ? regs_3 : _GEN_66; // @[register-file.scala 59:16]
  assign _GEN_68 = 5'h4 == io_readreg1 ? regs_4 : _GEN_67; // @[register-file.scala 59:16]
  assign _GEN_69 = 5'h5 == io_readreg1 ? regs_5 : _GEN_68; // @[register-file.scala 59:16]
  assign _GEN_70 = 5'h6 == io_readreg1 ? regs_6 : _GEN_69; // @[register-file.scala 59:16]
  assign _GEN_71 = 5'h7 == io_readreg1 ? regs_7 : _GEN_70; // @[register-file.scala 59:16]
  assign _GEN_72 = 5'h8 == io_readreg1 ? regs_8 : _GEN_71; // @[register-file.scala 59:16]
  assign _GEN_73 = 5'h9 == io_readreg1 ? regs_9 : _GEN_72; // @[register-file.scala 59:16]
  assign _GEN_74 = 5'ha == io_readreg1 ? regs_10 : _GEN_73; // @[register-file.scala 59:16]
  assign _GEN_75 = 5'hb == io_readreg1 ? regs_11 : _GEN_74; // @[register-file.scala 59:16]
  assign _GEN_76 = 5'hc == io_readreg1 ? regs_12 : _GEN_75; // @[register-file.scala 59:16]
  assign _GEN_77 = 5'hd == io_readreg1 ? regs_13 : _GEN_76; // @[register-file.scala 59:16]
  assign _GEN_78 = 5'he == io_readreg1 ? regs_14 : _GEN_77; // @[register-file.scala 59:16]
  assign _GEN_79 = 5'hf == io_readreg1 ? regs_15 : _GEN_78; // @[register-file.scala 59:16]
  assign _GEN_80 = 5'h10 == io_readreg1 ? regs_16 : _GEN_79; // @[register-file.scala 59:16]
  assign _GEN_81 = 5'h11 == io_readreg1 ? regs_17 : _GEN_80; // @[register-file.scala 59:16]
  assign _GEN_82 = 5'h12 == io_readreg1 ? regs_18 : _GEN_81; // @[register-file.scala 59:16]
  assign _GEN_83 = 5'h13 == io_readreg1 ? regs_19 : _GEN_82; // @[register-file.scala 59:16]
  assign _GEN_84 = 5'h14 == io_readreg1 ? regs_20 : _GEN_83; // @[register-file.scala 59:16]
  assign _GEN_85 = 5'h15 == io_readreg1 ? regs_21 : _GEN_84; // @[register-file.scala 59:16]
  assign _GEN_86 = 5'h16 == io_readreg1 ? regs_22 : _GEN_85; // @[register-file.scala 59:16]
  assign _GEN_87 = 5'h17 == io_readreg1 ? regs_23 : _GEN_86; // @[register-file.scala 59:16]
  assign _GEN_88 = 5'h18 == io_readreg1 ? regs_24 : _GEN_87; // @[register-file.scala 59:16]
  assign _GEN_89 = 5'h19 == io_readreg1 ? regs_25 : _GEN_88; // @[register-file.scala 59:16]
  assign _GEN_90 = 5'h1a == io_readreg1 ? regs_26 : _GEN_89; // @[register-file.scala 59:16]
  assign _GEN_91 = 5'h1b == io_readreg1 ? regs_27 : _GEN_90; // @[register-file.scala 59:16]
  assign _GEN_92 = 5'h1c == io_readreg1 ? regs_28 : _GEN_91; // @[register-file.scala 59:16]
  assign _GEN_93 = 5'h1d == io_readreg1 ? regs_29 : _GEN_92; // @[register-file.scala 59:16]
  assign _GEN_94 = 5'h1e == io_readreg1 ? regs_30 : _GEN_93; // @[register-file.scala 59:16]
  assign _GEN_97 = 5'h1 == io_readreg2 ? regs_1 : regs_0; // @[register-file.scala 60:16]
  assign _GEN_98 = 5'h2 == io_readreg2 ? regs_2 : _GEN_97; // @[register-file.scala 60:16]
  assign _GEN_99 = 5'h3 == io_readreg2 ? regs_3 : _GEN_98; // @[register-file.scala 60:16]
  assign _GEN_100 = 5'h4 == io_readreg2 ? regs_4 : _GEN_99; // @[register-file.scala 60:16]
  assign _GEN_101 = 5'h5 == io_readreg2 ? regs_5 : _GEN_100; // @[register-file.scala 60:16]
  assign _GEN_102 = 5'h6 == io_readreg2 ? regs_6 : _GEN_101; // @[register-file.scala 60:16]
  assign _GEN_103 = 5'h7 == io_readreg2 ? regs_7 : _GEN_102; // @[register-file.scala 60:16]
  assign _GEN_104 = 5'h8 == io_readreg2 ? regs_8 : _GEN_103; // @[register-file.scala 60:16]
  assign _GEN_105 = 5'h9 == io_readreg2 ? regs_9 : _GEN_104; // @[register-file.scala 60:16]
  assign _GEN_106 = 5'ha == io_readreg2 ? regs_10 : _GEN_105; // @[register-file.scala 60:16]
  assign _GEN_107 = 5'hb == io_readreg2 ? regs_11 : _GEN_106; // @[register-file.scala 60:16]
  assign _GEN_108 = 5'hc == io_readreg2 ? regs_12 : _GEN_107; // @[register-file.scala 60:16]
  assign _GEN_109 = 5'hd == io_readreg2 ? regs_13 : _GEN_108; // @[register-file.scala 60:16]
  assign _GEN_110 = 5'he == io_readreg2 ? regs_14 : _GEN_109; // @[register-file.scala 60:16]
  assign _GEN_111 = 5'hf == io_readreg2 ? regs_15 : _GEN_110; // @[register-file.scala 60:16]
  assign _GEN_112 = 5'h10 == io_readreg2 ? regs_16 : _GEN_111; // @[register-file.scala 60:16]
  assign _GEN_113 = 5'h11 == io_readreg2 ? regs_17 : _GEN_112; // @[register-file.scala 60:16]
  assign _GEN_114 = 5'h12 == io_readreg2 ? regs_18 : _GEN_113; // @[register-file.scala 60:16]
  assign _GEN_115 = 5'h13 == io_readreg2 ? regs_19 : _GEN_114; // @[register-file.scala 60:16]
  assign _GEN_116 = 5'h14 == io_readreg2 ? regs_20 : _GEN_115; // @[register-file.scala 60:16]
  assign _GEN_117 = 5'h15 == io_readreg2 ? regs_21 : _GEN_116; // @[register-file.scala 60:16]
  assign _GEN_118 = 5'h16 == io_readreg2 ? regs_22 : _GEN_117; // @[register-file.scala 60:16]
  assign _GEN_119 = 5'h17 == io_readreg2 ? regs_23 : _GEN_118; // @[register-file.scala 60:16]
  assign _GEN_120 = 5'h18 == io_readreg2 ? regs_24 : _GEN_119; // @[register-file.scala 60:16]
  assign _GEN_121 = 5'h19 == io_readreg2 ? regs_25 : _GEN_120; // @[register-file.scala 60:16]
  assign _GEN_122 = 5'h1a == io_readreg2 ? regs_26 : _GEN_121; // @[register-file.scala 60:16]
  assign _GEN_123 = 5'h1b == io_readreg2 ? regs_27 : _GEN_122; // @[register-file.scala 60:16]
  assign _GEN_124 = 5'h1c == io_readreg2 ? regs_28 : _GEN_123; // @[register-file.scala 60:16]
  assign _GEN_125 = 5'h1d == io_readreg2 ? regs_29 : _GEN_124; // @[register-file.scala 60:16]
  assign _GEN_126 = 5'h1e == io_readreg2 ? regs_30 : _GEN_125; // @[register-file.scala 60:16]
  assign io_readdata1 = 5'h1f == io_readreg1 ? regs_31 : _GEN_94; // @[register-file.scala 59:16]
  assign io_readdata2 = 5'h1f == io_readreg2 ? regs_31 : _GEN_126; // @[register-file.scala 60:16]
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
`ifdef RANDOMIZE
  integer initvar;
  initial begin
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
  regs_0 = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  regs_1 = _RAND_1[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  regs_2 = _RAND_2[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  regs_3 = _RAND_3[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  regs_4 = _RAND_4[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  regs_5 = _RAND_5[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  regs_6 = _RAND_6[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_7 = {1{`RANDOM}};
  regs_7 = _RAND_7[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_8 = {1{`RANDOM}};
  regs_8 = _RAND_8[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_9 = {1{`RANDOM}};
  regs_9 = _RAND_9[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_10 = {1{`RANDOM}};
  regs_10 = _RAND_10[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_11 = {1{`RANDOM}};
  regs_11 = _RAND_11[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_12 = {1{`RANDOM}};
  regs_12 = _RAND_12[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_13 = {1{`RANDOM}};
  regs_13 = _RAND_13[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_14 = {1{`RANDOM}};
  regs_14 = _RAND_14[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_15 = {1{`RANDOM}};
  regs_15 = _RAND_15[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_16 = {1{`RANDOM}};
  regs_16 = _RAND_16[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_17 = {1{`RANDOM}};
  regs_17 = _RAND_17[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_18 = {1{`RANDOM}};
  regs_18 = _RAND_18[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_19 = {1{`RANDOM}};
  regs_19 = _RAND_19[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_20 = {1{`RANDOM}};
  regs_20 = _RAND_20[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_21 = {1{`RANDOM}};
  regs_21 = _RAND_21[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_22 = {1{`RANDOM}};
  regs_22 = _RAND_22[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_23 = {1{`RANDOM}};
  regs_23 = _RAND_23[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_24 = {1{`RANDOM}};
  regs_24 = _RAND_24[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_25 = {1{`RANDOM}};
  regs_25 = _RAND_25[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_26 = {1{`RANDOM}};
  regs_26 = _RAND_26[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_27 = {1{`RANDOM}};
  regs_27 = _RAND_27[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_28 = {1{`RANDOM}};
  regs_28 = _RAND_28[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_29 = {1{`RANDOM}};
  regs_29 = _RAND_29[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_30 = {1{`RANDOM}};
  regs_30 = _RAND_30[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_31 = {1{`RANDOM}};
  regs_31 = _RAND_31[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (io_wen) begin
      if (5'h0 == io_writereg) begin
        regs_0 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1 == io_writereg) begin
        regs_1 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h2 == io_writereg) begin
        regs_2 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h3 == io_writereg) begin
        regs_3 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h4 == io_writereg) begin
        regs_4 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h5 == io_writereg) begin
        regs_5 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h6 == io_writereg) begin
        regs_6 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h7 == io_writereg) begin
        regs_7 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h8 == io_writereg) begin
        regs_8 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h9 == io_writereg) begin
        regs_9 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'ha == io_writereg) begin
        regs_10 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'hb == io_writereg) begin
        regs_11 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'hc == io_writereg) begin
        regs_12 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'hd == io_writereg) begin
        regs_13 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'he == io_writereg) begin
        regs_14 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'hf == io_writereg) begin
        regs_15 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h10 == io_writereg) begin
        regs_16 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h11 == io_writereg) begin
        regs_17 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h12 == io_writereg) begin
        regs_18 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h13 == io_writereg) begin
        regs_19 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h14 == io_writereg) begin
        regs_20 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h15 == io_writereg) begin
        regs_21 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h16 == io_writereg) begin
        regs_22 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h17 == io_writereg) begin
        regs_23 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h18 == io_writereg) begin
        regs_24 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h19 == io_writereg) begin
        regs_25 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1a == io_writereg) begin
        regs_26 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1b == io_writereg) begin
        regs_27 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1c == io_writereg) begin
        regs_28 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1d == io_writereg) begin
        regs_29 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1e == io_writereg) begin
        regs_30 <= io_writedata;
      end
    end
    if (io_wen) begin
      if (5'h1f == io_writereg) begin
        regs_31 <= io_writedata;
      end
    end
  end
endmodule
module ALUControl(
  input        io_add,
  input        io_immediate,
  input  [6:0] io_funct7,
  input  [2:0] io_funct3,
  output [3:0] io_operation
);
  wire  _T; // @[Conditional.scala 37:30]
  wire  _T_1; // @[alucontrol.scala 37:41]
  wire  _T_2; // @[alucontrol.scala 37:28]
  wire [1:0] _GEN_0; // @[alucontrol.scala 37:59]
  wire  _T_3; // @[Conditional.scala 37:30]
  wire  _T_4; // @[Conditional.scala 37:30]
  wire  _T_5; // @[Conditional.scala 37:30]
  wire  _T_6; // @[Conditional.scala 37:30]
  wire  _T_7; // @[Conditional.scala 37:30]
  wire [3:0] _GEN_1; // @[alucontrol.scala 48:43]
  wire  _T_9; // @[Conditional.scala 37:30]
  wire  _T_10; // @[Conditional.scala 37:30]
  wire [3:0] _GEN_2; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_3; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_4; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_5; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_6; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_7; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_8; // @[Conditional.scala 39:67]
  wire [3:0] _GEN_9; // @[Conditional.scala 40:58]
  assign _T = 3'h0 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_1 = io_funct7 == 7'h0; // @[alucontrol.scala 37:41]
  assign _T_2 = io_immediate | _T_1; // @[alucontrol.scala 37:28]
  assign _GEN_0 = _T_2 ? 2'h2 : 2'h3; // @[alucontrol.scala 37:59]
  assign _T_3 = 3'h1 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_4 = 3'h2 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_5 = 3'h3 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_6 = 3'h4 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_7 = 3'h5 == io_funct3; // @[Conditional.scala 37:30]
  assign _GEN_1 = _T_1 ? 4'h7 : 4'h8; // @[alucontrol.scala 48:43]
  assign _T_9 = 3'h6 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_10 = 3'h7 == io_funct3; // @[Conditional.scala 37:30]
  assign _GEN_2 = _T_10 ? 4'h0 : 4'hf; // @[Conditional.scala 39:67]
  assign _GEN_3 = _T_9 ? 4'h1 : _GEN_2; // @[Conditional.scala 39:67]
  assign _GEN_4 = _T_7 ? _GEN_1 : _GEN_3; // @[Conditional.scala 39:67]
  assign _GEN_5 = _T_6 ? 4'h9 : _GEN_4; // @[Conditional.scala 39:67]
  assign _GEN_6 = _T_5 ? 4'h5 : _GEN_5; // @[Conditional.scala 39:67]
  assign _GEN_7 = _T_4 ? 4'h4 : _GEN_6; // @[Conditional.scala 39:67]
  assign _GEN_8 = _T_3 ? 4'h6 : _GEN_7; // @[Conditional.scala 39:67]
  assign _GEN_9 = _T ? {{2'd0}, _GEN_0} : _GEN_8; // @[Conditional.scala 40:58]
  assign io_operation = io_add ? 4'h2 : _GEN_9; // @[alucontrol.scala 30:16 alucontrol.scala 33:18 alucontrol.scala 38:24 alucontrol.scala 40:24 alucontrol.scala 43:36 alucontrol.scala 44:36 alucontrol.scala 45:36 alucontrol.scala 46:36 alucontrol.scala 49:24 alucontrol.scala 51:24 alucontrol.scala 54:36 alucontrol.scala 55:36]
endmodule
module ALU(
  input  [3:0]  io_operation,
  input  [31:0] io_inputx,
  input  [31:0] io_inputy,
  output [31:0] io_result
);
  wire  _T; // @[Conditional.scala 37:30]
  wire [31:0] _T_1; // @[alu.scala 30:30]
  wire  _T_2; // @[Conditional.scala 37:30]
  wire [31:0] _T_3; // @[alu.scala 33:30]
  wire  _T_4; // @[Conditional.scala 37:30]
  wire [31:0] _T_6; // @[alu.scala 36:30]
  wire  _T_7; // @[Conditional.scala 37:30]
  wire [31:0] _T_9; // @[alu.scala 39:30]
  wire  _T_10; // @[Conditional.scala 37:30]
  wire [31:0] _T_11; // @[alu.scala 42:31]
  wire [31:0] _T_12; // @[alu.scala 42:50]
  wire  _T_13; // @[alu.scala 42:38]
  wire  _T_14; // @[Conditional.scala 37:30]
  wire  _T_15; // @[alu.scala 45:31]
  wire  _T_16; // @[Conditional.scala 37:30]
  wire [4:0] _T_17; // @[alu.scala 48:42]
  wire [62:0] _GEN_11; // @[alu.scala 48:30]
  wire [62:0] _T_18; // @[alu.scala 48:30]
  wire  _T_19; // @[Conditional.scala 37:30]
  wire [31:0] _T_21; // @[alu.scala 51:30]
  wire  _T_22; // @[Conditional.scala 37:30]
  wire [31:0] _T_25; // @[alu.scala 54:38]
  wire [31:0] _T_26; // @[alu.scala 54:57]
  wire  _T_27; // @[Conditional.scala 37:30]
  wire [31:0] _T_28; // @[alu.scala 57:30]
  wire  _T_29; // @[Conditional.scala 37:30]
  wire [31:0] _T_31; // @[alu.scala 60:20]
  wire [31:0] _GEN_0; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_1; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_2; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_3; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_4; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_5; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_6; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_7; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_8; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_9; // @[Conditional.scala 39:67]
  wire [62:0] _GEN_10; // @[Conditional.scala 40:58]
  assign _T = 4'h0 == io_operation; // @[Conditional.scala 37:30]
  assign _T_1 = io_inputx & io_inputy; // @[alu.scala 30:30]
  assign _T_2 = 4'h1 == io_operation; // @[Conditional.scala 37:30]
  assign _T_3 = io_inputx | io_inputy; // @[alu.scala 33:30]
  assign _T_4 = 4'h2 == io_operation; // @[Conditional.scala 37:30]
  assign _T_6 = io_inputx + io_inputy; // @[alu.scala 36:30]
  assign _T_7 = 4'h3 == io_operation; // @[Conditional.scala 37:30]
  assign _T_9 = io_inputx - io_inputy; // @[alu.scala 39:30]
  assign _T_10 = 4'h4 == io_operation; // @[Conditional.scala 37:30]
  assign _T_11 = $signed(io_inputx); // @[alu.scala 42:31]
  assign _T_12 = $signed(io_inputy); // @[alu.scala 42:50]
  assign _T_13 = $signed(_T_11) < $signed(_T_12); // @[alu.scala 42:38]
  assign _T_14 = 4'h5 == io_operation; // @[Conditional.scala 37:30]
  assign _T_15 = io_inputx < io_inputy; // @[alu.scala 45:31]
  assign _T_16 = 4'h6 == io_operation; // @[Conditional.scala 37:30]
  assign _T_17 = io_inputy[4:0]; // @[alu.scala 48:42]
  assign _GEN_11 = {{31'd0}, io_inputx}; // @[alu.scala 48:30]
  assign _T_18 = _GEN_11 << _T_17; // @[alu.scala 48:30]
  assign _T_19 = 4'h7 == io_operation; // @[Conditional.scala 37:30]
  assign _T_21 = io_inputx >> _T_17; // @[alu.scala 51:30]
  assign _T_22 = 4'h8 == io_operation; // @[Conditional.scala 37:30]
  assign _T_25 = $signed(_T_11) >>> _T_17; // @[alu.scala 54:38]
  assign _T_26 = $unsigned(_T_25); // @[alu.scala 54:57]
  assign _T_27 = 4'h9 == io_operation; // @[Conditional.scala 37:30]
  assign _T_28 = io_inputx ^ io_inputy; // @[alu.scala 57:30]
  assign _T_29 = 4'ha == io_operation; // @[Conditional.scala 37:30]
  assign _T_31 = ~ _T_3; // @[alu.scala 60:20]
  assign _GEN_0 = _T_29 ? _T_31 : 32'h0; // @[Conditional.scala 39:67]
  assign _GEN_1 = _T_27 ? _T_28 : _GEN_0; // @[Conditional.scala 39:67]
  assign _GEN_2 = _T_22 ? _T_26 : _GEN_1; // @[Conditional.scala 39:67]
  assign _GEN_3 = _T_19 ? _T_21 : _GEN_2; // @[Conditional.scala 39:67]
  assign _GEN_4 = _T_16 ? _T_18 : {{31'd0}, _GEN_3}; // @[Conditional.scala 39:67]
  assign _GEN_5 = _T_14 ? {{62'd0}, _T_15} : _GEN_4; // @[Conditional.scala 39:67]
  assign _GEN_6 = _T_10 ? {{62'd0}, _T_13} : _GEN_5; // @[Conditional.scala 39:67]
  assign _GEN_7 = _T_7 ? {{31'd0}, _T_9} : _GEN_6; // @[Conditional.scala 39:67]
  assign _GEN_8 = _T_4 ? {{31'd0}, _T_6} : _GEN_7; // @[Conditional.scala 39:67]
  assign _GEN_9 = _T_2 ? {{31'd0}, _T_3} : _GEN_8; // @[Conditional.scala 39:67]
  assign _GEN_10 = _T ? {{31'd0}, _T_1} : _GEN_9; // @[Conditional.scala 40:58]
  assign io_result = _GEN_10[31:0]; // @[alu.scala 26:13 alu.scala 30:17 alu.scala 33:17 alu.scala 36:17 alu.scala 39:17 alu.scala 42:17 alu.scala 45:17 alu.scala 48:17 alu.scala 51:17 alu.scala 54:17 alu.scala 57:17 alu.scala 60:17]
endmodule
module ImmediateGenerator(
  input  [31:0] io_instruction,
  output [31:0] io_sextImm
);
  wire [6:0] opcode; // @[helpers.scala 44:30]
  wire  _T; // @[Conditional.scala 37:30]
  wire [19:0] _T_1; // @[helpers.scala 47:31]
  wire [31:0] _T_3; // @[Cat.scala 30:58]
  wire  _T_4; // @[Conditional.scala 37:30]
  wire  _T_8; // @[Conditional.scala 37:30]
  wire  _T_9; // @[helpers.scala 55:35]
  wire [7:0] _T_10; // @[helpers.scala 55:55]
  wire  _T_11; // @[helpers.scala 56:35]
  wire [9:0] _T_12; // @[helpers.scala 56:55]
  wire [19:0] _T_15; // @[Cat.scala 30:58]
  wire  _T_16; // @[helpers.scala 57:36]
  wire [10:0] _T_18; // @[Bitwise.scala 72:12]
  wire [31:0] _T_20; // @[Cat.scala 30:58]
  wire  _T_21; // @[Conditional.scala 37:30]
  wire [11:0] _T_22; // @[helpers.scala 60:31]
  wire  _T_23; // @[helpers.scala 61:36]
  wire [19:0] _T_25; // @[Bitwise.scala 72:12]
  wire [31:0] _T_26; // @[Cat.scala 30:58]
  wire  _T_27; // @[Conditional.scala 37:30]
  wire  _T_29; // @[helpers.scala 64:55]
  wire [5:0] _T_30; // @[helpers.scala 65:35]
  wire [3:0] _T_31; // @[helpers.scala 65:58]
  wire [11:0] _T_34; // @[Cat.scala 30:58]
  wire  _T_35; // @[helpers.scala 66:37]
  wire [18:0] _T_37; // @[Bitwise.scala 72:12]
  wire [31:0] _T_39; // @[Cat.scala 30:58]
  wire  _T_40; // @[Conditional.scala 37:30]
  wire  _T_46; // @[Conditional.scala 37:30]
  wire [6:0] _T_47; // @[helpers.scala 73:35]
  wire [4:0] _T_48; // @[helpers.scala 73:59]
  wire [11:0] _T_49; // @[Cat.scala 30:58]
  wire  _T_50; // @[helpers.scala 74:36]
  wire [19:0] _T_52; // @[Bitwise.scala 72:12]
  wire [31:0] _T_53; // @[Cat.scala 30:58]
  wire  _T_54; // @[Conditional.scala 37:30]
  wire  _T_60; // @[Conditional.scala 37:30]
  wire [4:0] _T_62; // @[helpers.scala 81:53]
  wire [31:0] _T_63; // @[Cat.scala 30:58]
  wire [31:0] _GEN_0; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_1; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_2; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_3; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_4; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_5; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_6; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_7; // @[Conditional.scala 39:67]
  assign opcode = io_instruction[6:0]; // @[helpers.scala 44:30]
  assign _T = 7'h37 == opcode; // @[Conditional.scala 37:30]
  assign _T_1 = io_instruction[31:12]; // @[helpers.scala 47:31]
  assign _T_3 = {_T_1,12'h0}; // @[Cat.scala 30:58]
  assign _T_4 = 7'h17 == opcode; // @[Conditional.scala 37:30]
  assign _T_8 = 7'h6f == opcode; // @[Conditional.scala 37:30]
  assign _T_9 = io_instruction[31]; // @[helpers.scala 55:35]
  assign _T_10 = io_instruction[19:12]; // @[helpers.scala 55:55]
  assign _T_11 = io_instruction[20]; // @[helpers.scala 56:35]
  assign _T_12 = io_instruction[30:21]; // @[helpers.scala 56:55]
  assign _T_15 = {_T_9,_T_10,_T_11,_T_12}; // @[Cat.scala 30:58]
  assign _T_16 = _T_15[19]; // @[helpers.scala 57:36]
  assign _T_18 = _T_16 ? 11'h7ff : 11'h0; // @[Bitwise.scala 72:12]
  assign _T_20 = {_T_18,_T_9,_T_10,_T_11,_T_12,1'h0}; // @[Cat.scala 30:58]
  assign _T_21 = 7'h67 == opcode; // @[Conditional.scala 37:30]
  assign _T_22 = io_instruction[31:20]; // @[helpers.scala 60:31]
  assign _T_23 = _T_22[11]; // @[helpers.scala 61:36]
  assign _T_25 = _T_23 ? 20'hfffff : 20'h0; // @[Bitwise.scala 72:12]
  assign _T_26 = {_T_25,_T_22}; // @[Cat.scala 30:58]
  assign _T_27 = 7'h63 == opcode; // @[Conditional.scala 37:30]
  assign _T_29 = io_instruction[7]; // @[helpers.scala 64:55]
  assign _T_30 = io_instruction[30:25]; // @[helpers.scala 65:35]
  assign _T_31 = io_instruction[11:8]; // @[helpers.scala 65:58]
  assign _T_34 = {_T_9,_T_29,_T_30,_T_31}; // @[Cat.scala 30:58]
  assign _T_35 = _T_34[11]; // @[helpers.scala 66:37]
  assign _T_37 = _T_35 ? 19'h7ffff : 19'h0; // @[Bitwise.scala 72:12]
  assign _T_39 = {_T_37,_T_9,_T_29,_T_30,_T_31,1'h0}; // @[Cat.scala 30:58]
  assign _T_40 = 7'h3 == opcode; // @[Conditional.scala 37:30]
  assign _T_46 = 7'h23 == opcode; // @[Conditional.scala 37:30]
  assign _T_47 = io_instruction[31:25]; // @[helpers.scala 73:35]
  assign _T_48 = io_instruction[11:7]; // @[helpers.scala 73:59]
  assign _T_49 = {_T_47,_T_48}; // @[Cat.scala 30:58]
  assign _T_50 = _T_49[11]; // @[helpers.scala 74:36]
  assign _T_52 = _T_50 ? 20'hfffff : 20'h0; // @[Bitwise.scala 72:12]
  assign _T_53 = {_T_52,_T_47,_T_48}; // @[Cat.scala 30:58]
  assign _T_54 = 7'h13 == opcode; // @[Conditional.scala 37:30]
  assign _T_60 = 7'h73 == opcode; // @[Conditional.scala 37:30]
  assign _T_62 = io_instruction[19:15]; // @[helpers.scala 81:53]
  assign _T_63 = {27'h0,_T_62}; // @[Cat.scala 30:58]
  assign _GEN_0 = _T_60 ? _T_63 : 32'h0; // @[Conditional.scala 39:67]
  assign _GEN_1 = _T_54 ? _T_26 : _GEN_0; // @[Conditional.scala 39:67]
  assign _GEN_2 = _T_46 ? _T_53 : _GEN_1; // @[Conditional.scala 39:67]
  assign _GEN_3 = _T_40 ? _T_26 : _GEN_2; // @[Conditional.scala 39:67]
  assign _GEN_4 = _T_27 ? _T_39 : _GEN_3; // @[Conditional.scala 39:67]
  assign _GEN_5 = _T_21 ? _T_26 : _GEN_4; // @[Conditional.scala 39:67]
  assign _GEN_6 = _T_8 ? _T_20 : _GEN_5; // @[Conditional.scala 39:67]
  assign _GEN_7 = _T_4 ? _T_3 : _GEN_6; // @[Conditional.scala 39:67]
  assign io_sextImm = _T ? _T_3 : _GEN_7; // @[helpers.scala 42:14 helpers.scala 48:18 helpers.scala 52:18 helpers.scala 57:18 helpers.scala 61:18 helpers.scala 66:18 helpers.scala 70:18 helpers.scala 74:18 helpers.scala 78:18 helpers.scala 81:18]
endmodule
module BranchControl(
  input         io_branch,
  input  [2:0]  io_funct3,
  input  [31:0] io_inputx,
  input  [31:0] io_inputy,
  output        io_taken
);
  wire  _T; // @[Conditional.scala 37:30]
  wire  _T_1; // @[branch-control.scala 33:40]
  wire  _T_2; // @[Conditional.scala 37:30]
  wire  _T_3; // @[branch-control.scala 34:40]
  wire  _T_4; // @[Conditional.scala 37:30]
  wire [31:0] _T_5; // @[branch-control.scala 35:40]
  wire [31:0] _T_6; // @[branch-control.scala 35:59]
  wire  _T_7; // @[branch-control.scala 35:47]
  wire  _T_8; // @[Conditional.scala 37:30]
  wire  _T_11; // @[branch-control.scala 36:47]
  wire  _T_12; // @[Conditional.scala 37:30]
  wire  _T_13; // @[branch-control.scala 37:40]
  wire  _T_15; // @[branch-control.scala 38:40]
  wire  _GEN_1; // @[Conditional.scala 39:67]
  wire  _GEN_2; // @[Conditional.scala 39:67]
  wire  _GEN_3; // @[Conditional.scala 39:67]
  wire  _GEN_4; // @[Conditional.scala 39:67]
  wire  check; // @[Conditional.scala 40:58]
  assign _T = 3'h0 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_1 = io_inputx == io_inputy; // @[branch-control.scala 33:40]
  assign _T_2 = 3'h1 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_3 = io_inputx != io_inputy; // @[branch-control.scala 34:40]
  assign _T_4 = 3'h4 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_5 = $signed(io_inputx); // @[branch-control.scala 35:40]
  assign _T_6 = $signed(io_inputy); // @[branch-control.scala 35:59]
  assign _T_7 = $signed(_T_5) < $signed(_T_6); // @[branch-control.scala 35:47]
  assign _T_8 = 3'h5 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_11 = $signed(_T_5) >= $signed(_T_6); // @[branch-control.scala 36:47]
  assign _T_12 = 3'h6 == io_funct3; // @[Conditional.scala 37:30]
  assign _T_13 = io_inputx < io_inputy; // @[branch-control.scala 37:40]
  assign _T_15 = io_inputx >= io_inputy; // @[branch-control.scala 38:40]
  assign _GEN_1 = _T_12 ? _T_13 : _T_15; // @[Conditional.scala 39:67]
  assign _GEN_2 = _T_8 ? _T_11 : _GEN_1; // @[Conditional.scala 39:67]
  assign _GEN_3 = _T_4 ? _T_7 : _GEN_2; // @[Conditional.scala 39:67]
  assign _GEN_4 = _T_2 ? _T_3 : _GEN_3; // @[Conditional.scala 39:67]
  assign check = _T ? _T_1 : _GEN_4; // @[Conditional.scala 40:58]
  assign io_taken = check & io_branch; // @[branch-control.scala 41:12]
endmodule
module Adder(
  input  [31:0] io_inputx,
  input  [31:0] io_inputy,
  output [31:0] io_result
);
  assign io_result = io_inputx + io_inputy; // @[helpers.scala 23:13]
endmodule
module SingleCycleCPU(
  input         clock,
  input         reset,
  output [31:0] io_imem_address,
  input  [31:0] io_imem_instruction,
  output [31:0] io_dmem_address,
  output        io_dmem_memread,
  output        io_dmem_memwrite,
  output [1:0]  io_dmem_maskmode,
  output        io_dmem_sext,
  input  [31:0] io_dmem_readdata
);
  wire [6:0] control_io_opcode; // @[cpu.scala 20:26]
  wire  control_io_branch; // @[cpu.scala 20:26]
  wire  control_io_memread; // @[cpu.scala 20:26]
  wire [1:0] control_io_toreg; // @[cpu.scala 20:26]
  wire  control_io_add; // @[cpu.scala 20:26]
  wire  control_io_memwrite; // @[cpu.scala 20:26]
  wire  control_io_regwrite; // @[cpu.scala 20:26]
  wire  control_io_immediate; // @[cpu.scala 20:26]
  wire [1:0] control_io_alusrc1; // @[cpu.scala 20:26]
  wire [1:0] control_io_jump; // @[cpu.scala 20:26]
  wire  registers_clock; // @[cpu.scala 21:26]
  wire [4:0] registers_io_readreg1; // @[cpu.scala 21:26]
  wire [4:0] registers_io_readreg2; // @[cpu.scala 21:26]
  wire [4:0] registers_io_writereg; // @[cpu.scala 21:26]
  wire [31:0] registers_io_writedata; // @[cpu.scala 21:26]
  wire  registers_io_wen; // @[cpu.scala 21:26]
  wire [31:0] registers_io_readdata1; // @[cpu.scala 21:26]
  wire [31:0] registers_io_readdata2; // @[cpu.scala 21:26]
  wire  aluControl_io_add; // @[cpu.scala 22:26]
  wire  aluControl_io_immediate; // @[cpu.scala 22:26]
  wire [6:0] aluControl_io_funct7; // @[cpu.scala 22:26]
  wire [2:0] aluControl_io_funct3; // @[cpu.scala 22:26]
  wire [3:0] aluControl_io_operation; // @[cpu.scala 22:26]
  wire [3:0] alu_io_operation; // @[cpu.scala 23:26]
  wire [31:0] alu_io_inputx; // @[cpu.scala 23:26]
  wire [31:0] alu_io_inputy; // @[cpu.scala 23:26]
  wire [31:0] alu_io_result; // @[cpu.scala 23:26]
  wire [31:0] immGen_io_instruction; // @[cpu.scala 24:26]
  wire [31:0] immGen_io_sextImm; // @[cpu.scala 24:26]
  wire  branchCtrl_io_branch; // @[cpu.scala 25:26]
  wire [2:0] branchCtrl_io_funct3; // @[cpu.scala 25:26]
  wire [31:0] branchCtrl_io_inputx; // @[cpu.scala 25:26]
  wire [31:0] branchCtrl_io_inputy; // @[cpu.scala 25:26]
  wire  branchCtrl_io_taken; // @[cpu.scala 25:26]
  wire [31:0] pcPlusFour_io_inputx; // @[cpu.scala 26:26]
  wire [31:0] pcPlusFour_io_inputy; // @[cpu.scala 26:26]
  wire [31:0] pcPlusFour_io_result; // @[cpu.scala 26:26]
  wire [31:0] branchAdd_io_inputx; // @[cpu.scala 27:26]
  wire [31:0] branchAdd_io_inputy; // @[cpu.scala 27:26]
  wire [31:0] branchAdd_io_result; // @[cpu.scala 27:26]
  reg [31:0] pc; // @[cpu.scala 19:27]
  reg [31:0] _RAND_0;
  reg [29:0] value; // @[Counter.scala 26:33]
  reg [31:0] _RAND_1;
  wire [29:0] _T_2; // @[Counter.scala 35:22]
  wire  _T_7; // @[cpu.scala 44:74]
  wire  _T_12; // @[Conditional.scala 37:30]
  wire  _T_13; // @[Conditional.scala 37:30]
  wire [31:0] _GEN_2; // @[Conditional.scala 39:67]
  wire  _T_16; // @[cpu.scala 76:36]
  wire  _T_18; // @[cpu.scala 79:26]
  wire  _T_19; // @[cpu.scala 81:33]
  wire [31:0] _GEN_4; // @[cpu.scala 81:42]
  wire  _T_20; // @[cpu.scala 92:48]
  wire  _T_21; // @[cpu.scala 92:29]
  wire  _T_22; // @[cpu.scala 94:32]
  wire [31:0] _T_25; // @[cpu.scala 95:30]
  wire  _T_27; // @[cpu.scala 112:9]
  Control control ( // @[cpu.scala 20:26]
    .io_opcode(control_io_opcode),
    .io_branch(control_io_branch),
    .io_memread(control_io_memread),
    .io_toreg(control_io_toreg),
    .io_add(control_io_add),
    .io_memwrite(control_io_memwrite),
    .io_regwrite(control_io_regwrite),
    .io_immediate(control_io_immediate),
    .io_alusrc1(control_io_alusrc1),
    .io_jump(control_io_jump)
  );
  RegisterFile registers ( // @[cpu.scala 21:26]
    .clock(registers_clock),
    .io_readreg1(registers_io_readreg1),
    .io_readreg2(registers_io_readreg2),
    .io_writereg(registers_io_writereg),
    .io_writedata(registers_io_writedata),
    .io_wen(registers_io_wen),
    .io_readdata1(registers_io_readdata1),
    .io_readdata2(registers_io_readdata2)
  );
  ALUControl aluControl ( // @[cpu.scala 22:26]
    .io_add(aluControl_io_add),
    .io_immediate(aluControl_io_immediate),
    .io_funct7(aluControl_io_funct7),
    .io_funct3(aluControl_io_funct3),
    .io_operation(aluControl_io_operation)
  );
  ALU alu ( // @[cpu.scala 23:26]
    .io_operation(alu_io_operation),
    .io_inputx(alu_io_inputx),
    .io_inputy(alu_io_inputy),
    .io_result(alu_io_result)
  );
  ImmediateGenerator immGen ( // @[cpu.scala 24:26]
    .io_instruction(immGen_io_instruction),
    .io_sextImm(immGen_io_sextImm)
  );
  BranchControl branchCtrl ( // @[cpu.scala 25:26]
    .io_branch(branchCtrl_io_branch),
    .io_funct3(branchCtrl_io_funct3),
    .io_inputx(branchCtrl_io_inputx),
    .io_inputy(branchCtrl_io_inputy),
    .io_taken(branchCtrl_io_taken)
  );
  Adder pcPlusFour ( // @[cpu.scala 26:26]
    .io_inputx(pcPlusFour_io_inputx),
    .io_inputy(pcPlusFour_io_inputy),
    .io_result(pcPlusFour_io_result)
  );
  Adder branchAdd ( // @[cpu.scala 27:26]
    .io_inputx(branchAdd_io_inputx),
    .io_inputy(branchAdd_io_inputy),
    .io_result(branchAdd_io_result)
  );
  assign _T_2 = value + 30'h1; // @[Counter.scala 35:22]
  assign _T_7 = registers_io_writereg != 5'h0; // @[cpu.scala 44:74]
  assign _T_12 = 2'h0 == control_io_alusrc1; // @[Conditional.scala 37:30]
  assign _T_13 = 2'h1 == control_io_alusrc1; // @[Conditional.scala 37:30]
  assign _GEN_2 = _T_13 ? 32'h0 : pc; // @[Conditional.scala 39:67]
  assign _T_16 = io_imem_instruction[14]; // @[cpu.scala 76:36]
  assign _T_18 = control_io_toreg == 2'h1; // @[cpu.scala 79:26]
  assign _T_19 = control_io_toreg == 2'h2; // @[cpu.scala 81:33]
  assign _GEN_4 = _T_19 ? pcPlusFour_io_result : alu_io_result; // @[cpu.scala 81:42]
  assign _T_20 = control_io_jump == 2'h2; // @[cpu.scala 92:48]
  assign _T_21 = branchCtrl_io_taken | _T_20; // @[cpu.scala 92:29]
  assign _T_22 = control_io_jump == 2'h3; // @[cpu.scala 94:32]
  assign _T_25 = alu_io_result & 32'hfffffffe; // @[cpu.scala 95:30]
  assign _T_27 = reset == 1'h0; // @[cpu.scala 112:9]
  assign io_imem_address = pc; // @[cpu.scala 30:19]
  assign io_dmem_address = alu_io_result; // @[cpu.scala 71:21]
  assign io_dmem_memread = control_io_memread; // @[cpu.scala 73:21]
  assign io_dmem_memwrite = control_io_memwrite; // @[cpu.scala 74:21]
  assign io_dmem_maskmode = io_imem_instruction[13:12]; // @[cpu.scala 75:21]
  assign io_dmem_sext = ~ _T_16; // @[cpu.scala 76:21]
  assign control_io_opcode = io_imem_instruction[6:0]; // @[cpu.scala 38:21]
  assign registers_clock = clock;
  assign registers_io_readreg1 = io_imem_instruction[19:15]; // @[cpu.scala 40:25]
  assign registers_io_readreg2 = io_imem_instruction[24:20]; // @[cpu.scala 41:25]
  assign registers_io_writereg = io_imem_instruction[11:7]; // @[cpu.scala 43:25]
  assign registers_io_writedata = _T_18 ? io_dmem_readdata : _GEN_4; // @[cpu.scala 87:26]
  assign registers_io_wen = control_io_regwrite & _T_7; // @[cpu.scala 44:25]
  assign aluControl_io_add = control_io_add; // @[cpu.scala 46:27]
  assign aluControl_io_immediate = control_io_immediate; // @[cpu.scala 47:27]
  assign aluControl_io_funct7 = io_imem_instruction[31:25]; // @[cpu.scala 48:27]
  assign aluControl_io_funct3 = io_imem_instruction[14:12]; // @[cpu.scala 49:27]
  assign alu_io_operation = aluControl_io_operation; // @[cpu.scala 69:20]
  assign alu_io_inputx = _T_12 ? registers_io_readdata1 : _GEN_2; // @[cpu.scala 67:17]
  assign alu_io_inputy = control_io_immediate ? immGen_io_sextImm : registers_io_readdata2; // @[cpu.scala 68:17]
  assign immGen_io_instruction = io_imem_instruction; // @[cpu.scala 51:25]
  assign branchCtrl_io_branch = control_io_branch; // @[cpu.scala 54:24]
  assign branchCtrl_io_funct3 = io_imem_instruction[14:12]; // @[cpu.scala 55:24]
  assign branchCtrl_io_inputx = registers_io_readdata1; // @[cpu.scala 56:24]
  assign branchCtrl_io_inputy = registers_io_readdata2; // @[cpu.scala 57:24]
  assign pcPlusFour_io_inputx = pc; // @[cpu.scala 32:24]
  assign pcPlusFour_io_inputy = 32'h4; // @[cpu.scala 33:24]
  assign branchAdd_io_inputx = pc; // @[cpu.scala 89:23]
  assign branchAdd_io_inputy = immGen_io_sextImm; // @[cpu.scala 90:23]
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
`ifdef RANDOMIZE
  integer initvar;
  initial begin
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
  pc = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  value = _RAND_1[29:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      pc <= 32'h0;
    end else begin
      if (_T_21) begin
        pc <= branchAdd_io_result;
      end else begin
        if (_T_22) begin
          pc <= _T_25;
        end else begin
          pc <= pcPlusFour_io_result;
        end
      end
    end
    if (reset) begin
      value <= 30'h0;
    end else begin
      value <= _T_2;
    end
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"DASM(%x)\n",io_imem_instruction); // @[cpu.scala 112:9]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"CYCLE=%d\n",value); // @[cpu.scala 113:9]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"pc: %d\n",pc); // @[cpu.scala 114:9]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"control: Bundle(opcode -> %d, branch -> %d, memread -> %d, toreg -> %d, add -> %d, memwrite -> %d, regwrite -> %d, immediate -> %d, alusrc1 -> %d, jump -> %d)\n",control_io_opcode,control_io_branch,control_io_memread,control_io_toreg,control_io_add,control_io_memwrite,control_io_regwrite,control_io_immediate,control_io_alusrc1,control_io_jump); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"registers: Bundle(readreg1 -> %d, readreg2 -> %d, writereg -> %d, writedata -> %d, wen -> %d, readdata1 -> %d, readdata2 -> %d)\n",registers_io_readreg1,registers_io_readreg2,registers_io_writereg,registers_io_writedata,registers_io_wen,registers_io_readdata1,registers_io_readdata2); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"aluControl: Bundle(add -> %d, immediate -> %d, funct7 -> %d, funct3 -> %d, operation -> %d)\n",aluControl_io_add,aluControl_io_immediate,aluControl_io_funct7,aluControl_io_funct3,aluControl_io_operation); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"alu: Bundle(operation -> %d, inputx -> %d, inputy -> %d, result -> %d)\n",alu_io_operation,alu_io_inputx,alu_io_inputy,alu_io_result); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"immGen: Bundle(instruction -> %d, sextImm -> %d)\n",immGen_io_instruction,immGen_io_sextImm); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"branchCtrl: Bundle(branch -> %d, funct3 -> %d, inputx -> %d, inputy -> %d, taken -> %d)\n",branchCtrl_io_branch,branchCtrl_io_funct3,branchCtrl_io_inputx,branchCtrl_io_inputy,branchCtrl_io_taken); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"pcPlusFour: Bundle(inputx -> %d, inputy -> %d, result -> %d)\n",pcPlusFour_io_inputx,pcPlusFour_io_inputy,pcPlusFour_io_result); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"branchAdd: Bundle(inputx -> %d, inputy -> %d, result -> %d)\n",branchAdd_io_inputx,branchAdd_io_inputy,branchAdd_io_result); // @[cpu.scala 116:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_27) begin
          $fwrite(32'h80000002,"\n"); // @[cpu.scala 118:9]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
  end
endmodule
module DualPortedMemory(
  input         clock,
  input  [31:0] io_imem_address,
  output [31:0] io_imem_instruction,
  input  [31:0] io_dmem_address,
  input         io_dmem_memread,
  input         io_dmem_memwrite,
  input  [1:0]  io_dmem_maskmode,
  input         io_dmem_sext,
  output [31:0] io_dmem_readdata
);
  wire  memory_clk; // @[memory.scala 69:22]
  wire [31:0] memory_imem_address; // @[memory.scala 69:22]
  wire [31:0] memory_imem_instruction; // @[memory.scala 69:22]
  wire [31:0] memory_dmem_address; // @[memory.scala 69:22]
  wire [31:0] memory_dmem_writedata; // @[memory.scala 69:22]
  wire  memory_dmem_memread; // @[memory.scala 69:22]
  wire  memory_dmem_memwrite; // @[memory.scala 69:22]
  wire [1:0] memory_dmem_maskmode; // @[memory.scala 69:22]
  wire  memory_dmem_sext; // @[memory.scala 69:22]
  wire [31:0] memory_dmem_readdata; // @[memory.scala 69:22]
  wire  _T; // @[memory.scala 88:30]
  wire  _T_1; // @[memory.scala 89:65]
  wire [23:0] _T_3; // @[Bitwise.scala 72:12]
  wire [7:0] _T_4; // @[memory.scala 89:94]
  wire [31:0] _T_5; // @[Cat.scala 30:58]
  wire  _T_6; // @[memory.scala 91:65]
  wire [15:0] _T_8; // @[Bitwise.scala 72:12]
  wire [15:0] _T_9; // @[memory.scala 91:95]
  wire [31:0] _T_10; // @[Cat.scala 30:58]
  wire [31:0] _GEN_0; // @[memory.scala 88:39]
  DualPortedMemoryBlackBox memory ( // @[memory.scala 69:22]
    .clk(memory_clk),
    .imem_address(memory_imem_address),
    .imem_instruction(memory_imem_instruction),
    .dmem_address(memory_dmem_address),
    .dmem_writedata(memory_dmem_writedata),
    .dmem_memread(memory_dmem_memread),
    .dmem_memwrite(memory_dmem_memwrite),
    .dmem_maskmode(memory_dmem_maskmode),
    .dmem_sext(memory_dmem_sext),
    .dmem_readdata(memory_dmem_readdata)
  );
  assign _T = io_dmem_maskmode == 2'h0; // @[memory.scala 88:30]
  assign _T_1 = memory_dmem_readdata[7]; // @[memory.scala 89:65]
  assign _T_3 = _T_1 ? 24'hffffff : 24'h0; // @[Bitwise.scala 72:12]
  assign _T_4 = memory_dmem_readdata[7:0]; // @[memory.scala 89:94]
  assign _T_5 = {_T_3,_T_4}; // @[Cat.scala 30:58]
  assign _T_6 = memory_dmem_readdata[15]; // @[memory.scala 91:65]
  assign _T_8 = _T_6 ? 16'hffff : 16'h0; // @[Bitwise.scala 72:12]
  assign _T_9 = memory_dmem_readdata[15:0]; // @[memory.scala 91:95]
  assign _T_10 = {_T_8,_T_9}; // @[Cat.scala 30:58]
  assign _GEN_0 = _T ? _T_5 : _T_10; // @[memory.scala 88:39]
  assign io_imem_instruction = memory_imem_instruction; // @[memory.scala 73:23]
  assign io_dmem_readdata = io_dmem_sext ? _GEN_0 : memory_dmem_readdata; // @[memory.scala 89:26 memory.scala 91:26 memory.scala 94:24]
  assign memory_clk = clock; // @[memory.scala 85:17]
  assign memory_imem_address = io_imem_address; // @[memory.scala 75:26]
  assign memory_dmem_address = io_dmem_address; // @[memory.scala 79:26]
  assign memory_dmem_writedata = 32'h0;
  assign memory_dmem_memread = io_dmem_memread; // @[memory.scala 81:26]
  assign memory_dmem_memwrite = io_dmem_memwrite; // @[memory.scala 82:27]
  assign memory_dmem_maskmode = io_dmem_maskmode; // @[memory.scala 83:27]
  assign memory_dmem_sext = io_dmem_sext; // @[memory.scala 84:23]
endmodule
module Top(
  input   clock,
  input   reset,
  output  io_success
);
  wire  cpu_clock; // @[top.scala 14:21]
  wire  cpu_reset; // @[top.scala 14:21]
  wire [31:0] cpu_io_imem_address; // @[top.scala 14:21]
  wire [31:0] cpu_io_imem_instruction; // @[top.scala 14:21]
  wire [31:0] cpu_io_dmem_address; // @[top.scala 14:21]
  wire  cpu_io_dmem_memread; // @[top.scala 14:21]
  wire  cpu_io_dmem_memwrite; // @[top.scala 14:21]
  wire [1:0] cpu_io_dmem_maskmode; // @[top.scala 14:21]
  wire  cpu_io_dmem_sext; // @[top.scala 14:21]
  wire [31:0] cpu_io_dmem_readdata; // @[top.scala 14:21]
  wire  mem_clock; // @[top.scala 15:20]
  wire [31:0] mem_io_imem_address; // @[top.scala 15:20]
  wire [31:0] mem_io_imem_instruction; // @[top.scala 15:20]
  wire [31:0] mem_io_dmem_address; // @[top.scala 15:20]
  wire  mem_io_dmem_memread; // @[top.scala 15:20]
  wire  mem_io_dmem_memwrite; // @[top.scala 15:20]
  wire [1:0] mem_io_dmem_maskmode; // @[top.scala 15:20]
  wire  mem_io_dmem_sext; // @[top.scala 15:20]
  wire [31:0] mem_io_dmem_readdata; // @[top.scala 15:20]
  SingleCycleCPU cpu ( // @[top.scala 14:21]
    .clock(cpu_clock),
    .reset(cpu_reset),
    .io_imem_address(cpu_io_imem_address),
    .io_imem_instruction(cpu_io_imem_instruction),
    .io_dmem_address(cpu_io_dmem_address),
    .io_dmem_memread(cpu_io_dmem_memread),
    .io_dmem_memwrite(cpu_io_dmem_memwrite),
    .io_dmem_maskmode(cpu_io_dmem_maskmode),
    .io_dmem_sext(cpu_io_dmem_sext),
    .io_dmem_readdata(cpu_io_dmem_readdata)
  );
  DualPortedMemory mem ( // @[top.scala 15:20]
    .clock(mem_clock),
    .io_imem_address(mem_io_imem_address),
    .io_imem_instruction(mem_io_imem_instruction),
    .io_dmem_address(mem_io_dmem_address),
    .io_dmem_memread(mem_io_dmem_memread),
    .io_dmem_memwrite(mem_io_dmem_memwrite),
    .io_dmem_maskmode(mem_io_dmem_maskmode),
    .io_dmem_sext(mem_io_dmem_sext),
    .io_dmem_readdata(mem_io_dmem_readdata)
  );
  assign io_success = 1'h0;
  assign cpu_clock = clock;
  assign cpu_reset = reset;
  assign cpu_io_imem_instruction = mem_io_imem_instruction; // @[top.scala 17:15]
  assign cpu_io_dmem_readdata = mem_io_dmem_readdata; // @[top.scala 18:15]
  assign mem_clock = clock;
  assign mem_io_imem_address = cpu_io_imem_address; // @[top.scala 17:15]
  assign mem_io_dmem_address = cpu_io_dmem_address; // @[top.scala 18:15]
  assign mem_io_dmem_memread = cpu_io_dmem_memread; // @[top.scala 18:15]
  assign mem_io_dmem_memwrite = cpu_io_dmem_memwrite; // @[top.scala 18:15]
  assign mem_io_dmem_maskmode = cpu_io_dmem_maskmode; // @[top.scala 18:15]
  assign mem_io_dmem_sext = cpu_io_dmem_sext; // @[top.scala 18:15]
endmodule
