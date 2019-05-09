module Control(
  input  [6:0] io_opcode,
  output       io_validinst,
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
  wire  _T_19; // @[Lookup.scala 9:38]
  wire  _T_21; // @[Lookup.scala 11:37]
  wire  _T_22; // @[Lookup.scala 11:37]
  wire  _T_23; // @[Lookup.scala 11:37]
  wire  _T_24; // @[Lookup.scala 11:37]
  wire  _T_25; // @[Lookup.scala 11:37]
  wire  _T_26; // @[Lookup.scala 11:37]
  wire  _T_27; // @[Lookup.scala 11:37]
  wire  _T_28; // @[Lookup.scala 11:37]
  wire  _T_35; // @[Lookup.scala 11:37]
  wire  _T_36; // @[Lookup.scala 11:37]
  wire  _T_37; // @[Lookup.scala 11:37]
  wire  _T_46; // @[Lookup.scala 11:37]
  wire [2:0] _T_47; // @[Lookup.scala 11:37]
  wire [2:0] _T_48; // @[Lookup.scala 11:37]
  wire [2:0] _T_49; // @[Lookup.scala 11:37]
  wire [2:0] _T_50; // @[Lookup.scala 11:37]
  wire [2:0] _T_51; // @[Lookup.scala 11:37]
  wire [2:0] _T_52; // @[Lookup.scala 11:37]
  wire [2:0] _T_53; // @[Lookup.scala 11:37]
  wire [2:0] _T_54; // @[Lookup.scala 11:37]
  wire [2:0] _T_55; // @[Lookup.scala 11:37]
  wire [2:0] signals_3; // @[Lookup.scala 11:37]
  wire  _T_60; // @[Lookup.scala 11:37]
  wire  _T_61; // @[Lookup.scala 11:37]
  wire  _T_62; // @[Lookup.scala 11:37]
  wire  _T_63; // @[Lookup.scala 11:37]
  wire  _T_64; // @[Lookup.scala 11:37]
  wire  _T_72; // @[Lookup.scala 11:37]
  wire  _T_73; // @[Lookup.scala 11:37]
  wire  _T_76; // @[Lookup.scala 11:37]
  wire  _T_77; // @[Lookup.scala 11:37]
  wire  _T_78; // @[Lookup.scala 11:37]
  wire  _T_79; // @[Lookup.scala 11:37]
  wire  _T_80; // @[Lookup.scala 11:37]
  wire  _T_81; // @[Lookup.scala 11:37]
  wire  _T_82; // @[Lookup.scala 11:37]
  wire  _T_85; // @[Lookup.scala 11:37]
  wire  _T_86; // @[Lookup.scala 11:37]
  wire  _T_87; // @[Lookup.scala 11:37]
  wire  _T_88; // @[Lookup.scala 11:37]
  wire  _T_89; // @[Lookup.scala 11:37]
  wire  _T_90; // @[Lookup.scala 11:37]
  wire  _T_91; // @[Lookup.scala 11:37]
  wire [1:0] _T_95; // @[Lookup.scala 11:37]
  wire [1:0] _T_96; // @[Lookup.scala 11:37]
  wire [1:0] _T_97; // @[Lookup.scala 11:37]
  wire [1:0] _T_98; // @[Lookup.scala 11:37]
  wire [1:0] _T_99; // @[Lookup.scala 11:37]
  wire [1:0] _T_100; // @[Lookup.scala 11:37]
  wire [1:0] _T_102; // @[Lookup.scala 11:37]
  wire [1:0] _T_103; // @[Lookup.scala 11:37]
  wire [1:0] _T_104; // @[Lookup.scala 11:37]
  wire [1:0] _T_105; // @[Lookup.scala 11:37]
  wire [1:0] _T_106; // @[Lookup.scala 11:37]
  wire [1:0] _T_107; // @[Lookup.scala 11:37]
  wire [1:0] _T_108; // @[Lookup.scala 11:37]
  wire [1:0] _T_109; // @[Lookup.scala 11:37]
  assign _T_1 = 7'h33 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_3 = 7'h13 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_5 = 7'h3 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_7 = 7'h23 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_9 = 7'h63 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_11 = 7'h37 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_13 = 7'h17 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_15 = 7'h6f == io_opcode; // @[Lookup.scala 9:38]
  assign _T_17 = 7'h67 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_19 = 7'h73 == io_opcode; // @[Lookup.scala 9:38]
  assign _T_21 = _T_17 ? 1'h1 : _T_19; // @[Lookup.scala 11:37]
  assign _T_22 = _T_15 ? 1'h1 : _T_21; // @[Lookup.scala 11:37]
  assign _T_23 = _T_13 ? 1'h1 : _T_22; // @[Lookup.scala 11:37]
  assign _T_24 = _T_11 ? 1'h1 : _T_23; // @[Lookup.scala 11:37]
  assign _T_25 = _T_9 ? 1'h1 : _T_24; // @[Lookup.scala 11:37]
  assign _T_26 = _T_7 ? 1'h1 : _T_25; // @[Lookup.scala 11:37]
  assign _T_27 = _T_5 ? 1'h1 : _T_26; // @[Lookup.scala 11:37]
  assign _T_28 = _T_3 ? 1'h1 : _T_27; // @[Lookup.scala 11:37]
  assign _T_35 = _T_7 ? 1'h0 : _T_9; // @[Lookup.scala 11:37]
  assign _T_36 = _T_5 ? 1'h0 : _T_35; // @[Lookup.scala 11:37]
  assign _T_37 = _T_3 ? 1'h0 : _T_36; // @[Lookup.scala 11:37]
  assign _T_46 = _T_3 ? 1'h0 : _T_5; // @[Lookup.scala 11:37]
  assign _T_47 = _T_19 ? 3'h3 : 3'h4; // @[Lookup.scala 11:37]
  assign _T_48 = _T_17 ? 3'h2 : _T_47; // @[Lookup.scala 11:37]
  assign _T_49 = _T_15 ? 3'h2 : _T_48; // @[Lookup.scala 11:37]
  assign _T_50 = _T_13 ? 3'h0 : _T_49; // @[Lookup.scala 11:37]
  assign _T_51 = _T_11 ? 3'h0 : _T_50; // @[Lookup.scala 11:37]
  assign _T_52 = _T_9 ? 3'h0 : _T_51; // @[Lookup.scala 11:37]
  assign _T_53 = _T_7 ? 3'h0 : _T_52; // @[Lookup.scala 11:37]
  assign _T_54 = _T_5 ? 3'h1 : _T_53; // @[Lookup.scala 11:37]
  assign _T_55 = _T_3 ? 3'h0 : _T_54; // @[Lookup.scala 11:37]
  assign signals_3 = _T_1 ? 3'h0 : _T_55; // @[Lookup.scala 11:37]
  assign _T_60 = _T_11 ? 1'h1 : _T_13; // @[Lookup.scala 11:37]
  assign _T_61 = _T_9 ? 1'h0 : _T_60; // @[Lookup.scala 11:37]
  assign _T_62 = _T_7 ? 1'h1 : _T_61; // @[Lookup.scala 11:37]
  assign _T_63 = _T_5 ? 1'h1 : _T_62; // @[Lookup.scala 11:37]
  assign _T_64 = _T_3 ? 1'h0 : _T_63; // @[Lookup.scala 11:37]
  assign _T_72 = _T_5 ? 1'h0 : _T_7; // @[Lookup.scala 11:37]
  assign _T_73 = _T_3 ? 1'h0 : _T_72; // @[Lookup.scala 11:37]
  assign _T_76 = _T_15 ? 1'h0 : _T_17; // @[Lookup.scala 11:37]
  assign _T_77 = _T_13 ? 1'h1 : _T_76; // @[Lookup.scala 11:37]
  assign _T_78 = _T_11 ? 1'h1 : _T_77; // @[Lookup.scala 11:37]
  assign _T_79 = _T_9 ? 1'h0 : _T_78; // @[Lookup.scala 11:37]
  assign _T_80 = _T_7 ? 1'h1 : _T_79; // @[Lookup.scala 11:37]
  assign _T_81 = _T_5 ? 1'h1 : _T_80; // @[Lookup.scala 11:37]
  assign _T_82 = _T_3 ? 1'h1 : _T_81; // @[Lookup.scala 11:37]
  assign _T_85 = _T_15 ? 1'h1 : _T_17; // @[Lookup.scala 11:37]
  assign _T_86 = _T_13 ? 1'h1 : _T_85; // @[Lookup.scala 11:37]
  assign _T_87 = _T_11 ? 1'h1 : _T_86; // @[Lookup.scala 11:37]
  assign _T_88 = _T_9 ? 1'h0 : _T_87; // @[Lookup.scala 11:37]
  assign _T_89 = _T_7 ? 1'h0 : _T_88; // @[Lookup.scala 11:37]
  assign _T_90 = _T_5 ? 1'h1 : _T_89; // @[Lookup.scala 11:37]
  assign _T_91 = _T_3 ? 1'h1 : _T_90; // @[Lookup.scala 11:37]
  assign _T_95 = _T_13 ? 2'h2 : {{1'd0}, _T_15}; // @[Lookup.scala 11:37]
  assign _T_96 = _T_11 ? 2'h1 : _T_95; // @[Lookup.scala 11:37]
  assign _T_97 = _T_9 ? 2'h0 : _T_96; // @[Lookup.scala 11:37]
  assign _T_98 = _T_7 ? 2'h0 : _T_97; // @[Lookup.scala 11:37]
  assign _T_99 = _T_5 ? 2'h0 : _T_98; // @[Lookup.scala 11:37]
  assign _T_100 = _T_3 ? 2'h0 : _T_99; // @[Lookup.scala 11:37]
  assign _T_102 = _T_17 ? 2'h3 : 2'h0; // @[Lookup.scala 11:37]
  assign _T_103 = _T_15 ? 2'h2 : _T_102; // @[Lookup.scala 11:37]
  assign _T_104 = _T_13 ? 2'h0 : _T_103; // @[Lookup.scala 11:37]
  assign _T_105 = _T_11 ? 2'h0 : _T_104; // @[Lookup.scala 11:37]
  assign _T_106 = _T_9 ? 2'h0 : _T_105; // @[Lookup.scala 11:37]
  assign _T_107 = _T_7 ? 2'h0 : _T_106; // @[Lookup.scala 11:37]
  assign _T_108 = _T_5 ? 2'h0 : _T_107; // @[Lookup.scala 11:37]
  assign _T_109 = _T_3 ? 2'h0 : _T_108; // @[Lookup.scala 11:37]
  assign io_validinst = _T_1 ? 1'h1 : _T_28; // @[control.scala 68:16]
  assign io_branch = _T_1 ? 1'h0 : _T_37; // @[control.scala 69:13]
  assign io_memread = _T_1 ? 1'h0 : _T_46; // @[control.scala 70:14]
  assign io_toreg = signals_3[1:0]; // @[control.scala 71:12]
  assign io_add = _T_1 ? 1'h0 : _T_64; // @[control.scala 72:10]
  assign io_memwrite = _T_1 ? 1'h0 : _T_73; // @[control.scala 73:15]
  assign io_regwrite = _T_1 ? 1'h1 : _T_91; // @[control.scala 75:15]
  assign io_immediate = _T_1 ? 1'h0 : _T_82; // @[control.scala 74:16]
  assign io_alusrc1 = _T_1 ? 2'h0 : _T_100; // @[control.scala 76:14]
  assign io_jump = _T_1 ? 2'h0 : _T_109; // @[control.scala 77:11]
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
module CSRRegFile(
  input         clock,
  input         reset,
  output [31:0] io_rw_rdata,
  input  [31:0] io_rw_wdata,
  input  [31:0] io_decode_inst,
  input  [31:0] io_decode_immid,
  output        io_decode_read_illegal,
  output        io_decode_write_illegal,
  output        io_status_sd,
  output [7:0]  io_status_wpri1,
  output        io_status_tsr,
  output        io_status_tw,
  output        io_status_tvm,
  output        io_status_mxr,
  output        io_status_sum,
  output        io_status_mprv,
  output [1:0]  io_status_xs,
  output [1:0]  io_status_fs,
  output [1:0]  io_status_mpp,
  output [1:0]  io_status_wpri2,
  output        io_status_spp,
  output        io_status_mpie,
  output        io_status_wpri3,
  output        io_status_spie,
  output        io_status_upie,
  output        io_status_mie,
  output        io_status_wpri4,
  output        io_status_sie,
  output        io_status_uie,
  input         io_exception,
  input         io_retire,
  input  [31:0] io_pc,
  output [31:0] io_time
);
  reg [31:0] reg_mepc; // @[csr.scala 259:21]
  reg [31:0] _RAND_0;
  reg  reg_mcause_interrupt; // @[csr.scala 260:27]
  reg [31:0] _RAND_1;
  reg [30:0] reg_mcause_exceptioncode; // @[csr.scala 260:27]
  reg [31:0] _RAND_2;
  reg  reg_mip_mtix; // @[csr.scala 266:24]
  reg [31:0] _RAND_3;
  reg [5:0] _T_61; // @[helpers.scala 111:41]
  reg [31:0] _RAND_4;
  wire [6:0] _T_62; // @[helpers.scala 112:33]
  reg [57:0] _T_63; // @[helpers.scala 116:31]
  reg [63:0] _RAND_5;
  wire  _T_64; // @[helpers.scala 117:20]
  wire [57:0] _T_66; // @[helpers.scala 117:43]
  wire [63:0] _T_67; // @[Cat.scala 30:58]
  reg [5:0] _T_68; // @[helpers.scala 111:41]
  reg [31:0] _RAND_6;
  wire [6:0] _T_69; // @[helpers.scala 112:33]
  reg [57:0] _T_70; // @[helpers.scala 116:31]
  reg [63:0] _RAND_7;
  wire  _T_71; // @[helpers.scala 117:20]
  wire [57:0] _T_73; // @[helpers.scala 117:43]
  wire [63:0] _T_74; // @[Cat.scala 30:58]
  wire [10:0] _T_151; // @[csr.scala 277:38]
  wire [7:0] _T_155; // @[csr.scala 277:38]
  wire [31:0] read_mstatus; // @[csr.scala 277:38]
  wire [31:0] _T_215; // @[csr.scala 293:32]
  wire [31:0] _T_228; // @[csr.scala 298:38]
  wire [11:0] csr; // @[csr.scala 320:27]
  wire  _T_230; // @[csr.scala 325:66]
  wire  _T_231; // @[csr.scala 325:66]
  wire  _T_232; // @[csr.scala 325:66]
  wire  _T_233; // @[csr.scala 325:66]
  wire  _T_234; // @[csr.scala 325:66]
  wire  _T_235; // @[csr.scala 325:66]
  wire  _T_236; // @[csr.scala 325:66]
  wire  _T_237; // @[csr.scala 325:66]
  wire  _T_238; // @[csr.scala 325:66]
  wire  _T_239; // @[csr.scala 325:66]
  wire  _T_240; // @[csr.scala 325:66]
  wire  _T_241; // @[csr.scala 325:66]
  wire  _T_242; // @[csr.scala 325:66]
  wire  _T_243; // @[csr.scala 325:66]
  wire  _T_244; // @[csr.scala 325:66]
  wire  _T_245; // @[csr.scala 325:66]
  wire  _T_246; // @[csr.scala 325:66]
  wire  _T_247; // @[csr.scala 325:66]
  wire  _T_248; // @[csr.scala 325:66]
  wire [1:0] _T_250; // @[csr.scala 327:22]
  wire [1:0] _T_251; // @[csr.scala 327:30]
  wire  _T_289; // @[csr.scala 338:117]
  wire  _T_290; // @[csr.scala 338:117]
  wire  _T_291; // @[csr.scala 338:117]
  wire  _T_292; // @[csr.scala 338:117]
  wire  _T_293; // @[csr.scala 338:117]
  wire  _T_294; // @[csr.scala 338:117]
  wire  _T_295; // @[csr.scala 338:117]
  wire  _T_296; // @[csr.scala 338:117]
  wire  _T_297; // @[csr.scala 338:117]
  wire  _T_298; // @[csr.scala 338:117]
  wire  _T_299; // @[csr.scala 338:117]
  wire  _T_300; // @[csr.scala 338:117]
  wire  _T_301; // @[csr.scala 338:117]
  wire  _T_302; // @[csr.scala 338:117]
  wire  _T_303; // @[csr.scala 338:117]
  wire  _T_304; // @[csr.scala 338:117]
  wire  _T_305; // @[csr.scala 338:117]
  wire  _T_306; // @[csr.scala 338:117]
  wire [31:0] _GEN_2; // @[csr.scala 348:23]
  wire [30:0] _GEN_3; // @[csr.scala 348:23]
  wire [31:0] _GEN_5; // @[csr.scala 348:23]
  wire [1:0] _T_318; // @[Bitwise.scala 48:55]
  wire  _T_319; // @[csr.scala 355:52]
  wire  _T_321; // @[csr.scala 355:9]
  wire  _T_322; // @[csr.scala 355:9]
  wire [63:0] _T_332; // @[Mux.scala 19:72]
  wire [63:0] _T_333; // @[Mux.scala 19:72]
  wire [31:0] _T_337; // @[Mux.scala 19:72]
  wire [31:0] _T_338; // @[Mux.scala 19:72]
  wire [31:0] _T_339; // @[Mux.scala 19:72]
  wire [31:0] _T_340; // @[Mux.scala 19:72]
  wire [31:0] _T_343; // @[Mux.scala 19:72]
  wire [31:0] _T_345; // @[Mux.scala 19:72]
  wire [63:0] _T_351; // @[Mux.scala 19:72]
  wire [63:0] _GEN_146; // @[Mux.scala 19:72]
  wire [63:0] _T_355; // @[Mux.scala 19:72]
  wire [63:0] _GEN_147; // @[Mux.scala 19:72]
  wire [63:0] _T_356; // @[Mux.scala 19:72]
  wire [63:0] _GEN_148; // @[Mux.scala 19:72]
  wire [63:0] _T_357; // @[Mux.scala 19:72]
  wire [63:0] _GEN_149; // @[Mux.scala 19:72]
  wire [63:0] _T_358; // @[Mux.scala 19:72]
  wire [63:0] _GEN_150; // @[Mux.scala 19:72]
  wire [63:0] _T_361; // @[Mux.scala 19:72]
  wire [63:0] _GEN_151; // @[Mux.scala 19:72]
  wire [63:0] _T_363; // @[Mux.scala 19:72]
  wire [34:0] _GEN_141; // @[csr.scala 388:14]
  wire [31:0] _GEN_143; // @[csr.scala 388:14]
  assign _T_62 = _T_61 + 6'h1; // @[helpers.scala 112:33]
  assign _T_64 = _T_62[6]; // @[helpers.scala 117:20]
  assign _T_66 = _T_63 + 58'h1; // @[helpers.scala 117:43]
  assign _T_67 = {_T_63,_T_61}; // @[Cat.scala 30:58]
  assign _T_69 = _T_68 + 6'h1; // @[helpers.scala 112:33]
  assign _T_71 = _T_69[6]; // @[helpers.scala 117:20]
  assign _T_73 = _T_70 + 58'h1; // @[helpers.scala 117:43]
  assign _T_74 = {_T_70,_T_68}; // @[Cat.scala 30:58]
  assign _T_151 = {io_status_wpri2,io_status_spp,io_status_mpie,io_status_wpri3,io_status_spie,io_status_upie,io_status_mie,io_status_wpri4,io_status_sie,io_status_uie}; // @[csr.scala 277:38]
  assign _T_155 = {io_status_sum,io_status_mprv,io_status_xs,io_status_fs,io_status_mpp}; // @[csr.scala 277:38]
  assign read_mstatus = {io_status_sd,io_status_wpri1,io_status_tsr,io_status_tw,io_status_tvm,io_status_mxr,_T_155,_T_151}; // @[csr.scala 277:38]
  assign _T_215 = {23'h0,1'h0,reg_mip_mtix,1'h0,6'h0}; // @[csr.scala 293:32]
  assign _T_228 = {reg_mcause_interrupt,reg_mcause_exceptioncode}; // @[csr.scala 298:38]
  assign csr = io_decode_inst[31:20]; // @[csr.scala 320:27]
  assign _T_230 = csr == 12'h320; // @[csr.scala 325:66]
  assign _T_231 = csr == 12'hb00; // @[csr.scala 325:66]
  assign _T_232 = csr == 12'hb02; // @[csr.scala 325:66]
  assign _T_233 = csr == 12'hf13; // @[csr.scala 325:66]
  assign _T_234 = csr == 12'hf12; // @[csr.scala 325:66]
  assign _T_235 = csr == 12'hf11; // @[csr.scala 325:66]
  assign _T_236 = csr == 12'h301; // @[csr.scala 325:66]
  assign _T_237 = csr == 12'h300; // @[csr.scala 325:66]
  assign _T_238 = csr == 12'h305; // @[csr.scala 325:66]
  assign _T_239 = csr == 12'h344; // @[csr.scala 325:66]
  assign _T_240 = csr == 12'h304; // @[csr.scala 325:66]
  assign _T_241 = csr == 12'h340; // @[csr.scala 325:66]
  assign _T_242 = csr == 12'h341; // @[csr.scala 325:66]
  assign _T_243 = csr == 12'h343; // @[csr.scala 325:66]
  assign _T_244 = csr == 12'h342; // @[csr.scala 325:66]
  assign _T_245 = csr == 12'hf14; // @[csr.scala 325:66]
  assign _T_246 = csr == 12'h302; // @[csr.scala 325:66]
  assign _T_247 = csr == 12'hb80; // @[csr.scala 325:66]
  assign _T_248 = csr == 12'hb82; // @[csr.scala 325:66]
  assign _T_250 = csr[11:10]; // @[csr.scala 327:22]
  assign _T_251 = ~ _T_250; // @[csr.scala 327:30]
  assign _T_289 = _T_230 | _T_231; // @[csr.scala 338:117]
  assign _T_290 = _T_289 | _T_232; // @[csr.scala 338:117]
  assign _T_291 = _T_290 | _T_233; // @[csr.scala 338:117]
  assign _T_292 = _T_291 | _T_234; // @[csr.scala 338:117]
  assign _T_293 = _T_292 | _T_235; // @[csr.scala 338:117]
  assign _T_294 = _T_293 | _T_236; // @[csr.scala 338:117]
  assign _T_295 = _T_294 | _T_237; // @[csr.scala 338:117]
  assign _T_296 = _T_295 | _T_238; // @[csr.scala 338:117]
  assign _T_297 = _T_296 | _T_239; // @[csr.scala 338:117]
  assign _T_298 = _T_297 | _T_240; // @[csr.scala 338:117]
  assign _T_299 = _T_298 | _T_241; // @[csr.scala 338:117]
  assign _T_300 = _T_299 | _T_242; // @[csr.scala 338:117]
  assign _T_301 = _T_300 | _T_243; // @[csr.scala 338:117]
  assign _T_302 = _T_301 | _T_244; // @[csr.scala 338:117]
  assign _T_303 = _T_302 | _T_245; // @[csr.scala 338:117]
  assign _T_304 = _T_303 | _T_246; // @[csr.scala 338:117]
  assign _T_305 = _T_304 | _T_247; // @[csr.scala 338:117]
  assign _T_306 = _T_305 | _T_248; // @[csr.scala 338:117]
  assign _GEN_2 = io_exception ? 32'h0 : {{31'd0}, reg_mcause_interrupt}; // @[csr.scala 348:23]
  assign _GEN_3 = io_exception ? 31'h2 : reg_mcause_exceptioncode; // @[csr.scala 348:23]
  assign _GEN_5 = io_exception ? io_pc : reg_mepc; // @[csr.scala 348:23]
  assign _T_318 = {{1'd0}, io_exception}; // @[Bitwise.scala 48:55]
  assign _T_319 = _T_318 <= 2'h1; // @[csr.scala 355:52]
  assign _T_321 = _T_319 | reset; // @[csr.scala 355:9]
  assign _T_322 = _T_321 == 1'h0; // @[csr.scala 355:9]
  assign _T_332 = _T_231 ? _T_67 : 64'h0; // @[Mux.scala 19:72]
  assign _T_333 = _T_232 ? _T_74 : 64'h0; // @[Mux.scala 19:72]
  assign _T_337 = _T_236 ? 32'h100 : 32'h0; // @[Mux.scala 19:72]
  assign _T_338 = _T_237 ? read_mstatus : 32'h0; // @[Mux.scala 19:72]
  assign _T_339 = _T_238 ? 32'h80000000 : 32'h0; // @[Mux.scala 19:72]
  assign _T_340 = _T_239 ? _T_215 : 32'h0; // @[Mux.scala 19:72]
  assign _T_343 = _T_242 ? reg_mepc : 32'h0; // @[Mux.scala 19:72]
  assign _T_345 = _T_244 ? _T_228 : 32'h0; // @[Mux.scala 19:72]
  assign _T_351 = _T_332 | _T_333; // @[Mux.scala 19:72]
  assign _GEN_146 = {{32'd0}, _T_337}; // @[Mux.scala 19:72]
  assign _T_355 = _T_351 | _GEN_146; // @[Mux.scala 19:72]
  assign _GEN_147 = {{32'd0}, _T_338}; // @[Mux.scala 19:72]
  assign _T_356 = _T_355 | _GEN_147; // @[Mux.scala 19:72]
  assign _GEN_148 = {{32'd0}, _T_339}; // @[Mux.scala 19:72]
  assign _T_357 = _T_356 | _GEN_148; // @[Mux.scala 19:72]
  assign _GEN_149 = {{32'd0}, _T_340}; // @[Mux.scala 19:72]
  assign _T_358 = _T_357 | _GEN_149; // @[Mux.scala 19:72]
  assign _GEN_150 = {{32'd0}, _T_343}; // @[Mux.scala 19:72]
  assign _T_361 = _T_358 | _GEN_150; // @[Mux.scala 19:72]
  assign _GEN_151 = {{32'd0}, _T_345}; // @[Mux.scala 19:72]
  assign _T_363 = _T_361 | _GEN_151; // @[Mux.scala 19:72]
  assign _GEN_141 = {{3'd0}, _GEN_5}; // @[csr.scala 388:14]
  assign _GEN_143 = {{1'd0}, _GEN_3}; // @[csr.scala 388:14]
  assign io_rw_rdata = _T_363[31:0]; // @[csr.scala 386:15]
  assign io_decode_read_illegal = _T_306 == 1'h0; // @[csr.scala 339:26]
  assign io_decode_write_illegal = _T_251 == 2'h0; // @[csr.scala 340:27]
  assign io_status_sd = 1'h0; // @[csr.scala 343:13]
  assign io_status_wpri1 = 8'h0; // @[csr.scala 343:13]
  assign io_status_tsr = 1'h0; // @[csr.scala 343:13]
  assign io_status_tw = 1'h0; // @[csr.scala 343:13]
  assign io_status_tvm = 1'h0; // @[csr.scala 343:13]
  assign io_status_mxr = 1'h0; // @[csr.scala 343:13]
  assign io_status_sum = 1'h0; // @[csr.scala 343:13]
  assign io_status_mprv = 1'h0; // @[csr.scala 343:13]
  assign io_status_xs = 2'h0; // @[csr.scala 343:13]
  assign io_status_fs = 2'h0; // @[csr.scala 343:13]
  assign io_status_mpp = 2'h3; // @[csr.scala 343:13]
  assign io_status_wpri2 = 2'h0; // @[csr.scala 343:13]
  assign io_status_spp = 1'h0; // @[csr.scala 343:13]
  assign io_status_mpie = 1'h0; // @[csr.scala 343:13]
  assign io_status_wpri3 = 1'h0; // @[csr.scala 343:13]
  assign io_status_spie = 1'h0; // @[csr.scala 343:13]
  assign io_status_upie = 1'h0; // @[csr.scala 343:13]
  assign io_status_mie = 1'h0; // @[csr.scala 343:13]
  assign io_status_wpri4 = 1'h0; // @[csr.scala 343:13]
  assign io_status_sie = 1'h0; // @[csr.scala 343:13]
  assign io_status_uie = 1'h0; // @[csr.scala 343:13]
  assign io_time = _T_67[31:0]; // @[csr.scala 382:11]
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
  reg_mepc = _RAND_0[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  reg_mcause_interrupt = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  reg_mcause_exceptioncode = _RAND_2[30:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  reg_mip_mtix = _RAND_3[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  _T_61 = _RAND_4[5:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {2{`RANDOM}};
  _T_63 = _RAND_5[57:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  _T_68 = _RAND_6[5:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_7 = {2{`RANDOM}};
  _T_70 = _RAND_7[57:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    reg_mepc <= _GEN_141[31:0];
    if (reset) begin
      reg_mcause_interrupt <= 1'h0;
    end else begin
      reg_mcause_interrupt <= _GEN_2[0];
    end
    if (reset) begin
      reg_mcause_exceptioncode <= 31'h0;
    end else begin
      reg_mcause_exceptioncode <= _GEN_143[30:0];
    end
    if (reset) begin
      reg_mip_mtix <= 1'h0;
    end else begin
      reg_mip_mtix <= 1'h1;
    end
    if (reset) begin
      _T_61 <= 6'h0;
    end else begin
      _T_61 <= _T_62[5:0];
    end
    if (reset) begin
      _T_63 <= 58'h0;
    end else begin
      if (_T_64) begin
        _T_63 <= _T_66;
      end
    end
    if (reset) begin
      _T_68 <= 6'h0;
    end else begin
      _T_68 <= _T_69[5:0];
    end
    if (reset) begin
      _T_70 <= 58'h0;
    end else begin
      if (_T_71) begin
        _T_70 <= _T_73;
      end
    end
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_322) begin
          $fwrite(32'h80000002,"Assertion failed: these conditions must be mutually exclusive\n    at csr.scala:355 assert(PopCount(insn_ret :: io.exception :: Nil) <= 1, \"these conditions must be mutually exclusive\")\n"); // @[csr.scala 355:9]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef STOP_COND
      if (`STOP_COND) begin
    `endif
        if (_T_322) begin
          $fatal; // @[csr.scala 355:9]
        end
    `ifdef STOP_COND
      end
    `endif
    `endif // SYNTHESIS
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
  wire [6:0] opcode; // @[helpers.scala 260:30]
  wire  _T; // @[Conditional.scala 37:30]
  wire [19:0] _T_1; // @[helpers.scala 263:31]
  wire [31:0] _T_3; // @[Cat.scala 30:58]
  wire  _T_4; // @[Conditional.scala 37:30]
  wire  _T_8; // @[Conditional.scala 37:30]
  wire  _T_9; // @[helpers.scala 271:35]
  wire [7:0] _T_10; // @[helpers.scala 271:55]
  wire  _T_11; // @[helpers.scala 272:35]
  wire [9:0] _T_12; // @[helpers.scala 272:55]
  wire [19:0] _T_15; // @[Cat.scala 30:58]
  wire  _T_16; // @[helpers.scala 273:36]
  wire [10:0] _T_18; // @[Bitwise.scala 72:12]
  wire [31:0] _T_20; // @[Cat.scala 30:58]
  wire  _T_21; // @[Conditional.scala 37:30]
  wire [11:0] _T_22; // @[helpers.scala 276:31]
  wire  _T_23; // @[helpers.scala 277:36]
  wire [19:0] _T_25; // @[Bitwise.scala 72:12]
  wire [31:0] _T_26; // @[Cat.scala 30:58]
  wire  _T_27; // @[Conditional.scala 37:30]
  wire  _T_29; // @[helpers.scala 280:55]
  wire [5:0] _T_30; // @[helpers.scala 281:35]
  wire [3:0] _T_31; // @[helpers.scala 281:58]
  wire [11:0] _T_34; // @[Cat.scala 30:58]
  wire  _T_35; // @[helpers.scala 282:37]
  wire [18:0] _T_37; // @[Bitwise.scala 72:12]
  wire [31:0] _T_39; // @[Cat.scala 30:58]
  wire  _T_40; // @[Conditional.scala 37:30]
  wire  _T_46; // @[Conditional.scala 37:30]
  wire [6:0] _T_47; // @[helpers.scala 289:35]
  wire [4:0] _T_48; // @[helpers.scala 289:59]
  wire [11:0] _T_49; // @[Cat.scala 30:58]
  wire  _T_50; // @[helpers.scala 290:36]
  wire [19:0] _T_52; // @[Bitwise.scala 72:12]
  wire [31:0] _T_53; // @[Cat.scala 30:58]
  wire  _T_54; // @[Conditional.scala 37:30]
  wire  _T_60; // @[Conditional.scala 37:30]
  wire [4:0] _T_62; // @[helpers.scala 297:53]
  wire [31:0] _T_63; // @[Cat.scala 30:58]
  wire [31:0] _GEN_0; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_1; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_2; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_3; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_4; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_5; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_6; // @[Conditional.scala 39:67]
  wire [31:0] _GEN_7; // @[Conditional.scala 39:67]
  assign opcode = io_instruction[6:0]; // @[helpers.scala 260:30]
  assign _T = 7'h37 == opcode; // @[Conditional.scala 37:30]
  assign _T_1 = io_instruction[31:12]; // @[helpers.scala 263:31]
  assign _T_3 = {_T_1,12'h0}; // @[Cat.scala 30:58]
  assign _T_4 = 7'h17 == opcode; // @[Conditional.scala 37:30]
  assign _T_8 = 7'h6f == opcode; // @[Conditional.scala 37:30]
  assign _T_9 = io_instruction[31]; // @[helpers.scala 271:35]
  assign _T_10 = io_instruction[19:12]; // @[helpers.scala 271:55]
  assign _T_11 = io_instruction[20]; // @[helpers.scala 272:35]
  assign _T_12 = io_instruction[30:21]; // @[helpers.scala 272:55]
  assign _T_15 = {_T_9,_T_10,_T_11,_T_12}; // @[Cat.scala 30:58]
  assign _T_16 = _T_15[19]; // @[helpers.scala 273:36]
  assign _T_18 = _T_16 ? 11'h7ff : 11'h0; // @[Bitwise.scala 72:12]
  assign _T_20 = {_T_18,_T_9,_T_10,_T_11,_T_12,1'h0}; // @[Cat.scala 30:58]
  assign _T_21 = 7'h67 == opcode; // @[Conditional.scala 37:30]
  assign _T_22 = io_instruction[31:20]; // @[helpers.scala 276:31]
  assign _T_23 = _T_22[11]; // @[helpers.scala 277:36]
  assign _T_25 = _T_23 ? 20'hfffff : 20'h0; // @[Bitwise.scala 72:12]
  assign _T_26 = {_T_25,_T_22}; // @[Cat.scala 30:58]
  assign _T_27 = 7'h63 == opcode; // @[Conditional.scala 37:30]
  assign _T_29 = io_instruction[7]; // @[helpers.scala 280:55]
  assign _T_30 = io_instruction[30:25]; // @[helpers.scala 281:35]
  assign _T_31 = io_instruction[11:8]; // @[helpers.scala 281:58]
  assign _T_34 = {_T_9,_T_29,_T_30,_T_31}; // @[Cat.scala 30:58]
  assign _T_35 = _T_34[11]; // @[helpers.scala 282:37]
  assign _T_37 = _T_35 ? 19'h7ffff : 19'h0; // @[Bitwise.scala 72:12]
  assign _T_39 = {_T_37,_T_9,_T_29,_T_30,_T_31,1'h0}; // @[Cat.scala 30:58]
  assign _T_40 = 7'h3 == opcode; // @[Conditional.scala 37:30]
  assign _T_46 = 7'h23 == opcode; // @[Conditional.scala 37:30]
  assign _T_47 = io_instruction[31:25]; // @[helpers.scala 289:35]
  assign _T_48 = io_instruction[11:7]; // @[helpers.scala 289:59]
  assign _T_49 = {_T_47,_T_48}; // @[Cat.scala 30:58]
  assign _T_50 = _T_49[11]; // @[helpers.scala 290:36]
  assign _T_52 = _T_50 ? 20'hfffff : 20'h0; // @[Bitwise.scala 72:12]
  assign _T_53 = {_T_52,_T_47,_T_48}; // @[Cat.scala 30:58]
  assign _T_54 = 7'h13 == opcode; // @[Conditional.scala 37:30]
  assign _T_60 = 7'h73 == opcode; // @[Conditional.scala 37:30]
  assign _T_62 = io_instruction[19:15]; // @[helpers.scala 297:53]
  assign _T_63 = {27'h0,_T_62}; // @[Cat.scala 30:58]
  assign _GEN_0 = _T_60 ? _T_63 : 32'h0; // @[Conditional.scala 39:67]
  assign _GEN_1 = _T_54 ? _T_26 : _GEN_0; // @[Conditional.scala 39:67]
  assign _GEN_2 = _T_46 ? _T_53 : _GEN_1; // @[Conditional.scala 39:67]
  assign _GEN_3 = _T_40 ? _T_26 : _GEN_2; // @[Conditional.scala 39:67]
  assign _GEN_4 = _T_27 ? _T_39 : _GEN_3; // @[Conditional.scala 39:67]
  assign _GEN_5 = _T_21 ? _T_26 : _GEN_4; // @[Conditional.scala 39:67]
  assign _GEN_6 = _T_8 ? _T_20 : _GEN_5; // @[Conditional.scala 39:67]
  assign _GEN_7 = _T_4 ? _T_3 : _GEN_6; // @[Conditional.scala 39:67]
  assign io_sextImm = _T ? _T_3 : _GEN_7; // @[helpers.scala 258:14 helpers.scala 264:18 helpers.scala 268:18 helpers.scala 273:18 helpers.scala 277:18 helpers.scala 282:18 helpers.scala 286:18 helpers.scala 290:18 helpers.scala 294:18 helpers.scala 297:18]
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
  assign io_result = io_inputx + io_inputy; // @[helpers.scala 239:13]
endmodule
module SingleCycleCPU(
  input         clock,
  input         reset,
  output [31:0] io_imem_address,
  input  [31:0] io_imem_instruction,
  output [31:0] io_dmem_address,
  output [31:0] io_dmem_writedata,
  output        io_dmem_memread,
  output        io_dmem_memwrite,
  output [1:0]  io_dmem_maskmode,
  output        io_dmem_sext,
  input  [31:0] io_dmem_readdata
);
  wire [6:0] control_io_opcode; // @[cpu.scala 20:26]
  wire  control_io_validinst; // @[cpu.scala 20:26]
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
  wire  csr_clock; // @[cpu.scala 22:26]
  wire  csr_reset; // @[cpu.scala 22:26]
  wire [31:0] csr_io_rw_rdata; // @[cpu.scala 22:26]
  wire [31:0] csr_io_rw_wdata; // @[cpu.scala 22:26]
  wire [31:0] csr_io_decode_inst; // @[cpu.scala 22:26]
  wire [31:0] csr_io_decode_immid; // @[cpu.scala 22:26]
  wire  csr_io_decode_read_illegal; // @[cpu.scala 22:26]
  wire  csr_io_decode_write_illegal; // @[cpu.scala 22:26]
  wire  csr_io_status_sd; // @[cpu.scala 22:26]
  wire [7:0] csr_io_status_wpri1; // @[cpu.scala 22:26]
  wire  csr_io_status_tsr; // @[cpu.scala 22:26]
  wire  csr_io_status_tw; // @[cpu.scala 22:26]
  wire  csr_io_status_tvm; // @[cpu.scala 22:26]
  wire  csr_io_status_mxr; // @[cpu.scala 22:26]
  wire  csr_io_status_sum; // @[cpu.scala 22:26]
  wire  csr_io_status_mprv; // @[cpu.scala 22:26]
  wire [1:0] csr_io_status_xs; // @[cpu.scala 22:26]
  wire [1:0] csr_io_status_fs; // @[cpu.scala 22:26]
  wire [1:0] csr_io_status_mpp; // @[cpu.scala 22:26]
  wire [1:0] csr_io_status_wpri2; // @[cpu.scala 22:26]
  wire  csr_io_status_spp; // @[cpu.scala 22:26]
  wire  csr_io_status_mpie; // @[cpu.scala 22:26]
  wire  csr_io_status_wpri3; // @[cpu.scala 22:26]
  wire  csr_io_status_spie; // @[cpu.scala 22:26]
  wire  csr_io_status_upie; // @[cpu.scala 22:26]
  wire  csr_io_status_mie; // @[cpu.scala 22:26]
  wire  csr_io_status_wpri4; // @[cpu.scala 22:26]
  wire  csr_io_status_sie; // @[cpu.scala 22:26]
  wire  csr_io_status_uie; // @[cpu.scala 22:26]
  wire  csr_io_exception; // @[cpu.scala 22:26]
  wire  csr_io_retire; // @[cpu.scala 22:26]
  wire [31:0] csr_io_pc; // @[cpu.scala 22:26]
  wire [31:0] csr_io_time; // @[cpu.scala 22:26]
  wire  aluControl_io_add; // @[cpu.scala 23:26]
  wire  aluControl_io_immediate; // @[cpu.scala 23:26]
  wire [6:0] aluControl_io_funct7; // @[cpu.scala 23:26]
  wire [2:0] aluControl_io_funct3; // @[cpu.scala 23:26]
  wire [3:0] aluControl_io_operation; // @[cpu.scala 23:26]
  wire [3:0] alu_io_operation; // @[cpu.scala 24:26]
  wire [31:0] alu_io_inputx; // @[cpu.scala 24:26]
  wire [31:0] alu_io_inputy; // @[cpu.scala 24:26]
  wire [31:0] alu_io_result; // @[cpu.scala 24:26]
  wire [31:0] immGen_io_instruction; // @[cpu.scala 25:26]
  wire [31:0] immGen_io_sextImm; // @[cpu.scala 25:26]
  wire  branchCtrl_io_branch; // @[cpu.scala 26:26]
  wire [2:0] branchCtrl_io_funct3; // @[cpu.scala 26:26]
  wire [31:0] branchCtrl_io_inputx; // @[cpu.scala 26:26]
  wire [31:0] branchCtrl_io_inputy; // @[cpu.scala 26:26]
  wire  branchCtrl_io_taken; // @[cpu.scala 26:26]
  wire [31:0] pcPlusFour_io_inputx; // @[cpu.scala 27:26]
  wire [31:0] pcPlusFour_io_inputy; // @[cpu.scala 27:26]
  wire [31:0] pcPlusFour_io_result; // @[cpu.scala 27:26]
  wire [31:0] branchAdd_io_inputx; // @[cpu.scala 28:26]
  wire [31:0] branchAdd_io_inputy; // @[cpu.scala 28:26]
  wire [31:0] branchAdd_io_result; // @[cpu.scala 28:26]
  reg [31:0] pc; // @[cpu.scala 19:27]
  reg [31:0] _RAND_0;
  reg [29:0] value; // @[Counter.scala 26:33]
  reg [31:0] _RAND_1;
  wire [29:0] _T_2; // @[Counter.scala 35:22]
  wire  _T_7; // @[cpu.scala 47:74]
  wire  _T_12; // @[Conditional.scala 37:30]
  wire  _T_13; // @[Conditional.scala 37:30]
  wire [31:0] _GEN_2; // @[Conditional.scala 39:67]
  wire  _T_16; // @[cpu.scala 81:36]
  wire  _T_18; // @[cpu.scala 90:23]
  wire  _T_19; // @[cpu.scala 90:45]
  wire  _T_22; // @[cpu.scala 97:26]
  wire  _T_23; // @[cpu.scala 99:33]
  wire [31:0] _GEN_4; // @[cpu.scala 99:42]
  wire  _T_24; // @[cpu.scala 110:48]
  wire  _T_25; // @[cpu.scala 110:29]
  wire  _T_26; // @[cpu.scala 112:32]
  wire [31:0] _T_29; // @[cpu.scala 113:30]
  wire  _T_33; // @[cpu.scala 134:11]
  Control control ( // @[cpu.scala 20:26]
    .io_opcode(control_io_opcode),
    .io_validinst(control_io_validinst),
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
  CSRRegFile csr ( // @[cpu.scala 22:26]
    .clock(csr_clock),
    .reset(csr_reset),
    .io_rw_rdata(csr_io_rw_rdata),
    .io_rw_wdata(csr_io_rw_wdata),
    .io_decode_inst(csr_io_decode_inst),
    .io_decode_immid(csr_io_decode_immid),
    .io_decode_read_illegal(csr_io_decode_read_illegal),
    .io_decode_write_illegal(csr_io_decode_write_illegal),
    .io_status_sd(csr_io_status_sd),
    .io_status_wpri1(csr_io_status_wpri1),
    .io_status_tsr(csr_io_status_tsr),
    .io_status_tw(csr_io_status_tw),
    .io_status_tvm(csr_io_status_tvm),
    .io_status_mxr(csr_io_status_mxr),
    .io_status_sum(csr_io_status_sum),
    .io_status_mprv(csr_io_status_mprv),
    .io_status_xs(csr_io_status_xs),
    .io_status_fs(csr_io_status_fs),
    .io_status_mpp(csr_io_status_mpp),
    .io_status_wpri2(csr_io_status_wpri2),
    .io_status_spp(csr_io_status_spp),
    .io_status_mpie(csr_io_status_mpie),
    .io_status_wpri3(csr_io_status_wpri3),
    .io_status_spie(csr_io_status_spie),
    .io_status_upie(csr_io_status_upie),
    .io_status_mie(csr_io_status_mie),
    .io_status_wpri4(csr_io_status_wpri4),
    .io_status_sie(csr_io_status_sie),
    .io_status_uie(csr_io_status_uie),
    .io_exception(csr_io_exception),
    .io_retire(csr_io_retire),
    .io_pc(csr_io_pc),
    .io_time(csr_io_time)
  );
  ALUControl aluControl ( // @[cpu.scala 23:26]
    .io_add(aluControl_io_add),
    .io_immediate(aluControl_io_immediate),
    .io_funct7(aluControl_io_funct7),
    .io_funct3(aluControl_io_funct3),
    .io_operation(aluControl_io_operation)
  );
  ALU alu ( // @[cpu.scala 24:26]
    .io_operation(alu_io_operation),
    .io_inputx(alu_io_inputx),
    .io_inputy(alu_io_inputy),
    .io_result(alu_io_result)
  );
  ImmediateGenerator immGen ( // @[cpu.scala 25:26]
    .io_instruction(immGen_io_instruction),
    .io_sextImm(immGen_io_sextImm)
  );
  BranchControl branchCtrl ( // @[cpu.scala 26:26]
    .io_branch(branchCtrl_io_branch),
    .io_funct3(branchCtrl_io_funct3),
    .io_inputx(branchCtrl_io_inputx),
    .io_inputy(branchCtrl_io_inputy),
    .io_taken(branchCtrl_io_taken)
  );
  Adder pcPlusFour ( // @[cpu.scala 27:26]
    .io_inputx(pcPlusFour_io_inputx),
    .io_inputy(pcPlusFour_io_inputy),
    .io_result(pcPlusFour_io_result)
  );
  Adder branchAdd ( // @[cpu.scala 28:26]
    .io_inputx(branchAdd_io_inputx),
    .io_inputy(branchAdd_io_inputy),
    .io_result(branchAdd_io_result)
  );
  assign _T_2 = value + 30'h1; // @[Counter.scala 35:22]
  assign _T_7 = registers_io_writereg != 5'h0; // @[cpu.scala 47:74]
  assign _T_12 = 2'h0 == control_io_alusrc1; // @[Conditional.scala 37:30]
  assign _T_13 = 2'h1 == control_io_alusrc1; // @[Conditional.scala 37:30]
  assign _GEN_2 = _T_13 ? 32'h0 : pc; // @[Conditional.scala 39:67]
  assign _T_16 = io_imem_instruction[14]; // @[cpu.scala 81:36]
  assign _T_18 = control_io_validinst == 1'h0; // @[cpu.scala 90:23]
  assign _T_19 = _T_18 | csr_io_decode_read_illegal; // @[cpu.scala 90:45]
  assign _T_22 = control_io_toreg == 2'h1; // @[cpu.scala 97:26]
  assign _T_23 = control_io_toreg == 2'h2; // @[cpu.scala 99:33]
  assign _GEN_4 = _T_23 ? pcPlusFour_io_result : alu_io_result; // @[cpu.scala 99:42]
  assign _T_24 = control_io_jump == 2'h2; // @[cpu.scala 110:48]
  assign _T_25 = branchCtrl_io_taken | _T_24; // @[cpu.scala 110:29]
  assign _T_26 = control_io_jump == 2'h3; // @[cpu.scala 112:32]
  assign _T_29 = alu_io_result & 32'hfffffffe; // @[cpu.scala 113:30]
  assign _T_33 = reset == 1'h0; // @[cpu.scala 134:11]
  assign io_imem_address = pc; // @[cpu.scala 32:19]
  assign io_dmem_address = alu_io_result; // @[cpu.scala 76:21]
  assign io_dmem_writedata = registers_io_readdata2; // @[cpu.scala 77:21]
  assign io_dmem_memread = control_io_memread; // @[cpu.scala 78:21]
  assign io_dmem_memwrite = control_io_memwrite; // @[cpu.scala 79:21]
  assign io_dmem_maskmode = io_imem_instruction[13:12]; // @[cpu.scala 80:21]
  assign io_dmem_sext = ~ _T_16; // @[cpu.scala 81:21]
  assign control_io_opcode = io_imem_instruction[6:0]; // @[cpu.scala 41:21]
  assign registers_clock = clock;
  assign registers_io_readreg1 = io_imem_instruction[19:15]; // @[cpu.scala 43:25]
  assign registers_io_readreg2 = io_imem_instruction[24:20]; // @[cpu.scala 44:25]
  assign registers_io_writereg = io_imem_instruction[11:7]; // @[cpu.scala 46:25]
  assign registers_io_writedata = _T_22 ? io_dmem_readdata : _GEN_4; // @[cpu.scala 105:26]
  assign registers_io_wen = control_io_regwrite & _T_7; // @[cpu.scala 47:25]
  assign csr_clock = clock;
  assign csr_reset = reset;
  assign csr_io_rw_wdata = registers_io_readdata2; // @[cpu.scala 86:19]
  assign csr_io_decode_inst = io_imem_instruction; // @[cpu.scala 84:22]
  assign csr_io_decode_immid = immGen_io_sextImm; // @[cpu.scala 85:23]
  assign csr_io_exception = _T_19 | csr_io_decode_write_illegal; // @[cpu.scala 90:20]
  assign csr_io_retire = 1'h1; // @[cpu.scala 89:17]
  assign csr_io_pc = pc; // @[cpu.scala 92:13]
  assign aluControl_io_add = control_io_add; // @[cpu.scala 49:27]
  assign aluControl_io_immediate = control_io_immediate; // @[cpu.scala 50:27]
  assign aluControl_io_funct7 = io_imem_instruction[31:25]; // @[cpu.scala 51:27]
  assign aluControl_io_funct3 = io_imem_instruction[14:12]; // @[cpu.scala 52:27]
  assign alu_io_operation = aluControl_io_operation; // @[cpu.scala 73:20]
  assign alu_io_inputx = _T_12 ? registers_io_readdata1 : _GEN_2; // @[cpu.scala 71:17]
  assign alu_io_inputy = control_io_immediate ? immGen_io_sextImm : registers_io_readdata2; // @[cpu.scala 72:17]
  assign immGen_io_instruction = io_imem_instruction; // @[cpu.scala 54:25]
  assign branchCtrl_io_branch = control_io_branch; // @[cpu.scala 58:24]
  assign branchCtrl_io_funct3 = io_imem_instruction[14:12]; // @[cpu.scala 59:24]
  assign branchCtrl_io_inputx = registers_io_readdata1; // @[cpu.scala 60:24]
  assign branchCtrl_io_inputy = registers_io_readdata2; // @[cpu.scala 61:24]
  assign pcPlusFour_io_inputx = pc; // @[cpu.scala 34:24]
  assign pcPlusFour_io_inputy = 32'h4; // @[cpu.scala 35:24]
  assign branchAdd_io_inputx = pc; // @[cpu.scala 107:23]
  assign branchAdd_io_inputy = immGen_io_sextImm; // @[cpu.scala 108:23]
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
      if (_T_25) begin
        pc <= branchAdd_io_result;
      end else begin
        if (_T_26) begin
          pc <= _T_29;
        end else begin
          if (_T_18) begin
            pc <= 32'h80000004;
          end else begin
            pc <= pcPlusFour_io_result;
          end
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
        if (_T_33) begin
          $fwrite(32'h80000002,"DASM(%x)\n",io_imem_instruction); // @[cpu.scala 134:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"CYCLE=%d\n",value); // @[cpu.scala 135:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"pc: %d\n",pc); // @[cpu.scala 136:11]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"control: Bundle(opcode -> %d, validinst -> %d, branch -> %d, memread -> %d, toreg -> %d, add -> %d, memwrite -> %d, regwrite -> %d, immediate -> %d, alusrc1 -> %d, jump -> %d)\n",control_io_opcode,control_io_validinst,control_io_branch,control_io_memread,control_io_toreg,control_io_add,control_io_memwrite,control_io_regwrite,control_io_immediate,control_io_alusrc1,control_io_jump); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"registers: Bundle(readreg1 -> %d, readreg2 -> %d, writereg -> %d, writedata -> %d, wen -> %d, readdata1 -> %d, readdata2 -> %d)\n",registers_io_readreg1,registers_io_readreg2,registers_io_writereg,registers_io_writedata,registers_io_wen,registers_io_readdata1,registers_io_readdata2); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"aluControl: Bundle(add -> %d, immediate -> %d, funct7 -> %d, funct3 -> %d, operation -> %d)\n",aluControl_io_add,aluControl_io_immediate,aluControl_io_funct7,aluControl_io_funct3,aluControl_io_operation); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"alu: Bundle(operation -> %d, inputx -> %d, inputy -> %d, result -> %d)\n",alu_io_operation,alu_io_inputx,alu_io_inputy,alu_io_result); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"immGen: Bundle(instruction -> %d, sextImm -> %d)\n",immGen_io_instruction,immGen_io_sextImm); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"branchCtrl: Bundle(branch -> %d, funct3 -> %d, inputx -> %d, inputy -> %d, taken -> %d)\n",branchCtrl_io_branch,branchCtrl_io_funct3,branchCtrl_io_inputx,branchCtrl_io_inputy,branchCtrl_io_taken); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"pcPlusFour: Bundle(inputx -> %d, inputy -> %d, result -> %d)\n",pcPlusFour_io_inputx,pcPlusFour_io_inputy,pcPlusFour_io_result); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"branchAdd: Bundle(inputx -> %d, inputy -> %d, result -> %d)\n",branchAdd_io_inputx,branchAdd_io_inputy,branchAdd_io_result); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"csr: Bundle(rw -> Bundle(rdata -> %d, wdata -> %d), csr_stall -> %d, eret -> %d, decode -> Bundle(inst -> %d, immid -> %d, read_illegal -> %d, write_illegal -> %d, system_illegal -> %d), status -> Bundle(sd -> %d, wpri1 -> %d, tsr -> %d, tw -> %d, tvm -> %d, mxr -> %d, sum -> %d, mprv -> %d, xs -> %d, fs -> %d, mpp -> %d, wpri2 -> %d, spp -> %d, mpie -> %d, wpri3 -> %d, spie -> %d, upie -> %d, mie -> %d, wpri4 -> %d, sie -> %d, uie -> %d), evec -> %d, exception -> %d, retire -> %d, pc -> %d, time -> %d)\n",csr_io_rw_rdata,csr_io_rw_wdata,1'h0,1'h0,csr_io_decode_inst,csr_io_decode_immid,csr_io_decode_read_illegal,csr_io_decode_write_illegal,1'h0,1'h0,8'h0,1'h0,1'h0,1'h0,1'h0,1'h0,1'h0,2'h0,2'h0,2'h3,2'h0,1'h0,1'h0,1'h0,1'h0,1'h0,1'h0,1'h0,1'h0,1'h0,32'h80000004,csr_io_exception,csr_io_retire,csr_io_pc,csr_io_time); // @[cpu.scala 138:13]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_33) begin
          $fwrite(32'h80000002,"\n"); // @[cpu.scala 140:11]
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
  input  [31:0] io_dmem_writedata,
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
  assign memory_dmem_writedata = io_dmem_writedata; // @[memory.scala 80:28]
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
  wire [31:0] cpu_io_dmem_writedata; // @[top.scala 14:21]
  wire  cpu_io_dmem_memread; // @[top.scala 14:21]
  wire  cpu_io_dmem_memwrite; // @[top.scala 14:21]
  wire [1:0] cpu_io_dmem_maskmode; // @[top.scala 14:21]
  wire  cpu_io_dmem_sext; // @[top.scala 14:21]
  wire [31:0] cpu_io_dmem_readdata; // @[top.scala 14:21]
  wire  mem_clock; // @[top.scala 15:20]
  wire [31:0] mem_io_imem_address; // @[top.scala 15:20]
  wire [31:0] mem_io_imem_instruction; // @[top.scala 15:20]
  wire [31:0] mem_io_dmem_address; // @[top.scala 15:20]
  wire [31:0] mem_io_dmem_writedata; // @[top.scala 15:20]
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
    .io_dmem_writedata(cpu_io_dmem_writedata),
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
    .io_dmem_writedata(mem_io_dmem_writedata),
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
  assign mem_io_dmem_writedata = cpu_io_dmem_writedata; // @[top.scala 18:15]
  assign mem_io_dmem_memread = cpu_io_dmem_memread; // @[top.scala 18:15]
  assign mem_io_dmem_memwrite = cpu_io_dmem_memwrite; // @[top.scala 18:15]
  assign mem_io_dmem_maskmode = cpu_io_dmem_maskmode; // @[top.scala 18:15]
  assign mem_io_dmem_sext = cpu_io_dmem_sext; // @[top.scala 18:15]
endmodule
