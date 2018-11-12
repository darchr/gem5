module DebugModule( // @[:@3.2]
  input         clock, // @[:@4.4]
  input         reset, // @[:@5.4]
  output        io_dmi_req_ready, // @[:@6.4]
  input         io_dmi_req_valid, // @[:@6.4]
  input  [1:0]  io_dmi_req_bits_op, // @[:@6.4]
  input  [6:0]  io_dmi_req_bits_addr, // @[:@6.4]
  input  [31:0] io_dmi_req_bits_data, // @[:@6.4]
  output        io_dmi_resp_valid, // @[:@6.4]
  output [31:0] io_dmi_resp_bits_data, // @[:@6.4]
  output [4:0]  io_ddpath_addr, // @[:@6.4]
  output [31:0] io_ddpath_wdata, // @[:@6.4]
  input  [31:0] io_ddpath_rdata, // @[:@6.4]
  output        io_debugmem_req_valid, // @[:@6.4]
  output [31:0] io_debugmem_req_bits_addr, // @[:@6.4]
  output [31:0] io_debugmem_req_bits_data, // @[:@6.4]
  output        io_debugmem_req_bits_fcn, // @[:@6.4]
  input         io_debugmem_resp_valid, // @[:@6.4]
  input  [31:0] io_debugmem_resp_bits_data, // @[:@6.4]
  output        io_resetcore // @[:@6.4]
);
  reg  dmstatus_allrunning; // @[debug.scala 121:25:@54.4]
  reg [31:0] _RAND_0;
  reg  dmstatus_allhalted; // @[debug.scala 121:25:@54.4]
  reg [31:0] _RAND_1;
  reg  sbcs_sbsingleread; // @[debug.scala 129:21:@73.4]
  reg [31:0] _RAND_2;
  reg [2:0] sbcs_sbaccess; // @[debug.scala 129:21:@73.4]
  reg [31:0] _RAND_3;
  reg  sbcs_sbautoincrement; // @[debug.scala 129:21:@73.4]
  reg [31:0] _RAND_4;
  reg  sbcs_sbautoread; // @[debug.scala 129:21:@73.4]
  reg [31:0] _RAND_5;
  reg [2:0] sbcs_sberror; // @[debug.scala 129:21:@73.4]
  reg [31:0] _RAND_6;
  reg [2:0] abstractcs_cmderr; // @[debug.scala 134:27:@85.4]
  reg [31:0] _RAND_7;
  reg  command_postexec; // @[debug.scala 135:20:@86.4]
  reg [31:0] _RAND_8;
  reg  command_transfer; // @[debug.scala 135:20:@86.4]
  reg [31:0] _RAND_9;
  reg  command_write; // @[debug.scala 135:20:@86.4]
  reg [31:0] _RAND_10;
  reg [15:0] command_regno; // @[debug.scala 135:20:@86.4]
  reg [31:0] _RAND_11;
  reg  dmcontrol_haltreq; // @[debug.scala 136:22:@87.4]
  reg [31:0] _RAND_12;
  reg  dmcontrol_resumereq; // @[debug.scala 136:22:@87.4]
  reg [31:0] _RAND_13;
  reg  dmcontrol_hartreset; // @[debug.scala 136:22:@87.4]
  reg [31:0] _RAND_14;
  reg  dmcontrol_ndmreset; // @[debug.scala 136:22:@87.4]
  reg [31:0] _RAND_15;
  reg  dmcontrol_dmactive; // @[debug.scala 136:22:@87.4]
  reg [31:0] _RAND_16;
  reg [31:0] data0; // @[debug.scala 138:18:@89.4]
  reg [31:0] _RAND_17;
  reg [31:0] data1; // @[debug.scala 139:18:@90.4]
  reg [31:0] _RAND_18;
  reg [31:0] data2; // @[debug.scala 140:18:@91.4]
  reg [31:0] _RAND_19;
  reg [31:0] sbaddr; // @[debug.scala 141:19:@92.4]
  reg [31:0] _RAND_20;
  reg [31:0] sbdata; // @[debug.scala 142:19:@93.4]
  reg [31:0] _RAND_21;
  reg  memreadfire; // @[debug.scala 143:28:@94.4]
  reg [31:0] _RAND_22;
  reg  coreresetval; // @[debug.scala 144:29:@95.4]
  reg [31:0] _RAND_23;
  wire [3:0] _T_190; // @[debug.scala 147:47:@97.4]
  wire [11:0] _T_191; // @[debug.scala 147:47:@98.4]
  wire [31:0] _T_195; // @[debug.scala 147:47:@102.4]
  wire [1:0] _T_196; // @[debug.scala 148:45:@103.4]
  wire [25:0] _T_198; // @[debug.scala 148:45:@105.4]
  wire [1:0] _T_200; // @[debug.scala 148:45:@107.4]
  wire [2:0] _T_201; // @[debug.scala 148:45:@108.4]
  wire [5:0] _T_202; // @[debug.scala 148:45:@109.4]
  wire [31:0] _T_203; // @[debug.scala 148:45:@110.4]
  wire [1:0] _T_208; // @[debug.scala 149:44:@115.4]
  wire [3:0] _T_209; // @[debug.scala 149:44:@116.4]
  wire [9:0] _T_210; // @[debug.scala 149:44:@117.4]
  wire [1:0] _T_211; // @[debug.scala 149:44:@118.4]
  wire [3:0] _T_213; // @[debug.scala 149:44:@120.4]
  wire [21:0] _T_218; // @[debug.scala 149:44:@125.4]
  wire [31:0] _T_219; // @[debug.scala 149:44:@126.4]
  wire [16:0] _T_220; // @[debug.scala 150:41:@127.4]
  wire [1:0] _T_221; // @[debug.scala 150:41:@128.4]
  wire [18:0] _T_222; // @[debug.scala 150:41:@129.4]
  wire [31:0] _T_226; // @[debug.scala 150:41:@133.4]
  wire [1:0] _T_237; // @[debug.scala 163:35:@139.4]
  wire [4:0] _T_238; // @[debug.scala 163:35:@140.4]
  wire [11:0] _T_239; // @[debug.scala 163:35:@141.4]
  wire [14:0] _T_240; // @[debug.scala 163:35:@142.4]
  wire [19:0] _T_241; // @[debug.scala 163:35:@143.4]
  wire [31:0] _T_242; // @[debug.scala 163:35:@144.4]
  wire  _T_244; // @[debug.scala 166:79:@145.4]
  wire  _T_246; // @[debug.scala 166:79:@146.4]
  wire  _T_248; // @[debug.scala 166:79:@147.4]
  wire  _T_250; // @[debug.scala 166:79:@148.4]
  wire  _T_252; // @[debug.scala 166:79:@149.4]
  wire  _T_258; // @[debug.scala 166:79:@152.4]
  wire  _T_260; // @[debug.scala 166:79:@153.4]
  wire  _T_262; // @[debug.scala 166:79:@154.4]
  wire  _T_276; // @[debug.scala 166:79:@161.4]
  wire  _T_278; // @[debug.scala 166:79:@162.4]
  wire  _T_280; // @[debug.scala 166:79:@163.4]
  wire [31:0] _T_283; // @[Mux.scala 19:72:@164.4]
  wire [31:0] _T_285; // @[Mux.scala 19:72:@165.4]
  wire [31:0] _T_287; // @[Mux.scala 19:72:@166.4]
  wire [31:0] _T_289; // @[Mux.scala 19:72:@167.4]
  wire [20:0] _T_291; // @[Mux.scala 19:72:@168.4]
  wire [31:0] _T_297; // @[Mux.scala 19:72:@171.4]
  wire [31:0] _T_299; // @[Mux.scala 19:72:@172.4]
  wire [31:0] _T_301; // @[Mux.scala 19:72:@173.4]
  wire [31:0] _T_315; // @[Mux.scala 19:72:@180.4]
  wire [31:0] _T_317; // @[Mux.scala 19:72:@181.4]
  wire [31:0] _T_319; // @[Mux.scala 19:72:@182.4]
  wire [31:0] _T_320; // @[Mux.scala 19:72:@183.4]
  wire [31:0] _T_321; // @[Mux.scala 19:72:@184.4]
  wire [31:0] _T_322; // @[Mux.scala 19:72:@185.4]
  wire [31:0] _GEN_77; // @[Mux.scala 19:72:@186.4]
  wire [31:0] _T_323; // @[Mux.scala 19:72:@186.4]
  wire [31:0] _T_326; // @[Mux.scala 19:72:@189.4]
  wire [31:0] _T_327; // @[Mux.scala 19:72:@190.4]
  wire [31:0] _T_328; // @[Mux.scala 19:72:@191.4]
  wire [31:0] _T_335; // @[Mux.scala 19:72:@198.4]
  wire [31:0] _T_336; // @[Mux.scala 19:72:@199.4]
  wire  _T_344; // @[debug.scala 172:28:@209.4]
  wire  _T_345; // @[debug.scala 173:54:@211.6]
  wire [2:0] _T_353; // @[debug.scala 174:42:@220.8]
  wire [2:0] _GEN_0; // @[debug.scala 173:75:@212.6]
  wire [15:0] _T_364; // @[debug.scala 178:39:@238.8]
  wire  _T_365; // @[debug.scala 178:39:@240.8]
  wire  _T_366; // @[debug.scala 178:39:@242.8]
  wire  _T_367; // @[debug.scala 178:39:@244.8]
  wire [2:0] _T_369; // @[debug.scala 178:39:@248.8]
  wire  _T_373; // @[debug.scala 179:29:@254.8]
  wire  _GEN_1; // @[debug.scala 179:37:@255.8]
  wire [15:0] _GEN_2; // @[debug.scala 179:37:@255.8]
  wire  _GEN_3; // @[debug.scala 179:37:@255.8]
  wire  _GEN_4; // @[debug.scala 179:37:@255.8]
  wire [1:0] _GEN_5; // @[debug.scala 179:37:@255.8]
  wire  _GEN_6; // @[debug.scala 177:50:@234.6]
  wire [15:0] _GEN_7; // @[debug.scala 177:50:@234.6]
  wire  _GEN_8; // @[debug.scala 177:50:@234.6]
  wire  _GEN_9; // @[debug.scala 177:50:@234.6]
  wire [2:0] _GEN_10; // @[debug.scala 177:50:@234.6]
  wire  _T_381; // @[debug.scala 190:39:@270.8]
  wire  _T_382; // @[debug.scala 190:39:@272.8]
  wire  _T_387; // @[debug.scala 190:39:@282.8]
  wire  _T_388; // @[debug.scala 190:39:@284.8]
  wire  _T_389; // @[debug.scala 190:39:@286.8]
  wire  _GEN_11; // @[debug.scala 189:52:@266.6]
  wire  _GEN_12; // @[debug.scala 189:52:@266.6]
  wire  _GEN_13; // @[debug.scala 189:52:@266.6]
  wire  _GEN_14; // @[debug.scala 189:52:@266.6]
  wire  _GEN_15; // @[debug.scala 189:52:@266.6]
  wire [2:0] _T_401; // @[debug.scala 198:36:@310.8]
  wire  _T_402; // @[debug.scala 198:36:@312.8]
  wire [2:0] _T_404; // @[debug.scala 198:36:@316.8]
  wire  _T_405; // @[debug.scala 198:36:@318.8]
  wire  _GEN_16; // @[debug.scala 197:46:@294.6]
  wire [2:0] _GEN_17; // @[debug.scala 197:46:@294.6]
  wire  _GEN_18; // @[debug.scala 197:46:@294.6]
  wire  _GEN_19; // @[debug.scala 197:46:@294.6]
  wire [2:0] _GEN_20; // @[debug.scala 197:46:@294.6]
  wire [31:0] _GEN_21; // @[debug.scala 205:53:@328.6]
  wire  _T_407; // @[debug.scala 212:33:@337.8]
  wire [32:0] _T_409; // @[debug.scala 214:26:@339.10]
  wire [31:0] _T_410; // @[debug.scala 214:26:@340.10]
  wire [31:0] _GEN_22; // @[debug.scala 213:7:@338.8]
  wire [31:0] _GEN_23; // @[debug.scala 206:50:@331.6]
  wire [31:0] _GEN_28; // @[debug.scala 206:50:@331.6]
  wire [31:0] _GEN_29; // @[debug.scala 217:48:@344.6]
  wire [31:0] _GEN_30; // @[debug.scala 218:50:@347.6]
  wire [31:0] _GEN_31; // @[debug.scala 219:50:@350.6]
  wire [2:0] _GEN_32; // @[debug.scala 172:54:@210.4]
  wire  _GEN_42; // @[debug.scala 172:54:@210.4]
  wire [2:0] _GEN_43; // @[debug.scala 172:54:@210.4]
  wire  _GEN_44; // @[debug.scala 172:54:@210.4]
  wire  _GEN_45; // @[debug.scala 172:54:@210.4]
  wire [2:0] _GEN_46; // @[debug.scala 172:54:@210.4]
  wire [31:0] _GEN_47; // @[debug.scala 172:54:@210.4]
  wire [31:0] _GEN_48; // @[debug.scala 172:54:@210.4]
  wire [31:0] _GEN_53; // @[debug.scala 172:54:@210.4]
  wire [15:0] _T_412; // @[debug.scala 223:35:@354.4]
  wire  _T_414; // @[debug.scala 224:46:@356.4]
  wire  _T_415; // @[debug.scala 224:25:@357.4]
  wire [31:0] _GEN_58; // @[debug.scala 225:24:@359.6]
  wire [2:0] _GEN_62; // @[debug.scala 224:54:@358.4]
  wire  _T_420; // @[debug.scala 234:49:@369.4]
  wire  _T_422; // @[debug.scala 234:8:@370.4]
  wire  _GEN_63; // @[debug.scala 234:98:@371.4]
  reg  firstreaddone; // @[debug.scala 239:26:@374.4]
  reg [31:0] _RAND_24;
  reg  _T_426; // @[debug.scala 241:50:@375.4]
  reg [31:0] _RAND_25;
  wire  _T_429; // @[debug.scala 243:72:@379.4]
  wire  _T_430; // @[debug.scala 243:49:@380.4]
  wire  _T_431; // @[debug.scala 243:119:@381.4]
  wire  _T_432; // @[debug.scala 243:99:@382.4]
  wire [31:0] _GEN_64; // @[debug.scala 249:33:@387.6]
  wire [31:0] _GEN_68; // @[debug.scala 243:137:@383.4]
  wire  _GEN_69; // @[debug.scala 243:137:@383.4]
  wire  _GEN_70; // @[debug.scala 243:137:@383.4]
  wire  _T_435; // @[debug.scala 256:20:@393.4]
  wire [31:0] _GEN_71; // @[debug.scala 263:5:@397.6]
  wire  _GEN_73; // @[debug.scala 257:3:@394.4]
  wire  _T_441; // @[debug.scala 268:8:@403.4]
  wire  _T_444; // @[debug.scala 274:30:@408.4]
  wire  _T_445; // @[debug.scala 274:43:@409.4]
  wire  _GEN_76; // @[debug.scala 274:63:@410.4]
  assign _T_190 = {1'h0,abstractcs_cmderr}; // @[debug.scala 147:47:@97.4]
  assign _T_191 = {_T_190,8'h1}; // @[debug.scala 147:47:@98.4]
  assign _T_195 = {20'h4000,_T_191}; // @[debug.scala 147:47:@102.4]
  assign _T_196 = {dmcontrol_ndmreset,dmcontrol_dmactive}; // @[debug.scala 148:45:@103.4]
  assign _T_198 = {24'h0,_T_196}; // @[debug.scala 148:45:@105.4]
  assign _T_200 = {dmcontrol_haltreq,dmcontrol_resumereq}; // @[debug.scala 148:45:@107.4]
  assign _T_201 = {_T_200,dmcontrol_hartreset}; // @[debug.scala 148:45:@108.4]
  assign _T_202 = {_T_201,3'h0}; // @[debug.scala 148:45:@109.4]
  assign _T_203 = {_T_202,_T_198}; // @[debug.scala 148:45:@110.4]
  assign _T_208 = {dmstatus_allhalted,1'h0}; // @[debug.scala 149:44:@115.4]
  assign _T_209 = {_T_208,2'h2}; // @[debug.scala 149:44:@116.4]
  assign _T_210 = {_T_209,6'h2}; // @[debug.scala 149:44:@117.4]
  assign _T_211 = {dmstatus_allrunning,1'h0}; // @[debug.scala 149:44:@118.4]
  assign _T_213 = {2'h0,_T_211}; // @[debug.scala 149:44:@120.4]
  assign _T_218 = {18'h0,_T_213}; // @[debug.scala 149:44:@125.4]
  assign _T_219 = {_T_218,_T_210}; // @[debug.scala 149:44:@126.4]
  assign _T_220 = {command_write,command_regno}; // @[debug.scala 150:41:@127.4]
  assign _T_221 = {command_postexec,command_transfer}; // @[debug.scala 150:41:@128.4]
  assign _T_222 = {_T_221,_T_220}; // @[debug.scala 150:41:@129.4]
  assign _T_226 = {13'h0,_T_222}; // @[debug.scala 150:41:@133.4]
  assign _T_237 = {sbcs_sbautoincrement,sbcs_sbautoread}; // @[debug.scala 163:35:@139.4]
  assign _T_238 = {_T_237,sbcs_sberror}; // @[debug.scala 163:35:@140.4]
  assign _T_239 = {11'h0,sbcs_sbsingleread}; // @[debug.scala 163:35:@141.4]
  assign _T_240 = {_T_239,sbcs_sbaccess}; // @[debug.scala 163:35:@142.4]
  assign _T_241 = {_T_240,_T_238}; // @[debug.scala 163:35:@143.4]
  assign _T_242 = {_T_241,12'h404}; // @[debug.scala 163:35:@144.4]
  assign _T_244 = io_dmi_req_bits_addr == 7'h16; // @[debug.scala 166:79:@145.4]
  assign _T_246 = io_dmi_req_bits_addr == 7'h10; // @[debug.scala 166:79:@146.4]
  assign _T_248 = io_dmi_req_bits_addr == 7'h11; // @[debug.scala 166:79:@147.4]
  assign _T_250 = io_dmi_req_bits_addr == 7'h17; // @[debug.scala 166:79:@148.4]
  assign _T_252 = io_dmi_req_bits_addr == 7'h12; // @[debug.scala 166:79:@149.4]
  assign _T_258 = io_dmi_req_bits_addr == 7'h4; // @[debug.scala 166:79:@152.4]
  assign _T_260 = io_dmi_req_bits_addr == 7'h5; // @[debug.scala 166:79:@153.4]
  assign _T_262 = io_dmi_req_bits_addr == 7'h6; // @[debug.scala 166:79:@154.4]
  assign _T_276 = io_dmi_req_bits_addr == 7'h38; // @[debug.scala 166:79:@161.4]
  assign _T_278 = io_dmi_req_bits_addr == 7'h39; // @[debug.scala 166:79:@162.4]
  assign _T_280 = io_dmi_req_bits_addr == 7'h3c; // @[debug.scala 166:79:@163.4]
  assign _T_283 = _T_244 ? _T_195 : 32'h0; // @[Mux.scala 19:72:@164.4]
  assign _T_285 = _T_246 ? _T_203 : 32'h0; // @[Mux.scala 19:72:@165.4]
  assign _T_287 = _T_248 ? _T_219 : 32'h0; // @[Mux.scala 19:72:@166.4]
  assign _T_289 = _T_250 ? _T_226 : 32'h0; // @[Mux.scala 19:72:@167.4]
  assign _T_291 = _T_252 ? 21'h111bc0 : 21'h0; // @[Mux.scala 19:72:@168.4]
  assign _T_297 = _T_258 ? data0 : 32'h0; // @[Mux.scala 19:72:@171.4]
  assign _T_299 = _T_260 ? data1 : 32'h0; // @[Mux.scala 19:72:@172.4]
  assign _T_301 = _T_262 ? data2 : 32'h0; // @[Mux.scala 19:72:@173.4]
  assign _T_315 = _T_276 ? _T_242 : 32'h0; // @[Mux.scala 19:72:@180.4]
  assign _T_317 = _T_278 ? sbaddr : 32'h0; // @[Mux.scala 19:72:@181.4]
  assign _T_319 = _T_280 ? sbdata : 32'h0; // @[Mux.scala 19:72:@182.4]
  assign _T_320 = _T_283 | _T_285; // @[Mux.scala 19:72:@183.4]
  assign _T_321 = _T_320 | _T_287; // @[Mux.scala 19:72:@184.4]
  assign _T_322 = _T_321 | _T_289; // @[Mux.scala 19:72:@185.4]
  assign _GEN_77 = {{11'd0}, _T_291}; // @[Mux.scala 19:72:@186.4]
  assign _T_323 = _T_322 | _GEN_77; // @[Mux.scala 19:72:@186.4]
  assign _T_326 = _T_323 | _T_297; // @[Mux.scala 19:72:@189.4]
  assign _T_327 = _T_326 | _T_299; // @[Mux.scala 19:72:@190.4]
  assign _T_328 = _T_327 | _T_301; // @[Mux.scala 19:72:@191.4]
  assign _T_335 = _T_328 | _T_315; // @[Mux.scala 19:72:@198.4]
  assign _T_336 = _T_335 | _T_317; // @[Mux.scala 19:72:@199.4]
  assign _T_344 = io_dmi_req_bits_op == 2'h2; // @[debug.scala 172:28:@209.4]
  assign _T_345 = _T_244 & io_dmi_req_valid; // @[debug.scala 173:54:@211.6]
  assign _T_353 = io_dmi_req_bits_data[10:8]; // @[debug.scala 174:42:@220.8]
  assign _GEN_0 = _T_345 ? _T_353 : abstractcs_cmderr; // @[debug.scala 173:75:@212.6]
  assign _T_364 = io_dmi_req_bits_data[15:0]; // @[debug.scala 178:39:@238.8]
  assign _T_365 = io_dmi_req_bits_data[16]; // @[debug.scala 178:39:@240.8]
  assign _T_366 = io_dmi_req_bits_data[17]; // @[debug.scala 178:39:@242.8]
  assign _T_367 = io_dmi_req_bits_data[18]; // @[debug.scala 178:39:@244.8]
  assign _T_369 = io_dmi_req_bits_data[22:20]; // @[debug.scala 178:39:@248.8]
  assign _T_373 = _T_369 == 3'h2; // @[debug.scala 179:29:@254.8]
  assign _GEN_1 = _T_373 ? _T_367 : command_postexec; // @[debug.scala 179:37:@255.8]
  assign _GEN_2 = _T_373 ? _T_364 : command_regno; // @[debug.scala 179:37:@255.8]
  assign _GEN_3 = _T_373 ? _T_366 : command_transfer; // @[debug.scala 179:37:@255.8]
  assign _GEN_4 = _T_373 ? _T_365 : command_write; // @[debug.scala 179:37:@255.8]
  assign _GEN_5 = _T_373 ? 2'h1 : 2'h2; // @[debug.scala 179:37:@255.8]
  assign _GEN_6 = _T_250 ? _GEN_1 : command_postexec; // @[debug.scala 177:50:@234.6]
  assign _GEN_7 = _T_250 ? _GEN_2 : command_regno; // @[debug.scala 177:50:@234.6]
  assign _GEN_8 = _T_250 ? _GEN_3 : command_transfer; // @[debug.scala 177:50:@234.6]
  assign _GEN_9 = _T_250 ? _GEN_4 : command_write; // @[debug.scala 177:50:@234.6]
  assign _GEN_10 = _T_250 ? {{1'd0}, _GEN_5} : _GEN_0; // @[debug.scala 177:50:@234.6]
  assign _T_381 = io_dmi_req_bits_data[0]; // @[debug.scala 190:39:@270.8]
  assign _T_382 = io_dmi_req_bits_data[1]; // @[debug.scala 190:39:@272.8]
  assign _T_387 = io_dmi_req_bits_data[29]; // @[debug.scala 190:39:@282.8]
  assign _T_388 = io_dmi_req_bits_data[30]; // @[debug.scala 190:39:@284.8]
  assign _T_389 = io_dmi_req_bits_data[31]; // @[debug.scala 190:39:@286.8]
  assign _GEN_11 = _T_246 ? _T_389 : dmcontrol_haltreq; // @[debug.scala 189:52:@266.6]
  assign _GEN_12 = _T_246 ? _T_388 : dmcontrol_resumereq; // @[debug.scala 189:52:@266.6]
  assign _GEN_13 = _T_246 ? _T_387 : dmcontrol_hartreset; // @[debug.scala 189:52:@266.6]
  assign _GEN_14 = _T_246 ? _T_382 : dmcontrol_ndmreset; // @[debug.scala 189:52:@266.6]
  assign _GEN_15 = _T_246 ? _T_381 : dmcontrol_dmactive; // @[debug.scala 189:52:@266.6]
  assign _T_401 = io_dmi_req_bits_data[14:12]; // @[debug.scala 198:36:@310.8]
  assign _T_402 = io_dmi_req_bits_data[15]; // @[debug.scala 198:36:@312.8]
  assign _T_404 = io_dmi_req_bits_data[19:17]; // @[debug.scala 198:36:@316.8]
  assign _T_405 = io_dmi_req_bits_data[20]; // @[debug.scala 198:36:@318.8]
  assign _GEN_16 = _T_276 ? _T_405 : sbcs_sbsingleread; // @[debug.scala 197:46:@294.6]
  assign _GEN_17 = _T_276 ? _T_404 : sbcs_sbaccess; // @[debug.scala 197:46:@294.6]
  assign _GEN_18 = _T_276 ? _T_365 : sbcs_sbautoincrement; // @[debug.scala 197:46:@294.6]
  assign _GEN_19 = _T_276 ? _T_402 : sbcs_sbautoread; // @[debug.scala 197:46:@294.6]
  assign _GEN_20 = _T_276 ? _T_401 : sbcs_sberror; // @[debug.scala 197:46:@294.6]
  assign _GEN_21 = _T_278 ? io_dmi_req_bits_data : sbaddr; // @[debug.scala 205:53:@328.6]
  assign _T_407 = sbcs_sbautoincrement & io_dmi_req_valid; // @[debug.scala 212:33:@337.8]
  assign _T_409 = sbaddr + 32'h4; // @[debug.scala 214:26:@339.10]
  assign _T_410 = _T_409[31:0]; // @[debug.scala 214:26:@340.10]
  assign _GEN_22 = _T_407 ? _T_410 : _GEN_21; // @[debug.scala 213:7:@338.8]
  assign _GEN_23 = _T_280 ? io_dmi_req_bits_data : sbdata; // @[debug.scala 206:50:@331.6]
  assign _GEN_28 = _T_280 ? _GEN_22 : _GEN_21; // @[debug.scala 206:50:@331.6]
  assign _GEN_29 = _T_258 ? io_dmi_req_bits_data : data0; // @[debug.scala 217:48:@344.6]
  assign _GEN_30 = _T_260 ? io_dmi_req_bits_data : data1; // @[debug.scala 218:50:@347.6]
  assign _GEN_31 = _T_262 ? io_dmi_req_bits_data : data2; // @[debug.scala 219:50:@350.6]
  assign _GEN_32 = _T_344 ? _GEN_10 : abstractcs_cmderr; // @[debug.scala 172:54:@210.4]
  assign _GEN_42 = _T_344 ? _GEN_16 : sbcs_sbsingleread; // @[debug.scala 172:54:@210.4]
  assign _GEN_43 = _T_344 ? _GEN_17 : sbcs_sbaccess; // @[debug.scala 172:54:@210.4]
  assign _GEN_44 = _T_344 ? _GEN_18 : sbcs_sbautoincrement; // @[debug.scala 172:54:@210.4]
  assign _GEN_45 = _T_344 ? _GEN_19 : sbcs_sbautoread; // @[debug.scala 172:54:@210.4]
  assign _GEN_46 = _T_344 ? _GEN_20 : sbcs_sberror; // @[debug.scala 172:54:@210.4]
  assign _GEN_47 = _T_344 ? _GEN_28 : sbaddr; // @[debug.scala 172:54:@210.4]
  assign _GEN_48 = _T_344 ? _GEN_23 : sbdata; // @[debug.scala 172:54:@210.4]
  assign _GEN_53 = _T_344 ? _GEN_29 : data0; // @[debug.scala 172:54:@210.4]
  assign _T_412 = command_regno & 16'hfff; // @[debug.scala 223:35:@354.4]
  assign _T_414 = abstractcs_cmderr != 3'h0; // @[debug.scala 224:46:@356.4]
  assign _T_415 = command_transfer & _T_414; // @[debug.scala 224:25:@357.4]
  assign _GEN_58 = command_write ? _GEN_53 : io_ddpath_rdata; // @[debug.scala 225:24:@359.6]
  assign _GEN_62 = _T_415 ? 3'h0 : _GEN_32; // @[debug.scala 224:54:@358.4]
  assign _T_420 = _T_280 & _T_344; // @[debug.scala 234:49:@369.4]
  assign _T_422 = _T_420 == 1'h0; // @[debug.scala 234:8:@370.4]
  assign _GEN_63 = _T_422 ? 1'h0 : 1'h1; // @[debug.scala 234:98:@371.4]
  assign _T_429 = io_dmi_req_bits_op == 2'h1; // @[debug.scala 243:72:@379.4]
  assign _T_430 = _T_280 & _T_429; // @[debug.scala 243:49:@380.4]
  assign _T_431 = sbcs_sbautoread & firstreaddone; // @[debug.scala 243:119:@381.4]
  assign _T_432 = _T_430 | _T_431; // @[debug.scala 243:99:@382.4]
  assign _GEN_64 = io_debugmem_resp_valid ? io_debugmem_resp_bits_data : _GEN_48; // @[debug.scala 249:33:@387.6]
  assign _GEN_68 = _T_432 ? _GEN_64 : _GEN_48; // @[debug.scala 243:137:@383.4]
  assign _GEN_69 = _T_432 ? 1'h1 : memreadfire; // @[debug.scala 243:137:@383.4]
  assign _GEN_70 = _T_432 ? 1'h1 : firstreaddone; // @[debug.scala 243:137:@383.4]
  assign _T_435 = memreadfire & io_debugmem_resp_valid; // @[debug.scala 256:20:@393.4]
  assign _GEN_71 = sbcs_sbautoincrement ? _T_410 : _GEN_47; // @[debug.scala 263:5:@397.6]
  assign _GEN_73 = _T_435 ? 1'h0 : _GEN_69; // @[debug.scala 257:3:@394.4]
  assign _T_441 = _T_280 == 1'h0; // @[debug.scala 268:8:@403.4]
  assign _T_444 = io_dmi_req_bits_addr == 7'h44; // @[debug.scala 274:30:@408.4]
  assign _T_445 = _T_444 & io_dmi_req_valid; // @[debug.scala 274:43:@409.4]
  assign _GEN_76 = _T_445 ? 1'h0 : coreresetval; // @[debug.scala 274:63:@410.4]
  assign io_dmi_req_ready = io_dmi_req_valid; // @[debug.scala 114:20:@32.4]
  assign io_dmi_resp_valid = firstreaddone ? _T_426 : io_dmi_req_valid; // @[debug.scala 241:21:@378.4]
  assign io_dmi_resp_bits_data = _T_336 | _T_319; // @[debug.scala 167:25:@203.4]
  assign io_ddpath_addr = _T_412[4:0]; // @[debug.scala 223:18:@355.4]
  assign io_ddpath_wdata = data0; // @[debug.scala 226:23:@360.8]
  assign io_debugmem_req_valid = io_dmi_req_valid; // @[debug.scala 211:29:@336.8 debug.scala 246:27:@386.6]
  assign io_debugmem_req_bits_addr = sbaddr; // @[debug.scala 208:33:@333.8 debug.scala 244:31:@384.6]
  assign io_debugmem_req_bits_data = sbdata; // @[debug.scala 209:33:@334.8]
  assign io_debugmem_req_bits_fcn = _T_432 ? 1'h0 : _GEN_63; // @[debug.scala 210:32:@335.8 debug.scala 235:30:@372.6 debug.scala 245:30:@385.6]
  assign io_resetcore = coreresetval; // @[debug.scala 272:16:@407.4]
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
  dmstatus_allrunning = _RAND_0[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  dmstatus_allhalted = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  sbcs_sbsingleread = _RAND_2[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  sbcs_sbaccess = _RAND_3[2:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  sbcs_sbautoincrement = _RAND_4[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  sbcs_sbautoread = _RAND_5[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  sbcs_sberror = _RAND_6[2:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_7 = {1{`RANDOM}};
  abstractcs_cmderr = _RAND_7[2:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_8 = {1{`RANDOM}};
  command_postexec = _RAND_8[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_9 = {1{`RANDOM}};
  command_transfer = _RAND_9[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_10 = {1{`RANDOM}};
  command_write = _RAND_10[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_11 = {1{`RANDOM}};
  command_regno = _RAND_11[15:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_12 = {1{`RANDOM}};
  dmcontrol_haltreq = _RAND_12[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_13 = {1{`RANDOM}};
  dmcontrol_resumereq = _RAND_13[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_14 = {1{`RANDOM}};
  dmcontrol_hartreset = _RAND_14[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_15 = {1{`RANDOM}};
  dmcontrol_ndmreset = _RAND_15[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_16 = {1{`RANDOM}};
  dmcontrol_dmactive = _RAND_16[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_17 = {1{`RANDOM}};
  data0 = _RAND_17[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_18 = {1{`RANDOM}};
  data1 = _RAND_18[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_19 = {1{`RANDOM}};
  data2 = _RAND_19[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_20 = {1{`RANDOM}};
  sbaddr = _RAND_20[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_21 = {1{`RANDOM}};
  sbdata = _RAND_21[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_22 = {1{`RANDOM}};
  memreadfire = _RAND_22[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_23 = {1{`RANDOM}};
  coreresetval = _RAND_23[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_24 = {1{`RANDOM}};
  firstreaddone = _RAND_24[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_25 = {1{`RANDOM}};
  _T_426 = _RAND_25[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      dmstatus_allrunning <= 1'h0;
    end else begin
      dmstatus_allrunning <= dmcontrol_resumereq;
    end
    if (reset) begin
      dmstatus_allhalted <= 1'h0;
    end else begin
      dmstatus_allhalted <= dmcontrol_haltreq;
    end
    if (reset) begin
      sbcs_sbsingleread <= 1'h0;
    end else begin
      if (_T_344) begin
        if (_T_276) begin
          sbcs_sbsingleread <= _T_405;
        end
      end
    end
    if (reset) begin
      sbcs_sbaccess <= 3'h2;
    end else begin
      if (_T_344) begin
        if (_T_276) begin
          sbcs_sbaccess <= _T_404;
        end
      end
    end
    if (reset) begin
      sbcs_sbautoincrement <= 1'h0;
    end else begin
      if (_T_344) begin
        if (_T_276) begin
          sbcs_sbautoincrement <= _T_365;
        end
      end
    end
    if (reset) begin
      sbcs_sbautoread <= 1'h0;
    end else begin
      if (_T_344) begin
        if (_T_276) begin
          sbcs_sbautoread <= _T_402;
        end
      end
    end
    if (reset) begin
      sbcs_sberror <= 3'h0;
    end else begin
      if (_T_344) begin
        if (_T_276) begin
          sbcs_sberror <= _T_401;
        end
      end
    end
    if (reset) begin
      abstractcs_cmderr <= 3'h0;
    end else begin
      if (_T_415) begin
        abstractcs_cmderr <= 3'h0;
      end else begin
        if (_T_344) begin
          if (_T_250) begin
            abstractcs_cmderr <= {{1'd0}, _GEN_5};
          end else begin
            if (_T_345) begin
              abstractcs_cmderr <= _T_353;
            end
          end
        end
      end
    end
    if (_T_344) begin
      if (_T_250) begin
        if (_T_373) begin
          command_postexec <= _T_367;
        end
      end
    end
    if (_T_344) begin
      if (_T_250) begin
        if (_T_373) begin
          command_transfer <= _T_366;
        end
      end
    end
    if (_T_344) begin
      if (_T_250) begin
        if (_T_373) begin
          command_write <= _T_365;
        end
      end
    end
    if (_T_344) begin
      if (_T_250) begin
        if (_T_373) begin
          command_regno <= _T_364;
        end
      end
    end
    if (_T_344) begin
      if (_T_246) begin
        dmcontrol_haltreq <= _T_389;
      end
    end
    if (_T_344) begin
      if (_T_246) begin
        dmcontrol_resumereq <= _T_388;
      end
    end
    if (_T_344) begin
      if (_T_246) begin
        dmcontrol_hartreset <= _T_387;
      end
    end
    if (_T_344) begin
      if (_T_246) begin
        dmcontrol_ndmreset <= _T_382;
      end
    end
    if (_T_344) begin
      if (_T_246) begin
        dmcontrol_dmactive <= _T_381;
      end
    end
    if (_T_415) begin
      if (command_write) begin
        if (_T_344) begin
          if (_T_258) begin
            data0 <= io_dmi_req_bits_data;
          end
        end
      end else begin
        data0 <= io_ddpath_rdata;
      end
    end else begin
      if (_T_344) begin
        if (_T_258) begin
          data0 <= io_dmi_req_bits_data;
        end
      end
    end
    if (_T_344) begin
      if (_T_260) begin
        data1 <= io_dmi_req_bits_data;
      end
    end
    if (_T_344) begin
      if (_T_262) begin
        data2 <= io_dmi_req_bits_data;
      end
    end
    if (_T_435) begin
      if (sbcs_sbautoincrement) begin
        sbaddr <= _T_410;
      end else begin
        if (_T_344) begin
          if (_T_280) begin
            if (_T_407) begin
              sbaddr <= _T_410;
            end else begin
              if (_T_278) begin
                sbaddr <= io_dmi_req_bits_data;
              end
            end
          end else begin
            if (_T_278) begin
              sbaddr <= io_dmi_req_bits_data;
            end
          end
        end
      end
    end else begin
      if (_T_344) begin
        if (_T_280) begin
          if (_T_407) begin
            sbaddr <= _T_410;
          end else begin
            if (_T_278) begin
              sbaddr <= io_dmi_req_bits_data;
            end
          end
        end else begin
          if (_T_278) begin
            sbaddr <= io_dmi_req_bits_data;
          end
        end
      end
    end
    if (_T_435) begin
      sbdata <= io_debugmem_resp_bits_data;
    end else begin
      if (_T_432) begin
        if (io_debugmem_resp_valid) begin
          sbdata <= io_debugmem_resp_bits_data;
        end else begin
          if (_T_344) begin
            if (_T_280) begin
              sbdata <= io_dmi_req_bits_data;
            end
          end
        end
      end else begin
        if (_T_344) begin
          if (_T_280) begin
            sbdata <= io_dmi_req_bits_data;
          end
        end
      end
    end
    if (reset) begin
      memreadfire <= 1'h0;
    end else begin
      if (_T_435) begin
        memreadfire <= 1'h0;
      end else begin
        if (_T_432) begin
          memreadfire <= 1'h1;
        end
      end
    end
    if (reset) begin
      coreresetval <= 1'h1;
    end else begin
      if (_T_445) begin
        coreresetval <= 1'h0;
      end
    end
    if (_T_441) begin
      firstreaddone <= 1'h0;
    end else begin
      if (_T_432) begin
        firstreaddone <= 1'h1;
      end
    end
    _T_426 <= io_debugmem_resp_valid;
  end
endmodule
module CtlPath( // @[:@414.2]
  output        io_dmem_req_valid, // @[:@417.4]
  output        io_dmem_req_bits_fcn, // @[:@417.4]
  output [2:0]  io_dmem_req_bits_typ, // @[:@417.4]
  input         io_dmem_resp_valid, // @[:@417.4]
  input  [31:0] io_dat_inst, // @[:@417.4]
  input         io_dat_br_eq, // @[:@417.4]
  input         io_dat_br_lt, // @[:@417.4]
  input         io_dat_br_ltu, // @[:@417.4]
  input         io_dat_csr_eret, // @[:@417.4]
  output        io_ctl_stall, // @[:@417.4]
  output [2:0]  io_ctl_pc_sel, // @[:@417.4]
  output [1:0]  io_ctl_op1_sel, // @[:@417.4]
  output [1:0]  io_ctl_op2_sel, // @[:@417.4]
  output [3:0]  io_ctl_alu_fun, // @[:@417.4]
  output [1:0]  io_ctl_wb_sel, // @[:@417.4]
  output        io_ctl_rf_wen, // @[:@417.4]
  output [2:0]  io_ctl_csr_cmd, // @[:@417.4]
  output        io_ctl_exception // @[:@417.4]
);
  wire [31:0] _T_211; // @[Lookup.scala 9:38:@450.4]
  wire  _T_212; // @[Lookup.scala 9:38:@451.4]
  wire  _T_216; // @[Lookup.scala 9:38:@453.4]
  wire  _T_220; // @[Lookup.scala 9:38:@455.4]
  wire  _T_224; // @[Lookup.scala 9:38:@457.4]
  wire  _T_228; // @[Lookup.scala 9:38:@459.4]
  wire  _T_232; // @[Lookup.scala 9:38:@461.4]
  wire  _T_236; // @[Lookup.scala 9:38:@463.4]
  wire  _T_240; // @[Lookup.scala 9:38:@465.4]
  wire [31:0] _T_243; // @[Lookup.scala 9:38:@466.4]
  wire  _T_244; // @[Lookup.scala 9:38:@467.4]
  wire  _T_248; // @[Lookup.scala 9:38:@469.4]
  wire  _T_252; // @[Lookup.scala 9:38:@471.4]
  wire  _T_256; // @[Lookup.scala 9:38:@473.4]
  wire  _T_260; // @[Lookup.scala 9:38:@475.4]
  wire  _T_264; // @[Lookup.scala 9:38:@477.4]
  wire  _T_268; // @[Lookup.scala 9:38:@479.4]
  wire  _T_272; // @[Lookup.scala 9:38:@481.4]
  wire [31:0] _T_275; // @[Lookup.scala 9:38:@482.4]
  wire  _T_276; // @[Lookup.scala 9:38:@483.4]
  wire  _T_280; // @[Lookup.scala 9:38:@485.4]
  wire  _T_284; // @[Lookup.scala 9:38:@487.4]
  wire [31:0] _T_287; // @[Lookup.scala 9:38:@488.4]
  wire  _T_288; // @[Lookup.scala 9:38:@489.4]
  wire  _T_292; // @[Lookup.scala 9:38:@491.4]
  wire  _T_296; // @[Lookup.scala 9:38:@493.4]
  wire  _T_300; // @[Lookup.scala 9:38:@495.4]
  wire  _T_304; // @[Lookup.scala 9:38:@497.4]
  wire  _T_308; // @[Lookup.scala 9:38:@499.4]
  wire  _T_312; // @[Lookup.scala 9:38:@501.4]
  wire  _T_316; // @[Lookup.scala 9:38:@503.4]
  wire  _T_320; // @[Lookup.scala 9:38:@505.4]
  wire  _T_324; // @[Lookup.scala 9:38:@507.4]
  wire  _T_328; // @[Lookup.scala 9:38:@509.4]
  wire  _T_332; // @[Lookup.scala 9:38:@511.4]
  wire  _T_336; // @[Lookup.scala 9:38:@513.4]
  wire  _T_340; // @[Lookup.scala 9:38:@515.4]
  wire  _T_344; // @[Lookup.scala 9:38:@517.4]
  wire  _T_348; // @[Lookup.scala 9:38:@519.4]
  wire  _T_352; // @[Lookup.scala 9:38:@521.4]
  wire  _T_356; // @[Lookup.scala 9:38:@523.4]
  wire  _T_360; // @[Lookup.scala 9:38:@525.4]
  wire  _T_364; // @[Lookup.scala 9:38:@527.4]
  wire  _T_368; // @[Lookup.scala 9:38:@529.4]
  wire  _T_372; // @[Lookup.scala 9:38:@531.4]
  wire  _T_376; // @[Lookup.scala 9:38:@533.4]
  wire  _T_380; // @[Lookup.scala 9:38:@535.4]
  wire  _T_384; // @[Lookup.scala 9:38:@537.4]
  wire  _T_388; // @[Lookup.scala 9:38:@539.4]
  wire  _T_392; // @[Lookup.scala 9:38:@541.4]
  wire  _T_396; // @[Lookup.scala 9:38:@543.4]
  wire  _T_400; // @[Lookup.scala 9:38:@545.4]
  wire  _T_404; // @[Lookup.scala 9:38:@547.4]
  wire  _T_408; // @[Lookup.scala 9:38:@549.4]
  wire  _T_410; // @[Lookup.scala 11:37:@551.4]
  wire  _T_411; // @[Lookup.scala 11:37:@552.4]
  wire  _T_412; // @[Lookup.scala 11:37:@553.4]
  wire  _T_413; // @[Lookup.scala 11:37:@554.4]
  wire  _T_414; // @[Lookup.scala 11:37:@555.4]
  wire  _T_415; // @[Lookup.scala 11:37:@556.4]
  wire  _T_416; // @[Lookup.scala 11:37:@557.4]
  wire  _T_417; // @[Lookup.scala 11:37:@558.4]
  wire  _T_418; // @[Lookup.scala 11:37:@559.4]
  wire  _T_419; // @[Lookup.scala 11:37:@560.4]
  wire  _T_420; // @[Lookup.scala 11:37:@561.4]
  wire  _T_421; // @[Lookup.scala 11:37:@562.4]
  wire  _T_422; // @[Lookup.scala 11:37:@563.4]
  wire  _T_423; // @[Lookup.scala 11:37:@564.4]
  wire  _T_424; // @[Lookup.scala 11:37:@565.4]
  wire  _T_425; // @[Lookup.scala 11:37:@566.4]
  wire  _T_426; // @[Lookup.scala 11:37:@567.4]
  wire  _T_427; // @[Lookup.scala 11:37:@568.4]
  wire  _T_428; // @[Lookup.scala 11:37:@569.4]
  wire  _T_429; // @[Lookup.scala 11:37:@570.4]
  wire  _T_430; // @[Lookup.scala 11:37:@571.4]
  wire  _T_431; // @[Lookup.scala 11:37:@572.4]
  wire  _T_432; // @[Lookup.scala 11:37:@573.4]
  wire  _T_433; // @[Lookup.scala 11:37:@574.4]
  wire  _T_434; // @[Lookup.scala 11:37:@575.4]
  wire  _T_435; // @[Lookup.scala 11:37:@576.4]
  wire  _T_436; // @[Lookup.scala 11:37:@577.4]
  wire  _T_437; // @[Lookup.scala 11:37:@578.4]
  wire  _T_438; // @[Lookup.scala 11:37:@579.4]
  wire  _T_439; // @[Lookup.scala 11:37:@580.4]
  wire  _T_440; // @[Lookup.scala 11:37:@581.4]
  wire  _T_441; // @[Lookup.scala 11:37:@582.4]
  wire  _T_442; // @[Lookup.scala 11:37:@583.4]
  wire  _T_443; // @[Lookup.scala 11:37:@584.4]
  wire  _T_444; // @[Lookup.scala 11:37:@585.4]
  wire  _T_445; // @[Lookup.scala 11:37:@586.4]
  wire  _T_446; // @[Lookup.scala 11:37:@587.4]
  wire  _T_447; // @[Lookup.scala 11:37:@588.4]
  wire  _T_448; // @[Lookup.scala 11:37:@589.4]
  wire  _T_449; // @[Lookup.scala 11:37:@590.4]
  wire  _T_450; // @[Lookup.scala 11:37:@591.4]
  wire  _T_451; // @[Lookup.scala 11:37:@592.4]
  wire  _T_452; // @[Lookup.scala 11:37:@593.4]
  wire  _T_453; // @[Lookup.scala 11:37:@594.4]
  wire  _T_454; // @[Lookup.scala 11:37:@595.4]
  wire  _T_455; // @[Lookup.scala 11:37:@596.4]
  wire  _T_456; // @[Lookup.scala 11:37:@597.4]
  wire  _T_457; // @[Lookup.scala 11:37:@598.4]
  wire  cs_val_inst; // @[Lookup.scala 11:37:@599.4]
  wire [3:0] _T_471; // @[Lookup.scala 11:37:@613.4]
  wire [3:0] _T_472; // @[Lookup.scala 11:37:@614.4]
  wire [3:0] _T_473; // @[Lookup.scala 11:37:@615.4]
  wire [3:0] _T_474; // @[Lookup.scala 11:37:@616.4]
  wire [3:0] _T_475; // @[Lookup.scala 11:37:@617.4]
  wire [3:0] _T_476; // @[Lookup.scala 11:37:@618.4]
  wire [3:0] _T_477; // @[Lookup.scala 11:37:@619.4]
  wire [3:0] _T_478; // @[Lookup.scala 11:37:@620.4]
  wire [3:0] _T_479; // @[Lookup.scala 11:37:@621.4]
  wire [3:0] _T_480; // @[Lookup.scala 11:37:@622.4]
  wire [3:0] _T_481; // @[Lookup.scala 11:37:@623.4]
  wire [3:0] _T_482; // @[Lookup.scala 11:37:@624.4]
  wire [3:0] _T_483; // @[Lookup.scala 11:37:@625.4]
  wire [3:0] _T_484; // @[Lookup.scala 11:37:@626.4]
  wire [3:0] _T_485; // @[Lookup.scala 11:37:@627.4]
  wire [3:0] _T_486; // @[Lookup.scala 11:37:@628.4]
  wire [3:0] _T_487; // @[Lookup.scala 11:37:@629.4]
  wire [3:0] _T_488; // @[Lookup.scala 11:37:@630.4]
  wire [3:0] _T_489; // @[Lookup.scala 11:37:@631.4]
  wire [3:0] _T_490; // @[Lookup.scala 11:37:@632.4]
  wire [3:0] _T_491; // @[Lookup.scala 11:37:@633.4]
  wire [3:0] _T_492; // @[Lookup.scala 11:37:@634.4]
  wire [3:0] _T_493; // @[Lookup.scala 11:37:@635.4]
  wire [3:0] _T_494; // @[Lookup.scala 11:37:@636.4]
  wire [3:0] _T_495; // @[Lookup.scala 11:37:@637.4]
  wire [3:0] _T_496; // @[Lookup.scala 11:37:@638.4]
  wire [3:0] _T_497; // @[Lookup.scala 11:37:@639.4]
  wire [3:0] _T_498; // @[Lookup.scala 11:37:@640.4]
  wire [3:0] _T_499; // @[Lookup.scala 11:37:@641.4]
  wire [3:0] _T_500; // @[Lookup.scala 11:37:@642.4]
  wire [3:0] _T_501; // @[Lookup.scala 11:37:@643.4]
  wire [3:0] _T_502; // @[Lookup.scala 11:37:@644.4]
  wire [3:0] _T_503; // @[Lookup.scala 11:37:@645.4]
  wire [3:0] _T_504; // @[Lookup.scala 11:37:@646.4]
  wire [3:0] _T_505; // @[Lookup.scala 11:37:@647.4]
  wire [3:0] _T_506; // @[Lookup.scala 11:37:@648.4]
  wire [3:0] cs_br_type; // @[Lookup.scala 11:37:@649.4]
  wire [1:0] _T_517; // @[Lookup.scala 11:37:@660.4]
  wire [1:0] _T_518; // @[Lookup.scala 11:37:@661.4]
  wire [1:0] _T_519; // @[Lookup.scala 11:37:@662.4]
  wire [1:0] _T_520; // @[Lookup.scala 11:37:@663.4]
  wire [1:0] _T_521; // @[Lookup.scala 11:37:@664.4]
  wire [1:0] _T_522; // @[Lookup.scala 11:37:@665.4]
  wire [1:0] _T_523; // @[Lookup.scala 11:37:@666.4]
  wire [1:0] _T_524; // @[Lookup.scala 11:37:@667.4]
  wire [1:0] _T_525; // @[Lookup.scala 11:37:@668.4]
  wire [1:0] _T_526; // @[Lookup.scala 11:37:@669.4]
  wire [1:0] _T_527; // @[Lookup.scala 11:37:@670.4]
  wire [1:0] _T_528; // @[Lookup.scala 11:37:@671.4]
  wire [1:0] _T_529; // @[Lookup.scala 11:37:@672.4]
  wire [1:0] _T_530; // @[Lookup.scala 11:37:@673.4]
  wire [1:0] _T_531; // @[Lookup.scala 11:37:@674.4]
  wire [1:0] _T_532; // @[Lookup.scala 11:37:@675.4]
  wire [1:0] _T_533; // @[Lookup.scala 11:37:@676.4]
  wire [1:0] _T_534; // @[Lookup.scala 11:37:@677.4]
  wire [1:0] _T_535; // @[Lookup.scala 11:37:@678.4]
  wire [1:0] _T_536; // @[Lookup.scala 11:37:@679.4]
  wire [1:0] _T_537; // @[Lookup.scala 11:37:@680.4]
  wire [1:0] _T_538; // @[Lookup.scala 11:37:@681.4]
  wire [1:0] _T_539; // @[Lookup.scala 11:37:@682.4]
  wire [1:0] _T_540; // @[Lookup.scala 11:37:@683.4]
  wire [1:0] _T_541; // @[Lookup.scala 11:37:@684.4]
  wire [1:0] _T_542; // @[Lookup.scala 11:37:@685.4]
  wire [1:0] _T_543; // @[Lookup.scala 11:37:@686.4]
  wire [1:0] _T_544; // @[Lookup.scala 11:37:@687.4]
  wire [1:0] _T_545; // @[Lookup.scala 11:37:@688.4]
  wire [1:0] _T_546; // @[Lookup.scala 11:37:@689.4]
  wire [1:0] _T_547; // @[Lookup.scala 11:37:@690.4]
  wire [1:0] _T_548; // @[Lookup.scala 11:37:@691.4]
  wire [1:0] _T_549; // @[Lookup.scala 11:37:@692.4]
  wire [1:0] _T_550; // @[Lookup.scala 11:37:@693.4]
  wire [1:0] _T_551; // @[Lookup.scala 11:37:@694.4]
  wire [1:0] _T_552; // @[Lookup.scala 11:37:@695.4]
  wire [1:0] _T_553; // @[Lookup.scala 11:37:@696.4]
  wire [1:0] _T_554; // @[Lookup.scala 11:37:@697.4]
  wire [1:0] _T_555; // @[Lookup.scala 11:37:@698.4]
  wire [1:0] _T_575; // @[Lookup.scala 11:37:@719.4]
  wire [1:0] _T_576; // @[Lookup.scala 11:37:@720.4]
  wire [1:0] _T_577; // @[Lookup.scala 11:37:@721.4]
  wire [1:0] _T_578; // @[Lookup.scala 11:37:@722.4]
  wire [1:0] _T_579; // @[Lookup.scala 11:37:@723.4]
  wire [1:0] _T_580; // @[Lookup.scala 11:37:@724.4]
  wire [1:0] _T_581; // @[Lookup.scala 11:37:@725.4]
  wire [1:0] _T_582; // @[Lookup.scala 11:37:@726.4]
  wire [1:0] _T_583; // @[Lookup.scala 11:37:@727.4]
  wire [1:0] _T_584; // @[Lookup.scala 11:37:@728.4]
  wire [1:0] _T_585; // @[Lookup.scala 11:37:@729.4]
  wire [1:0] _T_586; // @[Lookup.scala 11:37:@730.4]
  wire [1:0] _T_587; // @[Lookup.scala 11:37:@731.4]
  wire [1:0] _T_588; // @[Lookup.scala 11:37:@732.4]
  wire [1:0] _T_589; // @[Lookup.scala 11:37:@733.4]
  wire [1:0] _T_590; // @[Lookup.scala 11:37:@734.4]
  wire [1:0] _T_591; // @[Lookup.scala 11:37:@735.4]
  wire [1:0] _T_592; // @[Lookup.scala 11:37:@736.4]
  wire [1:0] _T_593; // @[Lookup.scala 11:37:@737.4]
  wire [1:0] _T_594; // @[Lookup.scala 11:37:@738.4]
  wire [1:0] _T_595; // @[Lookup.scala 11:37:@739.4]
  wire [1:0] _T_596; // @[Lookup.scala 11:37:@740.4]
  wire [1:0] _T_597; // @[Lookup.scala 11:37:@741.4]
  wire [1:0] _T_598; // @[Lookup.scala 11:37:@742.4]
  wire [1:0] _T_599; // @[Lookup.scala 11:37:@743.4]
  wire [1:0] _T_600; // @[Lookup.scala 11:37:@744.4]
  wire [1:0] _T_601; // @[Lookup.scala 11:37:@745.4]
  wire [1:0] _T_602; // @[Lookup.scala 11:37:@746.4]
  wire [1:0] _T_603; // @[Lookup.scala 11:37:@747.4]
  wire [1:0] _T_604; // @[Lookup.scala 11:37:@748.4]
  wire [3:0] _T_612; // @[Lookup.scala 11:37:@757.4]
  wire [3:0] _T_613; // @[Lookup.scala 11:37:@758.4]
  wire [3:0] _T_614; // @[Lookup.scala 11:37:@759.4]
  wire [3:0] _T_615; // @[Lookup.scala 11:37:@760.4]
  wire [3:0] _T_616; // @[Lookup.scala 11:37:@761.4]
  wire [3:0] _T_617; // @[Lookup.scala 11:37:@762.4]
  wire [3:0] _T_618; // @[Lookup.scala 11:37:@763.4]
  wire [3:0] _T_619; // @[Lookup.scala 11:37:@764.4]
  wire [3:0] _T_620; // @[Lookup.scala 11:37:@765.4]
  wire [3:0] _T_621; // @[Lookup.scala 11:37:@766.4]
  wire [3:0] _T_622; // @[Lookup.scala 11:37:@767.4]
  wire [3:0] _T_623; // @[Lookup.scala 11:37:@768.4]
  wire [3:0] _T_624; // @[Lookup.scala 11:37:@769.4]
  wire [3:0] _T_625; // @[Lookup.scala 11:37:@770.4]
  wire [3:0] _T_626; // @[Lookup.scala 11:37:@771.4]
  wire [3:0] _T_627; // @[Lookup.scala 11:37:@772.4]
  wire [3:0] _T_628; // @[Lookup.scala 11:37:@773.4]
  wire [3:0] _T_629; // @[Lookup.scala 11:37:@774.4]
  wire [3:0] _T_630; // @[Lookup.scala 11:37:@775.4]
  wire [3:0] _T_631; // @[Lookup.scala 11:37:@776.4]
  wire [3:0] _T_632; // @[Lookup.scala 11:37:@777.4]
  wire [3:0] _T_633; // @[Lookup.scala 11:37:@778.4]
  wire [3:0] _T_634; // @[Lookup.scala 11:37:@779.4]
  wire [3:0] _T_635; // @[Lookup.scala 11:37:@780.4]
  wire [3:0] _T_636; // @[Lookup.scala 11:37:@781.4]
  wire [3:0] _T_637; // @[Lookup.scala 11:37:@782.4]
  wire [3:0] _T_638; // @[Lookup.scala 11:37:@783.4]
  wire [3:0] _T_639; // @[Lookup.scala 11:37:@784.4]
  wire [3:0] _T_640; // @[Lookup.scala 11:37:@785.4]
  wire [3:0] _T_641; // @[Lookup.scala 11:37:@786.4]
  wire [3:0] _T_642; // @[Lookup.scala 11:37:@787.4]
  wire [3:0] _T_643; // @[Lookup.scala 11:37:@788.4]
  wire [3:0] _T_644; // @[Lookup.scala 11:37:@789.4]
  wire [3:0] _T_645; // @[Lookup.scala 11:37:@790.4]
  wire [3:0] _T_646; // @[Lookup.scala 11:37:@791.4]
  wire [3:0] _T_647; // @[Lookup.scala 11:37:@792.4]
  wire [3:0] _T_648; // @[Lookup.scala 11:37:@793.4]
  wire [3:0] _T_649; // @[Lookup.scala 11:37:@794.4]
  wire [3:0] _T_650; // @[Lookup.scala 11:37:@795.4]
  wire [3:0] _T_651; // @[Lookup.scala 11:37:@796.4]
  wire [3:0] _T_652; // @[Lookup.scala 11:37:@797.4]
  wire [3:0] _T_653; // @[Lookup.scala 11:37:@798.4]
  wire [1:0] _T_661; // @[Lookup.scala 11:37:@807.4]
  wire [1:0] _T_662; // @[Lookup.scala 11:37:@808.4]
  wire [1:0] _T_663; // @[Lookup.scala 11:37:@809.4]
  wire [1:0] _T_664; // @[Lookup.scala 11:37:@810.4]
  wire [1:0] _T_665; // @[Lookup.scala 11:37:@811.4]
  wire [1:0] _T_666; // @[Lookup.scala 11:37:@812.4]
  wire [1:0] _T_667; // @[Lookup.scala 11:37:@813.4]
  wire [1:0] _T_668; // @[Lookup.scala 11:37:@814.4]
  wire [1:0] _T_669; // @[Lookup.scala 11:37:@815.4]
  wire [1:0] _T_670; // @[Lookup.scala 11:37:@816.4]
  wire [1:0] _T_671; // @[Lookup.scala 11:37:@817.4]
  wire [1:0] _T_672; // @[Lookup.scala 11:37:@818.4]
  wire [1:0] _T_673; // @[Lookup.scala 11:37:@819.4]
  wire [1:0] _T_674; // @[Lookup.scala 11:37:@820.4]
  wire [1:0] _T_675; // @[Lookup.scala 11:37:@821.4]
  wire [1:0] _T_676; // @[Lookup.scala 11:37:@822.4]
  wire [1:0] _T_677; // @[Lookup.scala 11:37:@823.4]
  wire [1:0] _T_678; // @[Lookup.scala 11:37:@824.4]
  wire [1:0] _T_679; // @[Lookup.scala 11:37:@825.4]
  wire [1:0] _T_680; // @[Lookup.scala 11:37:@826.4]
  wire [1:0] _T_681; // @[Lookup.scala 11:37:@827.4]
  wire [1:0] _T_682; // @[Lookup.scala 11:37:@828.4]
  wire [1:0] _T_683; // @[Lookup.scala 11:37:@829.4]
  wire [1:0] _T_684; // @[Lookup.scala 11:37:@830.4]
  wire [1:0] _T_685; // @[Lookup.scala 11:37:@831.4]
  wire [1:0] _T_686; // @[Lookup.scala 11:37:@832.4]
  wire [1:0] _T_687; // @[Lookup.scala 11:37:@833.4]
  wire [1:0] _T_688; // @[Lookup.scala 11:37:@834.4]
  wire [1:0] _T_689; // @[Lookup.scala 11:37:@835.4]
  wire [1:0] _T_690; // @[Lookup.scala 11:37:@836.4]
  wire [1:0] _T_691; // @[Lookup.scala 11:37:@837.4]
  wire [1:0] _T_692; // @[Lookup.scala 11:37:@838.4]
  wire [1:0] _T_693; // @[Lookup.scala 11:37:@839.4]
  wire [1:0] _T_694; // @[Lookup.scala 11:37:@840.4]
  wire [1:0] _T_695; // @[Lookup.scala 11:37:@841.4]
  wire [1:0] _T_696; // @[Lookup.scala 11:37:@842.4]
  wire [1:0] _T_697; // @[Lookup.scala 11:37:@843.4]
  wire [1:0] _T_698; // @[Lookup.scala 11:37:@844.4]
  wire [1:0] _T_699; // @[Lookup.scala 11:37:@845.4]
  wire [1:0] _T_700; // @[Lookup.scala 11:37:@846.4]
  wire [1:0] _T_701; // @[Lookup.scala 11:37:@847.4]
  wire [1:0] _T_702; // @[Lookup.scala 11:37:@848.4]
  wire  _T_711; // @[Lookup.scala 11:37:@858.4]
  wire  _T_712; // @[Lookup.scala 11:37:@859.4]
  wire  _T_713; // @[Lookup.scala 11:37:@860.4]
  wire  _T_714; // @[Lookup.scala 11:37:@861.4]
  wire  _T_715; // @[Lookup.scala 11:37:@862.4]
  wire  _T_716; // @[Lookup.scala 11:37:@863.4]
  wire  _T_717; // @[Lookup.scala 11:37:@864.4]
  wire  _T_718; // @[Lookup.scala 11:37:@865.4]
  wire  _T_719; // @[Lookup.scala 11:37:@866.4]
  wire  _T_720; // @[Lookup.scala 11:37:@867.4]
  wire  _T_721; // @[Lookup.scala 11:37:@868.4]
  wire  _T_722; // @[Lookup.scala 11:37:@869.4]
  wire  _T_723; // @[Lookup.scala 11:37:@870.4]
  wire  _T_724; // @[Lookup.scala 11:37:@871.4]
  wire  _T_725; // @[Lookup.scala 11:37:@872.4]
  wire  _T_726; // @[Lookup.scala 11:37:@873.4]
  wire  _T_727; // @[Lookup.scala 11:37:@874.4]
  wire  _T_728; // @[Lookup.scala 11:37:@875.4]
  wire  _T_729; // @[Lookup.scala 11:37:@876.4]
  wire  _T_730; // @[Lookup.scala 11:37:@877.4]
  wire  _T_731; // @[Lookup.scala 11:37:@878.4]
  wire  _T_732; // @[Lookup.scala 11:37:@879.4]
  wire  _T_733; // @[Lookup.scala 11:37:@880.4]
  wire  _T_734; // @[Lookup.scala 11:37:@881.4]
  wire  _T_735; // @[Lookup.scala 11:37:@882.4]
  wire  _T_736; // @[Lookup.scala 11:37:@883.4]
  wire  _T_737; // @[Lookup.scala 11:37:@884.4]
  wire  _T_738; // @[Lookup.scala 11:37:@885.4]
  wire  _T_739; // @[Lookup.scala 11:37:@886.4]
  wire  _T_740; // @[Lookup.scala 11:37:@887.4]
  wire  _T_741; // @[Lookup.scala 11:37:@888.4]
  wire  _T_742; // @[Lookup.scala 11:37:@889.4]
  wire  _T_743; // @[Lookup.scala 11:37:@890.4]
  wire  _T_744; // @[Lookup.scala 11:37:@891.4]
  wire  _T_745; // @[Lookup.scala 11:37:@892.4]
  wire  _T_746; // @[Lookup.scala 11:37:@893.4]
  wire  _T_747; // @[Lookup.scala 11:37:@894.4]
  wire  _T_748; // @[Lookup.scala 11:37:@895.4]
  wire  _T_749; // @[Lookup.scala 11:37:@896.4]
  wire  _T_750; // @[Lookup.scala 11:37:@897.4]
  wire  _T_751; // @[Lookup.scala 11:37:@898.4]
  wire  cs0_2; // @[Lookup.scala 11:37:@899.4]
  wire  _T_753; // @[Lookup.scala 11:37:@901.4]
  wire  _T_754; // @[Lookup.scala 11:37:@902.4]
  wire  _T_755; // @[Lookup.scala 11:37:@903.4]
  wire  _T_756; // @[Lookup.scala 11:37:@904.4]
  wire  _T_757; // @[Lookup.scala 11:37:@905.4]
  wire  _T_758; // @[Lookup.scala 11:37:@906.4]
  wire  _T_759; // @[Lookup.scala 11:37:@907.4]
  wire  _T_760; // @[Lookup.scala 11:37:@908.4]
  wire  _T_761; // @[Lookup.scala 11:37:@909.4]
  wire  _T_762; // @[Lookup.scala 11:37:@910.4]
  wire  _T_763; // @[Lookup.scala 11:37:@911.4]
  wire  _T_764; // @[Lookup.scala 11:37:@912.4]
  wire  _T_765; // @[Lookup.scala 11:37:@913.4]
  wire  _T_766; // @[Lookup.scala 11:37:@914.4]
  wire  _T_767; // @[Lookup.scala 11:37:@915.4]
  wire  _T_768; // @[Lookup.scala 11:37:@916.4]
  wire  _T_769; // @[Lookup.scala 11:37:@917.4]
  wire  _T_770; // @[Lookup.scala 11:37:@918.4]
  wire  _T_771; // @[Lookup.scala 11:37:@919.4]
  wire  _T_772; // @[Lookup.scala 11:37:@920.4]
  wire  _T_773; // @[Lookup.scala 11:37:@921.4]
  wire  _T_774; // @[Lookup.scala 11:37:@922.4]
  wire  _T_775; // @[Lookup.scala 11:37:@923.4]
  wire  _T_776; // @[Lookup.scala 11:37:@924.4]
  wire  _T_777; // @[Lookup.scala 11:37:@925.4]
  wire  _T_778; // @[Lookup.scala 11:37:@926.4]
  wire  _T_779; // @[Lookup.scala 11:37:@927.4]
  wire  _T_780; // @[Lookup.scala 11:37:@928.4]
  wire  _T_781; // @[Lookup.scala 11:37:@929.4]
  wire  _T_782; // @[Lookup.scala 11:37:@930.4]
  wire  _T_783; // @[Lookup.scala 11:37:@931.4]
  wire  _T_784; // @[Lookup.scala 11:37:@932.4]
  wire  _T_785; // @[Lookup.scala 11:37:@933.4]
  wire  _T_786; // @[Lookup.scala 11:37:@934.4]
  wire  _T_787; // @[Lookup.scala 11:37:@935.4]
  wire  _T_788; // @[Lookup.scala 11:37:@936.4]
  wire  _T_789; // @[Lookup.scala 11:37:@937.4]
  wire  _T_790; // @[Lookup.scala 11:37:@938.4]
  wire  _T_791; // @[Lookup.scala 11:37:@939.4]
  wire  _T_792; // @[Lookup.scala 11:37:@940.4]
  wire  _T_793; // @[Lookup.scala 11:37:@941.4]
  wire  _T_794; // @[Lookup.scala 11:37:@942.4]
  wire  _T_795; // @[Lookup.scala 11:37:@943.4]
  wire  _T_796; // @[Lookup.scala 11:37:@944.4]
  wire  _T_797; // @[Lookup.scala 11:37:@945.4]
  wire  _T_798; // @[Lookup.scala 11:37:@946.4]
  wire  _T_799; // @[Lookup.scala 11:37:@947.4]
  wire  _T_800; // @[Lookup.scala 11:37:@948.4]
  wire  cs0_3; // @[Lookup.scala 11:37:@949.4]
  wire  _T_844; // @[Lookup.scala 11:37:@993.4]
  wire  _T_845; // @[Lookup.scala 11:37:@994.4]
  wire  _T_846; // @[Lookup.scala 11:37:@995.4]
  wire  _T_847; // @[Lookup.scala 11:37:@996.4]
  wire  _T_848; // @[Lookup.scala 11:37:@997.4]
  wire  _T_849; // @[Lookup.scala 11:37:@998.4]
  wire [2:0] _T_892; // @[Lookup.scala 11:37:@1042.4]
  wire [2:0] _T_893; // @[Lookup.scala 11:37:@1043.4]
  wire [2:0] _T_894; // @[Lookup.scala 11:37:@1044.4]
  wire [2:0] _T_895; // @[Lookup.scala 11:37:@1045.4]
  wire [2:0] _T_896; // @[Lookup.scala 11:37:@1046.4]
  wire [2:0] _T_897; // @[Lookup.scala 11:37:@1047.4]
  wire [2:0] _T_898; // @[Lookup.scala 11:37:@1048.4]
  wire [2:0] _T_902; // @[Lookup.scala 11:37:@1053.4]
  wire [2:0] _T_903; // @[Lookup.scala 11:37:@1054.4]
  wire [2:0] _T_904; // @[Lookup.scala 11:37:@1055.4]
  wire [2:0] _T_905; // @[Lookup.scala 11:37:@1056.4]
  wire [2:0] _T_906; // @[Lookup.scala 11:37:@1057.4]
  wire [2:0] _T_907; // @[Lookup.scala 11:37:@1058.4]
  wire [2:0] _T_908; // @[Lookup.scala 11:37:@1059.4]
  wire [2:0] _T_909; // @[Lookup.scala 11:37:@1060.4]
  wire [2:0] _T_910; // @[Lookup.scala 11:37:@1061.4]
  wire [2:0] _T_911; // @[Lookup.scala 11:37:@1062.4]
  wire [2:0] _T_912; // @[Lookup.scala 11:37:@1063.4]
  wire [2:0] _T_913; // @[Lookup.scala 11:37:@1064.4]
  wire [2:0] _T_914; // @[Lookup.scala 11:37:@1065.4]
  wire [2:0] _T_915; // @[Lookup.scala 11:37:@1066.4]
  wire [2:0] _T_916; // @[Lookup.scala 11:37:@1067.4]
  wire [2:0] _T_917; // @[Lookup.scala 11:37:@1068.4]
  wire [2:0] _T_918; // @[Lookup.scala 11:37:@1069.4]
  wire [2:0] _T_919; // @[Lookup.scala 11:37:@1070.4]
  wire [2:0] _T_920; // @[Lookup.scala 11:37:@1071.4]
  wire [2:0] _T_921; // @[Lookup.scala 11:37:@1072.4]
  wire [2:0] _T_922; // @[Lookup.scala 11:37:@1073.4]
  wire [2:0] _T_923; // @[Lookup.scala 11:37:@1074.4]
  wire [2:0] _T_924; // @[Lookup.scala 11:37:@1075.4]
  wire [2:0] _T_925; // @[Lookup.scala 11:37:@1076.4]
  wire [2:0] _T_926; // @[Lookup.scala 11:37:@1077.4]
  wire [2:0] _T_927; // @[Lookup.scala 11:37:@1078.4]
  wire [2:0] _T_928; // @[Lookup.scala 11:37:@1079.4]
  wire [2:0] _T_929; // @[Lookup.scala 11:37:@1080.4]
  wire [2:0] _T_930; // @[Lookup.scala 11:37:@1081.4]
  wire [2:0] _T_931; // @[Lookup.scala 11:37:@1082.4]
  wire [2:0] _T_932; // @[Lookup.scala 11:37:@1083.4]
  wire [2:0] _T_933; // @[Lookup.scala 11:37:@1084.4]
  wire [2:0] _T_934; // @[Lookup.scala 11:37:@1085.4]
  wire [2:0] _T_935; // @[Lookup.scala 11:37:@1086.4]
  wire [2:0] _T_936; // @[Lookup.scala 11:37:@1087.4]
  wire [2:0] _T_937; // @[Lookup.scala 11:37:@1088.4]
  wire [2:0] _T_938; // @[Lookup.scala 11:37:@1089.4]
  wire [2:0] _T_939; // @[Lookup.scala 11:37:@1090.4]
  wire [2:0] _T_940; // @[Lookup.scala 11:37:@1091.4]
  wire [2:0] _T_941; // @[Lookup.scala 11:37:@1092.4]
  wire [2:0] _T_942; // @[Lookup.scala 11:37:@1093.4]
  wire [2:0] _T_943; // @[Lookup.scala 11:37:@1094.4]
  wire [2:0] _T_944; // @[Lookup.scala 11:37:@1095.4]
  wire [2:0] _T_945; // @[Lookup.scala 11:37:@1096.4]
  wire [2:0] _T_946; // @[Lookup.scala 11:37:@1097.4]
  wire [2:0] _T_947; // @[Lookup.scala 11:37:@1098.4]
  wire [2:0] cs0_6; // @[Lookup.scala 11:37:@1099.4]
  wire  _T_948; // @[cpath.scala 118:43:@1100.4]
  wire  _T_949; // @[cpath.scala 120:37:@1101.4]
  wire  _T_950; // @[cpath.scala 121:37:@1102.4]
  wire  _T_952; // @[cpath.scala 121:54:@1103.4]
  wire [2:0] _T_953; // @[cpath.scala 121:53:@1104.4]
  wire  _T_954; // @[cpath.scala 122:37:@1105.4]
  wire [2:0] _T_955; // @[cpath.scala 122:53:@1106.4]
  wire  _T_956; // @[cpath.scala 123:37:@1107.4]
  wire  _T_958; // @[cpath.scala 123:54:@1108.4]
  wire [2:0] _T_959; // @[cpath.scala 123:53:@1109.4]
  wire  _T_960; // @[cpath.scala 124:37:@1110.4]
  wire  _T_962; // @[cpath.scala 124:54:@1111.4]
  wire [2:0] _T_963; // @[cpath.scala 124:53:@1112.4]
  wire  _T_964; // @[cpath.scala 125:37:@1113.4]
  wire [2:0] _T_965; // @[cpath.scala 125:53:@1114.4]
  wire  _T_966; // @[cpath.scala 126:37:@1115.4]
  wire [2:0] _T_967; // @[cpath.scala 126:53:@1116.4]
  wire  _T_968; // @[cpath.scala 127:37:@1117.4]
  wire  _T_969; // @[cpath.scala 128:37:@1118.4]
  wire [2:0] _T_970; // @[cpath.scala 128:25:@1119.4]
  wire [2:0] _T_971; // @[cpath.scala 127:25:@1120.4]
  wire [2:0] _T_972; // @[cpath.scala 126:25:@1121.4]
  wire [2:0] _T_973; // @[cpath.scala 125:25:@1122.4]
  wire [2:0] _T_974; // @[cpath.scala 124:25:@1123.4]
  wire [2:0] _T_975; // @[cpath.scala 123:25:@1124.4]
  wire [2:0] _T_976; // @[cpath.scala 122:25:@1125.4]
  wire [2:0] _T_977; // @[cpath.scala 121:25:@1126.4]
  wire [2:0] _T_978; // @[cpath.scala 120:25:@1127.4]
  wire  _T_981; // @[cpath.scala 131:53:@1130.4]
  wire  _T_983; // @[cpath.scala 131:79:@1131.4]
  wire  _T_984; // @[cpath.scala 131:76:@1132.4]
  wire  stall; // @[cpath.scala 131:40:@1133.4]
  wire  _T_987; // @[cpath.scala 140:33:@1141.4]
  wire [4:0] rs1_addr; // @[cpath.scala 143:30:@1144.4]
  wire  _T_990; // @[cpath.scala 144:30:@1145.4]
  wire  _T_991; // @[cpath.scala 144:54:@1146.4]
  wire  _T_992; // @[cpath.scala 144:40:@1147.4]
  wire  _T_994; // @[cpath.scala 144:77:@1148.4]
  wire  csr_ren; // @[cpath.scala 144:65:@1149.4]
  wire [2:0] csr_cmd; // @[cpath.scala 145:21:@1150.4]
  assign _T_211 = io_dat_inst & 32'h707f; // @[Lookup.scala 9:38:@450.4]
  assign _T_212 = 32'h2003 == _T_211; // @[Lookup.scala 9:38:@451.4]
  assign _T_216 = 32'h3 == _T_211; // @[Lookup.scala 9:38:@453.4]
  assign _T_220 = 32'h4003 == _T_211; // @[Lookup.scala 9:38:@455.4]
  assign _T_224 = 32'h1003 == _T_211; // @[Lookup.scala 9:38:@457.4]
  assign _T_228 = 32'h5003 == _T_211; // @[Lookup.scala 9:38:@459.4]
  assign _T_232 = 32'h2023 == _T_211; // @[Lookup.scala 9:38:@461.4]
  assign _T_236 = 32'h23 == _T_211; // @[Lookup.scala 9:38:@463.4]
  assign _T_240 = 32'h1023 == _T_211; // @[Lookup.scala 9:38:@465.4]
  assign _T_243 = io_dat_inst & 32'h7f; // @[Lookup.scala 9:38:@466.4]
  assign _T_244 = 32'h17 == _T_243; // @[Lookup.scala 9:38:@467.4]
  assign _T_248 = 32'h37 == _T_243; // @[Lookup.scala 9:38:@469.4]
  assign _T_252 = 32'h13 == _T_211; // @[Lookup.scala 9:38:@471.4]
  assign _T_256 = 32'h7013 == _T_211; // @[Lookup.scala 9:38:@473.4]
  assign _T_260 = 32'h6013 == _T_211; // @[Lookup.scala 9:38:@475.4]
  assign _T_264 = 32'h4013 == _T_211; // @[Lookup.scala 9:38:@477.4]
  assign _T_268 = 32'h2013 == _T_211; // @[Lookup.scala 9:38:@479.4]
  assign _T_272 = 32'h3013 == _T_211; // @[Lookup.scala 9:38:@481.4]
  assign _T_275 = io_dat_inst & 32'hfc00707f; // @[Lookup.scala 9:38:@482.4]
  assign _T_276 = 32'h1013 == _T_275; // @[Lookup.scala 9:38:@483.4]
  assign _T_280 = 32'h40005013 == _T_275; // @[Lookup.scala 9:38:@485.4]
  assign _T_284 = 32'h5013 == _T_275; // @[Lookup.scala 9:38:@487.4]
  assign _T_287 = io_dat_inst & 32'hfe00707f; // @[Lookup.scala 9:38:@488.4]
  assign _T_288 = 32'h1033 == _T_287; // @[Lookup.scala 9:38:@489.4]
  assign _T_292 = 32'h33 == _T_287; // @[Lookup.scala 9:38:@491.4]
  assign _T_296 = 32'h40000033 == _T_287; // @[Lookup.scala 9:38:@493.4]
  assign _T_300 = 32'h2033 == _T_287; // @[Lookup.scala 9:38:@495.4]
  assign _T_304 = 32'h3033 == _T_287; // @[Lookup.scala 9:38:@497.4]
  assign _T_308 = 32'h7033 == _T_287; // @[Lookup.scala 9:38:@499.4]
  assign _T_312 = 32'h6033 == _T_287; // @[Lookup.scala 9:38:@501.4]
  assign _T_316 = 32'h4033 == _T_287; // @[Lookup.scala 9:38:@503.4]
  assign _T_320 = 32'h40005033 == _T_287; // @[Lookup.scala 9:38:@505.4]
  assign _T_324 = 32'h5033 == _T_287; // @[Lookup.scala 9:38:@507.4]
  assign _T_328 = 32'h6f == _T_243; // @[Lookup.scala 9:38:@509.4]
  assign _T_332 = 32'h67 == _T_211; // @[Lookup.scala 9:38:@511.4]
  assign _T_336 = 32'h63 == _T_211; // @[Lookup.scala 9:38:@513.4]
  assign _T_340 = 32'h1063 == _T_211; // @[Lookup.scala 9:38:@515.4]
  assign _T_344 = 32'h5063 == _T_211; // @[Lookup.scala 9:38:@517.4]
  assign _T_348 = 32'h7063 == _T_211; // @[Lookup.scala 9:38:@519.4]
  assign _T_352 = 32'h4063 == _T_211; // @[Lookup.scala 9:38:@521.4]
  assign _T_356 = 32'h6063 == _T_211; // @[Lookup.scala 9:38:@523.4]
  assign _T_360 = 32'h5073 == _T_211; // @[Lookup.scala 9:38:@525.4]
  assign _T_364 = 32'h6073 == _T_211; // @[Lookup.scala 9:38:@527.4]
  assign _T_368 = 32'h7073 == _T_211; // @[Lookup.scala 9:38:@529.4]
  assign _T_372 = 32'h1073 == _T_211; // @[Lookup.scala 9:38:@531.4]
  assign _T_376 = 32'h2073 == _T_211; // @[Lookup.scala 9:38:@533.4]
  assign _T_380 = 32'h3073 == _T_211; // @[Lookup.scala 9:38:@535.4]
  assign _T_384 = 32'h73 == io_dat_inst; // @[Lookup.scala 9:38:@537.4]
  assign _T_388 = 32'h30200073 == io_dat_inst; // @[Lookup.scala 9:38:@539.4]
  assign _T_392 = 32'h7b200073 == io_dat_inst; // @[Lookup.scala 9:38:@541.4]
  assign _T_396 = 32'h100073 == io_dat_inst; // @[Lookup.scala 9:38:@543.4]
  assign _T_400 = 32'h10500073 == io_dat_inst; // @[Lookup.scala 9:38:@545.4]
  assign _T_404 = 32'h100f == _T_211; // @[Lookup.scala 9:38:@547.4]
  assign _T_408 = 32'hf == _T_211; // @[Lookup.scala 9:38:@549.4]
  assign _T_410 = _T_404 ? 1'h1 : _T_408; // @[Lookup.scala 11:37:@551.4]
  assign _T_411 = _T_400 ? 1'h1 : _T_410; // @[Lookup.scala 11:37:@552.4]
  assign _T_412 = _T_396 ? 1'h1 : _T_411; // @[Lookup.scala 11:37:@553.4]
  assign _T_413 = _T_392 ? 1'h1 : _T_412; // @[Lookup.scala 11:37:@554.4]
  assign _T_414 = _T_388 ? 1'h1 : _T_413; // @[Lookup.scala 11:37:@555.4]
  assign _T_415 = _T_384 ? 1'h1 : _T_414; // @[Lookup.scala 11:37:@556.4]
  assign _T_416 = _T_380 ? 1'h1 : _T_415; // @[Lookup.scala 11:37:@557.4]
  assign _T_417 = _T_376 ? 1'h1 : _T_416; // @[Lookup.scala 11:37:@558.4]
  assign _T_418 = _T_372 ? 1'h1 : _T_417; // @[Lookup.scala 11:37:@559.4]
  assign _T_419 = _T_368 ? 1'h1 : _T_418; // @[Lookup.scala 11:37:@560.4]
  assign _T_420 = _T_364 ? 1'h1 : _T_419; // @[Lookup.scala 11:37:@561.4]
  assign _T_421 = _T_360 ? 1'h1 : _T_420; // @[Lookup.scala 11:37:@562.4]
  assign _T_422 = _T_356 ? 1'h1 : _T_421; // @[Lookup.scala 11:37:@563.4]
  assign _T_423 = _T_352 ? 1'h1 : _T_422; // @[Lookup.scala 11:37:@564.4]
  assign _T_424 = _T_348 ? 1'h1 : _T_423; // @[Lookup.scala 11:37:@565.4]
  assign _T_425 = _T_344 ? 1'h1 : _T_424; // @[Lookup.scala 11:37:@566.4]
  assign _T_426 = _T_340 ? 1'h1 : _T_425; // @[Lookup.scala 11:37:@567.4]
  assign _T_427 = _T_336 ? 1'h1 : _T_426; // @[Lookup.scala 11:37:@568.4]
  assign _T_428 = _T_332 ? 1'h1 : _T_427; // @[Lookup.scala 11:37:@569.4]
  assign _T_429 = _T_328 ? 1'h1 : _T_428; // @[Lookup.scala 11:37:@570.4]
  assign _T_430 = _T_324 ? 1'h1 : _T_429; // @[Lookup.scala 11:37:@571.4]
  assign _T_431 = _T_320 ? 1'h1 : _T_430; // @[Lookup.scala 11:37:@572.4]
  assign _T_432 = _T_316 ? 1'h1 : _T_431; // @[Lookup.scala 11:37:@573.4]
  assign _T_433 = _T_312 ? 1'h1 : _T_432; // @[Lookup.scala 11:37:@574.4]
  assign _T_434 = _T_308 ? 1'h1 : _T_433; // @[Lookup.scala 11:37:@575.4]
  assign _T_435 = _T_304 ? 1'h1 : _T_434; // @[Lookup.scala 11:37:@576.4]
  assign _T_436 = _T_300 ? 1'h1 : _T_435; // @[Lookup.scala 11:37:@577.4]
  assign _T_437 = _T_296 ? 1'h1 : _T_436; // @[Lookup.scala 11:37:@578.4]
  assign _T_438 = _T_292 ? 1'h1 : _T_437; // @[Lookup.scala 11:37:@579.4]
  assign _T_439 = _T_288 ? 1'h1 : _T_438; // @[Lookup.scala 11:37:@580.4]
  assign _T_440 = _T_284 ? 1'h1 : _T_439; // @[Lookup.scala 11:37:@581.4]
  assign _T_441 = _T_280 ? 1'h1 : _T_440; // @[Lookup.scala 11:37:@582.4]
  assign _T_442 = _T_276 ? 1'h1 : _T_441; // @[Lookup.scala 11:37:@583.4]
  assign _T_443 = _T_272 ? 1'h1 : _T_442; // @[Lookup.scala 11:37:@584.4]
  assign _T_444 = _T_268 ? 1'h1 : _T_443; // @[Lookup.scala 11:37:@585.4]
  assign _T_445 = _T_264 ? 1'h1 : _T_444; // @[Lookup.scala 11:37:@586.4]
  assign _T_446 = _T_260 ? 1'h1 : _T_445; // @[Lookup.scala 11:37:@587.4]
  assign _T_447 = _T_256 ? 1'h1 : _T_446; // @[Lookup.scala 11:37:@588.4]
  assign _T_448 = _T_252 ? 1'h1 : _T_447; // @[Lookup.scala 11:37:@589.4]
  assign _T_449 = _T_248 ? 1'h1 : _T_448; // @[Lookup.scala 11:37:@590.4]
  assign _T_450 = _T_244 ? 1'h1 : _T_449; // @[Lookup.scala 11:37:@591.4]
  assign _T_451 = _T_240 ? 1'h1 : _T_450; // @[Lookup.scala 11:37:@592.4]
  assign _T_452 = _T_236 ? 1'h1 : _T_451; // @[Lookup.scala 11:37:@593.4]
  assign _T_453 = _T_232 ? 1'h1 : _T_452; // @[Lookup.scala 11:37:@594.4]
  assign _T_454 = _T_228 ? 1'h1 : _T_453; // @[Lookup.scala 11:37:@595.4]
  assign _T_455 = _T_224 ? 1'h1 : _T_454; // @[Lookup.scala 11:37:@596.4]
  assign _T_456 = _T_220 ? 1'h1 : _T_455; // @[Lookup.scala 11:37:@597.4]
  assign _T_457 = _T_216 ? 1'h1 : _T_456; // @[Lookup.scala 11:37:@598.4]
  assign cs_val_inst = _T_212 ? 1'h1 : _T_457; // @[Lookup.scala 11:37:@599.4]
  assign _T_471 = _T_356 ? 4'h6 : 4'h0; // @[Lookup.scala 11:37:@613.4]
  assign _T_472 = _T_352 ? 4'h5 : _T_471; // @[Lookup.scala 11:37:@614.4]
  assign _T_473 = _T_348 ? 4'h4 : _T_472; // @[Lookup.scala 11:37:@615.4]
  assign _T_474 = _T_344 ? 4'h3 : _T_473; // @[Lookup.scala 11:37:@616.4]
  assign _T_475 = _T_340 ? 4'h1 : _T_474; // @[Lookup.scala 11:37:@617.4]
  assign _T_476 = _T_336 ? 4'h2 : _T_475; // @[Lookup.scala 11:37:@618.4]
  assign _T_477 = _T_332 ? 4'h8 : _T_476; // @[Lookup.scala 11:37:@619.4]
  assign _T_478 = _T_328 ? 4'h7 : _T_477; // @[Lookup.scala 11:37:@620.4]
  assign _T_479 = _T_324 ? 4'h0 : _T_478; // @[Lookup.scala 11:37:@621.4]
  assign _T_480 = _T_320 ? 4'h0 : _T_479; // @[Lookup.scala 11:37:@622.4]
  assign _T_481 = _T_316 ? 4'h0 : _T_480; // @[Lookup.scala 11:37:@623.4]
  assign _T_482 = _T_312 ? 4'h0 : _T_481; // @[Lookup.scala 11:37:@624.4]
  assign _T_483 = _T_308 ? 4'h0 : _T_482; // @[Lookup.scala 11:37:@625.4]
  assign _T_484 = _T_304 ? 4'h0 : _T_483; // @[Lookup.scala 11:37:@626.4]
  assign _T_485 = _T_300 ? 4'h0 : _T_484; // @[Lookup.scala 11:37:@627.4]
  assign _T_486 = _T_296 ? 4'h0 : _T_485; // @[Lookup.scala 11:37:@628.4]
  assign _T_487 = _T_292 ? 4'h0 : _T_486; // @[Lookup.scala 11:37:@629.4]
  assign _T_488 = _T_288 ? 4'h0 : _T_487; // @[Lookup.scala 11:37:@630.4]
  assign _T_489 = _T_284 ? 4'h0 : _T_488; // @[Lookup.scala 11:37:@631.4]
  assign _T_490 = _T_280 ? 4'h0 : _T_489; // @[Lookup.scala 11:37:@632.4]
  assign _T_491 = _T_276 ? 4'h0 : _T_490; // @[Lookup.scala 11:37:@633.4]
  assign _T_492 = _T_272 ? 4'h0 : _T_491; // @[Lookup.scala 11:37:@634.4]
  assign _T_493 = _T_268 ? 4'h0 : _T_492; // @[Lookup.scala 11:37:@635.4]
  assign _T_494 = _T_264 ? 4'h0 : _T_493; // @[Lookup.scala 11:37:@636.4]
  assign _T_495 = _T_260 ? 4'h0 : _T_494; // @[Lookup.scala 11:37:@637.4]
  assign _T_496 = _T_256 ? 4'h0 : _T_495; // @[Lookup.scala 11:37:@638.4]
  assign _T_497 = _T_252 ? 4'h0 : _T_496; // @[Lookup.scala 11:37:@639.4]
  assign _T_498 = _T_248 ? 4'h0 : _T_497; // @[Lookup.scala 11:37:@640.4]
  assign _T_499 = _T_244 ? 4'h0 : _T_498; // @[Lookup.scala 11:37:@641.4]
  assign _T_500 = _T_240 ? 4'h0 : _T_499; // @[Lookup.scala 11:37:@642.4]
  assign _T_501 = _T_236 ? 4'h0 : _T_500; // @[Lookup.scala 11:37:@643.4]
  assign _T_502 = _T_232 ? 4'h0 : _T_501; // @[Lookup.scala 11:37:@644.4]
  assign _T_503 = _T_228 ? 4'h0 : _T_502; // @[Lookup.scala 11:37:@645.4]
  assign _T_504 = _T_224 ? 4'h0 : _T_503; // @[Lookup.scala 11:37:@646.4]
  assign _T_505 = _T_220 ? 4'h0 : _T_504; // @[Lookup.scala 11:37:@647.4]
  assign _T_506 = _T_216 ? 4'h0 : _T_505; // @[Lookup.scala 11:37:@648.4]
  assign cs_br_type = _T_212 ? 4'h0 : _T_506; // @[Lookup.scala 11:37:@649.4]
  assign _T_517 = _T_368 ? 2'h2 : 2'h0; // @[Lookup.scala 11:37:@660.4]
  assign _T_518 = _T_364 ? 2'h2 : _T_517; // @[Lookup.scala 11:37:@661.4]
  assign _T_519 = _T_360 ? 2'h2 : _T_518; // @[Lookup.scala 11:37:@662.4]
  assign _T_520 = _T_356 ? 2'h0 : _T_519; // @[Lookup.scala 11:37:@663.4]
  assign _T_521 = _T_352 ? 2'h0 : _T_520; // @[Lookup.scala 11:37:@664.4]
  assign _T_522 = _T_348 ? 2'h0 : _T_521; // @[Lookup.scala 11:37:@665.4]
  assign _T_523 = _T_344 ? 2'h0 : _T_522; // @[Lookup.scala 11:37:@666.4]
  assign _T_524 = _T_340 ? 2'h0 : _T_523; // @[Lookup.scala 11:37:@667.4]
  assign _T_525 = _T_336 ? 2'h0 : _T_524; // @[Lookup.scala 11:37:@668.4]
  assign _T_526 = _T_332 ? 2'h0 : _T_525; // @[Lookup.scala 11:37:@669.4]
  assign _T_527 = _T_328 ? 2'h0 : _T_526; // @[Lookup.scala 11:37:@670.4]
  assign _T_528 = _T_324 ? 2'h0 : _T_527; // @[Lookup.scala 11:37:@671.4]
  assign _T_529 = _T_320 ? 2'h0 : _T_528; // @[Lookup.scala 11:37:@672.4]
  assign _T_530 = _T_316 ? 2'h0 : _T_529; // @[Lookup.scala 11:37:@673.4]
  assign _T_531 = _T_312 ? 2'h0 : _T_530; // @[Lookup.scala 11:37:@674.4]
  assign _T_532 = _T_308 ? 2'h0 : _T_531; // @[Lookup.scala 11:37:@675.4]
  assign _T_533 = _T_304 ? 2'h0 : _T_532; // @[Lookup.scala 11:37:@676.4]
  assign _T_534 = _T_300 ? 2'h0 : _T_533; // @[Lookup.scala 11:37:@677.4]
  assign _T_535 = _T_296 ? 2'h0 : _T_534; // @[Lookup.scala 11:37:@678.4]
  assign _T_536 = _T_292 ? 2'h0 : _T_535; // @[Lookup.scala 11:37:@679.4]
  assign _T_537 = _T_288 ? 2'h0 : _T_536; // @[Lookup.scala 11:37:@680.4]
  assign _T_538 = _T_284 ? 2'h0 : _T_537; // @[Lookup.scala 11:37:@681.4]
  assign _T_539 = _T_280 ? 2'h0 : _T_538; // @[Lookup.scala 11:37:@682.4]
  assign _T_540 = _T_276 ? 2'h0 : _T_539; // @[Lookup.scala 11:37:@683.4]
  assign _T_541 = _T_272 ? 2'h0 : _T_540; // @[Lookup.scala 11:37:@684.4]
  assign _T_542 = _T_268 ? 2'h0 : _T_541; // @[Lookup.scala 11:37:@685.4]
  assign _T_543 = _T_264 ? 2'h0 : _T_542; // @[Lookup.scala 11:37:@686.4]
  assign _T_544 = _T_260 ? 2'h0 : _T_543; // @[Lookup.scala 11:37:@687.4]
  assign _T_545 = _T_256 ? 2'h0 : _T_544; // @[Lookup.scala 11:37:@688.4]
  assign _T_546 = _T_252 ? 2'h0 : _T_545; // @[Lookup.scala 11:37:@689.4]
  assign _T_547 = _T_248 ? 2'h1 : _T_546; // @[Lookup.scala 11:37:@690.4]
  assign _T_548 = _T_244 ? 2'h1 : _T_547; // @[Lookup.scala 11:37:@691.4]
  assign _T_549 = _T_240 ? 2'h0 : _T_548; // @[Lookup.scala 11:37:@692.4]
  assign _T_550 = _T_236 ? 2'h0 : _T_549; // @[Lookup.scala 11:37:@693.4]
  assign _T_551 = _T_232 ? 2'h0 : _T_550; // @[Lookup.scala 11:37:@694.4]
  assign _T_552 = _T_228 ? 2'h0 : _T_551; // @[Lookup.scala 11:37:@695.4]
  assign _T_553 = _T_224 ? 2'h0 : _T_552; // @[Lookup.scala 11:37:@696.4]
  assign _T_554 = _T_220 ? 2'h0 : _T_553; // @[Lookup.scala 11:37:@697.4]
  assign _T_555 = _T_216 ? 2'h0 : _T_554; // @[Lookup.scala 11:37:@698.4]
  assign _T_575 = _T_332 ? 2'h1 : 2'h0; // @[Lookup.scala 11:37:@719.4]
  assign _T_576 = _T_328 ? 2'h0 : _T_575; // @[Lookup.scala 11:37:@720.4]
  assign _T_577 = _T_324 ? 2'h0 : _T_576; // @[Lookup.scala 11:37:@721.4]
  assign _T_578 = _T_320 ? 2'h0 : _T_577; // @[Lookup.scala 11:37:@722.4]
  assign _T_579 = _T_316 ? 2'h0 : _T_578; // @[Lookup.scala 11:37:@723.4]
  assign _T_580 = _T_312 ? 2'h0 : _T_579; // @[Lookup.scala 11:37:@724.4]
  assign _T_581 = _T_308 ? 2'h0 : _T_580; // @[Lookup.scala 11:37:@725.4]
  assign _T_582 = _T_304 ? 2'h0 : _T_581; // @[Lookup.scala 11:37:@726.4]
  assign _T_583 = _T_300 ? 2'h0 : _T_582; // @[Lookup.scala 11:37:@727.4]
  assign _T_584 = _T_296 ? 2'h0 : _T_583; // @[Lookup.scala 11:37:@728.4]
  assign _T_585 = _T_292 ? 2'h0 : _T_584; // @[Lookup.scala 11:37:@729.4]
  assign _T_586 = _T_288 ? 2'h0 : _T_585; // @[Lookup.scala 11:37:@730.4]
  assign _T_587 = _T_284 ? 2'h1 : _T_586; // @[Lookup.scala 11:37:@731.4]
  assign _T_588 = _T_280 ? 2'h1 : _T_587; // @[Lookup.scala 11:37:@732.4]
  assign _T_589 = _T_276 ? 2'h1 : _T_588; // @[Lookup.scala 11:37:@733.4]
  assign _T_590 = _T_272 ? 2'h1 : _T_589; // @[Lookup.scala 11:37:@734.4]
  assign _T_591 = _T_268 ? 2'h1 : _T_590; // @[Lookup.scala 11:37:@735.4]
  assign _T_592 = _T_264 ? 2'h1 : _T_591; // @[Lookup.scala 11:37:@736.4]
  assign _T_593 = _T_260 ? 2'h1 : _T_592; // @[Lookup.scala 11:37:@737.4]
  assign _T_594 = _T_256 ? 2'h1 : _T_593; // @[Lookup.scala 11:37:@738.4]
  assign _T_595 = _T_252 ? 2'h1 : _T_594; // @[Lookup.scala 11:37:@739.4]
  assign _T_596 = _T_248 ? 2'h0 : _T_595; // @[Lookup.scala 11:37:@740.4]
  assign _T_597 = _T_244 ? 2'h3 : _T_596; // @[Lookup.scala 11:37:@741.4]
  assign _T_598 = _T_240 ? 2'h2 : _T_597; // @[Lookup.scala 11:37:@742.4]
  assign _T_599 = _T_236 ? 2'h2 : _T_598; // @[Lookup.scala 11:37:@743.4]
  assign _T_600 = _T_232 ? 2'h2 : _T_599; // @[Lookup.scala 11:37:@744.4]
  assign _T_601 = _T_228 ? 2'h1 : _T_600; // @[Lookup.scala 11:37:@745.4]
  assign _T_602 = _T_224 ? 2'h1 : _T_601; // @[Lookup.scala 11:37:@746.4]
  assign _T_603 = _T_220 ? 2'h1 : _T_602; // @[Lookup.scala 11:37:@747.4]
  assign _T_604 = _T_216 ? 2'h1 : _T_603; // @[Lookup.scala 11:37:@748.4]
  assign _T_612 = _T_380 ? 4'hb : 4'h0; // @[Lookup.scala 11:37:@757.4]
  assign _T_613 = _T_376 ? 4'hb : _T_612; // @[Lookup.scala 11:37:@758.4]
  assign _T_614 = _T_372 ? 4'hb : _T_613; // @[Lookup.scala 11:37:@759.4]
  assign _T_615 = _T_368 ? 4'hb : _T_614; // @[Lookup.scala 11:37:@760.4]
  assign _T_616 = _T_364 ? 4'hb : _T_615; // @[Lookup.scala 11:37:@761.4]
  assign _T_617 = _T_360 ? 4'hb : _T_616; // @[Lookup.scala 11:37:@762.4]
  assign _T_618 = _T_356 ? 4'h0 : _T_617; // @[Lookup.scala 11:37:@763.4]
  assign _T_619 = _T_352 ? 4'h0 : _T_618; // @[Lookup.scala 11:37:@764.4]
  assign _T_620 = _T_348 ? 4'h0 : _T_619; // @[Lookup.scala 11:37:@765.4]
  assign _T_621 = _T_344 ? 4'h0 : _T_620; // @[Lookup.scala 11:37:@766.4]
  assign _T_622 = _T_340 ? 4'h0 : _T_621; // @[Lookup.scala 11:37:@767.4]
  assign _T_623 = _T_336 ? 4'h0 : _T_622; // @[Lookup.scala 11:37:@768.4]
  assign _T_624 = _T_332 ? 4'h0 : _T_623; // @[Lookup.scala 11:37:@769.4]
  assign _T_625 = _T_328 ? 4'h0 : _T_624; // @[Lookup.scala 11:37:@770.4]
  assign _T_626 = _T_324 ? 4'h4 : _T_625; // @[Lookup.scala 11:37:@771.4]
  assign _T_627 = _T_320 ? 4'h5 : _T_626; // @[Lookup.scala 11:37:@772.4]
  assign _T_628 = _T_316 ? 4'h8 : _T_627; // @[Lookup.scala 11:37:@773.4]
  assign _T_629 = _T_312 ? 4'h7 : _T_628; // @[Lookup.scala 11:37:@774.4]
  assign _T_630 = _T_308 ? 4'h6 : _T_629; // @[Lookup.scala 11:37:@775.4]
  assign _T_631 = _T_304 ? 4'ha : _T_630; // @[Lookup.scala 11:37:@776.4]
  assign _T_632 = _T_300 ? 4'h9 : _T_631; // @[Lookup.scala 11:37:@777.4]
  assign _T_633 = _T_296 ? 4'h2 : _T_632; // @[Lookup.scala 11:37:@778.4]
  assign _T_634 = _T_292 ? 4'h1 : _T_633; // @[Lookup.scala 11:37:@779.4]
  assign _T_635 = _T_288 ? 4'h3 : _T_634; // @[Lookup.scala 11:37:@780.4]
  assign _T_636 = _T_284 ? 4'h4 : _T_635; // @[Lookup.scala 11:37:@781.4]
  assign _T_637 = _T_280 ? 4'h5 : _T_636; // @[Lookup.scala 11:37:@782.4]
  assign _T_638 = _T_276 ? 4'h3 : _T_637; // @[Lookup.scala 11:37:@783.4]
  assign _T_639 = _T_272 ? 4'ha : _T_638; // @[Lookup.scala 11:37:@784.4]
  assign _T_640 = _T_268 ? 4'h9 : _T_639; // @[Lookup.scala 11:37:@785.4]
  assign _T_641 = _T_264 ? 4'h8 : _T_640; // @[Lookup.scala 11:37:@786.4]
  assign _T_642 = _T_260 ? 4'h7 : _T_641; // @[Lookup.scala 11:37:@787.4]
  assign _T_643 = _T_256 ? 4'h6 : _T_642; // @[Lookup.scala 11:37:@788.4]
  assign _T_644 = _T_252 ? 4'h1 : _T_643; // @[Lookup.scala 11:37:@789.4]
  assign _T_645 = _T_248 ? 4'hb : _T_644; // @[Lookup.scala 11:37:@790.4]
  assign _T_646 = _T_244 ? 4'h1 : _T_645; // @[Lookup.scala 11:37:@791.4]
  assign _T_647 = _T_240 ? 4'h1 : _T_646; // @[Lookup.scala 11:37:@792.4]
  assign _T_648 = _T_236 ? 4'h1 : _T_647; // @[Lookup.scala 11:37:@793.4]
  assign _T_649 = _T_232 ? 4'h1 : _T_648; // @[Lookup.scala 11:37:@794.4]
  assign _T_650 = _T_228 ? 4'h1 : _T_649; // @[Lookup.scala 11:37:@795.4]
  assign _T_651 = _T_224 ? 4'h1 : _T_650; // @[Lookup.scala 11:37:@796.4]
  assign _T_652 = _T_220 ? 4'h1 : _T_651; // @[Lookup.scala 11:37:@797.4]
  assign _T_653 = _T_216 ? 4'h1 : _T_652; // @[Lookup.scala 11:37:@798.4]
  assign _T_661 = _T_380 ? 2'h3 : 2'h0; // @[Lookup.scala 11:37:@807.4]
  assign _T_662 = _T_376 ? 2'h3 : _T_661; // @[Lookup.scala 11:37:@808.4]
  assign _T_663 = _T_372 ? 2'h3 : _T_662; // @[Lookup.scala 11:37:@809.4]
  assign _T_664 = _T_368 ? 2'h3 : _T_663; // @[Lookup.scala 11:37:@810.4]
  assign _T_665 = _T_364 ? 2'h3 : _T_664; // @[Lookup.scala 11:37:@811.4]
  assign _T_666 = _T_360 ? 2'h3 : _T_665; // @[Lookup.scala 11:37:@812.4]
  assign _T_667 = _T_356 ? 2'h0 : _T_666; // @[Lookup.scala 11:37:@813.4]
  assign _T_668 = _T_352 ? 2'h0 : _T_667; // @[Lookup.scala 11:37:@814.4]
  assign _T_669 = _T_348 ? 2'h0 : _T_668; // @[Lookup.scala 11:37:@815.4]
  assign _T_670 = _T_344 ? 2'h0 : _T_669; // @[Lookup.scala 11:37:@816.4]
  assign _T_671 = _T_340 ? 2'h0 : _T_670; // @[Lookup.scala 11:37:@817.4]
  assign _T_672 = _T_336 ? 2'h0 : _T_671; // @[Lookup.scala 11:37:@818.4]
  assign _T_673 = _T_332 ? 2'h2 : _T_672; // @[Lookup.scala 11:37:@819.4]
  assign _T_674 = _T_328 ? 2'h2 : _T_673; // @[Lookup.scala 11:37:@820.4]
  assign _T_675 = _T_324 ? 2'h0 : _T_674; // @[Lookup.scala 11:37:@821.4]
  assign _T_676 = _T_320 ? 2'h0 : _T_675; // @[Lookup.scala 11:37:@822.4]
  assign _T_677 = _T_316 ? 2'h0 : _T_676; // @[Lookup.scala 11:37:@823.4]
  assign _T_678 = _T_312 ? 2'h0 : _T_677; // @[Lookup.scala 11:37:@824.4]
  assign _T_679 = _T_308 ? 2'h0 : _T_678; // @[Lookup.scala 11:37:@825.4]
  assign _T_680 = _T_304 ? 2'h0 : _T_679; // @[Lookup.scala 11:37:@826.4]
  assign _T_681 = _T_300 ? 2'h0 : _T_680; // @[Lookup.scala 11:37:@827.4]
  assign _T_682 = _T_296 ? 2'h0 : _T_681; // @[Lookup.scala 11:37:@828.4]
  assign _T_683 = _T_292 ? 2'h0 : _T_682; // @[Lookup.scala 11:37:@829.4]
  assign _T_684 = _T_288 ? 2'h0 : _T_683; // @[Lookup.scala 11:37:@830.4]
  assign _T_685 = _T_284 ? 2'h0 : _T_684; // @[Lookup.scala 11:37:@831.4]
  assign _T_686 = _T_280 ? 2'h0 : _T_685; // @[Lookup.scala 11:37:@832.4]
  assign _T_687 = _T_276 ? 2'h0 : _T_686; // @[Lookup.scala 11:37:@833.4]
  assign _T_688 = _T_272 ? 2'h0 : _T_687; // @[Lookup.scala 11:37:@834.4]
  assign _T_689 = _T_268 ? 2'h0 : _T_688; // @[Lookup.scala 11:37:@835.4]
  assign _T_690 = _T_264 ? 2'h0 : _T_689; // @[Lookup.scala 11:37:@836.4]
  assign _T_691 = _T_260 ? 2'h0 : _T_690; // @[Lookup.scala 11:37:@837.4]
  assign _T_692 = _T_256 ? 2'h0 : _T_691; // @[Lookup.scala 11:37:@838.4]
  assign _T_693 = _T_252 ? 2'h0 : _T_692; // @[Lookup.scala 11:37:@839.4]
  assign _T_694 = _T_248 ? 2'h0 : _T_693; // @[Lookup.scala 11:37:@840.4]
  assign _T_695 = _T_244 ? 2'h0 : _T_694; // @[Lookup.scala 11:37:@841.4]
  assign _T_696 = _T_240 ? 2'h0 : _T_695; // @[Lookup.scala 11:37:@842.4]
  assign _T_697 = _T_236 ? 2'h0 : _T_696; // @[Lookup.scala 11:37:@843.4]
  assign _T_698 = _T_232 ? 2'h0 : _T_697; // @[Lookup.scala 11:37:@844.4]
  assign _T_699 = _T_228 ? 2'h1 : _T_698; // @[Lookup.scala 11:37:@845.4]
  assign _T_700 = _T_224 ? 2'h1 : _T_699; // @[Lookup.scala 11:37:@846.4]
  assign _T_701 = _T_220 ? 2'h1 : _T_700; // @[Lookup.scala 11:37:@847.4]
  assign _T_702 = _T_216 ? 2'h1 : _T_701; // @[Lookup.scala 11:37:@848.4]
  assign _T_711 = _T_376 ? 1'h1 : _T_380; // @[Lookup.scala 11:37:@858.4]
  assign _T_712 = _T_372 ? 1'h1 : _T_711; // @[Lookup.scala 11:37:@859.4]
  assign _T_713 = _T_368 ? 1'h1 : _T_712; // @[Lookup.scala 11:37:@860.4]
  assign _T_714 = _T_364 ? 1'h1 : _T_713; // @[Lookup.scala 11:37:@861.4]
  assign _T_715 = _T_360 ? 1'h1 : _T_714; // @[Lookup.scala 11:37:@862.4]
  assign _T_716 = _T_356 ? 1'h0 : _T_715; // @[Lookup.scala 11:37:@863.4]
  assign _T_717 = _T_352 ? 1'h0 : _T_716; // @[Lookup.scala 11:37:@864.4]
  assign _T_718 = _T_348 ? 1'h0 : _T_717; // @[Lookup.scala 11:37:@865.4]
  assign _T_719 = _T_344 ? 1'h0 : _T_718; // @[Lookup.scala 11:37:@866.4]
  assign _T_720 = _T_340 ? 1'h0 : _T_719; // @[Lookup.scala 11:37:@867.4]
  assign _T_721 = _T_336 ? 1'h0 : _T_720; // @[Lookup.scala 11:37:@868.4]
  assign _T_722 = _T_332 ? 1'h1 : _T_721; // @[Lookup.scala 11:37:@869.4]
  assign _T_723 = _T_328 ? 1'h1 : _T_722; // @[Lookup.scala 11:37:@870.4]
  assign _T_724 = _T_324 ? 1'h1 : _T_723; // @[Lookup.scala 11:37:@871.4]
  assign _T_725 = _T_320 ? 1'h1 : _T_724; // @[Lookup.scala 11:37:@872.4]
  assign _T_726 = _T_316 ? 1'h1 : _T_725; // @[Lookup.scala 11:37:@873.4]
  assign _T_727 = _T_312 ? 1'h1 : _T_726; // @[Lookup.scala 11:37:@874.4]
  assign _T_728 = _T_308 ? 1'h1 : _T_727; // @[Lookup.scala 11:37:@875.4]
  assign _T_729 = _T_304 ? 1'h1 : _T_728; // @[Lookup.scala 11:37:@876.4]
  assign _T_730 = _T_300 ? 1'h1 : _T_729; // @[Lookup.scala 11:37:@877.4]
  assign _T_731 = _T_296 ? 1'h1 : _T_730; // @[Lookup.scala 11:37:@878.4]
  assign _T_732 = _T_292 ? 1'h1 : _T_731; // @[Lookup.scala 11:37:@879.4]
  assign _T_733 = _T_288 ? 1'h1 : _T_732; // @[Lookup.scala 11:37:@880.4]
  assign _T_734 = _T_284 ? 1'h1 : _T_733; // @[Lookup.scala 11:37:@881.4]
  assign _T_735 = _T_280 ? 1'h1 : _T_734; // @[Lookup.scala 11:37:@882.4]
  assign _T_736 = _T_276 ? 1'h1 : _T_735; // @[Lookup.scala 11:37:@883.4]
  assign _T_737 = _T_272 ? 1'h1 : _T_736; // @[Lookup.scala 11:37:@884.4]
  assign _T_738 = _T_268 ? 1'h1 : _T_737; // @[Lookup.scala 11:37:@885.4]
  assign _T_739 = _T_264 ? 1'h1 : _T_738; // @[Lookup.scala 11:37:@886.4]
  assign _T_740 = _T_260 ? 1'h1 : _T_739; // @[Lookup.scala 11:37:@887.4]
  assign _T_741 = _T_256 ? 1'h1 : _T_740; // @[Lookup.scala 11:37:@888.4]
  assign _T_742 = _T_252 ? 1'h1 : _T_741; // @[Lookup.scala 11:37:@889.4]
  assign _T_743 = _T_248 ? 1'h1 : _T_742; // @[Lookup.scala 11:37:@890.4]
  assign _T_744 = _T_244 ? 1'h1 : _T_743; // @[Lookup.scala 11:37:@891.4]
  assign _T_745 = _T_240 ? 1'h0 : _T_744; // @[Lookup.scala 11:37:@892.4]
  assign _T_746 = _T_236 ? 1'h0 : _T_745; // @[Lookup.scala 11:37:@893.4]
  assign _T_747 = _T_232 ? 1'h0 : _T_746; // @[Lookup.scala 11:37:@894.4]
  assign _T_748 = _T_228 ? 1'h1 : _T_747; // @[Lookup.scala 11:37:@895.4]
  assign _T_749 = _T_224 ? 1'h1 : _T_748; // @[Lookup.scala 11:37:@896.4]
  assign _T_750 = _T_220 ? 1'h1 : _T_749; // @[Lookup.scala 11:37:@897.4]
  assign _T_751 = _T_216 ? 1'h1 : _T_750; // @[Lookup.scala 11:37:@898.4]
  assign cs0_2 = _T_212 ? 1'h1 : _T_751; // @[Lookup.scala 11:37:@899.4]
  assign _T_753 = _T_404 ? 1'h0 : _T_408; // @[Lookup.scala 11:37:@901.4]
  assign _T_754 = _T_400 ? 1'h0 : _T_753; // @[Lookup.scala 11:37:@902.4]
  assign _T_755 = _T_396 ? 1'h0 : _T_754; // @[Lookup.scala 11:37:@903.4]
  assign _T_756 = _T_392 ? 1'h0 : _T_755; // @[Lookup.scala 11:37:@904.4]
  assign _T_757 = _T_388 ? 1'h0 : _T_756; // @[Lookup.scala 11:37:@905.4]
  assign _T_758 = _T_384 ? 1'h0 : _T_757; // @[Lookup.scala 11:37:@906.4]
  assign _T_759 = _T_380 ? 1'h0 : _T_758; // @[Lookup.scala 11:37:@907.4]
  assign _T_760 = _T_376 ? 1'h0 : _T_759; // @[Lookup.scala 11:37:@908.4]
  assign _T_761 = _T_372 ? 1'h0 : _T_760; // @[Lookup.scala 11:37:@909.4]
  assign _T_762 = _T_368 ? 1'h0 : _T_761; // @[Lookup.scala 11:37:@910.4]
  assign _T_763 = _T_364 ? 1'h0 : _T_762; // @[Lookup.scala 11:37:@911.4]
  assign _T_764 = _T_360 ? 1'h0 : _T_763; // @[Lookup.scala 11:37:@912.4]
  assign _T_765 = _T_356 ? 1'h0 : _T_764; // @[Lookup.scala 11:37:@913.4]
  assign _T_766 = _T_352 ? 1'h0 : _T_765; // @[Lookup.scala 11:37:@914.4]
  assign _T_767 = _T_348 ? 1'h0 : _T_766; // @[Lookup.scala 11:37:@915.4]
  assign _T_768 = _T_344 ? 1'h0 : _T_767; // @[Lookup.scala 11:37:@916.4]
  assign _T_769 = _T_340 ? 1'h0 : _T_768; // @[Lookup.scala 11:37:@917.4]
  assign _T_770 = _T_336 ? 1'h0 : _T_769; // @[Lookup.scala 11:37:@918.4]
  assign _T_771 = _T_332 ? 1'h0 : _T_770; // @[Lookup.scala 11:37:@919.4]
  assign _T_772 = _T_328 ? 1'h0 : _T_771; // @[Lookup.scala 11:37:@920.4]
  assign _T_773 = _T_324 ? 1'h0 : _T_772; // @[Lookup.scala 11:37:@921.4]
  assign _T_774 = _T_320 ? 1'h0 : _T_773; // @[Lookup.scala 11:37:@922.4]
  assign _T_775 = _T_316 ? 1'h0 : _T_774; // @[Lookup.scala 11:37:@923.4]
  assign _T_776 = _T_312 ? 1'h0 : _T_775; // @[Lookup.scala 11:37:@924.4]
  assign _T_777 = _T_308 ? 1'h0 : _T_776; // @[Lookup.scala 11:37:@925.4]
  assign _T_778 = _T_304 ? 1'h0 : _T_777; // @[Lookup.scala 11:37:@926.4]
  assign _T_779 = _T_300 ? 1'h0 : _T_778; // @[Lookup.scala 11:37:@927.4]
  assign _T_780 = _T_296 ? 1'h0 : _T_779; // @[Lookup.scala 11:37:@928.4]
  assign _T_781 = _T_292 ? 1'h0 : _T_780; // @[Lookup.scala 11:37:@929.4]
  assign _T_782 = _T_288 ? 1'h0 : _T_781; // @[Lookup.scala 11:37:@930.4]
  assign _T_783 = _T_284 ? 1'h0 : _T_782; // @[Lookup.scala 11:37:@931.4]
  assign _T_784 = _T_280 ? 1'h0 : _T_783; // @[Lookup.scala 11:37:@932.4]
  assign _T_785 = _T_276 ? 1'h0 : _T_784; // @[Lookup.scala 11:37:@933.4]
  assign _T_786 = _T_272 ? 1'h0 : _T_785; // @[Lookup.scala 11:37:@934.4]
  assign _T_787 = _T_268 ? 1'h0 : _T_786; // @[Lookup.scala 11:37:@935.4]
  assign _T_788 = _T_264 ? 1'h0 : _T_787; // @[Lookup.scala 11:37:@936.4]
  assign _T_789 = _T_260 ? 1'h0 : _T_788; // @[Lookup.scala 11:37:@937.4]
  assign _T_790 = _T_256 ? 1'h0 : _T_789; // @[Lookup.scala 11:37:@938.4]
  assign _T_791 = _T_252 ? 1'h0 : _T_790; // @[Lookup.scala 11:37:@939.4]
  assign _T_792 = _T_248 ? 1'h0 : _T_791; // @[Lookup.scala 11:37:@940.4]
  assign _T_793 = _T_244 ? 1'h0 : _T_792; // @[Lookup.scala 11:37:@941.4]
  assign _T_794 = _T_240 ? 1'h1 : _T_793; // @[Lookup.scala 11:37:@942.4]
  assign _T_795 = _T_236 ? 1'h1 : _T_794; // @[Lookup.scala 11:37:@943.4]
  assign _T_796 = _T_232 ? 1'h1 : _T_795; // @[Lookup.scala 11:37:@944.4]
  assign _T_797 = _T_228 ? 1'h1 : _T_796; // @[Lookup.scala 11:37:@945.4]
  assign _T_798 = _T_224 ? 1'h1 : _T_797; // @[Lookup.scala 11:37:@946.4]
  assign _T_799 = _T_220 ? 1'h1 : _T_798; // @[Lookup.scala 11:37:@947.4]
  assign _T_800 = _T_216 ? 1'h1 : _T_799; // @[Lookup.scala 11:37:@948.4]
  assign cs0_3 = _T_212 ? 1'h1 : _T_800; // @[Lookup.scala 11:37:@949.4]
  assign _T_844 = _T_236 ? 1'h1 : _T_240; // @[Lookup.scala 11:37:@993.4]
  assign _T_845 = _T_232 ? 1'h1 : _T_844; // @[Lookup.scala 11:37:@994.4]
  assign _T_846 = _T_228 ? 1'h0 : _T_845; // @[Lookup.scala 11:37:@995.4]
  assign _T_847 = _T_224 ? 1'h0 : _T_846; // @[Lookup.scala 11:37:@996.4]
  assign _T_848 = _T_220 ? 1'h0 : _T_847; // @[Lookup.scala 11:37:@997.4]
  assign _T_849 = _T_216 ? 1'h0 : _T_848; // @[Lookup.scala 11:37:@998.4]
  assign _T_892 = _T_240 ? 3'h2 : 3'h0; // @[Lookup.scala 11:37:@1042.4]
  assign _T_893 = _T_236 ? 3'h1 : _T_892; // @[Lookup.scala 11:37:@1043.4]
  assign _T_894 = _T_232 ? 3'h3 : _T_893; // @[Lookup.scala 11:37:@1044.4]
  assign _T_895 = _T_228 ? 3'h6 : _T_894; // @[Lookup.scala 11:37:@1045.4]
  assign _T_896 = _T_224 ? 3'h2 : _T_895; // @[Lookup.scala 11:37:@1046.4]
  assign _T_897 = _T_220 ? 3'h5 : _T_896; // @[Lookup.scala 11:37:@1047.4]
  assign _T_898 = _T_216 ? 3'h1 : _T_897; // @[Lookup.scala 11:37:@1048.4]
  assign _T_902 = _T_396 ? 3'h4 : 3'h0; // @[Lookup.scala 11:37:@1053.4]
  assign _T_903 = _T_392 ? 3'h4 : _T_902; // @[Lookup.scala 11:37:@1054.4]
  assign _T_904 = _T_388 ? 3'h4 : _T_903; // @[Lookup.scala 11:37:@1055.4]
  assign _T_905 = _T_384 ? 3'h4 : _T_904; // @[Lookup.scala 11:37:@1056.4]
  assign _T_906 = _T_380 ? 3'h3 : _T_905; // @[Lookup.scala 11:37:@1057.4]
  assign _T_907 = _T_376 ? 3'h2 : _T_906; // @[Lookup.scala 11:37:@1058.4]
  assign _T_908 = _T_372 ? 3'h1 : _T_907; // @[Lookup.scala 11:37:@1059.4]
  assign _T_909 = _T_368 ? 3'h3 : _T_908; // @[Lookup.scala 11:37:@1060.4]
  assign _T_910 = _T_364 ? 3'h2 : _T_909; // @[Lookup.scala 11:37:@1061.4]
  assign _T_911 = _T_360 ? 3'h1 : _T_910; // @[Lookup.scala 11:37:@1062.4]
  assign _T_912 = _T_356 ? 3'h0 : _T_911; // @[Lookup.scala 11:37:@1063.4]
  assign _T_913 = _T_352 ? 3'h0 : _T_912; // @[Lookup.scala 11:37:@1064.4]
  assign _T_914 = _T_348 ? 3'h0 : _T_913; // @[Lookup.scala 11:37:@1065.4]
  assign _T_915 = _T_344 ? 3'h0 : _T_914; // @[Lookup.scala 11:37:@1066.4]
  assign _T_916 = _T_340 ? 3'h0 : _T_915; // @[Lookup.scala 11:37:@1067.4]
  assign _T_917 = _T_336 ? 3'h0 : _T_916; // @[Lookup.scala 11:37:@1068.4]
  assign _T_918 = _T_332 ? 3'h0 : _T_917; // @[Lookup.scala 11:37:@1069.4]
  assign _T_919 = _T_328 ? 3'h0 : _T_918; // @[Lookup.scala 11:37:@1070.4]
  assign _T_920 = _T_324 ? 3'h0 : _T_919; // @[Lookup.scala 11:37:@1071.4]
  assign _T_921 = _T_320 ? 3'h0 : _T_920; // @[Lookup.scala 11:37:@1072.4]
  assign _T_922 = _T_316 ? 3'h0 : _T_921; // @[Lookup.scala 11:37:@1073.4]
  assign _T_923 = _T_312 ? 3'h0 : _T_922; // @[Lookup.scala 11:37:@1074.4]
  assign _T_924 = _T_308 ? 3'h0 : _T_923; // @[Lookup.scala 11:37:@1075.4]
  assign _T_925 = _T_304 ? 3'h0 : _T_924; // @[Lookup.scala 11:37:@1076.4]
  assign _T_926 = _T_300 ? 3'h0 : _T_925; // @[Lookup.scala 11:37:@1077.4]
  assign _T_927 = _T_296 ? 3'h0 : _T_926; // @[Lookup.scala 11:37:@1078.4]
  assign _T_928 = _T_292 ? 3'h0 : _T_927; // @[Lookup.scala 11:37:@1079.4]
  assign _T_929 = _T_288 ? 3'h0 : _T_928; // @[Lookup.scala 11:37:@1080.4]
  assign _T_930 = _T_284 ? 3'h0 : _T_929; // @[Lookup.scala 11:37:@1081.4]
  assign _T_931 = _T_280 ? 3'h0 : _T_930; // @[Lookup.scala 11:37:@1082.4]
  assign _T_932 = _T_276 ? 3'h0 : _T_931; // @[Lookup.scala 11:37:@1083.4]
  assign _T_933 = _T_272 ? 3'h0 : _T_932; // @[Lookup.scala 11:37:@1084.4]
  assign _T_934 = _T_268 ? 3'h0 : _T_933; // @[Lookup.scala 11:37:@1085.4]
  assign _T_935 = _T_264 ? 3'h0 : _T_934; // @[Lookup.scala 11:37:@1086.4]
  assign _T_936 = _T_260 ? 3'h0 : _T_935; // @[Lookup.scala 11:37:@1087.4]
  assign _T_937 = _T_256 ? 3'h0 : _T_936; // @[Lookup.scala 11:37:@1088.4]
  assign _T_938 = _T_252 ? 3'h0 : _T_937; // @[Lookup.scala 11:37:@1089.4]
  assign _T_939 = _T_248 ? 3'h0 : _T_938; // @[Lookup.scala 11:37:@1090.4]
  assign _T_940 = _T_244 ? 3'h0 : _T_939; // @[Lookup.scala 11:37:@1091.4]
  assign _T_941 = _T_240 ? 3'h0 : _T_940; // @[Lookup.scala 11:37:@1092.4]
  assign _T_942 = _T_236 ? 3'h0 : _T_941; // @[Lookup.scala 11:37:@1093.4]
  assign _T_943 = _T_232 ? 3'h0 : _T_942; // @[Lookup.scala 11:37:@1094.4]
  assign _T_944 = _T_228 ? 3'h0 : _T_943; // @[Lookup.scala 11:37:@1095.4]
  assign _T_945 = _T_224 ? 3'h0 : _T_944; // @[Lookup.scala 11:37:@1096.4]
  assign _T_946 = _T_220 ? 3'h0 : _T_945; // @[Lookup.scala 11:37:@1097.4]
  assign _T_947 = _T_216 ? 3'h0 : _T_946; // @[Lookup.scala 11:37:@1098.4]
  assign cs0_6 = _T_212 ? 3'h0 : _T_947; // @[Lookup.scala 11:37:@1099.4]
  assign _T_948 = io_dat_csr_eret | io_ctl_exception; // @[cpath.scala 118:43:@1100.4]
  assign _T_949 = cs_br_type == 4'h0; // @[cpath.scala 120:37:@1101.4]
  assign _T_950 = cs_br_type == 4'h1; // @[cpath.scala 121:37:@1102.4]
  assign _T_952 = io_dat_br_eq == 1'h0; // @[cpath.scala 121:54:@1103.4]
  assign _T_953 = _T_952 ? 3'h1 : 3'h0; // @[cpath.scala 121:53:@1104.4]
  assign _T_954 = cs_br_type == 4'h2; // @[cpath.scala 122:37:@1105.4]
  assign _T_955 = io_dat_br_eq ? 3'h1 : 3'h0; // @[cpath.scala 122:53:@1106.4]
  assign _T_956 = cs_br_type == 4'h3; // @[cpath.scala 123:37:@1107.4]
  assign _T_958 = io_dat_br_lt == 1'h0; // @[cpath.scala 123:54:@1108.4]
  assign _T_959 = _T_958 ? 3'h1 : 3'h0; // @[cpath.scala 123:53:@1109.4]
  assign _T_960 = cs_br_type == 4'h4; // @[cpath.scala 124:37:@1110.4]
  assign _T_962 = io_dat_br_ltu == 1'h0; // @[cpath.scala 124:54:@1111.4]
  assign _T_963 = _T_962 ? 3'h1 : 3'h0; // @[cpath.scala 124:53:@1112.4]
  assign _T_964 = cs_br_type == 4'h5; // @[cpath.scala 125:37:@1113.4]
  assign _T_965 = io_dat_br_lt ? 3'h1 : 3'h0; // @[cpath.scala 125:53:@1114.4]
  assign _T_966 = cs_br_type == 4'h6; // @[cpath.scala 126:37:@1115.4]
  assign _T_967 = io_dat_br_ltu ? 3'h1 : 3'h0; // @[cpath.scala 126:53:@1116.4]
  assign _T_968 = cs_br_type == 4'h7; // @[cpath.scala 127:37:@1117.4]
  assign _T_969 = cs_br_type == 4'h8; // @[cpath.scala 128:37:@1118.4]
  assign _T_970 = _T_969 ? 3'h3 : 3'h0; // @[cpath.scala 128:25:@1119.4]
  assign _T_971 = _T_968 ? 3'h2 : _T_970; // @[cpath.scala 127:25:@1120.4]
  assign _T_972 = _T_966 ? _T_967 : _T_971; // @[cpath.scala 126:25:@1121.4]
  assign _T_973 = _T_964 ? _T_965 : _T_972; // @[cpath.scala 125:25:@1122.4]
  assign _T_974 = _T_960 ? _T_963 : _T_973; // @[cpath.scala 124:25:@1123.4]
  assign _T_975 = _T_956 ? _T_959 : _T_974; // @[cpath.scala 123:25:@1124.4]
  assign _T_976 = _T_954 ? _T_955 : _T_975; // @[cpath.scala 122:25:@1125.4]
  assign _T_977 = _T_950 ? _T_953 : _T_976; // @[cpath.scala 121:25:@1126.4]
  assign _T_978 = _T_949 ? 3'h0 : _T_977; // @[cpath.scala 120:25:@1127.4]
  assign _T_981 = cs0_3 & io_dmem_resp_valid; // @[cpath.scala 131:53:@1130.4]
  assign _T_983 = cs0_3 == 1'h0; // @[cpath.scala 131:79:@1131.4]
  assign _T_984 = _T_981 | _T_983; // @[cpath.scala 131:76:@1132.4]
  assign stall = _T_984 == 1'h0; // @[cpath.scala 131:40:@1133.4]
  assign _T_987 = stall | io_ctl_exception; // @[cpath.scala 140:33:@1141.4]
  assign rs1_addr = io_dat_inst[19:15]; // @[cpath.scala 143:30:@1144.4]
  assign _T_990 = cs0_6 == 3'h2; // @[cpath.scala 144:30:@1145.4]
  assign _T_991 = cs0_6 == 3'h3; // @[cpath.scala 144:54:@1146.4]
  assign _T_992 = _T_990 | _T_991; // @[cpath.scala 144:40:@1147.4]
  assign _T_994 = rs1_addr == 5'h0; // @[cpath.scala 144:77:@1148.4]
  assign csr_ren = _T_992 & _T_994; // @[cpath.scala 144:65:@1149.4]
  assign csr_cmd = csr_ren ? 3'h5 : cs0_6; // @[cpath.scala 145:21:@1150.4]
  assign io_dmem_req_valid = _T_212 ? 1'h1 : _T_800; // @[cpath.scala 154:25:@1156.4]
  assign io_dmem_req_bits_fcn = _T_212 ? 1'h0 : _T_849; // @[cpath.scala 155:25:@1157.4]
  assign io_dmem_req_bits_typ = _T_212 ? 3'h3 : _T_898; // @[cpath.scala 156:25:@1158.4]
  assign io_ctl_stall = _T_984 == 1'h0; // @[cpath.scala 134:20:@1135.4]
  assign io_ctl_pc_sel = _T_948 ? 3'h4 : _T_978; // @[cpath.scala 135:20:@1136.4]
  assign io_ctl_op1_sel = _T_212 ? 2'h0 : _T_555; // @[cpath.scala 136:20:@1137.4]
  assign io_ctl_op2_sel = _T_212 ? 2'h1 : _T_604; // @[cpath.scala 137:20:@1138.4]
  assign io_ctl_alu_fun = _T_212 ? 4'h1 : _T_653; // @[cpath.scala 138:20:@1139.4]
  assign io_ctl_wb_sel = _T_212 ? 2'h1 : _T_702; // @[cpath.scala 139:20:@1140.4]
  assign io_ctl_rf_wen = _T_987 ? 1'h0 : cs0_2; // @[cpath.scala 140:20:@1143.4]
  assign io_ctl_csr_cmd = stall ? 3'h0 : csr_cmd; // @[cpath.scala 147:20:@1152.4]
  assign io_ctl_exception = cs_val_inst == 1'h0; // @[cpath.scala 164:21:@1161.4]
endmodule
module CSRFile( // @[:@1163.2]
  input         clock, // @[:@1164.4]
  input         reset, // @[:@1165.4]
  input  [2:0]  io_rw_cmd, // @[:@1166.4]
  output [31:0] io_rw_rdata, // @[:@1166.4]
  input  [31:0] io_rw_wdata, // @[:@1166.4]
  output        io_eret, // @[:@1166.4]
  input  [11:0] io_decode_csr, // @[:@1166.4]
  output        io_status_debug, // @[:@1166.4]
  output [1:0]  io_status_prv, // @[:@1166.4]
  output        io_status_sd, // @[:@1166.4]
  output [7:0]  io_status_zero1, // @[:@1166.4]
  output        io_status_tsr, // @[:@1166.4]
  output        io_status_tw, // @[:@1166.4]
  output        io_status_tvm, // @[:@1166.4]
  output        io_status_mxr, // @[:@1166.4]
  output        io_status_sum, // @[:@1166.4]
  output        io_status_mprv, // @[:@1166.4]
  output [1:0]  io_status_xs, // @[:@1166.4]
  output [1:0]  io_status_fs, // @[:@1166.4]
  output [1:0]  io_status_mpp, // @[:@1166.4]
  output [1:0]  io_status_hpp, // @[:@1166.4]
  output        io_status_spp, // @[:@1166.4]
  output        io_status_mpie, // @[:@1166.4]
  output        io_status_hpie, // @[:@1166.4]
  output        io_status_spie, // @[:@1166.4]
  output        io_status_upie, // @[:@1166.4]
  output        io_status_mie, // @[:@1166.4]
  output        io_status_hie, // @[:@1166.4]
  output        io_status_sie, // @[:@1166.4]
  output        io_status_uie, // @[:@1166.4]
  output [31:0] io_evec, // @[:@1166.4]
  input         io_exception, // @[:@1166.4]
  input         io_retire, // @[:@1166.4]
  input  [31:0] io_pc, // @[:@1166.4]
  output [31:0] io_time // @[:@1166.4]
);
  reg [1:0] reg_mstatus_prv; // @[csr.scala 163:28:@1342.4]
  reg [31:0] _RAND_0;
  reg  reg_mstatus_mpie; // @[csr.scala 163:28:@1342.4]
  reg [31:0] _RAND_1;
  reg  reg_mstatus_mie; // @[csr.scala 163:28:@1342.4]
  reg [31:0] _RAND_2;
  reg [31:0] reg_mepc; // @[csr.scala 164:21:@1343.4]
  reg [31:0] _RAND_3;
  reg [31:0] reg_mcause; // @[csr.scala 165:23:@1344.4]
  reg [31:0] _RAND_4;
  reg [31:0] reg_mtval; // @[csr.scala 166:22:@1345.4]
  reg [31:0] _RAND_5;
  reg [31:0] reg_mscratch; // @[csr.scala 167:25:@1346.4]
  reg [31:0] _RAND_6;
  reg [31:0] reg_medeleg; // @[csr.scala 169:24:@1348.4]
  reg [31:0] _RAND_7;
  reg  reg_mip_mtip; // @[csr.scala 171:24:@1384.4]
  reg [31:0] _RAND_8;
  reg  reg_mip_msip; // @[csr.scala 171:24:@1384.4]
  reg [31:0] _RAND_9;
  reg  reg_mie_mtip; // @[csr.scala 172:24:@1420.4]
  reg [31:0] _RAND_10;
  reg  reg_mie_msip; // @[csr.scala 172:24:@1420.4]
  reg [31:0] _RAND_11;
  reg [5:0] _T_258; // @[util.scala 114:41:@1423.4]
  reg [31:0] _RAND_12;
  wire [6:0] _T_259; // @[util.scala 115:33:@1424.4]
  reg [57:0] _T_262; // @[util.scala 119:31:@1426.4]
  reg [63:0] _RAND_13;
  wire  _T_263; // @[util.scala 120:20:@1427.4]
  wire [58:0] _T_265; // @[util.scala 120:43:@1429.6]
  wire [57:0] _T_266; // @[util.scala 120:43:@1430.6]
  wire [57:0] _GEN_0; // @[util.scala 120:34:@1428.4]
  wire [63:0] _T_267; // @[Cat.scala 30:58:@1433.4]
  reg [5:0] _T_270; // @[util.scala 114:41:@1434.4]
  reg [31:0] _RAND_14;
  wire [5:0] _GEN_153; // @[util.scala 115:33:@1435.4]
  wire [6:0] _T_271; // @[util.scala 115:33:@1435.4]
  reg [57:0] _T_274; // @[util.scala 119:31:@1437.4]
  reg [63:0] _RAND_15;
  wire  _T_275; // @[util.scala 120:20:@1438.4]
  wire [58:0] _T_277; // @[util.scala 120:43:@1440.6]
  wire [57:0] _T_278; // @[util.scala 120:43:@1441.6]
  wire [57:0] _GEN_1; // @[util.scala 120:34:@1439.4]
  wire [63:0] _T_279; // @[Cat.scala 30:58:@1444.4]
  reg [39:0] _T_282; // @[util.scala 114:74:@1446.4]
  reg [63:0] _RAND_16;
  wire [40:0] _T_283; // @[util.scala 115:33:@1447.4]
  reg [39:0] _T_285; // @[util.scala 114:74:@1449.4]
  reg [63:0] _RAND_17;
  wire [40:0] _T_286; // @[util.scala 115:33:@1450.4]
  reg [39:0] _T_288; // @[util.scala 114:74:@1452.4]
  reg [63:0] _RAND_18;
  wire [40:0] _T_289; // @[util.scala 115:33:@1453.4]
  reg [39:0] _T_291; // @[util.scala 114:74:@1455.4]
  reg [63:0] _RAND_19;
  wire [40:0] _T_292; // @[util.scala 115:33:@1456.4]
  reg [39:0] _T_294; // @[util.scala 114:74:@1458.4]
  reg [63:0] _RAND_20;
  wire [40:0] _T_295; // @[util.scala 115:33:@1459.4]
  reg [39:0] _T_297; // @[util.scala 114:74:@1461.4]
  reg [63:0] _RAND_21;
  wire [40:0] _T_298; // @[util.scala 115:33:@1462.4]
  reg [39:0] _T_300; // @[util.scala 114:74:@1464.4]
  reg [63:0] _RAND_22;
  wire [40:0] _T_301; // @[util.scala 115:33:@1465.4]
  reg [39:0] _T_303; // @[util.scala 114:74:@1467.4]
  reg [63:0] _RAND_23;
  wire [40:0] _T_304; // @[util.scala 115:33:@1468.4]
  reg [39:0] _T_306; // @[util.scala 114:74:@1470.4]
  reg [63:0] _RAND_24;
  wire [40:0] _T_307; // @[util.scala 115:33:@1471.4]
  reg [39:0] _T_309; // @[util.scala 114:74:@1473.4]
  reg [63:0] _RAND_25;
  wire [40:0] _T_310; // @[util.scala 115:33:@1474.4]
  reg [39:0] _T_312; // @[util.scala 114:74:@1476.4]
  reg [63:0] _RAND_26;
  wire [40:0] _T_313; // @[util.scala 115:33:@1477.4]
  reg [39:0] _T_315; // @[util.scala 114:74:@1479.4]
  reg [63:0] _RAND_27;
  wire [40:0] _T_316; // @[util.scala 115:33:@1480.4]
  reg [39:0] _T_318; // @[util.scala 114:74:@1482.4]
  reg [63:0] _RAND_28;
  wire [40:0] _T_319; // @[util.scala 115:33:@1483.4]
  reg [39:0] _T_321; // @[util.scala 114:74:@1485.4]
  reg [63:0] _RAND_29;
  wire [40:0] _T_322; // @[util.scala 115:33:@1486.4]
  reg [39:0] _T_324; // @[util.scala 114:74:@1488.4]
  reg [63:0] _RAND_30;
  wire [40:0] _T_325; // @[util.scala 115:33:@1489.4]
  reg [39:0] _T_327; // @[util.scala 114:74:@1491.4]
  reg [63:0] _RAND_31;
  wire [40:0] _T_328; // @[util.scala 115:33:@1492.4]
  reg [39:0] _T_330; // @[util.scala 114:74:@1494.4]
  reg [63:0] _RAND_32;
  wire [40:0] _T_331; // @[util.scala 115:33:@1495.4]
  reg [39:0] _T_333; // @[util.scala 114:74:@1497.4]
  reg [63:0] _RAND_33;
  wire [40:0] _T_334; // @[util.scala 115:33:@1498.4]
  reg [39:0] _T_336; // @[util.scala 114:74:@1500.4]
  reg [63:0] _RAND_34;
  wire [40:0] _T_337; // @[util.scala 115:33:@1501.4]
  reg [39:0] _T_339; // @[util.scala 114:74:@1503.4]
  reg [63:0] _RAND_35;
  wire [40:0] _T_340; // @[util.scala 115:33:@1504.4]
  reg [39:0] _T_342; // @[util.scala 114:74:@1506.4]
  reg [63:0] _RAND_36;
  wire [40:0] _T_343; // @[util.scala 115:33:@1507.4]
  reg [39:0] _T_345; // @[util.scala 114:74:@1509.4]
  reg [63:0] _RAND_37;
  wire [40:0] _T_346; // @[util.scala 115:33:@1510.4]
  reg [39:0] _T_348; // @[util.scala 114:74:@1512.4]
  reg [63:0] _RAND_38;
  wire [40:0] _T_349; // @[util.scala 115:33:@1513.4]
  reg [39:0] _T_351; // @[util.scala 114:74:@1515.4]
  reg [63:0] _RAND_39;
  wire [40:0] _T_352; // @[util.scala 115:33:@1516.4]
  reg [39:0] _T_354; // @[util.scala 114:74:@1518.4]
  reg [63:0] _RAND_40;
  wire [40:0] _T_355; // @[util.scala 115:33:@1519.4]
  reg [39:0] _T_357; // @[util.scala 114:74:@1521.4]
  reg [63:0] _RAND_41;
  wire [40:0] _T_358; // @[util.scala 115:33:@1522.4]
  reg [39:0] _T_360; // @[util.scala 114:74:@1524.4]
  reg [63:0] _RAND_42;
  wire [40:0] _T_361; // @[util.scala 115:33:@1525.4]
  reg [39:0] _T_363; // @[util.scala 114:74:@1527.4]
  reg [63:0] _RAND_43;
  wire [40:0] _T_364; // @[util.scala 115:33:@1528.4]
  reg [39:0] _T_366; // @[util.scala 114:74:@1530.4]
  reg [63:0] _RAND_44;
  wire [40:0] _T_367; // @[util.scala 115:33:@1531.4]
  reg [39:0] _T_369; // @[util.scala 114:74:@1533.4]
  reg [63:0] _RAND_45;
  wire [40:0] _T_370; // @[util.scala 115:33:@1534.4]
  reg [39:0] _T_372; // @[util.scala 114:74:@1536.4]
  reg [63:0] _RAND_46;
  wire [40:0] _T_373; // @[util.scala 115:33:@1537.4]
  reg [39:0] _T_375; // @[util.scala 114:74:@1539.4]
  reg [63:0] _RAND_47;
  wire [40:0] _T_376; // @[util.scala 115:33:@1540.4]
  reg [31:0] reg_dpc; // @[csr.scala 188:20:@1630.4]
  reg [31:0] _RAND_48;
  reg [31:0] reg_dscratch; // @[csr.scala 189:25:@1631.4]
  reg [31:0] _RAND_49;
  reg  reg_dcsr_ebreakm; // @[csr.scala 194:25:@1684.4]
  reg [31:0] _RAND_50;
  reg  reg_dcsr_step; // @[csr.scala 194:25:@1684.4]
  reg [31:0] _RAND_51;
  wire  system_insn; // @[csr.scala 196:31:@1685.4]
  wire  _T_492; // @[csr.scala 197:27:@1686.4]
  wire  _T_494; // @[csr.scala 197:40:@1687.4]
  wire  cpu_ren; // @[csr.scala 197:37:@1688.4]
  wire [1:0] _T_495; // @[csr.scala 199:38:@1689.4]
  wire [1:0] _T_496; // @[csr.scala 199:38:@1690.4]
  wire [2:0] _T_497; // @[csr.scala 199:38:@1691.4]
  wire [4:0] _T_498; // @[csr.scala 199:38:@1692.4]
  wire [1:0] _T_499; // @[csr.scala 199:38:@1693.4]
  wire [2:0] _T_500; // @[csr.scala 199:38:@1694.4]
  wire [3:0] _T_501; // @[csr.scala 199:38:@1695.4]
  wire [4:0] _T_502; // @[csr.scala 199:38:@1696.4]
  wire [7:0] _T_503; // @[csr.scala 199:38:@1697.4]
  wire [12:0] _T_504; // @[csr.scala 199:38:@1698.4]
  wire [2:0] _T_505; // @[csr.scala 199:38:@1699.4]
  wire [4:0] _T_506; // @[csr.scala 199:38:@1700.4]
  wire [1:0] _T_507; // @[csr.scala 199:38:@1701.4]
  wire [2:0] _T_508; // @[csr.scala 199:38:@1702.4]
  wire [7:0] _T_509; // @[csr.scala 199:38:@1703.4]
  wire [8:0] _T_510; // @[csr.scala 199:38:@1704.4]
  wire [9:0] _T_511; // @[csr.scala 199:38:@1705.4]
  wire [2:0] _T_512; // @[csr.scala 199:38:@1706.4]
  wire [3:0] _T_513; // @[csr.scala 199:38:@1707.4]
  wire [13:0] _T_514; // @[csr.scala 199:38:@1708.4]
  wire [21:0] _T_515; // @[csr.scala 199:38:@1709.4]
  wire [34:0] read_mstatus; // @[csr.scala 199:38:@1710.4]
  wire [1:0] _T_523; // @[csr.scala 215:31:@1712.4]
  wire [3:0] _T_524; // @[csr.scala 215:31:@1713.4]
  wire [1:0] _T_526; // @[csr.scala 215:31:@1715.4]
  wire [3:0] _T_527; // @[csr.scala 215:31:@1716.4]
  wire [7:0] _T_528; // @[csr.scala 215:31:@1717.4]
  wire [15:0] _T_536; // @[csr.scala 215:31:@1725.4]
  wire [1:0] _T_538; // @[csr.scala 216:31:@1727.4]
  wire [3:0] _T_539; // @[csr.scala 216:31:@1728.4]
  wire [1:0] _T_541; // @[csr.scala 216:31:@1730.4]
  wire [3:0] _T_542; // @[csr.scala 216:31:@1731.4]
  wire [7:0] _T_543; // @[csr.scala 216:31:@1732.4]
  wire [15:0] _T_551; // @[csr.scala 216:31:@1740.4]
  wire [2:0] _T_552; // @[csr.scala 222:27:@1741.4]
  wire [4:0] _T_553; // @[csr.scala 222:27:@1742.4]
  wire [10:0] _T_557; // @[csr.scala 222:27:@1746.4]
  wire [12:0] _T_561; // @[csr.scala 222:27:@1750.4]
  wire [16:0] _T_563; // @[csr.scala 222:27:@1752.4]
  wire [20:0] _T_564; // @[csr.scala 222:27:@1753.4]
  wire [31:0] _T_565; // @[csr.scala 222:27:@1754.4]
  wire  _T_569; // @[csr.scala 259:76:@1755.4]
  wire  _T_571; // @[csr.scala 259:76:@1756.4]
  wire  _T_573; // @[csr.scala 259:76:@1757.4]
  wire  _T_579; // @[csr.scala 259:76:@1760.4]
  wire  _T_581; // @[csr.scala 259:76:@1761.4]
  wire  _T_583; // @[csr.scala 259:76:@1762.4]
  wire  _T_585; // @[csr.scala 259:76:@1763.4]
  wire  _T_587; // @[csr.scala 259:76:@1764.4]
  wire  _T_589; // @[csr.scala 259:76:@1765.4]
  wire  _T_591; // @[csr.scala 259:76:@1766.4]
  wire  _T_593; // @[csr.scala 259:76:@1767.4]
  wire  _T_595; // @[csr.scala 259:76:@1768.4]
  wire  _T_599; // @[csr.scala 259:76:@1770.4]
  wire  _T_601; // @[csr.scala 259:76:@1771.4]
  wire  _T_603; // @[csr.scala 259:76:@1772.4]
  wire  _T_605; // @[csr.scala 259:76:@1773.4]
  wire  _T_607; // @[csr.scala 259:76:@1774.4]
  wire  _T_609; // @[csr.scala 259:76:@1775.4]
  wire  _T_611; // @[csr.scala 259:76:@1776.4]
  wire  _T_613; // @[csr.scala 259:76:@1777.4]
  wire  _T_615; // @[csr.scala 259:76:@1778.4]
  wire  _T_617; // @[csr.scala 259:76:@1779.4]
  wire  _T_619; // @[csr.scala 259:76:@1780.4]
  wire  _T_621; // @[csr.scala 259:76:@1781.4]
  wire  _T_623; // @[csr.scala 259:76:@1782.4]
  wire  _T_625; // @[csr.scala 259:76:@1783.4]
  wire  _T_627; // @[csr.scala 259:76:@1784.4]
  wire  _T_629; // @[csr.scala 259:76:@1785.4]
  wire  _T_631; // @[csr.scala 259:76:@1786.4]
  wire  _T_633; // @[csr.scala 259:76:@1787.4]
  wire  _T_635; // @[csr.scala 259:76:@1788.4]
  wire  _T_637; // @[csr.scala 259:76:@1789.4]
  wire  _T_639; // @[csr.scala 259:76:@1790.4]
  wire  _T_641; // @[csr.scala 259:76:@1791.4]
  wire  _T_643; // @[csr.scala 259:76:@1792.4]
  wire  _T_645; // @[csr.scala 259:76:@1793.4]
  wire  _T_647; // @[csr.scala 259:76:@1794.4]
  wire  _T_649; // @[csr.scala 259:76:@1795.4]
  wire  _T_651; // @[csr.scala 259:76:@1796.4]
  wire  _T_653; // @[csr.scala 259:76:@1797.4]
  wire  _T_655; // @[csr.scala 259:76:@1798.4]
  wire  _T_657; // @[csr.scala 259:76:@1799.4]
  wire  _T_659; // @[csr.scala 259:76:@1800.4]
  wire  _T_661; // @[csr.scala 259:76:@1801.4]
  wire  _T_663; // @[csr.scala 259:76:@1802.4]
  wire  _T_665; // @[csr.scala 259:76:@1803.4]
  wire  _T_667; // @[csr.scala 259:76:@1804.4]
  wire  _T_669; // @[csr.scala 259:76:@1805.4]
  wire  _T_671; // @[csr.scala 259:76:@1806.4]
  wire  _T_673; // @[csr.scala 259:76:@1807.4]
  wire  _T_675; // @[csr.scala 259:76:@1808.4]
  wire  _T_677; // @[csr.scala 259:76:@1809.4]
  wire  _T_679; // @[csr.scala 259:76:@1810.4]
  wire  _T_681; // @[csr.scala 259:76:@1811.4]
  wire  _T_683; // @[csr.scala 259:76:@1812.4]
  wire  _T_685; // @[csr.scala 259:76:@1813.4]
  wire  _T_687; // @[csr.scala 259:76:@1814.4]
  wire  _T_689; // @[csr.scala 259:76:@1815.4]
  wire  _T_691; // @[csr.scala 259:76:@1816.4]
  wire  _T_693; // @[csr.scala 259:76:@1817.4]
  wire  _T_695; // @[csr.scala 259:76:@1818.4]
  wire  _T_697; // @[csr.scala 259:76:@1819.4]
  wire  _T_699; // @[csr.scala 259:76:@1820.4]
  wire  _T_701; // @[csr.scala 259:76:@1821.4]
  wire  _T_703; // @[csr.scala 259:76:@1822.4]
  wire  _T_705; // @[csr.scala 259:76:@1823.4]
  wire  _T_707; // @[csr.scala 259:76:@1824.4]
  wire  _T_709; // @[csr.scala 259:76:@1825.4]
  wire  _T_711; // @[csr.scala 259:76:@1826.4]
  wire  _T_713; // @[csr.scala 259:76:@1827.4]
  wire  _T_715; // @[csr.scala 259:76:@1828.4]
  wire  _T_717; // @[csr.scala 259:76:@1829.4]
  wire  _T_719; // @[csr.scala 259:76:@1830.4]
  wire  _T_721; // @[csr.scala 259:76:@1831.4]
  wire  _T_723; // @[csr.scala 259:76:@1832.4]
  wire  _T_725; // @[csr.scala 259:76:@1833.4]
  wire  _T_727; // @[csr.scala 259:76:@1834.4]
  wire  _T_729; // @[csr.scala 259:76:@1835.4]
  wire  _T_731; // @[csr.scala 259:76:@1836.4]
  wire  _T_733; // @[csr.scala 259:76:@1837.4]
  wire  _T_735; // @[csr.scala 259:76:@1838.4]
  wire  _T_737; // @[csr.scala 259:76:@1839.4]
  wire [1:0] _T_738; // @[csr.scala 261:57:@1840.4]
  wire  priv_sufficient; // @[csr.scala 261:41:@1841.4]
  wire [1:0] _T_739; // @[csr.scala 262:32:@1842.4]
  wire [1:0] _T_740; // @[csr.scala 262:40:@1843.4]
  wire  read_only; // @[csr.scala 262:40:@1844.4]
  wire  _T_742; // @[csr.scala 263:38:@1845.4]
  wire  _T_743; // @[csr.scala 263:25:@1846.4]
  wire  cpu_wen; // @[csr.scala 263:48:@1847.4]
  wire  _T_745; // @[csr.scala 264:24:@1848.4]
  wire  wen; // @[csr.scala 264:21:@1849.4]
  wire  _T_746; // @[util.scala 25:47:@1850.4]
  wire  _T_747; // @[util.scala 25:47:@1851.4]
  wire  _T_748; // @[util.scala 25:62:@1852.4]
  wire [31:0] _T_750; // @[csr.scala 390:9:@1853.4]
  wire [31:0] _T_751; // @[csr.scala 390:49:@1854.4]
  wire [31:0] _T_754; // @[csr.scala 390:64:@1856.4]
  wire [31:0] _T_755; // @[csr.scala 390:60:@1857.4]
  wire [31:0] wdata; // @[csr.scala 390:58:@1858.4]
  wire [2:0] _T_757; // @[csr.scala 267:36:@1859.4]
  wire [7:0] opcode; // @[csr.scala 267:20:@1860.4]
  wire  _T_758; // @[csr.scala 268:40:@1861.4]
  wire  insn_call; // @[csr.scala 268:31:@1862.4]
  wire  _T_759; // @[csr.scala 269:41:@1863.4]
  wire  insn_break; // @[csr.scala 269:32:@1864.4]
  wire  _T_760; // @[csr.scala 270:39:@1865.4]
  wire  _T_761; // @[csr.scala 270:30:@1866.4]
  wire  insn_ret; // @[csr.scala 270:43:@1867.4]
  wire  _T_1044; // @[csr.scala 282:24:@2085.4]
  wire [31:0] _GEN_2; // @[csr.scala 285:23:@2088.4]
  wire [31:0] _GEN_4; // @[csr.scala 285:23:@2088.4]
  wire [1:0] _T_1048; // @[Bitwise.scala 48:55:@2093.4]
  wire  _T_1050; // @[csr.scala 291:52:@2094.4]
  wire  _T_1052; // @[csr.scala 291:9:@2096.4]
  wire  _T_1054; // @[csr.scala 291:9:@2097.4]
  wire  _T_1057; // @[csr.scala 298:33:@2106.4]
  wire  _T_1058; // @[csr.scala 298:17:@2107.4]
  wire [1:0] _GEN_6; // @[csr.scala 298:38:@2108.4]
  wire [31:0] _GEN_8; // @[csr.scala 298:38:@2108.4]
  wire  _T_1062; // @[csr.scala 305:21:@2114.4]
  wire  _T_1063; // @[csr.scala 305:18:@2115.4]
  wire  _GEN_9; // @[csr.scala 305:41:@2116.4]
  wire  _GEN_10; // @[csr.scala 305:41:@2116.4]
  wire [1:0] new_prv; // @[csr.scala 305:41:@2116.4]
  wire [31:0] _GEN_12; // @[csr.scala 305:41:@2116.4]
  wire [3:0] _GEN_154; // @[csr.scala 315:35:@2124.6]
  wire [4:0] _T_1067; // @[csr.scala 315:35:@2124.6]
  wire [3:0] _T_1068; // @[csr.scala 315:35:@2125.6]
  wire [31:0] _GEN_13; // @[csr.scala 313:18:@2122.4]
  wire [31:0] _GEN_14; // @[csr.scala 313:18:@2122.4]
  wire [31:0] _GEN_16; // @[csr.scala 319:19:@2128.4]
  wire [63:0] _T_1073; // @[Mux.scala 19:72:@2134.4]
  wire [63:0] _T_1075; // @[Mux.scala 19:72:@2135.4]
  wire [15:0] _T_1077; // @[Mux.scala 19:72:@2136.4]
  wire [8:0] _T_1083; // @[Mux.scala 19:72:@2139.4]
  wire [34:0] _T_1085; // @[Mux.scala 19:72:@2140.4]
  wire [8:0] _T_1087; // @[Mux.scala 19:72:@2141.4]
  wire [15:0] _T_1089; // @[Mux.scala 19:72:@2142.4]
  wire [15:0] _T_1091; // @[Mux.scala 19:72:@2143.4]
  wire [31:0] _T_1093; // @[Mux.scala 19:72:@2144.4]
  wire [31:0] _T_1095; // @[Mux.scala 19:72:@2145.4]
  wire [31:0] _T_1097; // @[Mux.scala 19:72:@2146.4]
  wire [31:0] _T_1099; // @[Mux.scala 19:72:@2147.4]
  wire [31:0] _T_1103; // @[Mux.scala 19:72:@2149.4]
  wire [31:0] _T_1105; // @[Mux.scala 19:72:@2150.4]
  wire [31:0] _T_1107; // @[Mux.scala 19:72:@2151.4]
  wire [31:0] _T_1109; // @[Mux.scala 19:72:@2152.4]
  wire [39:0] _T_1111; // @[Mux.scala 19:72:@2153.4]
  wire [39:0] _T_1113; // @[Mux.scala 19:72:@2154.4]
  wire [39:0] _T_1115; // @[Mux.scala 19:72:@2155.4]
  wire [39:0] _T_1117; // @[Mux.scala 19:72:@2156.4]
  wire [39:0] _T_1119; // @[Mux.scala 19:72:@2157.4]
  wire [39:0] _T_1121; // @[Mux.scala 19:72:@2158.4]
  wire [39:0] _T_1123; // @[Mux.scala 19:72:@2159.4]
  wire [39:0] _T_1125; // @[Mux.scala 19:72:@2160.4]
  wire [39:0] _T_1127; // @[Mux.scala 19:72:@2161.4]
  wire [39:0] _T_1129; // @[Mux.scala 19:72:@2162.4]
  wire [39:0] _T_1131; // @[Mux.scala 19:72:@2163.4]
  wire [39:0] _T_1133; // @[Mux.scala 19:72:@2164.4]
  wire [39:0] _T_1135; // @[Mux.scala 19:72:@2165.4]
  wire [39:0] _T_1137; // @[Mux.scala 19:72:@2166.4]
  wire [39:0] _T_1139; // @[Mux.scala 19:72:@2167.4]
  wire [39:0] _T_1141; // @[Mux.scala 19:72:@2168.4]
  wire [39:0] _T_1143; // @[Mux.scala 19:72:@2169.4]
  wire [39:0] _T_1145; // @[Mux.scala 19:72:@2170.4]
  wire [39:0] _T_1147; // @[Mux.scala 19:72:@2171.4]
  wire [39:0] _T_1149; // @[Mux.scala 19:72:@2172.4]
  wire [39:0] _T_1151; // @[Mux.scala 19:72:@2173.4]
  wire [39:0] _T_1153; // @[Mux.scala 19:72:@2174.4]
  wire [39:0] _T_1155; // @[Mux.scala 19:72:@2175.4]
  wire [39:0] _T_1157; // @[Mux.scala 19:72:@2176.4]
  wire [39:0] _T_1159; // @[Mux.scala 19:72:@2177.4]
  wire [39:0] _T_1161; // @[Mux.scala 19:72:@2178.4]
  wire [39:0] _T_1163; // @[Mux.scala 19:72:@2179.4]
  wire [39:0] _T_1165; // @[Mux.scala 19:72:@2180.4]
  wire [39:0] _T_1167; // @[Mux.scala 19:72:@2181.4]
  wire [39:0] _T_1169; // @[Mux.scala 19:72:@2182.4]
  wire [39:0] _T_1171; // @[Mux.scala 19:72:@2183.4]
  wire [39:0] _T_1173; // @[Mux.scala 19:72:@2184.4]
  wire [39:0] _T_1175; // @[Mux.scala 19:72:@2185.4]
  wire [39:0] _T_1177; // @[Mux.scala 19:72:@2186.4]
  wire [39:0] _T_1179; // @[Mux.scala 19:72:@2187.4]
  wire [39:0] _T_1181; // @[Mux.scala 19:72:@2188.4]
  wire [39:0] _T_1183; // @[Mux.scala 19:72:@2189.4]
  wire [39:0] _T_1185; // @[Mux.scala 19:72:@2190.4]
  wire [39:0] _T_1187; // @[Mux.scala 19:72:@2191.4]
  wire [39:0] _T_1189; // @[Mux.scala 19:72:@2192.4]
  wire [39:0] _T_1191; // @[Mux.scala 19:72:@2193.4]
  wire [39:0] _T_1193; // @[Mux.scala 19:72:@2194.4]
  wire [39:0] _T_1195; // @[Mux.scala 19:72:@2195.4]
  wire [39:0] _T_1197; // @[Mux.scala 19:72:@2196.4]
  wire [39:0] _T_1199; // @[Mux.scala 19:72:@2197.4]
  wire [39:0] _T_1201; // @[Mux.scala 19:72:@2198.4]
  wire [39:0] _T_1203; // @[Mux.scala 19:72:@2199.4]
  wire [39:0] _T_1205; // @[Mux.scala 19:72:@2200.4]
  wire [39:0] _T_1207; // @[Mux.scala 19:72:@2201.4]
  wire [39:0] _T_1209; // @[Mux.scala 19:72:@2202.4]
  wire [39:0] _T_1211; // @[Mux.scala 19:72:@2203.4]
  wire [39:0] _T_1213; // @[Mux.scala 19:72:@2204.4]
  wire [39:0] _T_1215; // @[Mux.scala 19:72:@2205.4]
  wire [39:0] _T_1217; // @[Mux.scala 19:72:@2206.4]
  wire [39:0] _T_1219; // @[Mux.scala 19:72:@2207.4]
  wire [39:0] _T_1221; // @[Mux.scala 19:72:@2208.4]
  wire [39:0] _T_1223; // @[Mux.scala 19:72:@2209.4]
  wire [39:0] _T_1225; // @[Mux.scala 19:72:@2210.4]
  wire [39:0] _T_1227; // @[Mux.scala 19:72:@2211.4]
  wire [39:0] _T_1229; // @[Mux.scala 19:72:@2212.4]
  wire [39:0] _T_1231; // @[Mux.scala 19:72:@2213.4]
  wire [39:0] _T_1233; // @[Mux.scala 19:72:@2214.4]
  wire [39:0] _T_1235; // @[Mux.scala 19:72:@2215.4]
  wire [39:0] _T_1237; // @[Mux.scala 19:72:@2216.4]
  wire [63:0] _T_1242; // @[Mux.scala 19:72:@2219.4]
  wire [63:0] _GEN_155; // @[Mux.scala 19:72:@2220.4]
  wire [63:0] _T_1243; // @[Mux.scala 19:72:@2220.4]
  wire [63:0] _GEN_156; // @[Mux.scala 19:72:@2223.4]
  wire [63:0] _T_1246; // @[Mux.scala 19:72:@2223.4]
  wire [63:0] _GEN_157; // @[Mux.scala 19:72:@2224.4]
  wire [63:0] _T_1247; // @[Mux.scala 19:72:@2224.4]
  wire [63:0] _GEN_158; // @[Mux.scala 19:72:@2225.4]
  wire [63:0] _T_1248; // @[Mux.scala 19:72:@2225.4]
  wire [63:0] _GEN_159; // @[Mux.scala 19:72:@2226.4]
  wire [63:0] _T_1249; // @[Mux.scala 19:72:@2226.4]
  wire [63:0] _GEN_160; // @[Mux.scala 19:72:@2227.4]
  wire [63:0] _T_1250; // @[Mux.scala 19:72:@2227.4]
  wire [63:0] _GEN_161; // @[Mux.scala 19:72:@2228.4]
  wire [63:0] _T_1251; // @[Mux.scala 19:72:@2228.4]
  wire [63:0] _GEN_162; // @[Mux.scala 19:72:@2229.4]
  wire [63:0] _T_1252; // @[Mux.scala 19:72:@2229.4]
  wire [63:0] _GEN_163; // @[Mux.scala 19:72:@2230.4]
  wire [63:0] _T_1253; // @[Mux.scala 19:72:@2230.4]
  wire [63:0] _GEN_164; // @[Mux.scala 19:72:@2231.4]
  wire [63:0] _T_1254; // @[Mux.scala 19:72:@2231.4]
  wire [63:0] _GEN_165; // @[Mux.scala 19:72:@2233.4]
  wire [63:0] _T_1256; // @[Mux.scala 19:72:@2233.4]
  wire [63:0] _GEN_166; // @[Mux.scala 19:72:@2234.4]
  wire [63:0] _T_1257; // @[Mux.scala 19:72:@2234.4]
  wire [63:0] _GEN_167; // @[Mux.scala 19:72:@2235.4]
  wire [63:0] _T_1258; // @[Mux.scala 19:72:@2235.4]
  wire [63:0] _GEN_168; // @[Mux.scala 19:72:@2236.4]
  wire [63:0] _T_1259; // @[Mux.scala 19:72:@2236.4]
  wire [63:0] _GEN_169; // @[Mux.scala 19:72:@2237.4]
  wire [63:0] _T_1260; // @[Mux.scala 19:72:@2237.4]
  wire [63:0] _GEN_170; // @[Mux.scala 19:72:@2238.4]
  wire [63:0] _T_1261; // @[Mux.scala 19:72:@2238.4]
  wire [63:0] _GEN_171; // @[Mux.scala 19:72:@2239.4]
  wire [63:0] _T_1262; // @[Mux.scala 19:72:@2239.4]
  wire [63:0] _GEN_172; // @[Mux.scala 19:72:@2240.4]
  wire [63:0] _T_1263; // @[Mux.scala 19:72:@2240.4]
  wire [63:0] _GEN_173; // @[Mux.scala 19:72:@2241.4]
  wire [63:0] _T_1264; // @[Mux.scala 19:72:@2241.4]
  wire [63:0] _GEN_174; // @[Mux.scala 19:72:@2242.4]
  wire [63:0] _T_1265; // @[Mux.scala 19:72:@2242.4]
  wire [63:0] _GEN_175; // @[Mux.scala 19:72:@2243.4]
  wire [63:0] _T_1266; // @[Mux.scala 19:72:@2243.4]
  wire [63:0] _GEN_176; // @[Mux.scala 19:72:@2244.4]
  wire [63:0] _T_1267; // @[Mux.scala 19:72:@2244.4]
  wire [63:0] _GEN_177; // @[Mux.scala 19:72:@2245.4]
  wire [63:0] _T_1268; // @[Mux.scala 19:72:@2245.4]
  wire [63:0] _GEN_178; // @[Mux.scala 19:72:@2246.4]
  wire [63:0] _T_1269; // @[Mux.scala 19:72:@2246.4]
  wire [63:0] _GEN_179; // @[Mux.scala 19:72:@2247.4]
  wire [63:0] _T_1270; // @[Mux.scala 19:72:@2247.4]
  wire [63:0] _GEN_180; // @[Mux.scala 19:72:@2248.4]
  wire [63:0] _T_1271; // @[Mux.scala 19:72:@2248.4]
  wire [63:0] _GEN_181; // @[Mux.scala 19:72:@2249.4]
  wire [63:0] _T_1272; // @[Mux.scala 19:72:@2249.4]
  wire [63:0] _GEN_182; // @[Mux.scala 19:72:@2250.4]
  wire [63:0] _T_1273; // @[Mux.scala 19:72:@2250.4]
  wire [63:0] _GEN_183; // @[Mux.scala 19:72:@2251.4]
  wire [63:0] _T_1274; // @[Mux.scala 19:72:@2251.4]
  wire [63:0] _GEN_184; // @[Mux.scala 19:72:@2252.4]
  wire [63:0] _T_1275; // @[Mux.scala 19:72:@2252.4]
  wire [63:0] _GEN_185; // @[Mux.scala 19:72:@2253.4]
  wire [63:0] _T_1276; // @[Mux.scala 19:72:@2253.4]
  wire [63:0] _GEN_186; // @[Mux.scala 19:72:@2254.4]
  wire [63:0] _T_1277; // @[Mux.scala 19:72:@2254.4]
  wire [63:0] _GEN_187; // @[Mux.scala 19:72:@2255.4]
  wire [63:0] _T_1278; // @[Mux.scala 19:72:@2255.4]
  wire [63:0] _GEN_188; // @[Mux.scala 19:72:@2256.4]
  wire [63:0] _T_1279; // @[Mux.scala 19:72:@2256.4]
  wire [63:0] _GEN_189; // @[Mux.scala 19:72:@2257.4]
  wire [63:0] _T_1280; // @[Mux.scala 19:72:@2257.4]
  wire [63:0] _GEN_190; // @[Mux.scala 19:72:@2258.4]
  wire [63:0] _T_1281; // @[Mux.scala 19:72:@2258.4]
  wire [63:0] _GEN_191; // @[Mux.scala 19:72:@2259.4]
  wire [63:0] _T_1282; // @[Mux.scala 19:72:@2259.4]
  wire [63:0] _GEN_192; // @[Mux.scala 19:72:@2260.4]
  wire [63:0] _T_1283; // @[Mux.scala 19:72:@2260.4]
  wire [63:0] _GEN_193; // @[Mux.scala 19:72:@2261.4]
  wire [63:0] _T_1284; // @[Mux.scala 19:72:@2261.4]
  wire [63:0] _GEN_194; // @[Mux.scala 19:72:@2262.4]
  wire [63:0] _T_1285; // @[Mux.scala 19:72:@2262.4]
  wire [63:0] _GEN_195; // @[Mux.scala 19:72:@2263.4]
  wire [63:0] _T_1286; // @[Mux.scala 19:72:@2263.4]
  wire [63:0] _GEN_196; // @[Mux.scala 19:72:@2264.4]
  wire [63:0] _T_1287; // @[Mux.scala 19:72:@2264.4]
  wire [63:0] _GEN_197; // @[Mux.scala 19:72:@2265.4]
  wire [63:0] _T_1288; // @[Mux.scala 19:72:@2265.4]
  wire [63:0] _GEN_198; // @[Mux.scala 19:72:@2266.4]
  wire [63:0] _T_1289; // @[Mux.scala 19:72:@2266.4]
  wire [63:0] _GEN_199; // @[Mux.scala 19:72:@2267.4]
  wire [63:0] _T_1290; // @[Mux.scala 19:72:@2267.4]
  wire [63:0] _GEN_200; // @[Mux.scala 19:72:@2268.4]
  wire [63:0] _T_1291; // @[Mux.scala 19:72:@2268.4]
  wire [63:0] _GEN_201; // @[Mux.scala 19:72:@2269.4]
  wire [63:0] _T_1292; // @[Mux.scala 19:72:@2269.4]
  wire [63:0] _GEN_202; // @[Mux.scala 19:72:@2270.4]
  wire [63:0] _T_1293; // @[Mux.scala 19:72:@2270.4]
  wire [63:0] _GEN_203; // @[Mux.scala 19:72:@2271.4]
  wire [63:0] _T_1294; // @[Mux.scala 19:72:@2271.4]
  wire [63:0] _GEN_204; // @[Mux.scala 19:72:@2272.4]
  wire [63:0] _T_1295; // @[Mux.scala 19:72:@2272.4]
  wire [63:0] _GEN_205; // @[Mux.scala 19:72:@2273.4]
  wire [63:0] _T_1296; // @[Mux.scala 19:72:@2273.4]
  wire [63:0] _GEN_206; // @[Mux.scala 19:72:@2274.4]
  wire [63:0] _T_1297; // @[Mux.scala 19:72:@2274.4]
  wire [63:0] _GEN_207; // @[Mux.scala 19:72:@2275.4]
  wire [63:0] _T_1298; // @[Mux.scala 19:72:@2275.4]
  wire [63:0] _GEN_208; // @[Mux.scala 19:72:@2276.4]
  wire [63:0] _T_1299; // @[Mux.scala 19:72:@2276.4]
  wire [63:0] _GEN_209; // @[Mux.scala 19:72:@2277.4]
  wire [63:0] _T_1300; // @[Mux.scala 19:72:@2277.4]
  wire [63:0] _GEN_210; // @[Mux.scala 19:72:@2278.4]
  wire [63:0] _T_1301; // @[Mux.scala 19:72:@2278.4]
  wire [63:0] _GEN_211; // @[Mux.scala 19:72:@2279.4]
  wire [63:0] _T_1302; // @[Mux.scala 19:72:@2279.4]
  wire [63:0] _GEN_212; // @[Mux.scala 19:72:@2280.4]
  wire [63:0] _T_1303; // @[Mux.scala 19:72:@2280.4]
  wire [63:0] _GEN_213; // @[Mux.scala 19:72:@2281.4]
  wire [63:0] _T_1304; // @[Mux.scala 19:72:@2281.4]
  wire [63:0] _GEN_214; // @[Mux.scala 19:72:@2282.4]
  wire [63:0] _T_1305; // @[Mux.scala 19:72:@2282.4]
  wire [63:0] _GEN_215; // @[Mux.scala 19:72:@2283.4]
  wire [63:0] _T_1306; // @[Mux.scala 19:72:@2283.4]
  wire [63:0] _GEN_216; // @[Mux.scala 19:72:@2284.4]
  wire [63:0] _T_1307; // @[Mux.scala 19:72:@2284.4]
  wire [63:0] _GEN_217; // @[Mux.scala 19:72:@2285.4]
  wire [63:0] _T_1308; // @[Mux.scala 19:72:@2285.4]
  wire [63:0] _GEN_218; // @[Mux.scala 19:72:@2286.4]
  wire [63:0] _T_1309; // @[Mux.scala 19:72:@2286.4]
  wire [63:0] _GEN_219; // @[Mux.scala 19:72:@2287.4]
  wire [63:0] _T_1310; // @[Mux.scala 19:72:@2287.4]
  wire [63:0] _GEN_220; // @[Mux.scala 19:72:@2288.4]
  wire [63:0] _T_1311; // @[Mux.scala 19:72:@2288.4]
  wire [63:0] _GEN_221; // @[Mux.scala 19:72:@2289.4]
  wire [63:0] _T_1312; // @[Mux.scala 19:72:@2289.4]
  wire [63:0] _GEN_222; // @[Mux.scala 19:72:@2290.4]
  wire [63:0] _T_1313; // @[Mux.scala 19:72:@2290.4]
  wire [63:0] _GEN_223; // @[Mux.scala 19:72:@2291.4]
  wire [63:0] _T_1314; // @[Mux.scala 19:72:@2291.4]
  wire [63:0] _GEN_224; // @[Mux.scala 19:72:@2292.4]
  wire [63:0] _T_1315; // @[Mux.scala 19:72:@2292.4]
  wire [63:0] _GEN_225; // @[Mux.scala 19:72:@2293.4]
  wire [63:0] _T_1316; // @[Mux.scala 19:72:@2293.4]
  wire [63:0] _GEN_226; // @[Mux.scala 19:72:@2294.4]
  wire [63:0] _T_1317; // @[Mux.scala 19:72:@2294.4]
  wire [63:0] _GEN_227; // @[Mux.scala 19:72:@2295.4]
  wire [63:0] _T_1318; // @[Mux.scala 19:72:@2295.4]
  wire [63:0] _GEN_228; // @[Mux.scala 19:72:@2296.4]
  wire [63:0] _T_1319; // @[Mux.scala 19:72:@2296.4]
  wire [63:0] _GEN_229; // @[Mux.scala 19:72:@2297.4]
  wire [63:0] _T_1320; // @[Mux.scala 19:72:@2297.4]
  wire [63:0] _GEN_230; // @[Mux.scala 19:72:@2298.4]
  wire [63:0] _T_1321; // @[Mux.scala 19:72:@2298.4]
  wire [63:0] _GEN_231; // @[Mux.scala 19:72:@2299.4]
  wire [63:0] _T_1322; // @[Mux.scala 19:72:@2299.4]
  wire [63:0] _GEN_232; // @[Mux.scala 19:72:@2300.4]
  wire [63:0] _T_1323; // @[Mux.scala 19:72:@2300.4]
  wire  _T_1334; // @[csr.scala 334:38:@2313.8]
  wire  _T_1344; // @[csr.scala 334:38:@2333.8]
  wire  _GEN_17; // @[csr.scala 333:36:@2307.6]
  wire  _GEN_18; // @[csr.scala 333:36:@2307.6]
  wire [34:0] _T_1352; // @[:@2346.8 :@2347.8]
  wire  _T_1356; // @[csr.scala 341:39:@2354.8]
  wire  _T_1360; // @[csr.scala 341:39:@2362.8]
  wire  _GEN_19; // @[csr.scala 340:39:@2344.6]
  wire  _GEN_20; // @[csr.scala 340:39:@2344.6]
  wire [15:0] _T_1380; // @[:@2399.8 :@2400.8]
  wire  _T_1384; // @[csr.scala 346:35:@2407.8]
  wire  _T_1388; // @[csr.scala 346:35:@2415.8]
  wire  _GEN_21; // @[csr.scala 345:35:@2397.6]
  wire  _GEN_22; // @[csr.scala 349:35:@2435.6]
  wire  _GEN_23; // @[csr.scala 349:35:@2435.6]
  wire [7:0] _T_1418; // @[csr.scala 386:47:@2475.8]
  wire [31:0] _T_1419; // @[csr.scala 386:72:@2476.8]
  wire [39:0] _T_1420; // @[Cat.scala 30:58:@2477.8]
  wire [40:0] _GEN_24; // @[csr.scala 386:29:@2474.6]
  wire [7:0] _T_1421; // @[csr.scala 387:45:@2481.8]
  wire [39:0] _T_1422; // @[Cat.scala 30:58:@2482.8]
  wire [40:0] _GEN_25; // @[csr.scala 387:29:@2480.6]
  wire [31:0] _T_1424; // @[csr.scala 386:72:@2487.8]
  wire [39:0] _T_1425; // @[Cat.scala 30:58:@2488.8]
  wire [40:0] _GEN_26; // @[csr.scala 386:29:@2485.6]
  wire [7:0] _T_1426; // @[csr.scala 387:45:@2492.8]
  wire [39:0] _T_1427; // @[Cat.scala 30:58:@2493.8]
  wire [40:0] _GEN_27; // @[csr.scala 387:29:@2491.6]
  wire [31:0] _T_1429; // @[csr.scala 386:72:@2498.8]
  wire [39:0] _T_1430; // @[Cat.scala 30:58:@2499.8]
  wire [40:0] _GEN_28; // @[csr.scala 386:29:@2496.6]
  wire [7:0] _T_1431; // @[csr.scala 387:45:@2503.8]
  wire [39:0] _T_1432; // @[Cat.scala 30:58:@2504.8]
  wire [40:0] _GEN_29; // @[csr.scala 387:29:@2502.6]
  wire [31:0] _T_1434; // @[csr.scala 386:72:@2509.8]
  wire [39:0] _T_1435; // @[Cat.scala 30:58:@2510.8]
  wire [40:0] _GEN_30; // @[csr.scala 386:29:@2507.6]
  wire [7:0] _T_1436; // @[csr.scala 387:45:@2514.8]
  wire [39:0] _T_1437; // @[Cat.scala 30:58:@2515.8]
  wire [40:0] _GEN_31; // @[csr.scala 387:29:@2513.6]
  wire [31:0] _T_1439; // @[csr.scala 386:72:@2520.8]
  wire [39:0] _T_1440; // @[Cat.scala 30:58:@2521.8]
  wire [40:0] _GEN_32; // @[csr.scala 386:29:@2518.6]
  wire [7:0] _T_1441; // @[csr.scala 387:45:@2525.8]
  wire [39:0] _T_1442; // @[Cat.scala 30:58:@2526.8]
  wire [40:0] _GEN_33; // @[csr.scala 387:29:@2524.6]
  wire [31:0] _T_1444; // @[csr.scala 386:72:@2531.8]
  wire [39:0] _T_1445; // @[Cat.scala 30:58:@2532.8]
  wire [40:0] _GEN_34; // @[csr.scala 386:29:@2529.6]
  wire [7:0] _T_1446; // @[csr.scala 387:45:@2536.8]
  wire [39:0] _T_1447; // @[Cat.scala 30:58:@2537.8]
  wire [40:0] _GEN_35; // @[csr.scala 387:29:@2535.6]
  wire [31:0] _T_1449; // @[csr.scala 386:72:@2542.8]
  wire [39:0] _T_1450; // @[Cat.scala 30:58:@2543.8]
  wire [40:0] _GEN_36; // @[csr.scala 386:29:@2540.6]
  wire [7:0] _T_1451; // @[csr.scala 387:45:@2547.8]
  wire [39:0] _T_1452; // @[Cat.scala 30:58:@2548.8]
  wire [40:0] _GEN_37; // @[csr.scala 387:29:@2546.6]
  wire [31:0] _T_1454; // @[csr.scala 386:72:@2553.8]
  wire [39:0] _T_1455; // @[Cat.scala 30:58:@2554.8]
  wire [40:0] _GEN_38; // @[csr.scala 386:29:@2551.6]
  wire [7:0] _T_1456; // @[csr.scala 387:45:@2558.8]
  wire [39:0] _T_1457; // @[Cat.scala 30:58:@2559.8]
  wire [40:0] _GEN_39; // @[csr.scala 387:29:@2557.6]
  wire [31:0] _T_1459; // @[csr.scala 386:72:@2564.8]
  wire [39:0] _T_1460; // @[Cat.scala 30:58:@2565.8]
  wire [40:0] _GEN_40; // @[csr.scala 386:29:@2562.6]
  wire [7:0] _T_1461; // @[csr.scala 387:45:@2569.8]
  wire [39:0] _T_1462; // @[Cat.scala 30:58:@2570.8]
  wire [40:0] _GEN_41; // @[csr.scala 387:29:@2568.6]
  wire [31:0] _T_1464; // @[csr.scala 386:72:@2575.8]
  wire [39:0] _T_1465; // @[Cat.scala 30:58:@2576.8]
  wire [40:0] _GEN_42; // @[csr.scala 386:29:@2573.6]
  wire [7:0] _T_1466; // @[csr.scala 387:45:@2580.8]
  wire [39:0] _T_1467; // @[Cat.scala 30:58:@2581.8]
  wire [40:0] _GEN_43; // @[csr.scala 387:29:@2579.6]
  wire [31:0] _T_1469; // @[csr.scala 386:72:@2586.8]
  wire [39:0] _T_1470; // @[Cat.scala 30:58:@2587.8]
  wire [40:0] _GEN_44; // @[csr.scala 386:29:@2584.6]
  wire [7:0] _T_1471; // @[csr.scala 387:45:@2591.8]
  wire [39:0] _T_1472; // @[Cat.scala 30:58:@2592.8]
  wire [40:0] _GEN_45; // @[csr.scala 387:29:@2590.6]
  wire [31:0] _T_1474; // @[csr.scala 386:72:@2597.8]
  wire [39:0] _T_1475; // @[Cat.scala 30:58:@2598.8]
  wire [40:0] _GEN_46; // @[csr.scala 386:29:@2595.6]
  wire [7:0] _T_1476; // @[csr.scala 387:45:@2602.8]
  wire [39:0] _T_1477; // @[Cat.scala 30:58:@2603.8]
  wire [40:0] _GEN_47; // @[csr.scala 387:29:@2601.6]
  wire [31:0] _T_1479; // @[csr.scala 386:72:@2608.8]
  wire [39:0] _T_1480; // @[Cat.scala 30:58:@2609.8]
  wire [40:0] _GEN_48; // @[csr.scala 386:29:@2606.6]
  wire [7:0] _T_1481; // @[csr.scala 387:45:@2613.8]
  wire [39:0] _T_1482; // @[Cat.scala 30:58:@2614.8]
  wire [40:0] _GEN_49; // @[csr.scala 387:29:@2612.6]
  wire [31:0] _T_1484; // @[csr.scala 386:72:@2619.8]
  wire [39:0] _T_1485; // @[Cat.scala 30:58:@2620.8]
  wire [40:0] _GEN_50; // @[csr.scala 386:29:@2617.6]
  wire [7:0] _T_1486; // @[csr.scala 387:45:@2624.8]
  wire [39:0] _T_1487; // @[Cat.scala 30:58:@2625.8]
  wire [40:0] _GEN_51; // @[csr.scala 387:29:@2623.6]
  wire [31:0] _T_1489; // @[csr.scala 386:72:@2630.8]
  wire [39:0] _T_1490; // @[Cat.scala 30:58:@2631.8]
  wire [40:0] _GEN_52; // @[csr.scala 386:29:@2628.6]
  wire [7:0] _T_1491; // @[csr.scala 387:45:@2635.8]
  wire [39:0] _T_1492; // @[Cat.scala 30:58:@2636.8]
  wire [40:0] _GEN_53; // @[csr.scala 387:29:@2634.6]
  wire [31:0] _T_1494; // @[csr.scala 386:72:@2641.8]
  wire [39:0] _T_1495; // @[Cat.scala 30:58:@2642.8]
  wire [40:0] _GEN_54; // @[csr.scala 386:29:@2639.6]
  wire [7:0] _T_1496; // @[csr.scala 387:45:@2646.8]
  wire [39:0] _T_1497; // @[Cat.scala 30:58:@2647.8]
  wire [40:0] _GEN_55; // @[csr.scala 387:29:@2645.6]
  wire [31:0] _T_1499; // @[csr.scala 386:72:@2652.8]
  wire [39:0] _T_1500; // @[Cat.scala 30:58:@2653.8]
  wire [40:0] _GEN_56; // @[csr.scala 386:29:@2650.6]
  wire [7:0] _T_1501; // @[csr.scala 387:45:@2657.8]
  wire [39:0] _T_1502; // @[Cat.scala 30:58:@2658.8]
  wire [40:0] _GEN_57; // @[csr.scala 387:29:@2656.6]
  wire [31:0] _T_1504; // @[csr.scala 386:72:@2663.8]
  wire [39:0] _T_1505; // @[Cat.scala 30:58:@2664.8]
  wire [40:0] _GEN_58; // @[csr.scala 386:29:@2661.6]
  wire [7:0] _T_1506; // @[csr.scala 387:45:@2668.8]
  wire [39:0] _T_1507; // @[Cat.scala 30:58:@2669.8]
  wire [40:0] _GEN_59; // @[csr.scala 387:29:@2667.6]
  wire [31:0] _T_1509; // @[csr.scala 386:72:@2674.8]
  wire [39:0] _T_1510; // @[Cat.scala 30:58:@2675.8]
  wire [40:0] _GEN_60; // @[csr.scala 386:29:@2672.6]
  wire [7:0] _T_1511; // @[csr.scala 387:45:@2679.8]
  wire [39:0] _T_1512; // @[Cat.scala 30:58:@2680.8]
  wire [40:0] _GEN_61; // @[csr.scala 387:29:@2678.6]
  wire [31:0] _T_1514; // @[csr.scala 386:72:@2685.8]
  wire [39:0] _T_1515; // @[Cat.scala 30:58:@2686.8]
  wire [40:0] _GEN_62; // @[csr.scala 386:29:@2683.6]
  wire [7:0] _T_1516; // @[csr.scala 387:45:@2690.8]
  wire [39:0] _T_1517; // @[Cat.scala 30:58:@2691.8]
  wire [40:0] _GEN_63; // @[csr.scala 387:29:@2689.6]
  wire [31:0] _T_1519; // @[csr.scala 386:72:@2696.8]
  wire [39:0] _T_1520; // @[Cat.scala 30:58:@2697.8]
  wire [40:0] _GEN_64; // @[csr.scala 386:29:@2694.6]
  wire [7:0] _T_1521; // @[csr.scala 387:45:@2701.8]
  wire [39:0] _T_1522; // @[Cat.scala 30:58:@2702.8]
  wire [40:0] _GEN_65; // @[csr.scala 387:29:@2700.6]
  wire [31:0] _T_1524; // @[csr.scala 386:72:@2707.8]
  wire [39:0] _T_1525; // @[Cat.scala 30:58:@2708.8]
  wire [40:0] _GEN_66; // @[csr.scala 386:29:@2705.6]
  wire [7:0] _T_1526; // @[csr.scala 387:45:@2712.8]
  wire [39:0] _T_1527; // @[Cat.scala 30:58:@2713.8]
  wire [40:0] _GEN_67; // @[csr.scala 387:29:@2711.6]
  wire [31:0] _T_1529; // @[csr.scala 386:72:@2718.8]
  wire [39:0] _T_1530; // @[Cat.scala 30:58:@2719.8]
  wire [40:0] _GEN_68; // @[csr.scala 386:29:@2716.6]
  wire [7:0] _T_1531; // @[csr.scala 387:45:@2723.8]
  wire [39:0] _T_1532; // @[Cat.scala 30:58:@2724.8]
  wire [40:0] _GEN_69; // @[csr.scala 387:29:@2722.6]
  wire [31:0] _T_1534; // @[csr.scala 386:72:@2729.8]
  wire [39:0] _T_1535; // @[Cat.scala 30:58:@2730.8]
  wire [40:0] _GEN_70; // @[csr.scala 386:29:@2727.6]
  wire [7:0] _T_1536; // @[csr.scala 387:45:@2734.8]
  wire [39:0] _T_1537; // @[Cat.scala 30:58:@2735.8]
  wire [40:0] _GEN_71; // @[csr.scala 387:29:@2733.6]
  wire [31:0] _T_1539; // @[csr.scala 386:72:@2740.8]
  wire [39:0] _T_1540; // @[Cat.scala 30:58:@2741.8]
  wire [40:0] _GEN_72; // @[csr.scala 386:29:@2738.6]
  wire [7:0] _T_1541; // @[csr.scala 387:45:@2745.8]
  wire [39:0] _T_1542; // @[Cat.scala 30:58:@2746.8]
  wire [40:0] _GEN_73; // @[csr.scala 387:29:@2744.6]
  wire [31:0] _T_1544; // @[csr.scala 386:72:@2751.8]
  wire [39:0] _T_1545; // @[Cat.scala 30:58:@2752.8]
  wire [40:0] _GEN_74; // @[csr.scala 386:29:@2749.6]
  wire [7:0] _T_1546; // @[csr.scala 387:45:@2756.8]
  wire [39:0] _T_1547; // @[Cat.scala 30:58:@2757.8]
  wire [40:0] _GEN_75; // @[csr.scala 387:29:@2755.6]
  wire [31:0] _T_1549; // @[csr.scala 386:72:@2762.8]
  wire [39:0] _T_1550; // @[Cat.scala 30:58:@2763.8]
  wire [40:0] _GEN_76; // @[csr.scala 386:29:@2760.6]
  wire [7:0] _T_1551; // @[csr.scala 387:45:@2767.8]
  wire [39:0] _T_1552; // @[Cat.scala 30:58:@2768.8]
  wire [40:0] _GEN_77; // @[csr.scala 387:29:@2766.6]
  wire [31:0] _T_1554; // @[csr.scala 386:72:@2773.8]
  wire [39:0] _T_1555; // @[Cat.scala 30:58:@2774.8]
  wire [40:0] _GEN_78; // @[csr.scala 386:29:@2771.6]
  wire [7:0] _T_1556; // @[csr.scala 387:45:@2778.8]
  wire [39:0] _T_1557; // @[Cat.scala 30:58:@2779.8]
  wire [40:0] _GEN_79; // @[csr.scala 387:29:@2777.6]
  wire [31:0] _T_1559; // @[csr.scala 386:72:@2784.8]
  wire [39:0] _T_1560; // @[Cat.scala 30:58:@2785.8]
  wire [40:0] _GEN_80; // @[csr.scala 386:29:@2782.6]
  wire [7:0] _T_1561; // @[csr.scala 387:45:@2789.8]
  wire [39:0] _T_1562; // @[Cat.scala 30:58:@2790.8]
  wire [40:0] _GEN_81; // @[csr.scala 387:29:@2788.6]
  wire [31:0] _T_1564; // @[csr.scala 386:72:@2795.8]
  wire [39:0] _T_1565; // @[Cat.scala 30:58:@2796.8]
  wire [40:0] _GEN_82; // @[csr.scala 386:29:@2793.6]
  wire [7:0] _T_1566; // @[csr.scala 387:45:@2800.8]
  wire [39:0] _T_1567; // @[Cat.scala 30:58:@2801.8]
  wire [40:0] _GEN_83; // @[csr.scala 387:29:@2799.6]
  wire [31:0] _T_1569; // @[csr.scala 386:72:@2806.8]
  wire [39:0] _T_1570; // @[Cat.scala 30:58:@2807.8]
  wire [40:0] _GEN_84; // @[csr.scala 386:29:@2804.6]
  wire [7:0] _T_1571; // @[csr.scala 387:45:@2811.8]
  wire [39:0] _T_1572; // @[Cat.scala 30:58:@2812.8]
  wire [40:0] _GEN_85; // @[csr.scala 387:29:@2810.6]
  wire [31:0] _T_1574; // @[csr.scala 386:72:@2817.8]
  wire [39:0] _T_1575; // @[Cat.scala 30:58:@2818.8]
  wire [40:0] _GEN_86; // @[csr.scala 386:29:@2815.6]
  wire [7:0] _T_1576; // @[csr.scala 387:45:@2822.8]
  wire [39:0] _T_1577; // @[Cat.scala 30:58:@2823.8]
  wire [40:0] _GEN_87; // @[csr.scala 387:29:@2821.6]
  wire [31:0] _T_1579; // @[csr.scala 386:72:@2828.8]
  wire [63:0] _T_1580; // @[Cat.scala 30:58:@2829.8]
  wire [57:0] _T_1581; // @[util.scala 135:28:@2831.8]
  wire [63:0] _GEN_88; // @[csr.scala 386:29:@2826.6]
  wire [57:0] _GEN_89; // @[csr.scala 386:29:@2826.6]
  wire [31:0] _T_1582; // @[csr.scala 387:45:@2835.8]
  wire [63:0] _T_1583; // @[Cat.scala 30:58:@2836.8]
  wire [57:0] _T_1584; // @[util.scala 135:28:@2838.8]
  wire [63:0] _GEN_90; // @[csr.scala 387:29:@2834.6]
  wire [57:0] _GEN_91; // @[csr.scala 387:29:@2834.6]
  wire [31:0] _T_1586; // @[csr.scala 386:72:@2843.8]
  wire [63:0] _T_1587; // @[Cat.scala 30:58:@2844.8]
  wire [57:0] _T_1588; // @[util.scala 135:28:@2846.8]
  wire [63:0] _GEN_92; // @[csr.scala 386:29:@2841.6]
  wire [57:0] _GEN_93; // @[csr.scala 386:29:@2841.6]
  wire [31:0] _T_1589; // @[csr.scala 387:45:@2850.8]
  wire [63:0] _T_1590; // @[Cat.scala 30:58:@2851.8]
  wire [57:0] _T_1591; // @[util.scala 135:28:@2853.8]
  wire [63:0] _GEN_94; // @[csr.scala 387:29:@2849.6]
  wire [57:0] _GEN_95; // @[csr.scala 387:29:@2849.6]
  wire [31:0] _GEN_96; // @[csr.scala 365:40:@2856.6]
  wire [31:0] _GEN_97; // @[csr.scala 366:40:@2859.6]
  wire [31:0] _T_1594; // @[csr.scala 368:78:@2864.8]
  wire [34:0] _GEN_233; // @[csr.scala 368:86:@2865.8]
  wire [34:0] _T_1596; // @[csr.scala 368:86:@2865.8]
  wire [34:0] _GEN_98; // @[csr.scala 368:40:@2862.6]
  wire [31:0] _GEN_99; // @[csr.scala 369:40:@2868.6]
  wire [31:0] _T_1598; // @[csr.scala 370:62:@2872.8]
  wire [31:0] _GEN_100; // @[csr.scala 370:40:@2871.6]
  wire [31:0] _GEN_101; // @[csr.scala 371:40:@2875.6]
  wire [31:0] _GEN_102; // @[csr.scala 372:42:@2879.6]
  wire  _GEN_103; // @[csr.scala 331:14:@2306.4]
  wire  _GEN_104; // @[csr.scala 331:14:@2306.4]
  wire  _GEN_105; // @[csr.scala 331:14:@2306.4]
  wire  _GEN_106; // @[csr.scala 331:14:@2306.4]
  wire  _GEN_107; // @[csr.scala 331:14:@2306.4]
  wire  _GEN_108; // @[csr.scala 331:14:@2306.4]
  wire  _GEN_109; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_110; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_111; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_112; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_113; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_114; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_115; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_116; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_117; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_118; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_119; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_120; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_121; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_122; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_123; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_124; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_125; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_126; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_127; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_128; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_129; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_130; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_131; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_132; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_133; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_134; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_135; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_136; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_137; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_138; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_139; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_140; // @[csr.scala 331:14:@2306.4]
  wire [40:0] _GEN_141; // @[csr.scala 331:14:@2306.4]
  wire [63:0] _GEN_142; // @[csr.scala 331:14:@2306.4]
  wire [57:0] _GEN_143; // @[csr.scala 331:14:@2306.4]
  wire [63:0] _GEN_144; // @[csr.scala 331:14:@2306.4]
  wire [57:0] _GEN_145; // @[csr.scala 331:14:@2306.4]
  wire [34:0] _GEN_148; // @[csr.scala 331:14:@2306.4]
  assign _T_259 = _T_258 + 6'h1; // @[util.scala 115:33:@1424.4]
  assign _T_263 = _T_259[6]; // @[util.scala 120:20:@1427.4]
  assign _T_265 = _T_262 + 58'h1; // @[util.scala 120:43:@1429.6]
  assign _T_266 = _T_265[57:0]; // @[util.scala 120:43:@1430.6]
  assign _GEN_0 = _T_263 ? _T_266 : _T_262; // @[util.scala 120:34:@1428.4]
  assign _T_267 = {_T_262,_T_258}; // @[Cat.scala 30:58:@1433.4]
  assign _GEN_153 = {{5'd0}, io_retire}; // @[util.scala 115:33:@1435.4]
  assign _T_271 = _T_270 + _GEN_153; // @[util.scala 115:33:@1435.4]
  assign _T_275 = _T_271[6]; // @[util.scala 120:20:@1438.4]
  assign _T_277 = _T_274 + 58'h1; // @[util.scala 120:43:@1440.6]
  assign _T_278 = _T_277[57:0]; // @[util.scala 120:43:@1441.6]
  assign _GEN_1 = _T_275 ? _T_278 : _T_274; // @[util.scala 120:34:@1439.4]
  assign _T_279 = {_T_274,_T_270}; // @[Cat.scala 30:58:@1444.4]
  assign _T_283 = {{1'd0}, _T_282}; // @[util.scala 115:33:@1447.4]
  assign _T_286 = {{1'd0}, _T_285}; // @[util.scala 115:33:@1450.4]
  assign _T_289 = {{1'd0}, _T_288}; // @[util.scala 115:33:@1453.4]
  assign _T_292 = {{1'd0}, _T_291}; // @[util.scala 115:33:@1456.4]
  assign _T_295 = {{1'd0}, _T_294}; // @[util.scala 115:33:@1459.4]
  assign _T_298 = {{1'd0}, _T_297}; // @[util.scala 115:33:@1462.4]
  assign _T_301 = {{1'd0}, _T_300}; // @[util.scala 115:33:@1465.4]
  assign _T_304 = {{1'd0}, _T_303}; // @[util.scala 115:33:@1468.4]
  assign _T_307 = {{1'd0}, _T_306}; // @[util.scala 115:33:@1471.4]
  assign _T_310 = {{1'd0}, _T_309}; // @[util.scala 115:33:@1474.4]
  assign _T_313 = {{1'd0}, _T_312}; // @[util.scala 115:33:@1477.4]
  assign _T_316 = {{1'd0}, _T_315}; // @[util.scala 115:33:@1480.4]
  assign _T_319 = {{1'd0}, _T_318}; // @[util.scala 115:33:@1483.4]
  assign _T_322 = {{1'd0}, _T_321}; // @[util.scala 115:33:@1486.4]
  assign _T_325 = {{1'd0}, _T_324}; // @[util.scala 115:33:@1489.4]
  assign _T_328 = {{1'd0}, _T_327}; // @[util.scala 115:33:@1492.4]
  assign _T_331 = {{1'd0}, _T_330}; // @[util.scala 115:33:@1495.4]
  assign _T_334 = {{1'd0}, _T_333}; // @[util.scala 115:33:@1498.4]
  assign _T_337 = {{1'd0}, _T_336}; // @[util.scala 115:33:@1501.4]
  assign _T_340 = {{1'd0}, _T_339}; // @[util.scala 115:33:@1504.4]
  assign _T_343 = {{1'd0}, _T_342}; // @[util.scala 115:33:@1507.4]
  assign _T_346 = {{1'd0}, _T_345}; // @[util.scala 115:33:@1510.4]
  assign _T_349 = {{1'd0}, _T_348}; // @[util.scala 115:33:@1513.4]
  assign _T_352 = {{1'd0}, _T_351}; // @[util.scala 115:33:@1516.4]
  assign _T_355 = {{1'd0}, _T_354}; // @[util.scala 115:33:@1519.4]
  assign _T_358 = {{1'd0}, _T_357}; // @[util.scala 115:33:@1522.4]
  assign _T_361 = {{1'd0}, _T_360}; // @[util.scala 115:33:@1525.4]
  assign _T_364 = {{1'd0}, _T_363}; // @[util.scala 115:33:@1528.4]
  assign _T_367 = {{1'd0}, _T_366}; // @[util.scala 115:33:@1531.4]
  assign _T_370 = {{1'd0}, _T_369}; // @[util.scala 115:33:@1534.4]
  assign _T_373 = {{1'd0}, _T_372}; // @[util.scala 115:33:@1537.4]
  assign _T_376 = {{1'd0}, _T_375}; // @[util.scala 115:33:@1540.4]
  assign system_insn = io_rw_cmd == 3'h4; // @[csr.scala 196:31:@1685.4]
  assign _T_492 = io_rw_cmd != 3'h0; // @[csr.scala 197:27:@1686.4]
  assign _T_494 = system_insn == 1'h0; // @[csr.scala 197:40:@1687.4]
  assign cpu_ren = _T_492 & _T_494; // @[csr.scala 197:37:@1688.4]
  assign _T_495 = {io_status_sie,io_status_uie}; // @[csr.scala 199:38:@1689.4]
  assign _T_496 = {io_status_upie,io_status_mie}; // @[csr.scala 199:38:@1690.4]
  assign _T_497 = {_T_496,io_status_hie}; // @[csr.scala 199:38:@1691.4]
  assign _T_498 = {_T_497,_T_495}; // @[csr.scala 199:38:@1692.4]
  assign _T_499 = {io_status_mpie,io_status_hpie}; // @[csr.scala 199:38:@1693.4]
  assign _T_500 = {_T_499,io_status_spie}; // @[csr.scala 199:38:@1694.4]
  assign _T_501 = {io_status_mpp,io_status_hpp}; // @[csr.scala 199:38:@1695.4]
  assign _T_502 = {_T_501,io_status_spp}; // @[csr.scala 199:38:@1696.4]
  assign _T_503 = {_T_502,_T_500}; // @[csr.scala 199:38:@1697.4]
  assign _T_504 = {_T_503,_T_498}; // @[csr.scala 199:38:@1698.4]
  assign _T_505 = {io_status_mprv,io_status_xs}; // @[csr.scala 199:38:@1699.4]
  assign _T_506 = {_T_505,io_status_fs}; // @[csr.scala 199:38:@1700.4]
  assign _T_507 = {io_status_tvm,io_status_mxr}; // @[csr.scala 199:38:@1701.4]
  assign _T_508 = {_T_507,io_status_sum}; // @[csr.scala 199:38:@1702.4]
  assign _T_509 = {_T_508,_T_506}; // @[csr.scala 199:38:@1703.4]
  assign _T_510 = {io_status_zero1,io_status_tsr}; // @[csr.scala 199:38:@1704.4]
  assign _T_511 = {_T_510,io_status_tw}; // @[csr.scala 199:38:@1705.4]
  assign _T_512 = {io_status_debug,io_status_prv}; // @[csr.scala 199:38:@1706.4]
  assign _T_513 = {_T_512,io_status_sd}; // @[csr.scala 199:38:@1707.4]
  assign _T_514 = {_T_513,_T_511}; // @[csr.scala 199:38:@1708.4]
  assign _T_515 = {_T_514,_T_509}; // @[csr.scala 199:38:@1709.4]
  assign read_mstatus = {_T_515,_T_504}; // @[csr.scala 199:38:@1710.4]
  assign _T_523 = {reg_mip_msip,1'h0}; // @[csr.scala 215:31:@1712.4]
  assign _T_524 = {_T_523,2'h0}; // @[csr.scala 215:31:@1713.4]
  assign _T_526 = {reg_mip_mtip,1'h0}; // @[csr.scala 215:31:@1715.4]
  assign _T_527 = {_T_526,2'h0}; // @[csr.scala 215:31:@1716.4]
  assign _T_528 = {_T_527,_T_524}; // @[csr.scala 215:31:@1717.4]
  assign _T_536 = {8'h0,_T_528}; // @[csr.scala 215:31:@1725.4]
  assign _T_538 = {reg_mie_msip,1'h0}; // @[csr.scala 216:31:@1727.4]
  assign _T_539 = {_T_538,2'h0}; // @[csr.scala 216:31:@1728.4]
  assign _T_541 = {reg_mie_mtip,1'h0}; // @[csr.scala 216:31:@1730.4]
  assign _T_542 = {_T_541,2'h0}; // @[csr.scala 216:31:@1731.4]
  assign _T_543 = {_T_542,_T_539}; // @[csr.scala 216:31:@1732.4]
  assign _T_551 = {8'h0,_T_543}; // @[csr.scala 216:31:@1740.4]
  assign _T_552 = {2'h0,reg_dcsr_step}; // @[csr.scala 222:27:@1741.4]
  assign _T_553 = {_T_552,2'h3}; // @[csr.scala 222:27:@1742.4]
  assign _T_557 = {6'h0,_T_553}; // @[csr.scala 222:27:@1746.4]
  assign _T_561 = {12'h0,reg_dcsr_ebreakm}; // @[csr.scala 222:27:@1750.4]
  assign _T_563 = {4'h4,_T_561}; // @[csr.scala 222:27:@1752.4]
  assign _T_564 = {_T_563,4'h0}; // @[csr.scala 222:27:@1753.4]
  assign _T_565 = {_T_564,_T_557}; // @[csr.scala 222:27:@1754.4]
  assign _T_569 = io_decode_csr == 12'hb00; // @[csr.scala 259:76:@1755.4]
  assign _T_571 = io_decode_csr == 12'hb02; // @[csr.scala 259:76:@1756.4]
  assign _T_573 = io_decode_csr == 12'hf13; // @[csr.scala 259:76:@1757.4]
  assign _T_579 = io_decode_csr == 12'h301; // @[csr.scala 259:76:@1760.4]
  assign _T_581 = io_decode_csr == 12'h300; // @[csr.scala 259:76:@1761.4]
  assign _T_583 = io_decode_csr == 12'h305; // @[csr.scala 259:76:@1762.4]
  assign _T_585 = io_decode_csr == 12'h344; // @[csr.scala 259:76:@1763.4]
  assign _T_587 = io_decode_csr == 12'h304; // @[csr.scala 259:76:@1764.4]
  assign _T_589 = io_decode_csr == 12'h340; // @[csr.scala 259:76:@1765.4]
  assign _T_591 = io_decode_csr == 12'h341; // @[csr.scala 259:76:@1766.4]
  assign _T_593 = io_decode_csr == 12'h343; // @[csr.scala 259:76:@1767.4]
  assign _T_595 = io_decode_csr == 12'h342; // @[csr.scala 259:76:@1768.4]
  assign _T_599 = io_decode_csr == 12'h7b0; // @[csr.scala 259:76:@1770.4]
  assign _T_601 = io_decode_csr == 12'h7b1; // @[csr.scala 259:76:@1771.4]
  assign _T_603 = io_decode_csr == 12'h7b2; // @[csr.scala 259:76:@1772.4]
  assign _T_605 = io_decode_csr == 12'h302; // @[csr.scala 259:76:@1773.4]
  assign _T_607 = io_decode_csr == 12'hb03; // @[csr.scala 259:76:@1774.4]
  assign _T_609 = io_decode_csr == 12'hb83; // @[csr.scala 259:76:@1775.4]
  assign _T_611 = io_decode_csr == 12'hb04; // @[csr.scala 259:76:@1776.4]
  assign _T_613 = io_decode_csr == 12'hb84; // @[csr.scala 259:76:@1777.4]
  assign _T_615 = io_decode_csr == 12'hb05; // @[csr.scala 259:76:@1778.4]
  assign _T_617 = io_decode_csr == 12'hb85; // @[csr.scala 259:76:@1779.4]
  assign _T_619 = io_decode_csr == 12'hb06; // @[csr.scala 259:76:@1780.4]
  assign _T_621 = io_decode_csr == 12'hb86; // @[csr.scala 259:76:@1781.4]
  assign _T_623 = io_decode_csr == 12'hb07; // @[csr.scala 259:76:@1782.4]
  assign _T_625 = io_decode_csr == 12'hb87; // @[csr.scala 259:76:@1783.4]
  assign _T_627 = io_decode_csr == 12'hb08; // @[csr.scala 259:76:@1784.4]
  assign _T_629 = io_decode_csr == 12'hb88; // @[csr.scala 259:76:@1785.4]
  assign _T_631 = io_decode_csr == 12'hb09; // @[csr.scala 259:76:@1786.4]
  assign _T_633 = io_decode_csr == 12'hb89; // @[csr.scala 259:76:@1787.4]
  assign _T_635 = io_decode_csr == 12'hb0a; // @[csr.scala 259:76:@1788.4]
  assign _T_637 = io_decode_csr == 12'hb8a; // @[csr.scala 259:76:@1789.4]
  assign _T_639 = io_decode_csr == 12'hb0b; // @[csr.scala 259:76:@1790.4]
  assign _T_641 = io_decode_csr == 12'hb8b; // @[csr.scala 259:76:@1791.4]
  assign _T_643 = io_decode_csr == 12'hb0c; // @[csr.scala 259:76:@1792.4]
  assign _T_645 = io_decode_csr == 12'hb8c; // @[csr.scala 259:76:@1793.4]
  assign _T_647 = io_decode_csr == 12'hb0d; // @[csr.scala 259:76:@1794.4]
  assign _T_649 = io_decode_csr == 12'hb8d; // @[csr.scala 259:76:@1795.4]
  assign _T_651 = io_decode_csr == 12'hb0e; // @[csr.scala 259:76:@1796.4]
  assign _T_653 = io_decode_csr == 12'hb8e; // @[csr.scala 259:76:@1797.4]
  assign _T_655 = io_decode_csr == 12'hb0f; // @[csr.scala 259:76:@1798.4]
  assign _T_657 = io_decode_csr == 12'hb8f; // @[csr.scala 259:76:@1799.4]
  assign _T_659 = io_decode_csr == 12'hb10; // @[csr.scala 259:76:@1800.4]
  assign _T_661 = io_decode_csr == 12'hb90; // @[csr.scala 259:76:@1801.4]
  assign _T_663 = io_decode_csr == 12'hb11; // @[csr.scala 259:76:@1802.4]
  assign _T_665 = io_decode_csr == 12'hb91; // @[csr.scala 259:76:@1803.4]
  assign _T_667 = io_decode_csr == 12'hb12; // @[csr.scala 259:76:@1804.4]
  assign _T_669 = io_decode_csr == 12'hb92; // @[csr.scala 259:76:@1805.4]
  assign _T_671 = io_decode_csr == 12'hb13; // @[csr.scala 259:76:@1806.4]
  assign _T_673 = io_decode_csr == 12'hb93; // @[csr.scala 259:76:@1807.4]
  assign _T_675 = io_decode_csr == 12'hb14; // @[csr.scala 259:76:@1808.4]
  assign _T_677 = io_decode_csr == 12'hb94; // @[csr.scala 259:76:@1809.4]
  assign _T_679 = io_decode_csr == 12'hb15; // @[csr.scala 259:76:@1810.4]
  assign _T_681 = io_decode_csr == 12'hb95; // @[csr.scala 259:76:@1811.4]
  assign _T_683 = io_decode_csr == 12'hb16; // @[csr.scala 259:76:@1812.4]
  assign _T_685 = io_decode_csr == 12'hb96; // @[csr.scala 259:76:@1813.4]
  assign _T_687 = io_decode_csr == 12'hb17; // @[csr.scala 259:76:@1814.4]
  assign _T_689 = io_decode_csr == 12'hb97; // @[csr.scala 259:76:@1815.4]
  assign _T_691 = io_decode_csr == 12'hb18; // @[csr.scala 259:76:@1816.4]
  assign _T_693 = io_decode_csr == 12'hb98; // @[csr.scala 259:76:@1817.4]
  assign _T_695 = io_decode_csr == 12'hb19; // @[csr.scala 259:76:@1818.4]
  assign _T_697 = io_decode_csr == 12'hb99; // @[csr.scala 259:76:@1819.4]
  assign _T_699 = io_decode_csr == 12'hb1a; // @[csr.scala 259:76:@1820.4]
  assign _T_701 = io_decode_csr == 12'hb9a; // @[csr.scala 259:76:@1821.4]
  assign _T_703 = io_decode_csr == 12'hb1b; // @[csr.scala 259:76:@1822.4]
  assign _T_705 = io_decode_csr == 12'hb9b; // @[csr.scala 259:76:@1823.4]
  assign _T_707 = io_decode_csr == 12'hb1c; // @[csr.scala 259:76:@1824.4]
  assign _T_709 = io_decode_csr == 12'hb9c; // @[csr.scala 259:76:@1825.4]
  assign _T_711 = io_decode_csr == 12'hb1d; // @[csr.scala 259:76:@1826.4]
  assign _T_713 = io_decode_csr == 12'hb9d; // @[csr.scala 259:76:@1827.4]
  assign _T_715 = io_decode_csr == 12'hb1e; // @[csr.scala 259:76:@1828.4]
  assign _T_717 = io_decode_csr == 12'hb9e; // @[csr.scala 259:76:@1829.4]
  assign _T_719 = io_decode_csr == 12'hb1f; // @[csr.scala 259:76:@1830.4]
  assign _T_721 = io_decode_csr == 12'hb9f; // @[csr.scala 259:76:@1831.4]
  assign _T_723 = io_decode_csr == 12'hb20; // @[csr.scala 259:76:@1832.4]
  assign _T_725 = io_decode_csr == 12'hba0; // @[csr.scala 259:76:@1833.4]
  assign _T_727 = io_decode_csr == 12'hb21; // @[csr.scala 259:76:@1834.4]
  assign _T_729 = io_decode_csr == 12'hba1; // @[csr.scala 259:76:@1835.4]
  assign _T_731 = io_decode_csr == 12'hb22; // @[csr.scala 259:76:@1836.4]
  assign _T_733 = io_decode_csr == 12'hba2; // @[csr.scala 259:76:@1837.4]
  assign _T_735 = io_decode_csr == 12'hb80; // @[csr.scala 259:76:@1838.4]
  assign _T_737 = io_decode_csr == 12'hb82; // @[csr.scala 259:76:@1839.4]
  assign _T_738 = io_decode_csr[9:8]; // @[csr.scala 261:57:@1840.4]
  assign priv_sufficient = reg_mstatus_prv >= _T_738; // @[csr.scala 261:41:@1841.4]
  assign _T_739 = io_decode_csr[11:10]; // @[csr.scala 262:32:@1842.4]
  assign _T_740 = ~ _T_739; // @[csr.scala 262:40:@1843.4]
  assign read_only = _T_740 == 2'h0; // @[csr.scala 262:40:@1844.4]
  assign _T_742 = io_rw_cmd != 3'h5; // @[csr.scala 263:38:@1845.4]
  assign _T_743 = cpu_ren & _T_742; // @[csr.scala 263:25:@1846.4]
  assign cpu_wen = _T_743 & priv_sufficient; // @[csr.scala 263:48:@1847.4]
  assign _T_745 = read_only == 1'h0; // @[csr.scala 264:24:@1848.4]
  assign wen = cpu_wen & _T_745; // @[csr.scala 264:21:@1849.4]
  assign _T_746 = io_rw_cmd == 3'h2; // @[util.scala 25:47:@1850.4]
  assign _T_747 = io_rw_cmd == 3'h3; // @[util.scala 25:47:@1851.4]
  assign _T_748 = _T_746 | _T_747; // @[util.scala 25:62:@1852.4]
  assign _T_750 = _T_748 ? io_rw_rdata : 32'h0; // @[csr.scala 390:9:@1853.4]
  assign _T_751 = _T_750 | io_rw_wdata; // @[csr.scala 390:49:@1854.4]
  assign _T_754 = _T_747 ? io_rw_wdata : 32'h0; // @[csr.scala 390:64:@1856.4]
  assign _T_755 = ~ _T_754; // @[csr.scala 390:60:@1857.4]
  assign wdata = _T_751 & _T_755; // @[csr.scala 390:58:@1858.4]
  assign _T_757 = io_decode_csr[2:0]; // @[csr.scala 267:36:@1859.4]
  assign opcode = 8'h1 << _T_757; // @[csr.scala 267:20:@1860.4]
  assign _T_758 = opcode[0]; // @[csr.scala 268:40:@1861.4]
  assign insn_call = system_insn & _T_758; // @[csr.scala 268:31:@1862.4]
  assign _T_759 = opcode[1]; // @[csr.scala 269:41:@1863.4]
  assign insn_break = system_insn & _T_759; // @[csr.scala 269:32:@1864.4]
  assign _T_760 = opcode[2]; // @[csr.scala 270:39:@1865.4]
  assign _T_761 = system_insn & _T_760; // @[csr.scala 270:30:@1866.4]
  assign insn_ret = _T_761 & priv_sufficient; // @[csr.scala 270:43:@1867.4]
  assign _T_1044 = insn_call | insn_break; // @[csr.scala 282:24:@2085.4]
  assign _GEN_2 = io_exception ? 32'h2 : reg_mcause; // @[csr.scala 285:23:@2088.4]
  assign _GEN_4 = io_exception ? io_pc : reg_mepc; // @[csr.scala 285:23:@2088.4]
  assign _T_1048 = insn_ret + io_exception; // @[Bitwise.scala 48:55:@2093.4]
  assign _T_1050 = _T_1048 <= 2'h1; // @[csr.scala 291:52:@2094.4]
  assign _T_1052 = _T_1050 | reset; // @[csr.scala 291:9:@2096.4]
  assign _T_1054 = _T_1052 == 1'h0; // @[csr.scala 291:9:@2097.4]
  assign _T_1057 = io_decode_csr[10]; // @[csr.scala 298:33:@2106.4]
  assign _T_1058 = insn_ret & _T_1057; // @[csr.scala 298:17:@2107.4]
  assign _GEN_6 = _T_1058 ? 2'h3 : reg_mstatus_prv; // @[csr.scala 298:38:@2108.4]
  assign _GEN_8 = _T_1058 ? reg_dpc : 32'h80000004; // @[csr.scala 298:38:@2108.4]
  assign _T_1062 = _T_1057 == 1'h0; // @[csr.scala 305:21:@2114.4]
  assign _T_1063 = insn_ret & _T_1062; // @[csr.scala 305:18:@2115.4]
  assign _GEN_9 = _T_1063 ? reg_mstatus_mpie : reg_mstatus_mie; // @[csr.scala 305:41:@2116.4]
  assign _GEN_10 = _T_1063 ? 1'h1 : reg_mstatus_mpie; // @[csr.scala 305:41:@2116.4]
  assign new_prv = _T_1063 ? 2'h3 : _GEN_6; // @[csr.scala 305:41:@2116.4]
  assign _GEN_12 = _T_1063 ? reg_mepc : _GEN_8; // @[csr.scala 305:41:@2116.4]
  assign _GEN_154 = {{2'd0}, reg_mstatus_prv}; // @[csr.scala 315:35:@2124.6]
  assign _T_1067 = _GEN_154 + 4'h8; // @[csr.scala 315:35:@2124.6]
  assign _T_1068 = _T_1067[3:0]; // @[csr.scala 315:35:@2125.6]
  assign _GEN_13 = insn_call ? 32'h80000004 : _GEN_12; // @[csr.scala 313:18:@2122.4]
  assign _GEN_14 = insn_call ? {{28'd0}, _T_1068} : _GEN_2; // @[csr.scala 313:18:@2122.4]
  assign _GEN_16 = insn_break ? 32'h3 : _GEN_14; // @[csr.scala 319:19:@2128.4]
  assign _T_1073 = _T_569 ? _T_267 : 64'h0; // @[Mux.scala 19:72:@2134.4]
  assign _T_1075 = _T_571 ? _T_279 : 64'h0; // @[Mux.scala 19:72:@2135.4]
  assign _T_1077 = _T_573 ? 16'h8000 : 16'h0; // @[Mux.scala 19:72:@2136.4]
  assign _T_1083 = _T_579 ? 9'h100 : 9'h0; // @[Mux.scala 19:72:@2139.4]
  assign _T_1085 = _T_581 ? read_mstatus : 35'h0; // @[Mux.scala 19:72:@2140.4]
  assign _T_1087 = _T_583 ? 9'h100 : 9'h0; // @[Mux.scala 19:72:@2141.4]
  assign _T_1089 = _T_585 ? _T_536 : 16'h0; // @[Mux.scala 19:72:@2142.4]
  assign _T_1091 = _T_587 ? _T_551 : 16'h0; // @[Mux.scala 19:72:@2143.4]
  assign _T_1093 = _T_589 ? reg_mscratch : 32'h0; // @[Mux.scala 19:72:@2144.4]
  assign _T_1095 = _T_591 ? reg_mepc : 32'h0; // @[Mux.scala 19:72:@2145.4]
  assign _T_1097 = _T_593 ? reg_mtval : 32'h0; // @[Mux.scala 19:72:@2146.4]
  assign _T_1099 = _T_595 ? reg_mcause : 32'h0; // @[Mux.scala 19:72:@2147.4]
  assign _T_1103 = _T_599 ? _T_565 : 32'h0; // @[Mux.scala 19:72:@2149.4]
  assign _T_1105 = _T_601 ? reg_dpc : 32'h0; // @[Mux.scala 19:72:@2150.4]
  assign _T_1107 = _T_603 ? reg_dscratch : 32'h0; // @[Mux.scala 19:72:@2151.4]
  assign _T_1109 = _T_605 ? reg_medeleg : 32'h0; // @[Mux.scala 19:72:@2152.4]
  assign _T_1111 = _T_607 ? _T_282 : 40'h0; // @[Mux.scala 19:72:@2153.4]
  assign _T_1113 = _T_609 ? _T_282 : 40'h0; // @[Mux.scala 19:72:@2154.4]
  assign _T_1115 = _T_611 ? _T_285 : 40'h0; // @[Mux.scala 19:72:@2155.4]
  assign _T_1117 = _T_613 ? _T_285 : 40'h0; // @[Mux.scala 19:72:@2156.4]
  assign _T_1119 = _T_615 ? _T_288 : 40'h0; // @[Mux.scala 19:72:@2157.4]
  assign _T_1121 = _T_617 ? _T_288 : 40'h0; // @[Mux.scala 19:72:@2158.4]
  assign _T_1123 = _T_619 ? _T_291 : 40'h0; // @[Mux.scala 19:72:@2159.4]
  assign _T_1125 = _T_621 ? _T_291 : 40'h0; // @[Mux.scala 19:72:@2160.4]
  assign _T_1127 = _T_623 ? _T_294 : 40'h0; // @[Mux.scala 19:72:@2161.4]
  assign _T_1129 = _T_625 ? _T_294 : 40'h0; // @[Mux.scala 19:72:@2162.4]
  assign _T_1131 = _T_627 ? _T_297 : 40'h0; // @[Mux.scala 19:72:@2163.4]
  assign _T_1133 = _T_629 ? _T_297 : 40'h0; // @[Mux.scala 19:72:@2164.4]
  assign _T_1135 = _T_631 ? _T_300 : 40'h0; // @[Mux.scala 19:72:@2165.4]
  assign _T_1137 = _T_633 ? _T_300 : 40'h0; // @[Mux.scala 19:72:@2166.4]
  assign _T_1139 = _T_635 ? _T_303 : 40'h0; // @[Mux.scala 19:72:@2167.4]
  assign _T_1141 = _T_637 ? _T_303 : 40'h0; // @[Mux.scala 19:72:@2168.4]
  assign _T_1143 = _T_639 ? _T_306 : 40'h0; // @[Mux.scala 19:72:@2169.4]
  assign _T_1145 = _T_641 ? _T_306 : 40'h0; // @[Mux.scala 19:72:@2170.4]
  assign _T_1147 = _T_643 ? _T_309 : 40'h0; // @[Mux.scala 19:72:@2171.4]
  assign _T_1149 = _T_645 ? _T_309 : 40'h0; // @[Mux.scala 19:72:@2172.4]
  assign _T_1151 = _T_647 ? _T_312 : 40'h0; // @[Mux.scala 19:72:@2173.4]
  assign _T_1153 = _T_649 ? _T_312 : 40'h0; // @[Mux.scala 19:72:@2174.4]
  assign _T_1155 = _T_651 ? _T_315 : 40'h0; // @[Mux.scala 19:72:@2175.4]
  assign _T_1157 = _T_653 ? _T_315 : 40'h0; // @[Mux.scala 19:72:@2176.4]
  assign _T_1159 = _T_655 ? _T_318 : 40'h0; // @[Mux.scala 19:72:@2177.4]
  assign _T_1161 = _T_657 ? _T_318 : 40'h0; // @[Mux.scala 19:72:@2178.4]
  assign _T_1163 = _T_659 ? _T_321 : 40'h0; // @[Mux.scala 19:72:@2179.4]
  assign _T_1165 = _T_661 ? _T_321 : 40'h0; // @[Mux.scala 19:72:@2180.4]
  assign _T_1167 = _T_663 ? _T_324 : 40'h0; // @[Mux.scala 19:72:@2181.4]
  assign _T_1169 = _T_665 ? _T_324 : 40'h0; // @[Mux.scala 19:72:@2182.4]
  assign _T_1171 = _T_667 ? _T_327 : 40'h0; // @[Mux.scala 19:72:@2183.4]
  assign _T_1173 = _T_669 ? _T_327 : 40'h0; // @[Mux.scala 19:72:@2184.4]
  assign _T_1175 = _T_671 ? _T_330 : 40'h0; // @[Mux.scala 19:72:@2185.4]
  assign _T_1177 = _T_673 ? _T_330 : 40'h0; // @[Mux.scala 19:72:@2186.4]
  assign _T_1179 = _T_675 ? _T_333 : 40'h0; // @[Mux.scala 19:72:@2187.4]
  assign _T_1181 = _T_677 ? _T_333 : 40'h0; // @[Mux.scala 19:72:@2188.4]
  assign _T_1183 = _T_679 ? _T_336 : 40'h0; // @[Mux.scala 19:72:@2189.4]
  assign _T_1185 = _T_681 ? _T_336 : 40'h0; // @[Mux.scala 19:72:@2190.4]
  assign _T_1187 = _T_683 ? _T_339 : 40'h0; // @[Mux.scala 19:72:@2191.4]
  assign _T_1189 = _T_685 ? _T_339 : 40'h0; // @[Mux.scala 19:72:@2192.4]
  assign _T_1191 = _T_687 ? _T_342 : 40'h0; // @[Mux.scala 19:72:@2193.4]
  assign _T_1193 = _T_689 ? _T_342 : 40'h0; // @[Mux.scala 19:72:@2194.4]
  assign _T_1195 = _T_691 ? _T_345 : 40'h0; // @[Mux.scala 19:72:@2195.4]
  assign _T_1197 = _T_693 ? _T_345 : 40'h0; // @[Mux.scala 19:72:@2196.4]
  assign _T_1199 = _T_695 ? _T_348 : 40'h0; // @[Mux.scala 19:72:@2197.4]
  assign _T_1201 = _T_697 ? _T_348 : 40'h0; // @[Mux.scala 19:72:@2198.4]
  assign _T_1203 = _T_699 ? _T_351 : 40'h0; // @[Mux.scala 19:72:@2199.4]
  assign _T_1205 = _T_701 ? _T_351 : 40'h0; // @[Mux.scala 19:72:@2200.4]
  assign _T_1207 = _T_703 ? _T_354 : 40'h0; // @[Mux.scala 19:72:@2201.4]
  assign _T_1209 = _T_705 ? _T_354 : 40'h0; // @[Mux.scala 19:72:@2202.4]
  assign _T_1211 = _T_707 ? _T_357 : 40'h0; // @[Mux.scala 19:72:@2203.4]
  assign _T_1213 = _T_709 ? _T_357 : 40'h0; // @[Mux.scala 19:72:@2204.4]
  assign _T_1215 = _T_711 ? _T_360 : 40'h0; // @[Mux.scala 19:72:@2205.4]
  assign _T_1217 = _T_713 ? _T_360 : 40'h0; // @[Mux.scala 19:72:@2206.4]
  assign _T_1219 = _T_715 ? _T_363 : 40'h0; // @[Mux.scala 19:72:@2207.4]
  assign _T_1221 = _T_717 ? _T_363 : 40'h0; // @[Mux.scala 19:72:@2208.4]
  assign _T_1223 = _T_719 ? _T_366 : 40'h0; // @[Mux.scala 19:72:@2209.4]
  assign _T_1225 = _T_721 ? _T_366 : 40'h0; // @[Mux.scala 19:72:@2210.4]
  assign _T_1227 = _T_723 ? _T_369 : 40'h0; // @[Mux.scala 19:72:@2211.4]
  assign _T_1229 = _T_725 ? _T_369 : 40'h0; // @[Mux.scala 19:72:@2212.4]
  assign _T_1231 = _T_727 ? _T_372 : 40'h0; // @[Mux.scala 19:72:@2213.4]
  assign _T_1233 = _T_729 ? _T_372 : 40'h0; // @[Mux.scala 19:72:@2214.4]
  assign _T_1235 = _T_731 ? _T_375 : 40'h0; // @[Mux.scala 19:72:@2215.4]
  assign _T_1237 = _T_733 ? _T_375 : 40'h0; // @[Mux.scala 19:72:@2216.4]
  assign _T_1242 = _T_1073 | _T_1075; // @[Mux.scala 19:72:@2219.4]
  assign _GEN_155 = {{48'd0}, _T_1077}; // @[Mux.scala 19:72:@2220.4]
  assign _T_1243 = _T_1242 | _GEN_155; // @[Mux.scala 19:72:@2220.4]
  assign _GEN_156 = {{55'd0}, _T_1083}; // @[Mux.scala 19:72:@2223.4]
  assign _T_1246 = _T_1243 | _GEN_156; // @[Mux.scala 19:72:@2223.4]
  assign _GEN_157 = {{29'd0}, _T_1085}; // @[Mux.scala 19:72:@2224.4]
  assign _T_1247 = _T_1246 | _GEN_157; // @[Mux.scala 19:72:@2224.4]
  assign _GEN_158 = {{55'd0}, _T_1087}; // @[Mux.scala 19:72:@2225.4]
  assign _T_1248 = _T_1247 | _GEN_158; // @[Mux.scala 19:72:@2225.4]
  assign _GEN_159 = {{48'd0}, _T_1089}; // @[Mux.scala 19:72:@2226.4]
  assign _T_1249 = _T_1248 | _GEN_159; // @[Mux.scala 19:72:@2226.4]
  assign _GEN_160 = {{48'd0}, _T_1091}; // @[Mux.scala 19:72:@2227.4]
  assign _T_1250 = _T_1249 | _GEN_160; // @[Mux.scala 19:72:@2227.4]
  assign _GEN_161 = {{32'd0}, _T_1093}; // @[Mux.scala 19:72:@2228.4]
  assign _T_1251 = _T_1250 | _GEN_161; // @[Mux.scala 19:72:@2228.4]
  assign _GEN_162 = {{32'd0}, _T_1095}; // @[Mux.scala 19:72:@2229.4]
  assign _T_1252 = _T_1251 | _GEN_162; // @[Mux.scala 19:72:@2229.4]
  assign _GEN_163 = {{32'd0}, _T_1097}; // @[Mux.scala 19:72:@2230.4]
  assign _T_1253 = _T_1252 | _GEN_163; // @[Mux.scala 19:72:@2230.4]
  assign _GEN_164 = {{32'd0}, _T_1099}; // @[Mux.scala 19:72:@2231.4]
  assign _T_1254 = _T_1253 | _GEN_164; // @[Mux.scala 19:72:@2231.4]
  assign _GEN_165 = {{32'd0}, _T_1103}; // @[Mux.scala 19:72:@2233.4]
  assign _T_1256 = _T_1254 | _GEN_165; // @[Mux.scala 19:72:@2233.4]
  assign _GEN_166 = {{32'd0}, _T_1105}; // @[Mux.scala 19:72:@2234.4]
  assign _T_1257 = _T_1256 | _GEN_166; // @[Mux.scala 19:72:@2234.4]
  assign _GEN_167 = {{32'd0}, _T_1107}; // @[Mux.scala 19:72:@2235.4]
  assign _T_1258 = _T_1257 | _GEN_167; // @[Mux.scala 19:72:@2235.4]
  assign _GEN_168 = {{32'd0}, _T_1109}; // @[Mux.scala 19:72:@2236.4]
  assign _T_1259 = _T_1258 | _GEN_168; // @[Mux.scala 19:72:@2236.4]
  assign _GEN_169 = {{24'd0}, _T_1111}; // @[Mux.scala 19:72:@2237.4]
  assign _T_1260 = _T_1259 | _GEN_169; // @[Mux.scala 19:72:@2237.4]
  assign _GEN_170 = {{24'd0}, _T_1113}; // @[Mux.scala 19:72:@2238.4]
  assign _T_1261 = _T_1260 | _GEN_170; // @[Mux.scala 19:72:@2238.4]
  assign _GEN_171 = {{24'd0}, _T_1115}; // @[Mux.scala 19:72:@2239.4]
  assign _T_1262 = _T_1261 | _GEN_171; // @[Mux.scala 19:72:@2239.4]
  assign _GEN_172 = {{24'd0}, _T_1117}; // @[Mux.scala 19:72:@2240.4]
  assign _T_1263 = _T_1262 | _GEN_172; // @[Mux.scala 19:72:@2240.4]
  assign _GEN_173 = {{24'd0}, _T_1119}; // @[Mux.scala 19:72:@2241.4]
  assign _T_1264 = _T_1263 | _GEN_173; // @[Mux.scala 19:72:@2241.4]
  assign _GEN_174 = {{24'd0}, _T_1121}; // @[Mux.scala 19:72:@2242.4]
  assign _T_1265 = _T_1264 | _GEN_174; // @[Mux.scala 19:72:@2242.4]
  assign _GEN_175 = {{24'd0}, _T_1123}; // @[Mux.scala 19:72:@2243.4]
  assign _T_1266 = _T_1265 | _GEN_175; // @[Mux.scala 19:72:@2243.4]
  assign _GEN_176 = {{24'd0}, _T_1125}; // @[Mux.scala 19:72:@2244.4]
  assign _T_1267 = _T_1266 | _GEN_176; // @[Mux.scala 19:72:@2244.4]
  assign _GEN_177 = {{24'd0}, _T_1127}; // @[Mux.scala 19:72:@2245.4]
  assign _T_1268 = _T_1267 | _GEN_177; // @[Mux.scala 19:72:@2245.4]
  assign _GEN_178 = {{24'd0}, _T_1129}; // @[Mux.scala 19:72:@2246.4]
  assign _T_1269 = _T_1268 | _GEN_178; // @[Mux.scala 19:72:@2246.4]
  assign _GEN_179 = {{24'd0}, _T_1131}; // @[Mux.scala 19:72:@2247.4]
  assign _T_1270 = _T_1269 | _GEN_179; // @[Mux.scala 19:72:@2247.4]
  assign _GEN_180 = {{24'd0}, _T_1133}; // @[Mux.scala 19:72:@2248.4]
  assign _T_1271 = _T_1270 | _GEN_180; // @[Mux.scala 19:72:@2248.4]
  assign _GEN_181 = {{24'd0}, _T_1135}; // @[Mux.scala 19:72:@2249.4]
  assign _T_1272 = _T_1271 | _GEN_181; // @[Mux.scala 19:72:@2249.4]
  assign _GEN_182 = {{24'd0}, _T_1137}; // @[Mux.scala 19:72:@2250.4]
  assign _T_1273 = _T_1272 | _GEN_182; // @[Mux.scala 19:72:@2250.4]
  assign _GEN_183 = {{24'd0}, _T_1139}; // @[Mux.scala 19:72:@2251.4]
  assign _T_1274 = _T_1273 | _GEN_183; // @[Mux.scala 19:72:@2251.4]
  assign _GEN_184 = {{24'd0}, _T_1141}; // @[Mux.scala 19:72:@2252.4]
  assign _T_1275 = _T_1274 | _GEN_184; // @[Mux.scala 19:72:@2252.4]
  assign _GEN_185 = {{24'd0}, _T_1143}; // @[Mux.scala 19:72:@2253.4]
  assign _T_1276 = _T_1275 | _GEN_185; // @[Mux.scala 19:72:@2253.4]
  assign _GEN_186 = {{24'd0}, _T_1145}; // @[Mux.scala 19:72:@2254.4]
  assign _T_1277 = _T_1276 | _GEN_186; // @[Mux.scala 19:72:@2254.4]
  assign _GEN_187 = {{24'd0}, _T_1147}; // @[Mux.scala 19:72:@2255.4]
  assign _T_1278 = _T_1277 | _GEN_187; // @[Mux.scala 19:72:@2255.4]
  assign _GEN_188 = {{24'd0}, _T_1149}; // @[Mux.scala 19:72:@2256.4]
  assign _T_1279 = _T_1278 | _GEN_188; // @[Mux.scala 19:72:@2256.4]
  assign _GEN_189 = {{24'd0}, _T_1151}; // @[Mux.scala 19:72:@2257.4]
  assign _T_1280 = _T_1279 | _GEN_189; // @[Mux.scala 19:72:@2257.4]
  assign _GEN_190 = {{24'd0}, _T_1153}; // @[Mux.scala 19:72:@2258.4]
  assign _T_1281 = _T_1280 | _GEN_190; // @[Mux.scala 19:72:@2258.4]
  assign _GEN_191 = {{24'd0}, _T_1155}; // @[Mux.scala 19:72:@2259.4]
  assign _T_1282 = _T_1281 | _GEN_191; // @[Mux.scala 19:72:@2259.4]
  assign _GEN_192 = {{24'd0}, _T_1157}; // @[Mux.scala 19:72:@2260.4]
  assign _T_1283 = _T_1282 | _GEN_192; // @[Mux.scala 19:72:@2260.4]
  assign _GEN_193 = {{24'd0}, _T_1159}; // @[Mux.scala 19:72:@2261.4]
  assign _T_1284 = _T_1283 | _GEN_193; // @[Mux.scala 19:72:@2261.4]
  assign _GEN_194 = {{24'd0}, _T_1161}; // @[Mux.scala 19:72:@2262.4]
  assign _T_1285 = _T_1284 | _GEN_194; // @[Mux.scala 19:72:@2262.4]
  assign _GEN_195 = {{24'd0}, _T_1163}; // @[Mux.scala 19:72:@2263.4]
  assign _T_1286 = _T_1285 | _GEN_195; // @[Mux.scala 19:72:@2263.4]
  assign _GEN_196 = {{24'd0}, _T_1165}; // @[Mux.scala 19:72:@2264.4]
  assign _T_1287 = _T_1286 | _GEN_196; // @[Mux.scala 19:72:@2264.4]
  assign _GEN_197 = {{24'd0}, _T_1167}; // @[Mux.scala 19:72:@2265.4]
  assign _T_1288 = _T_1287 | _GEN_197; // @[Mux.scala 19:72:@2265.4]
  assign _GEN_198 = {{24'd0}, _T_1169}; // @[Mux.scala 19:72:@2266.4]
  assign _T_1289 = _T_1288 | _GEN_198; // @[Mux.scala 19:72:@2266.4]
  assign _GEN_199 = {{24'd0}, _T_1171}; // @[Mux.scala 19:72:@2267.4]
  assign _T_1290 = _T_1289 | _GEN_199; // @[Mux.scala 19:72:@2267.4]
  assign _GEN_200 = {{24'd0}, _T_1173}; // @[Mux.scala 19:72:@2268.4]
  assign _T_1291 = _T_1290 | _GEN_200; // @[Mux.scala 19:72:@2268.4]
  assign _GEN_201 = {{24'd0}, _T_1175}; // @[Mux.scala 19:72:@2269.4]
  assign _T_1292 = _T_1291 | _GEN_201; // @[Mux.scala 19:72:@2269.4]
  assign _GEN_202 = {{24'd0}, _T_1177}; // @[Mux.scala 19:72:@2270.4]
  assign _T_1293 = _T_1292 | _GEN_202; // @[Mux.scala 19:72:@2270.4]
  assign _GEN_203 = {{24'd0}, _T_1179}; // @[Mux.scala 19:72:@2271.4]
  assign _T_1294 = _T_1293 | _GEN_203; // @[Mux.scala 19:72:@2271.4]
  assign _GEN_204 = {{24'd0}, _T_1181}; // @[Mux.scala 19:72:@2272.4]
  assign _T_1295 = _T_1294 | _GEN_204; // @[Mux.scala 19:72:@2272.4]
  assign _GEN_205 = {{24'd0}, _T_1183}; // @[Mux.scala 19:72:@2273.4]
  assign _T_1296 = _T_1295 | _GEN_205; // @[Mux.scala 19:72:@2273.4]
  assign _GEN_206 = {{24'd0}, _T_1185}; // @[Mux.scala 19:72:@2274.4]
  assign _T_1297 = _T_1296 | _GEN_206; // @[Mux.scala 19:72:@2274.4]
  assign _GEN_207 = {{24'd0}, _T_1187}; // @[Mux.scala 19:72:@2275.4]
  assign _T_1298 = _T_1297 | _GEN_207; // @[Mux.scala 19:72:@2275.4]
  assign _GEN_208 = {{24'd0}, _T_1189}; // @[Mux.scala 19:72:@2276.4]
  assign _T_1299 = _T_1298 | _GEN_208; // @[Mux.scala 19:72:@2276.4]
  assign _GEN_209 = {{24'd0}, _T_1191}; // @[Mux.scala 19:72:@2277.4]
  assign _T_1300 = _T_1299 | _GEN_209; // @[Mux.scala 19:72:@2277.4]
  assign _GEN_210 = {{24'd0}, _T_1193}; // @[Mux.scala 19:72:@2278.4]
  assign _T_1301 = _T_1300 | _GEN_210; // @[Mux.scala 19:72:@2278.4]
  assign _GEN_211 = {{24'd0}, _T_1195}; // @[Mux.scala 19:72:@2279.4]
  assign _T_1302 = _T_1301 | _GEN_211; // @[Mux.scala 19:72:@2279.4]
  assign _GEN_212 = {{24'd0}, _T_1197}; // @[Mux.scala 19:72:@2280.4]
  assign _T_1303 = _T_1302 | _GEN_212; // @[Mux.scala 19:72:@2280.4]
  assign _GEN_213 = {{24'd0}, _T_1199}; // @[Mux.scala 19:72:@2281.4]
  assign _T_1304 = _T_1303 | _GEN_213; // @[Mux.scala 19:72:@2281.4]
  assign _GEN_214 = {{24'd0}, _T_1201}; // @[Mux.scala 19:72:@2282.4]
  assign _T_1305 = _T_1304 | _GEN_214; // @[Mux.scala 19:72:@2282.4]
  assign _GEN_215 = {{24'd0}, _T_1203}; // @[Mux.scala 19:72:@2283.4]
  assign _T_1306 = _T_1305 | _GEN_215; // @[Mux.scala 19:72:@2283.4]
  assign _GEN_216 = {{24'd0}, _T_1205}; // @[Mux.scala 19:72:@2284.4]
  assign _T_1307 = _T_1306 | _GEN_216; // @[Mux.scala 19:72:@2284.4]
  assign _GEN_217 = {{24'd0}, _T_1207}; // @[Mux.scala 19:72:@2285.4]
  assign _T_1308 = _T_1307 | _GEN_217; // @[Mux.scala 19:72:@2285.4]
  assign _GEN_218 = {{24'd0}, _T_1209}; // @[Mux.scala 19:72:@2286.4]
  assign _T_1309 = _T_1308 | _GEN_218; // @[Mux.scala 19:72:@2286.4]
  assign _GEN_219 = {{24'd0}, _T_1211}; // @[Mux.scala 19:72:@2287.4]
  assign _T_1310 = _T_1309 | _GEN_219; // @[Mux.scala 19:72:@2287.4]
  assign _GEN_220 = {{24'd0}, _T_1213}; // @[Mux.scala 19:72:@2288.4]
  assign _T_1311 = _T_1310 | _GEN_220; // @[Mux.scala 19:72:@2288.4]
  assign _GEN_221 = {{24'd0}, _T_1215}; // @[Mux.scala 19:72:@2289.4]
  assign _T_1312 = _T_1311 | _GEN_221; // @[Mux.scala 19:72:@2289.4]
  assign _GEN_222 = {{24'd0}, _T_1217}; // @[Mux.scala 19:72:@2290.4]
  assign _T_1313 = _T_1312 | _GEN_222; // @[Mux.scala 19:72:@2290.4]
  assign _GEN_223 = {{24'd0}, _T_1219}; // @[Mux.scala 19:72:@2291.4]
  assign _T_1314 = _T_1313 | _GEN_223; // @[Mux.scala 19:72:@2291.4]
  assign _GEN_224 = {{24'd0}, _T_1221}; // @[Mux.scala 19:72:@2292.4]
  assign _T_1315 = _T_1314 | _GEN_224; // @[Mux.scala 19:72:@2292.4]
  assign _GEN_225 = {{24'd0}, _T_1223}; // @[Mux.scala 19:72:@2293.4]
  assign _T_1316 = _T_1315 | _GEN_225; // @[Mux.scala 19:72:@2293.4]
  assign _GEN_226 = {{24'd0}, _T_1225}; // @[Mux.scala 19:72:@2294.4]
  assign _T_1317 = _T_1316 | _GEN_226; // @[Mux.scala 19:72:@2294.4]
  assign _GEN_227 = {{24'd0}, _T_1227}; // @[Mux.scala 19:72:@2295.4]
  assign _T_1318 = _T_1317 | _GEN_227; // @[Mux.scala 19:72:@2295.4]
  assign _GEN_228 = {{24'd0}, _T_1229}; // @[Mux.scala 19:72:@2296.4]
  assign _T_1319 = _T_1318 | _GEN_228; // @[Mux.scala 19:72:@2296.4]
  assign _GEN_229 = {{24'd0}, _T_1231}; // @[Mux.scala 19:72:@2297.4]
  assign _T_1320 = _T_1319 | _GEN_229; // @[Mux.scala 19:72:@2297.4]
  assign _GEN_230 = {{24'd0}, _T_1233}; // @[Mux.scala 19:72:@2298.4]
  assign _T_1321 = _T_1320 | _GEN_230; // @[Mux.scala 19:72:@2298.4]
  assign _GEN_231 = {{24'd0}, _T_1235}; // @[Mux.scala 19:72:@2299.4]
  assign _T_1322 = _T_1321 | _GEN_231; // @[Mux.scala 19:72:@2299.4]
  assign _GEN_232 = {{24'd0}, _T_1237}; // @[Mux.scala 19:72:@2300.4]
  assign _T_1323 = _T_1322 | _GEN_232; // @[Mux.scala 19:72:@2300.4]
  assign _T_1334 = wdata[2]; // @[csr.scala 334:38:@2313.8]
  assign _T_1344 = wdata[15]; // @[csr.scala 334:38:@2333.8]
  assign _GEN_17 = _T_599 ? _T_1334 : reg_dcsr_step; // @[csr.scala 333:36:@2307.6]
  assign _GEN_18 = _T_599 ? _T_1344 : reg_dcsr_ebreakm; // @[csr.scala 333:36:@2307.6]
  assign _T_1352 = {{3'd0}, wdata}; // @[:@2346.8 :@2347.8]
  assign _T_1356 = _T_1352[3]; // @[csr.scala 341:39:@2354.8]
  assign _T_1360 = _T_1352[7]; // @[csr.scala 341:39:@2362.8]
  assign _GEN_19 = _T_581 ? _T_1356 : _GEN_9; // @[csr.scala 340:39:@2344.6]
  assign _GEN_20 = _T_581 ? _T_1360 : _GEN_10; // @[csr.scala 340:39:@2344.6]
  assign _T_1380 = wdata[15:0]; // @[:@2399.8 :@2400.8]
  assign _T_1384 = _T_1380[3]; // @[csr.scala 346:35:@2407.8]
  assign _T_1388 = _T_1380[7]; // @[csr.scala 346:35:@2415.8]
  assign _GEN_21 = _T_585 ? _T_1384 : reg_mip_msip; // @[csr.scala 345:35:@2397.6]
  assign _GEN_22 = _T_587 ? _T_1384 : reg_mie_msip; // @[csr.scala 349:35:@2435.6]
  assign _GEN_23 = _T_587 ? _T_1388 : reg_mie_mtip; // @[csr.scala 349:35:@2435.6]
  assign _T_1418 = wdata[7:0]; // @[csr.scala 386:47:@2475.8]
  assign _T_1419 = _T_282[31:0]; // @[csr.scala 386:72:@2476.8]
  assign _T_1420 = {_T_1418,_T_1419}; // @[Cat.scala 30:58:@2477.8]
  assign _GEN_24 = _T_609 ? {{1'd0}, _T_1420} : _T_283; // @[csr.scala 386:29:@2474.6]
  assign _T_1421 = _T_282[39:32]; // @[csr.scala 387:45:@2481.8]
  assign _T_1422 = {_T_1421,wdata}; // @[Cat.scala 30:58:@2482.8]
  assign _GEN_25 = _T_607 ? {{1'd0}, _T_1422} : _GEN_24; // @[csr.scala 387:29:@2480.6]
  assign _T_1424 = _T_285[31:0]; // @[csr.scala 386:72:@2487.8]
  assign _T_1425 = {_T_1418,_T_1424}; // @[Cat.scala 30:58:@2488.8]
  assign _GEN_26 = _T_613 ? {{1'd0}, _T_1425} : _T_286; // @[csr.scala 386:29:@2485.6]
  assign _T_1426 = _T_285[39:32]; // @[csr.scala 387:45:@2492.8]
  assign _T_1427 = {_T_1426,wdata}; // @[Cat.scala 30:58:@2493.8]
  assign _GEN_27 = _T_611 ? {{1'd0}, _T_1427} : _GEN_26; // @[csr.scala 387:29:@2491.6]
  assign _T_1429 = _T_288[31:0]; // @[csr.scala 386:72:@2498.8]
  assign _T_1430 = {_T_1418,_T_1429}; // @[Cat.scala 30:58:@2499.8]
  assign _GEN_28 = _T_617 ? {{1'd0}, _T_1430} : _T_289; // @[csr.scala 386:29:@2496.6]
  assign _T_1431 = _T_288[39:32]; // @[csr.scala 387:45:@2503.8]
  assign _T_1432 = {_T_1431,wdata}; // @[Cat.scala 30:58:@2504.8]
  assign _GEN_29 = _T_615 ? {{1'd0}, _T_1432} : _GEN_28; // @[csr.scala 387:29:@2502.6]
  assign _T_1434 = _T_291[31:0]; // @[csr.scala 386:72:@2509.8]
  assign _T_1435 = {_T_1418,_T_1434}; // @[Cat.scala 30:58:@2510.8]
  assign _GEN_30 = _T_621 ? {{1'd0}, _T_1435} : _T_292; // @[csr.scala 386:29:@2507.6]
  assign _T_1436 = _T_291[39:32]; // @[csr.scala 387:45:@2514.8]
  assign _T_1437 = {_T_1436,wdata}; // @[Cat.scala 30:58:@2515.8]
  assign _GEN_31 = _T_619 ? {{1'd0}, _T_1437} : _GEN_30; // @[csr.scala 387:29:@2513.6]
  assign _T_1439 = _T_294[31:0]; // @[csr.scala 386:72:@2520.8]
  assign _T_1440 = {_T_1418,_T_1439}; // @[Cat.scala 30:58:@2521.8]
  assign _GEN_32 = _T_625 ? {{1'd0}, _T_1440} : _T_295; // @[csr.scala 386:29:@2518.6]
  assign _T_1441 = _T_294[39:32]; // @[csr.scala 387:45:@2525.8]
  assign _T_1442 = {_T_1441,wdata}; // @[Cat.scala 30:58:@2526.8]
  assign _GEN_33 = _T_623 ? {{1'd0}, _T_1442} : _GEN_32; // @[csr.scala 387:29:@2524.6]
  assign _T_1444 = _T_297[31:0]; // @[csr.scala 386:72:@2531.8]
  assign _T_1445 = {_T_1418,_T_1444}; // @[Cat.scala 30:58:@2532.8]
  assign _GEN_34 = _T_629 ? {{1'd0}, _T_1445} : _T_298; // @[csr.scala 386:29:@2529.6]
  assign _T_1446 = _T_297[39:32]; // @[csr.scala 387:45:@2536.8]
  assign _T_1447 = {_T_1446,wdata}; // @[Cat.scala 30:58:@2537.8]
  assign _GEN_35 = _T_627 ? {{1'd0}, _T_1447} : _GEN_34; // @[csr.scala 387:29:@2535.6]
  assign _T_1449 = _T_300[31:0]; // @[csr.scala 386:72:@2542.8]
  assign _T_1450 = {_T_1418,_T_1449}; // @[Cat.scala 30:58:@2543.8]
  assign _GEN_36 = _T_633 ? {{1'd0}, _T_1450} : _T_301; // @[csr.scala 386:29:@2540.6]
  assign _T_1451 = _T_300[39:32]; // @[csr.scala 387:45:@2547.8]
  assign _T_1452 = {_T_1451,wdata}; // @[Cat.scala 30:58:@2548.8]
  assign _GEN_37 = _T_631 ? {{1'd0}, _T_1452} : _GEN_36; // @[csr.scala 387:29:@2546.6]
  assign _T_1454 = _T_303[31:0]; // @[csr.scala 386:72:@2553.8]
  assign _T_1455 = {_T_1418,_T_1454}; // @[Cat.scala 30:58:@2554.8]
  assign _GEN_38 = _T_637 ? {{1'd0}, _T_1455} : _T_304; // @[csr.scala 386:29:@2551.6]
  assign _T_1456 = _T_303[39:32]; // @[csr.scala 387:45:@2558.8]
  assign _T_1457 = {_T_1456,wdata}; // @[Cat.scala 30:58:@2559.8]
  assign _GEN_39 = _T_635 ? {{1'd0}, _T_1457} : _GEN_38; // @[csr.scala 387:29:@2557.6]
  assign _T_1459 = _T_306[31:0]; // @[csr.scala 386:72:@2564.8]
  assign _T_1460 = {_T_1418,_T_1459}; // @[Cat.scala 30:58:@2565.8]
  assign _GEN_40 = _T_641 ? {{1'd0}, _T_1460} : _T_307; // @[csr.scala 386:29:@2562.6]
  assign _T_1461 = _T_306[39:32]; // @[csr.scala 387:45:@2569.8]
  assign _T_1462 = {_T_1461,wdata}; // @[Cat.scala 30:58:@2570.8]
  assign _GEN_41 = _T_639 ? {{1'd0}, _T_1462} : _GEN_40; // @[csr.scala 387:29:@2568.6]
  assign _T_1464 = _T_309[31:0]; // @[csr.scala 386:72:@2575.8]
  assign _T_1465 = {_T_1418,_T_1464}; // @[Cat.scala 30:58:@2576.8]
  assign _GEN_42 = _T_645 ? {{1'd0}, _T_1465} : _T_310; // @[csr.scala 386:29:@2573.6]
  assign _T_1466 = _T_309[39:32]; // @[csr.scala 387:45:@2580.8]
  assign _T_1467 = {_T_1466,wdata}; // @[Cat.scala 30:58:@2581.8]
  assign _GEN_43 = _T_643 ? {{1'd0}, _T_1467} : _GEN_42; // @[csr.scala 387:29:@2579.6]
  assign _T_1469 = _T_312[31:0]; // @[csr.scala 386:72:@2586.8]
  assign _T_1470 = {_T_1418,_T_1469}; // @[Cat.scala 30:58:@2587.8]
  assign _GEN_44 = _T_649 ? {{1'd0}, _T_1470} : _T_313; // @[csr.scala 386:29:@2584.6]
  assign _T_1471 = _T_312[39:32]; // @[csr.scala 387:45:@2591.8]
  assign _T_1472 = {_T_1471,wdata}; // @[Cat.scala 30:58:@2592.8]
  assign _GEN_45 = _T_647 ? {{1'd0}, _T_1472} : _GEN_44; // @[csr.scala 387:29:@2590.6]
  assign _T_1474 = _T_315[31:0]; // @[csr.scala 386:72:@2597.8]
  assign _T_1475 = {_T_1418,_T_1474}; // @[Cat.scala 30:58:@2598.8]
  assign _GEN_46 = _T_653 ? {{1'd0}, _T_1475} : _T_316; // @[csr.scala 386:29:@2595.6]
  assign _T_1476 = _T_315[39:32]; // @[csr.scala 387:45:@2602.8]
  assign _T_1477 = {_T_1476,wdata}; // @[Cat.scala 30:58:@2603.8]
  assign _GEN_47 = _T_651 ? {{1'd0}, _T_1477} : _GEN_46; // @[csr.scala 387:29:@2601.6]
  assign _T_1479 = _T_318[31:0]; // @[csr.scala 386:72:@2608.8]
  assign _T_1480 = {_T_1418,_T_1479}; // @[Cat.scala 30:58:@2609.8]
  assign _GEN_48 = _T_657 ? {{1'd0}, _T_1480} : _T_319; // @[csr.scala 386:29:@2606.6]
  assign _T_1481 = _T_318[39:32]; // @[csr.scala 387:45:@2613.8]
  assign _T_1482 = {_T_1481,wdata}; // @[Cat.scala 30:58:@2614.8]
  assign _GEN_49 = _T_655 ? {{1'd0}, _T_1482} : _GEN_48; // @[csr.scala 387:29:@2612.6]
  assign _T_1484 = _T_321[31:0]; // @[csr.scala 386:72:@2619.8]
  assign _T_1485 = {_T_1418,_T_1484}; // @[Cat.scala 30:58:@2620.8]
  assign _GEN_50 = _T_661 ? {{1'd0}, _T_1485} : _T_322; // @[csr.scala 386:29:@2617.6]
  assign _T_1486 = _T_321[39:32]; // @[csr.scala 387:45:@2624.8]
  assign _T_1487 = {_T_1486,wdata}; // @[Cat.scala 30:58:@2625.8]
  assign _GEN_51 = _T_659 ? {{1'd0}, _T_1487} : _GEN_50; // @[csr.scala 387:29:@2623.6]
  assign _T_1489 = _T_324[31:0]; // @[csr.scala 386:72:@2630.8]
  assign _T_1490 = {_T_1418,_T_1489}; // @[Cat.scala 30:58:@2631.8]
  assign _GEN_52 = _T_665 ? {{1'd0}, _T_1490} : _T_325; // @[csr.scala 386:29:@2628.6]
  assign _T_1491 = _T_324[39:32]; // @[csr.scala 387:45:@2635.8]
  assign _T_1492 = {_T_1491,wdata}; // @[Cat.scala 30:58:@2636.8]
  assign _GEN_53 = _T_663 ? {{1'd0}, _T_1492} : _GEN_52; // @[csr.scala 387:29:@2634.6]
  assign _T_1494 = _T_327[31:0]; // @[csr.scala 386:72:@2641.8]
  assign _T_1495 = {_T_1418,_T_1494}; // @[Cat.scala 30:58:@2642.8]
  assign _GEN_54 = _T_669 ? {{1'd0}, _T_1495} : _T_328; // @[csr.scala 386:29:@2639.6]
  assign _T_1496 = _T_327[39:32]; // @[csr.scala 387:45:@2646.8]
  assign _T_1497 = {_T_1496,wdata}; // @[Cat.scala 30:58:@2647.8]
  assign _GEN_55 = _T_667 ? {{1'd0}, _T_1497} : _GEN_54; // @[csr.scala 387:29:@2645.6]
  assign _T_1499 = _T_330[31:0]; // @[csr.scala 386:72:@2652.8]
  assign _T_1500 = {_T_1418,_T_1499}; // @[Cat.scala 30:58:@2653.8]
  assign _GEN_56 = _T_673 ? {{1'd0}, _T_1500} : _T_331; // @[csr.scala 386:29:@2650.6]
  assign _T_1501 = _T_330[39:32]; // @[csr.scala 387:45:@2657.8]
  assign _T_1502 = {_T_1501,wdata}; // @[Cat.scala 30:58:@2658.8]
  assign _GEN_57 = _T_671 ? {{1'd0}, _T_1502} : _GEN_56; // @[csr.scala 387:29:@2656.6]
  assign _T_1504 = _T_333[31:0]; // @[csr.scala 386:72:@2663.8]
  assign _T_1505 = {_T_1418,_T_1504}; // @[Cat.scala 30:58:@2664.8]
  assign _GEN_58 = _T_677 ? {{1'd0}, _T_1505} : _T_334; // @[csr.scala 386:29:@2661.6]
  assign _T_1506 = _T_333[39:32]; // @[csr.scala 387:45:@2668.8]
  assign _T_1507 = {_T_1506,wdata}; // @[Cat.scala 30:58:@2669.8]
  assign _GEN_59 = _T_675 ? {{1'd0}, _T_1507} : _GEN_58; // @[csr.scala 387:29:@2667.6]
  assign _T_1509 = _T_336[31:0]; // @[csr.scala 386:72:@2674.8]
  assign _T_1510 = {_T_1418,_T_1509}; // @[Cat.scala 30:58:@2675.8]
  assign _GEN_60 = _T_681 ? {{1'd0}, _T_1510} : _T_337; // @[csr.scala 386:29:@2672.6]
  assign _T_1511 = _T_336[39:32]; // @[csr.scala 387:45:@2679.8]
  assign _T_1512 = {_T_1511,wdata}; // @[Cat.scala 30:58:@2680.8]
  assign _GEN_61 = _T_679 ? {{1'd0}, _T_1512} : _GEN_60; // @[csr.scala 387:29:@2678.6]
  assign _T_1514 = _T_339[31:0]; // @[csr.scala 386:72:@2685.8]
  assign _T_1515 = {_T_1418,_T_1514}; // @[Cat.scala 30:58:@2686.8]
  assign _GEN_62 = _T_685 ? {{1'd0}, _T_1515} : _T_340; // @[csr.scala 386:29:@2683.6]
  assign _T_1516 = _T_339[39:32]; // @[csr.scala 387:45:@2690.8]
  assign _T_1517 = {_T_1516,wdata}; // @[Cat.scala 30:58:@2691.8]
  assign _GEN_63 = _T_683 ? {{1'd0}, _T_1517} : _GEN_62; // @[csr.scala 387:29:@2689.6]
  assign _T_1519 = _T_342[31:0]; // @[csr.scala 386:72:@2696.8]
  assign _T_1520 = {_T_1418,_T_1519}; // @[Cat.scala 30:58:@2697.8]
  assign _GEN_64 = _T_689 ? {{1'd0}, _T_1520} : _T_343; // @[csr.scala 386:29:@2694.6]
  assign _T_1521 = _T_342[39:32]; // @[csr.scala 387:45:@2701.8]
  assign _T_1522 = {_T_1521,wdata}; // @[Cat.scala 30:58:@2702.8]
  assign _GEN_65 = _T_687 ? {{1'd0}, _T_1522} : _GEN_64; // @[csr.scala 387:29:@2700.6]
  assign _T_1524 = _T_345[31:0]; // @[csr.scala 386:72:@2707.8]
  assign _T_1525 = {_T_1418,_T_1524}; // @[Cat.scala 30:58:@2708.8]
  assign _GEN_66 = _T_693 ? {{1'd0}, _T_1525} : _T_346; // @[csr.scala 386:29:@2705.6]
  assign _T_1526 = _T_345[39:32]; // @[csr.scala 387:45:@2712.8]
  assign _T_1527 = {_T_1526,wdata}; // @[Cat.scala 30:58:@2713.8]
  assign _GEN_67 = _T_691 ? {{1'd0}, _T_1527} : _GEN_66; // @[csr.scala 387:29:@2711.6]
  assign _T_1529 = _T_348[31:0]; // @[csr.scala 386:72:@2718.8]
  assign _T_1530 = {_T_1418,_T_1529}; // @[Cat.scala 30:58:@2719.8]
  assign _GEN_68 = _T_697 ? {{1'd0}, _T_1530} : _T_349; // @[csr.scala 386:29:@2716.6]
  assign _T_1531 = _T_348[39:32]; // @[csr.scala 387:45:@2723.8]
  assign _T_1532 = {_T_1531,wdata}; // @[Cat.scala 30:58:@2724.8]
  assign _GEN_69 = _T_695 ? {{1'd0}, _T_1532} : _GEN_68; // @[csr.scala 387:29:@2722.6]
  assign _T_1534 = _T_351[31:0]; // @[csr.scala 386:72:@2729.8]
  assign _T_1535 = {_T_1418,_T_1534}; // @[Cat.scala 30:58:@2730.8]
  assign _GEN_70 = _T_701 ? {{1'd0}, _T_1535} : _T_352; // @[csr.scala 386:29:@2727.6]
  assign _T_1536 = _T_351[39:32]; // @[csr.scala 387:45:@2734.8]
  assign _T_1537 = {_T_1536,wdata}; // @[Cat.scala 30:58:@2735.8]
  assign _GEN_71 = _T_699 ? {{1'd0}, _T_1537} : _GEN_70; // @[csr.scala 387:29:@2733.6]
  assign _T_1539 = _T_354[31:0]; // @[csr.scala 386:72:@2740.8]
  assign _T_1540 = {_T_1418,_T_1539}; // @[Cat.scala 30:58:@2741.8]
  assign _GEN_72 = _T_705 ? {{1'd0}, _T_1540} : _T_355; // @[csr.scala 386:29:@2738.6]
  assign _T_1541 = _T_354[39:32]; // @[csr.scala 387:45:@2745.8]
  assign _T_1542 = {_T_1541,wdata}; // @[Cat.scala 30:58:@2746.8]
  assign _GEN_73 = _T_703 ? {{1'd0}, _T_1542} : _GEN_72; // @[csr.scala 387:29:@2744.6]
  assign _T_1544 = _T_357[31:0]; // @[csr.scala 386:72:@2751.8]
  assign _T_1545 = {_T_1418,_T_1544}; // @[Cat.scala 30:58:@2752.8]
  assign _GEN_74 = _T_709 ? {{1'd0}, _T_1545} : _T_358; // @[csr.scala 386:29:@2749.6]
  assign _T_1546 = _T_357[39:32]; // @[csr.scala 387:45:@2756.8]
  assign _T_1547 = {_T_1546,wdata}; // @[Cat.scala 30:58:@2757.8]
  assign _GEN_75 = _T_707 ? {{1'd0}, _T_1547} : _GEN_74; // @[csr.scala 387:29:@2755.6]
  assign _T_1549 = _T_360[31:0]; // @[csr.scala 386:72:@2762.8]
  assign _T_1550 = {_T_1418,_T_1549}; // @[Cat.scala 30:58:@2763.8]
  assign _GEN_76 = _T_713 ? {{1'd0}, _T_1550} : _T_361; // @[csr.scala 386:29:@2760.6]
  assign _T_1551 = _T_360[39:32]; // @[csr.scala 387:45:@2767.8]
  assign _T_1552 = {_T_1551,wdata}; // @[Cat.scala 30:58:@2768.8]
  assign _GEN_77 = _T_711 ? {{1'd0}, _T_1552} : _GEN_76; // @[csr.scala 387:29:@2766.6]
  assign _T_1554 = _T_363[31:0]; // @[csr.scala 386:72:@2773.8]
  assign _T_1555 = {_T_1418,_T_1554}; // @[Cat.scala 30:58:@2774.8]
  assign _GEN_78 = _T_717 ? {{1'd0}, _T_1555} : _T_364; // @[csr.scala 386:29:@2771.6]
  assign _T_1556 = _T_363[39:32]; // @[csr.scala 387:45:@2778.8]
  assign _T_1557 = {_T_1556,wdata}; // @[Cat.scala 30:58:@2779.8]
  assign _GEN_79 = _T_715 ? {{1'd0}, _T_1557} : _GEN_78; // @[csr.scala 387:29:@2777.6]
  assign _T_1559 = _T_366[31:0]; // @[csr.scala 386:72:@2784.8]
  assign _T_1560 = {_T_1418,_T_1559}; // @[Cat.scala 30:58:@2785.8]
  assign _GEN_80 = _T_721 ? {{1'd0}, _T_1560} : _T_367; // @[csr.scala 386:29:@2782.6]
  assign _T_1561 = _T_366[39:32]; // @[csr.scala 387:45:@2789.8]
  assign _T_1562 = {_T_1561,wdata}; // @[Cat.scala 30:58:@2790.8]
  assign _GEN_81 = _T_719 ? {{1'd0}, _T_1562} : _GEN_80; // @[csr.scala 387:29:@2788.6]
  assign _T_1564 = _T_369[31:0]; // @[csr.scala 386:72:@2795.8]
  assign _T_1565 = {_T_1418,_T_1564}; // @[Cat.scala 30:58:@2796.8]
  assign _GEN_82 = _T_725 ? {{1'd0}, _T_1565} : _T_370; // @[csr.scala 386:29:@2793.6]
  assign _T_1566 = _T_369[39:32]; // @[csr.scala 387:45:@2800.8]
  assign _T_1567 = {_T_1566,wdata}; // @[Cat.scala 30:58:@2801.8]
  assign _GEN_83 = _T_723 ? {{1'd0}, _T_1567} : _GEN_82; // @[csr.scala 387:29:@2799.6]
  assign _T_1569 = _T_372[31:0]; // @[csr.scala 386:72:@2806.8]
  assign _T_1570 = {_T_1418,_T_1569}; // @[Cat.scala 30:58:@2807.8]
  assign _GEN_84 = _T_729 ? {{1'd0}, _T_1570} : _T_373; // @[csr.scala 386:29:@2804.6]
  assign _T_1571 = _T_372[39:32]; // @[csr.scala 387:45:@2811.8]
  assign _T_1572 = {_T_1571,wdata}; // @[Cat.scala 30:58:@2812.8]
  assign _GEN_85 = _T_727 ? {{1'd0}, _T_1572} : _GEN_84; // @[csr.scala 387:29:@2810.6]
  assign _T_1574 = _T_375[31:0]; // @[csr.scala 386:72:@2817.8]
  assign _T_1575 = {_T_1418,_T_1574}; // @[Cat.scala 30:58:@2818.8]
  assign _GEN_86 = _T_733 ? {{1'd0}, _T_1575} : _T_376; // @[csr.scala 386:29:@2815.6]
  assign _T_1576 = _T_375[39:32]; // @[csr.scala 387:45:@2822.8]
  assign _T_1577 = {_T_1576,wdata}; // @[Cat.scala 30:58:@2823.8]
  assign _GEN_87 = _T_731 ? {{1'd0}, _T_1577} : _GEN_86; // @[csr.scala 387:29:@2821.6]
  assign _T_1579 = _T_267[31:0]; // @[csr.scala 386:72:@2828.8]
  assign _T_1580 = {wdata,_T_1579}; // @[Cat.scala 30:58:@2829.8]
  assign _T_1581 = _T_1580[63:6]; // @[util.scala 135:28:@2831.8]
  assign _GEN_88 = _T_735 ? _T_1580 : {{57'd0}, _T_259}; // @[csr.scala 386:29:@2826.6]
  assign _GEN_89 = _T_735 ? _T_1581 : _GEN_0; // @[csr.scala 386:29:@2826.6]
  assign _T_1582 = _T_267[63:32]; // @[csr.scala 387:45:@2835.8]
  assign _T_1583 = {_T_1582,wdata}; // @[Cat.scala 30:58:@2836.8]
  assign _T_1584 = _T_1583[63:6]; // @[util.scala 135:28:@2838.8]
  assign _GEN_90 = _T_569 ? _T_1583 : _GEN_88; // @[csr.scala 387:29:@2834.6]
  assign _GEN_91 = _T_569 ? _T_1584 : _GEN_89; // @[csr.scala 387:29:@2834.6]
  assign _T_1586 = _T_279[31:0]; // @[csr.scala 386:72:@2843.8]
  assign _T_1587 = {wdata,_T_1586}; // @[Cat.scala 30:58:@2844.8]
  assign _T_1588 = _T_1587[63:6]; // @[util.scala 135:28:@2846.8]
  assign _GEN_92 = _T_737 ? _T_1587 : {{57'd0}, _T_271}; // @[csr.scala 386:29:@2841.6]
  assign _GEN_93 = _T_737 ? _T_1588 : _GEN_1; // @[csr.scala 386:29:@2841.6]
  assign _T_1589 = _T_279[63:32]; // @[csr.scala 387:45:@2850.8]
  assign _T_1590 = {_T_1589,wdata}; // @[Cat.scala 30:58:@2851.8]
  assign _T_1591 = _T_1590[63:6]; // @[util.scala 135:28:@2853.8]
  assign _GEN_94 = _T_571 ? _T_1590 : _GEN_92; // @[csr.scala 387:29:@2849.6]
  assign _GEN_95 = _T_571 ? _T_1591 : _GEN_93; // @[csr.scala 387:29:@2849.6]
  assign _GEN_96 = _T_601 ? wdata : reg_dpc; // @[csr.scala 365:40:@2856.6]
  assign _GEN_97 = _T_603 ? wdata : reg_dscratch; // @[csr.scala 366:40:@2859.6]
  assign _T_1594 = wdata >> 2'h2; // @[csr.scala 368:78:@2864.8]
  assign _GEN_233 = {{3'd0}, _T_1594}; // @[csr.scala 368:86:@2865.8]
  assign _T_1596 = _GEN_233 << 2'h2; // @[csr.scala 368:86:@2865.8]
  assign _GEN_98 = _T_591 ? _T_1596 : {{3'd0}, _GEN_4}; // @[csr.scala 368:40:@2862.6]
  assign _GEN_99 = _T_589 ? wdata : reg_mscratch; // @[csr.scala 369:40:@2868.6]
  assign _T_1598 = wdata & 32'h8000001f; // @[csr.scala 370:62:@2872.8]
  assign _GEN_100 = _T_595 ? _T_1598 : _GEN_16; // @[csr.scala 370:40:@2871.6]
  assign _GEN_101 = _T_593 ? wdata : reg_mtval; // @[csr.scala 371:40:@2875.6]
  assign _GEN_102 = _T_605 ? wdata : reg_medeleg; // @[csr.scala 372:42:@2879.6]
  assign _GEN_103 = wen ? _GEN_17 : reg_dcsr_step; // @[csr.scala 331:14:@2306.4]
  assign _GEN_104 = wen ? _GEN_18 : reg_dcsr_ebreakm; // @[csr.scala 331:14:@2306.4]
  assign _GEN_105 = wen ? _GEN_19 : _GEN_9; // @[csr.scala 331:14:@2306.4]
  assign _GEN_106 = wen ? _GEN_20 : _GEN_10; // @[csr.scala 331:14:@2306.4]
  assign _GEN_107 = wen ? _GEN_21 : reg_mip_msip; // @[csr.scala 331:14:@2306.4]
  assign _GEN_108 = wen ? _GEN_22 : reg_mie_msip; // @[csr.scala 331:14:@2306.4]
  assign _GEN_109 = wen ? _GEN_23 : reg_mie_mtip; // @[csr.scala 331:14:@2306.4]
  assign _GEN_110 = wen ? _GEN_25 : _T_283; // @[csr.scala 331:14:@2306.4]
  assign _GEN_111 = wen ? _GEN_27 : _T_286; // @[csr.scala 331:14:@2306.4]
  assign _GEN_112 = wen ? _GEN_29 : _T_289; // @[csr.scala 331:14:@2306.4]
  assign _GEN_113 = wen ? _GEN_31 : _T_292; // @[csr.scala 331:14:@2306.4]
  assign _GEN_114 = wen ? _GEN_33 : _T_295; // @[csr.scala 331:14:@2306.4]
  assign _GEN_115 = wen ? _GEN_35 : _T_298; // @[csr.scala 331:14:@2306.4]
  assign _GEN_116 = wen ? _GEN_37 : _T_301; // @[csr.scala 331:14:@2306.4]
  assign _GEN_117 = wen ? _GEN_39 : _T_304; // @[csr.scala 331:14:@2306.4]
  assign _GEN_118 = wen ? _GEN_41 : _T_307; // @[csr.scala 331:14:@2306.4]
  assign _GEN_119 = wen ? _GEN_43 : _T_310; // @[csr.scala 331:14:@2306.4]
  assign _GEN_120 = wen ? _GEN_45 : _T_313; // @[csr.scala 331:14:@2306.4]
  assign _GEN_121 = wen ? _GEN_47 : _T_316; // @[csr.scala 331:14:@2306.4]
  assign _GEN_122 = wen ? _GEN_49 : _T_319; // @[csr.scala 331:14:@2306.4]
  assign _GEN_123 = wen ? _GEN_51 : _T_322; // @[csr.scala 331:14:@2306.4]
  assign _GEN_124 = wen ? _GEN_53 : _T_325; // @[csr.scala 331:14:@2306.4]
  assign _GEN_125 = wen ? _GEN_55 : _T_328; // @[csr.scala 331:14:@2306.4]
  assign _GEN_126 = wen ? _GEN_57 : _T_331; // @[csr.scala 331:14:@2306.4]
  assign _GEN_127 = wen ? _GEN_59 : _T_334; // @[csr.scala 331:14:@2306.4]
  assign _GEN_128 = wen ? _GEN_61 : _T_337; // @[csr.scala 331:14:@2306.4]
  assign _GEN_129 = wen ? _GEN_63 : _T_340; // @[csr.scala 331:14:@2306.4]
  assign _GEN_130 = wen ? _GEN_65 : _T_343; // @[csr.scala 331:14:@2306.4]
  assign _GEN_131 = wen ? _GEN_67 : _T_346; // @[csr.scala 331:14:@2306.4]
  assign _GEN_132 = wen ? _GEN_69 : _T_349; // @[csr.scala 331:14:@2306.4]
  assign _GEN_133 = wen ? _GEN_71 : _T_352; // @[csr.scala 331:14:@2306.4]
  assign _GEN_134 = wen ? _GEN_73 : _T_355; // @[csr.scala 331:14:@2306.4]
  assign _GEN_135 = wen ? _GEN_75 : _T_358; // @[csr.scala 331:14:@2306.4]
  assign _GEN_136 = wen ? _GEN_77 : _T_361; // @[csr.scala 331:14:@2306.4]
  assign _GEN_137 = wen ? _GEN_79 : _T_364; // @[csr.scala 331:14:@2306.4]
  assign _GEN_138 = wen ? _GEN_81 : _T_367; // @[csr.scala 331:14:@2306.4]
  assign _GEN_139 = wen ? _GEN_83 : _T_370; // @[csr.scala 331:14:@2306.4]
  assign _GEN_140 = wen ? _GEN_85 : _T_373; // @[csr.scala 331:14:@2306.4]
  assign _GEN_141 = wen ? _GEN_87 : _T_376; // @[csr.scala 331:14:@2306.4]
  assign _GEN_142 = wen ? _GEN_90 : {{57'd0}, _T_259}; // @[csr.scala 331:14:@2306.4]
  assign _GEN_143 = wen ? _GEN_91 : _GEN_0; // @[csr.scala 331:14:@2306.4]
  assign _GEN_144 = wen ? _GEN_94 : {{57'd0}, _T_271}; // @[csr.scala 331:14:@2306.4]
  assign _GEN_145 = wen ? _GEN_95 : _GEN_1; // @[csr.scala 331:14:@2306.4]
  assign _GEN_148 = wen ? _GEN_98 : {{3'd0}, _GEN_4}; // @[csr.scala 331:14:@2306.4]
  assign io_rw_rdata = _T_1323[31:0]; // @[csr.scala 329:15:@2305.4]
  assign io_eret = _T_1044 | insn_ret; // @[csr.scala 282:11:@2087.4]
  assign io_status_debug = 1'h0; // @[csr.scala 280:13:@2084.4]
  assign io_status_prv = reg_mstatus_prv; // @[csr.scala 280:13:@2083.4]
  assign io_status_sd = 1'h0; // @[csr.scala 280:13:@2082.4]
  assign io_status_zero1 = 8'h0; // @[csr.scala 280:13:@2081.4]
  assign io_status_tsr = 1'h0; // @[csr.scala 280:13:@2080.4]
  assign io_status_tw = 1'h0; // @[csr.scala 280:13:@2079.4]
  assign io_status_tvm = 1'h0; // @[csr.scala 280:13:@2078.4]
  assign io_status_mxr = 1'h0; // @[csr.scala 280:13:@2077.4]
  assign io_status_sum = 1'h0; // @[csr.scala 280:13:@2076.4]
  assign io_status_mprv = 1'h0; // @[csr.scala 280:13:@2075.4]
  assign io_status_xs = 2'h0; // @[csr.scala 280:13:@2074.4]
  assign io_status_fs = 2'h0; // @[csr.scala 280:13:@2073.4]
  assign io_status_mpp = 2'h3; // @[csr.scala 280:13:@2072.4]
  assign io_status_hpp = 2'h0; // @[csr.scala 280:13:@2071.4]
  assign io_status_spp = 1'h0; // @[csr.scala 280:13:@2070.4]
  assign io_status_mpie = reg_mstatus_mpie; // @[csr.scala 280:13:@2069.4]
  assign io_status_hpie = 1'h0; // @[csr.scala 280:13:@2068.4]
  assign io_status_spie = 1'h0; // @[csr.scala 280:13:@2067.4]
  assign io_status_upie = 1'h0; // @[csr.scala 280:13:@2066.4]
  assign io_status_mie = reg_mstatus_mie; // @[csr.scala 280:13:@2065.4]
  assign io_status_hie = 1'h0; // @[csr.scala 280:13:@2064.4]
  assign io_status_sie = 1'h0; // @[csr.scala 280:13:@2063.4]
  assign io_status_uie = 1'h0; // @[csr.scala 280:13:@2062.4]
  assign io_evec = insn_break ? 32'h80000004 : _GEN_13; // @[csr.scala 287:13:@2090.6 csr.scala 301:13:@2111.6 csr.scala 309:13:@2120.6 csr.scala 314:13:@2123.6 csr.scala 320:13:@2129.6]
  assign io_time = _T_267[31:0]; // @[csr.scala 325:11:@2132.4]
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
  reg_mstatus_prv = _RAND_0[1:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  reg_mstatus_mpie = _RAND_1[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_2 = {1{`RANDOM}};
  reg_mstatus_mie = _RAND_2[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_3 = {1{`RANDOM}};
  reg_mepc = _RAND_3[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_4 = {1{`RANDOM}};
  reg_mcause = _RAND_4[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_5 = {1{`RANDOM}};
  reg_mtval = _RAND_5[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_6 = {1{`RANDOM}};
  reg_mscratch = _RAND_6[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_7 = {1{`RANDOM}};
  reg_medeleg = _RAND_7[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_8 = {1{`RANDOM}};
  reg_mip_mtip = _RAND_8[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_9 = {1{`RANDOM}};
  reg_mip_msip = _RAND_9[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_10 = {1{`RANDOM}};
  reg_mie_mtip = _RAND_10[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_11 = {1{`RANDOM}};
  reg_mie_msip = _RAND_11[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_12 = {1{`RANDOM}};
  _T_258 = _RAND_12[5:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_13 = {2{`RANDOM}};
  _T_262 = _RAND_13[57:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_14 = {1{`RANDOM}};
  _T_270 = _RAND_14[5:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_15 = {2{`RANDOM}};
  _T_274 = _RAND_15[57:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_16 = {2{`RANDOM}};
  _T_282 = _RAND_16[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_17 = {2{`RANDOM}};
  _T_285 = _RAND_17[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_18 = {2{`RANDOM}};
  _T_288 = _RAND_18[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_19 = {2{`RANDOM}};
  _T_291 = _RAND_19[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_20 = {2{`RANDOM}};
  _T_294 = _RAND_20[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_21 = {2{`RANDOM}};
  _T_297 = _RAND_21[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_22 = {2{`RANDOM}};
  _T_300 = _RAND_22[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_23 = {2{`RANDOM}};
  _T_303 = _RAND_23[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_24 = {2{`RANDOM}};
  _T_306 = _RAND_24[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_25 = {2{`RANDOM}};
  _T_309 = _RAND_25[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_26 = {2{`RANDOM}};
  _T_312 = _RAND_26[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_27 = {2{`RANDOM}};
  _T_315 = _RAND_27[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_28 = {2{`RANDOM}};
  _T_318 = _RAND_28[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_29 = {2{`RANDOM}};
  _T_321 = _RAND_29[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_30 = {2{`RANDOM}};
  _T_324 = _RAND_30[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_31 = {2{`RANDOM}};
  _T_327 = _RAND_31[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_32 = {2{`RANDOM}};
  _T_330 = _RAND_32[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_33 = {2{`RANDOM}};
  _T_333 = _RAND_33[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_34 = {2{`RANDOM}};
  _T_336 = _RAND_34[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_35 = {2{`RANDOM}};
  _T_339 = _RAND_35[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_36 = {2{`RANDOM}};
  _T_342 = _RAND_36[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_37 = {2{`RANDOM}};
  _T_345 = _RAND_37[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_38 = {2{`RANDOM}};
  _T_348 = _RAND_38[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_39 = {2{`RANDOM}};
  _T_351 = _RAND_39[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_40 = {2{`RANDOM}};
  _T_354 = _RAND_40[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_41 = {2{`RANDOM}};
  _T_357 = _RAND_41[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_42 = {2{`RANDOM}};
  _T_360 = _RAND_42[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_43 = {2{`RANDOM}};
  _T_363 = _RAND_43[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_44 = {2{`RANDOM}};
  _T_366 = _RAND_44[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_45 = {2{`RANDOM}};
  _T_369 = _RAND_45[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_46 = {2{`RANDOM}};
  _T_372 = _RAND_46[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_47 = {2{`RANDOM}};
  _T_375 = _RAND_47[39:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_48 = {1{`RANDOM}};
  reg_dpc = _RAND_48[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_49 = {1{`RANDOM}};
  reg_dscratch = _RAND_49[31:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_50 = {1{`RANDOM}};
  reg_dcsr_ebreakm = _RAND_50[0:0];
  `endif // RANDOMIZE_REG_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_51 = {1{`RANDOM}};
  reg_dcsr_step = _RAND_51[0:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if (reset) begin
      reg_mstatus_prv <= 2'h3;
    end else begin
      if (_T_1063) begin
        reg_mstatus_prv <= 2'h3;
      end else begin
        if (_T_1058) begin
          reg_mstatus_prv <= 2'h3;
        end
      end
    end
    if (reset) begin
      reg_mstatus_mpie <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_581) begin
          reg_mstatus_mpie <= _T_1360;
        end else begin
          if (_T_1063) begin
            reg_mstatus_mpie <= 1'h1;
          end
        end
      end else begin
        if (_T_1063) begin
          reg_mstatus_mpie <= 1'h1;
        end
      end
    end
    if (reset) begin
      reg_mstatus_mie <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_581) begin
          reg_mstatus_mie <= _T_1356;
        end else begin
          if (_T_1063) begin
            reg_mstatus_mie <= reg_mstatus_mpie;
          end
        end
      end else begin
        if (_T_1063) begin
          reg_mstatus_mie <= reg_mstatus_mpie;
        end
      end
    end
    reg_mepc <= _GEN_148[31:0];
    if (wen) begin
      if (_T_595) begin
        reg_mcause <= _T_1598;
      end else begin
        if (insn_break) begin
          reg_mcause <= 32'h3;
        end else begin
          if (insn_call) begin
            reg_mcause <= {{28'd0}, _T_1068};
          end else begin
            if (io_exception) begin
              reg_mcause <= 32'h2;
            end
          end
        end
      end
    end else begin
      if (insn_break) begin
        reg_mcause <= 32'h3;
      end else begin
        if (insn_call) begin
          reg_mcause <= {{28'd0}, _T_1068};
        end else begin
          if (io_exception) begin
            reg_mcause <= 32'h2;
          end
        end
      end
    end
    if (wen) begin
      if (_T_593) begin
        reg_mtval <= wdata;
      end
    end
    if (wen) begin
      if (_T_589) begin
        reg_mscratch <= wdata;
      end
    end
    if (wen) begin
      if (_T_605) begin
        reg_medeleg <= wdata;
      end
    end
    if (reset) begin
      reg_mip_mtip <= 1'h0;
    end else begin
      reg_mip_mtip <= 1'h1;
    end
    if (reset) begin
      reg_mip_msip <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_585) begin
          reg_mip_msip <= _T_1384;
        end
      end
    end
    if (reset) begin
      reg_mie_mtip <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_587) begin
          reg_mie_mtip <= _T_1388;
        end
      end
    end
    if (reset) begin
      reg_mie_msip <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_587) begin
          reg_mie_msip <= _T_1384;
        end
      end
    end
    if (reset) begin
      _T_258 <= 6'h0;
    end else begin
      _T_258 <= _GEN_142[5:0];
    end
    if (reset) begin
      _T_262 <= 58'h0;
    end else begin
      if (wen) begin
        if (_T_569) begin
          _T_262 <= _T_1584;
        end else begin
          if (_T_735) begin
            _T_262 <= _T_1581;
          end else begin
            if (_T_263) begin
              _T_262 <= _T_266;
            end
          end
        end
      end else begin
        if (_T_263) begin
          _T_262 <= _T_266;
        end
      end
    end
    if (reset) begin
      _T_270 <= 6'h0;
    end else begin
      _T_270 <= _GEN_144[5:0];
    end
    if (reset) begin
      _T_274 <= 58'h0;
    end else begin
      if (wen) begin
        if (_T_571) begin
          _T_274 <= _T_1591;
        end else begin
          if (_T_737) begin
            _T_274 <= _T_1588;
          end else begin
            if (_T_275) begin
              _T_274 <= _T_278;
            end
          end
        end
      end else begin
        if (_T_275) begin
          _T_274 <= _T_278;
        end
      end
    end
    _T_282 <= _GEN_110[39:0];
    _T_285 <= _GEN_111[39:0];
    _T_288 <= _GEN_112[39:0];
    _T_291 <= _GEN_113[39:0];
    _T_294 <= _GEN_114[39:0];
    _T_297 <= _GEN_115[39:0];
    _T_300 <= _GEN_116[39:0];
    _T_303 <= _GEN_117[39:0];
    _T_306 <= _GEN_118[39:0];
    _T_309 <= _GEN_119[39:0];
    _T_312 <= _GEN_120[39:0];
    _T_315 <= _GEN_121[39:0];
    _T_318 <= _GEN_122[39:0];
    _T_321 <= _GEN_123[39:0];
    _T_324 <= _GEN_124[39:0];
    _T_327 <= _GEN_125[39:0];
    _T_330 <= _GEN_126[39:0];
    _T_333 <= _GEN_127[39:0];
    _T_336 <= _GEN_128[39:0];
    _T_339 <= _GEN_129[39:0];
    _T_342 <= _GEN_130[39:0];
    _T_345 <= _GEN_131[39:0];
    _T_348 <= _GEN_132[39:0];
    _T_351 <= _GEN_133[39:0];
    _T_354 <= _GEN_134[39:0];
    _T_357 <= _GEN_135[39:0];
    _T_360 <= _GEN_136[39:0];
    _T_363 <= _GEN_137[39:0];
    _T_366 <= _GEN_138[39:0];
    _T_369 <= _GEN_139[39:0];
    _T_372 <= _GEN_140[39:0];
    _T_375 <= _GEN_141[39:0];
    if (wen) begin
      if (_T_601) begin
        reg_dpc <= wdata;
      end
    end
    if (wen) begin
      if (_T_603) begin
        reg_dscratch <= wdata;
      end
    end
    if (reset) begin
      reg_dcsr_ebreakm <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_599) begin
          reg_dcsr_ebreakm <= _T_1344;
        end
      end
    end
    if (reset) begin
      reg_dcsr_step <= 1'h0;
    end else begin
      if (wen) begin
        if (_T_599) begin
          reg_dcsr_step <= _T_1334;
        end
      end
    end
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_1054) begin
          $fwrite(32'h80000002,"Assertion failed: these conditions must be mutually exclusive\n    at csr.scala:291 assert(PopCount(insn_ret :: io.exception :: Nil) <= 1, \"these conditions must be mutually exclusive\")\n"); // @[csr.scala 291:9:@2099.6]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
    `ifndef SYNTHESIS
    `ifdef STOP_COND
      if (`STOP_COND) begin
    `endif
        if (_T_1054) begin
          $fatal; // @[csr.scala 291:9:@2100.6]
        end
    `ifdef STOP_COND
      end
    `endif
    `endif // SYNTHESIS
  end
endmodule
module DatPath( // @[:@2886.2]
  input         clock, // @[:@2887.4]
  input         reset, // @[:@2888.4]
  input  [4:0]  io_ddpath_addr, // @[:@2889.4]
  input  [31:0] io_ddpath_wdata, // @[:@2889.4]
  output [31:0] io_ddpath_rdata, // @[:@2889.4]
  output [31:0] io_imem_req_bits_addr, // @[:@2889.4]
  input  [31:0] io_imem_resp_bits_data, // @[:@2889.4]
  output [31:0] io_dmem_req_bits_addr, // @[:@2889.4]
  output [31:0] io_dmem_req_bits_data, // @[:@2889.4]
  input  [31:0] io_dmem_resp_bits_data, // @[:@2889.4]
  input         io_ctl_stall, // @[:@2889.4]
  input  [2:0]  io_ctl_pc_sel, // @[:@2889.4]
  input  [1:0]  io_ctl_op1_sel, // @[:@2889.4]
  input  [1:0]  io_ctl_op2_sel, // @[:@2889.4]
  input  [3:0]  io_ctl_alu_fun, // @[:@2889.4]
  input  [1:0]  io_ctl_wb_sel, // @[:@2889.4]
  input         io_ctl_rf_wen, // @[:@2889.4]
  input  [2:0]  io_ctl_csr_cmd, // @[:@2889.4]
  input         io_ctl_exception, // @[:@2889.4]
  output [31:0] io_dat_inst, // @[:@2889.4]
  output        io_dat_br_eq, // @[:@2889.4]
  output        io_dat_br_lt, // @[:@2889.4]
  output        io_dat_br_ltu, // @[:@2889.4]
  output        io_dat_csr_eret // @[:@2889.4]
);
  reg [31:0] regfile [0:31]; // @[dpath.scala 83:21:@2958.4]
  reg [31:0] _RAND_0;
  wire [31:0] regfile__T_180_data; // @[dpath.scala 83:21:@2958.4]
  wire [4:0] regfile__T_180_addr; // @[dpath.scala 83:21:@2958.4]
  wire [31:0] regfile__T_184_data; // @[dpath.scala 83:21:@2958.4]
  wire [4:0] regfile__T_184_addr; // @[dpath.scala 83:21:@2958.4]
  wire [31:0] regfile__T_188_data; // @[dpath.scala 83:21:@2958.4]
  wire [4:0] regfile__T_188_addr; // @[dpath.scala 83:21:@2958.4]
  wire [31:0] regfile__T_179_data; // @[dpath.scala 83:21:@2958.4]
  wire [4:0] regfile__T_179_addr; // @[dpath.scala 83:21:@2958.4]
  wire  regfile__T_179_mask; // @[dpath.scala 83:21:@2958.4]
  wire  regfile__T_179_en; // @[dpath.scala 83:21:@2958.4]
  wire [31:0] regfile__T_181_data; // @[dpath.scala 83:21:@2958.4]
  wire [4:0] regfile__T_181_addr; // @[dpath.scala 83:21:@2958.4]
  wire  regfile__T_181_mask; // @[dpath.scala 83:21:@2958.4]
  wire  regfile__T_181_en; // @[dpath.scala 83:21:@2958.4]
  wire  csr_clock; // @[dpath.scala 157:20:@3087.4]
  wire  csr_reset; // @[dpath.scala 157:20:@3087.4]
  wire [2:0] csr_io_rw_cmd; // @[dpath.scala 157:20:@3087.4]
  wire [31:0] csr_io_rw_rdata; // @[dpath.scala 157:20:@3087.4]
  wire [31:0] csr_io_rw_wdata; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_eret; // @[dpath.scala 157:20:@3087.4]
  wire [11:0] csr_io_decode_csr; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_debug; // @[dpath.scala 157:20:@3087.4]
  wire [1:0] csr_io_status_prv; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_sd; // @[dpath.scala 157:20:@3087.4]
  wire [7:0] csr_io_status_zero1; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_tsr; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_tw; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_tvm; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_mxr; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_sum; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_mprv; // @[dpath.scala 157:20:@3087.4]
  wire [1:0] csr_io_status_xs; // @[dpath.scala 157:20:@3087.4]
  wire [1:0] csr_io_status_fs; // @[dpath.scala 157:20:@3087.4]
  wire [1:0] csr_io_status_mpp; // @[dpath.scala 157:20:@3087.4]
  wire [1:0] csr_io_status_hpp; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_spp; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_mpie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_hpie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_spie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_upie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_mie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_hie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_sie; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_status_uie; // @[dpath.scala 157:20:@3087.4]
  wire [31:0] csr_io_evec; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_exception; // @[dpath.scala 157:20:@3087.4]
  wire  csr_io_retire; // @[dpath.scala 157:20:@3087.4]
  wire [31:0] csr_io_pc; // @[dpath.scala 157:20:@3087.4]
  wire [31:0] csr_io_time; // @[dpath.scala 157:20:@3087.4]
  wire  _T_153; // @[dpath.scala 53:34:@2932.4]
  wire  _T_154; // @[dpath.scala 54:34:@2933.4]
  wire  _T_155; // @[dpath.scala 55:34:@2934.4]
  wire  _T_156; // @[dpath.scala 56:34:@2935.4]
  wire  _T_157; // @[dpath.scala 57:34:@2936.4]
  wire [31:0] exception_target; // @[dpath.scala 49:31:@2931.4 dpath.scala 166:21:@3197.4]
  reg [31:0] pc_reg; // @[dpath.scala 60:24:@2943.4]
  reg [31:0] _RAND_1;
  wire [32:0] _T_167; // @[dpath.scala 67:24:@2948.4]
  wire [31:0] pc_plus4; // @[dpath.scala 67:24:@2949.4]
  wire [31:0] _T_158; // @[Mux.scala 61:16:@2937.4]
  wire [4:0] rs1_addr; // @[dpath.scala 76:23:@2954.4]
  wire  _T_183; // @[dpath.scala 97:33:@2973.4]
  wire [31:0] rs1_data; // @[dpath.scala 97:22:@2975.4]
  wire [11:0] imm_i; // @[dpath.scala 102:20:@2979.4]
  wire  _T_210; // @[dpath.scala 110:38:@3001.4]
  wire [19:0] _T_214; // @[Bitwise.scala 72:12:@3003.4]
  wire [31:0] imm_i_sext; // @[Cat.scala 30:58:@3004.4]
  wire [32:0] _T_299; // @[dpath.scala 154:42:@3084.4]
  wire [31:0] jump_reg_target; // @[dpath.scala 154:42:@3085.4]
  wire [31:0] _T_159; // @[Mux.scala 61:16:@2938.4]
  wire  _T_198; // @[dpath.scala 106:24:@2991.4]
  wire [7:0] _T_199; // @[dpath.scala 106:34:@2992.4]
  wire [8:0] _T_203; // @[Cat.scala 30:58:@2996.4]
  wire  _T_200; // @[dpath.scala 106:47:@2993.4]
  wire [9:0] _T_201; // @[dpath.scala 106:57:@2994.4]
  wire [10:0] _T_202; // @[Cat.scala 30:58:@2995.4]
  wire [19:0] imm_j; // @[Cat.scala 30:58:@2997.4]
  wire  _T_232; // @[dpath.scala 114:38:@3016.4]
  wire [10:0] _T_236; // @[Bitwise.scala 72:12:@3018.4]
  wire [30:0] _T_238; // @[Cat.scala 30:58:@3019.4]
  wire [31:0] imm_j_sext; // @[Cat.scala 30:58:@3020.4]
  wire [32:0] _T_297; // @[dpath.scala 153:30:@3081.4]
  wire [31:0] jmp_target; // @[dpath.scala 153:30:@3082.4]
  wire [31:0] _T_160; // @[Mux.scala 61:16:@2939.4]
  wire  _T_193; // @[dpath.scala 104:34:@2984.4]
  wire [1:0] _T_197; // @[Cat.scala 30:58:@2988.4]
  wire [5:0] _T_194; // @[dpath.scala 104:43:@2985.4]
  wire [3:0] _T_195; // @[dpath.scala 104:56:@2986.4]
  wire [9:0] _T_196; // @[Cat.scala 30:58:@2987.4]
  wire [11:0] imm_b; // @[Cat.scala 30:58:@2989.4]
  wire  _T_220; // @[dpath.scala 112:38:@3009.4]
  wire [18:0] _T_224; // @[Bitwise.scala 72:12:@3011.4]
  wire [30:0] _T_226; // @[Cat.scala 30:58:@3012.4]
  wire [31:0] imm_b_sext; // @[Cat.scala 30:58:@3013.4]
  wire [32:0] _T_295; // @[dpath.scala 152:30:@3078.4]
  wire [31:0] br_target; // @[dpath.scala 152:30:@3079.4]
  wire [31:0] _T_161; // @[Mux.scala 61:16:@2940.4]
  wire [31:0] pc_next; // @[Mux.scala 61:16:@2941.4]
  wire  _T_165; // @[dpath.scala 62:10:@2944.4]
  wire [31:0] _GEN_0; // @[dpath.scala 63:4:@2945.4]
  wire [4:0] rs2_addr; // @[dpath.scala 77:23:@2955.4]
  wire [4:0] wb_addr; // @[dpath.scala 78:23:@2956.4]
  wire  _T_174; // @[dpath.scala 85:36:@2959.4]
  wire  _T_175; // @[dpath.scala 85:24:@2960.4]
  wire  _T_177; // @[dpath.scala 85:48:@2961.4]
  wire  _T_364; // @[dpath.scala 175:34:@3259.4]
  wire  _T_255; // @[dpath.scala 138:35:@3037.4]
  wire  _T_240; // @[dpath.scala 118:32:@3021.4]
  wire  _T_241; // @[dpath.scala 119:32:@3022.4]
  wire [19:0] imm_u; // @[dpath.scala 105:20:@2990.4]
  wire [31:0] imm_u_sext; // @[Cat.scala 30:58:@3015.4]
  wire  _T_242; // @[dpath.scala 120:32:@3023.4]
  wire [31:0] imm_z; // @[Cat.scala 30:58:@3000.4]
  wire [31:0] _T_243; // @[Mux.scala 61:16:@3024.4]
  wire [31:0] _T_244; // @[Mux.scala 61:16:@3025.4]
  wire [31:0] alu_op1; // @[Mux.scala 61:16:@3026.4]
  wire  _T_246; // @[dpath.scala 124:32:@3027.4]
  wire  _T_187; // @[dpath.scala 98:33:@2976.4]
  wire [31:0] rs2_data; // @[dpath.scala 98:22:@2978.4]
  wire  _T_247; // @[dpath.scala 125:32:@3028.4]
  wire  _T_248; // @[dpath.scala 126:32:@3029.4]
  wire  _T_249; // @[dpath.scala 127:32:@3030.4]
  wire [6:0] _T_190; // @[dpath.scala 103:24:@2980.4]
  wire [11:0] imm_s; // @[Cat.scala 30:58:@2982.4]
  wire  _T_215; // @[dpath.scala 111:38:@3005.4]
  wire [19:0] _T_219; // @[Bitwise.scala 72:12:@3007.4]
  wire [31:0] imm_s_sext; // @[Cat.scala 30:58:@3008.4]
  wire [31:0] _T_250; // @[Mux.scala 61:16:@3031.4]
  wire [31:0] _T_251; // @[Mux.scala 61:16:@3032.4]
  wire [31:0] _T_252; // @[Mux.scala 61:16:@3033.4]
  wire [31:0] alu_op2; // @[Mux.scala 61:16:@3034.4]
  wire [32:0] _T_256; // @[dpath.scala 138:61:@3038.4]
  wire [31:0] _T_257; // @[dpath.scala 138:61:@3039.4]
  wire  _T_258; // @[dpath.scala 139:35:@3040.4]
  wire [32:0] _T_259; // @[dpath.scala 139:61:@3041.4]
  wire [32:0] _T_260; // @[dpath.scala 139:61:@3042.4]
  wire [31:0] _T_261; // @[dpath.scala 139:61:@3043.4]
  wire  _T_262; // @[dpath.scala 140:35:@3044.4]
  wire [31:0] _T_263; // @[dpath.scala 140:61:@3045.4]
  wire  _T_264; // @[dpath.scala 141:35:@3046.4]
  wire [31:0] _T_265; // @[dpath.scala 141:61:@3047.4]
  wire  _T_266; // @[dpath.scala 142:35:@3048.4]
  wire [31:0] _T_267; // @[dpath.scala 142:61:@3049.4]
  wire  _T_268; // @[dpath.scala 143:35:@3050.4]
  wire [31:0] _T_269; // @[dpath.scala 143:67:@3051.4]
  wire [31:0] _T_270; // @[dpath.scala 143:86:@3052.4]
  wire  _T_271; // @[dpath.scala 143:70:@3053.4]
  wire  _T_272; // @[dpath.scala 144:35:@3054.4]
  wire  _T_273; // @[dpath.scala 144:61:@3055.4]
  wire  _T_274; // @[dpath.scala 145:35:@3056.4]
  wire [4:0] alu_shamt; // @[dpath.scala 135:27:@3036.4]
  wire [62:0] _GEN_25; // @[dpath.scala 145:62:@3057.4]
  wire [62:0] _T_275; // @[dpath.scala 145:62:@3057.4]
  wire [31:0] _T_276; // @[dpath.scala 145:75:@3058.4]
  wire  _T_277; // @[dpath.scala 146:35:@3059.4]
  wire [31:0] _T_279; // @[dpath.scala 146:70:@3061.4]
  wire [31:0] _T_280; // @[dpath.scala 146:90:@3062.4]
  wire  _T_281; // @[dpath.scala 147:35:@3063.4]
  wire [31:0] _T_282; // @[dpath.scala 147:61:@3064.4]
  wire  _T_283; // @[dpath.scala 148:35:@3065.4]
  wire [31:0] _T_284; // @[Mux.scala 61:16:@3066.4]
  wire [31:0] _T_285; // @[Mux.scala 61:16:@3067.4]
  wire [31:0] _T_286; // @[Mux.scala 61:16:@3068.4]
  wire [31:0] _T_287; // @[Mux.scala 61:16:@3069.4]
  wire [31:0] _T_288; // @[Mux.scala 61:16:@3070.4]
  wire [31:0] _T_289; // @[Mux.scala 61:16:@3071.4]
  wire [31:0] _T_290; // @[Mux.scala 61:16:@3072.4]
  wire [31:0] _T_291; // @[Mux.scala 61:16:@3073.4]
  wire [31:0] _T_292; // @[Mux.scala 61:16:@3074.4]
  wire [31:0] _T_293; // @[Mux.scala 61:16:@3075.4]
  wire [31:0] alu_out; // @[Mux.scala 61:16:@3076.4]
  wire  _T_365; // @[dpath.scala 176:34:@3260.4]
  wire  _T_366; // @[dpath.scala 177:34:@3261.4]
  wire  _T_367; // @[dpath.scala 178:34:@3262.4]
  wire [31:0] _T_368; // @[Mux.scala 61:16:@3263.4]
  wire [31:0] _T_369; // @[Mux.scala 61:16:@3264.4]
  wire [31:0] _T_370; // @[Mux.scala 61:16:@3265.4]
  wire [31:0] wb_data; // @[Mux.scala 61:16:@3266.4]
  wire [31:0] _T_373; // @[dpath.scala 185:37:@3271.4]
  wire [31:0] _T_374; // @[dpath.scala 185:57:@3272.4]
  wire [31:0] _T_377; // @[dpath.scala 197:20:@3279.4]
  wire [7:0] _T_380; // @[dpath.scala 200:12:@3280.4]
  wire [7:0] _T_383; // @[dpath.scala 203:12:@3281.4]
  wire [7:0] _T_386; // @[dpath.scala 208:12:@3282.4]
  wire [7:0] _T_403; // @[dpath.scala 213:13:@3288.4]
  wire [7:0] _T_404; // @[dpath.scala 212:13:@3289.4]
  wire [7:0] _T_405; // @[dpath.scala 211:13:@3290.4]
  wire [7:0] _T_406; // @[dpath.scala 210:13:@3291.4]
  wire [7:0] _T_407; // @[dpath.scala 209:12:@3292.4]
  wire  _T_410; // @[dpath.scala 196:10:@3294.4]
  CSRFile csr ( // @[dpath.scala 157:20:@3087.4]
    .clock(csr_clock),
    .reset(csr_reset),
    .io_rw_cmd(csr_io_rw_cmd),
    .io_rw_rdata(csr_io_rw_rdata),
    .io_rw_wdata(csr_io_rw_wdata),
    .io_eret(csr_io_eret),
    .io_decode_csr(csr_io_decode_csr),
    .io_status_debug(csr_io_status_debug),
    .io_status_prv(csr_io_status_prv),
    .io_status_sd(csr_io_status_sd),
    .io_status_zero1(csr_io_status_zero1),
    .io_status_tsr(csr_io_status_tsr),
    .io_status_tw(csr_io_status_tw),
    .io_status_tvm(csr_io_status_tvm),
    .io_status_mxr(csr_io_status_mxr),
    .io_status_sum(csr_io_status_sum),
    .io_status_mprv(csr_io_status_mprv),
    .io_status_xs(csr_io_status_xs),
    .io_status_fs(csr_io_status_fs),
    .io_status_mpp(csr_io_status_mpp),
    .io_status_hpp(csr_io_status_hpp),
    .io_status_spp(csr_io_status_spp),
    .io_status_mpie(csr_io_status_mpie),
    .io_status_hpie(csr_io_status_hpie),
    .io_status_spie(csr_io_status_spie),
    .io_status_upie(csr_io_status_upie),
    .io_status_mie(csr_io_status_mie),
    .io_status_hie(csr_io_status_hie),
    .io_status_sie(csr_io_status_sie),
    .io_status_uie(csr_io_status_uie),
    .io_evec(csr_io_evec),
    .io_exception(csr_io_exception),
    .io_retire(csr_io_retire),
    .io_pc(csr_io_pc),
    .io_time(csr_io_time)
  );
  assign regfile__T_180_addr = io_ddpath_addr;
  assign regfile__T_180_data = regfile[regfile__T_180_addr]; // @[dpath.scala 83:21:@2958.4]
  assign regfile__T_184_addr = io_imem_resp_bits_data[19:15];
  assign regfile__T_184_data = regfile[regfile__T_184_addr]; // @[dpath.scala 83:21:@2958.4]
  assign regfile__T_188_addr = io_imem_resp_bits_data[24:20];
  assign regfile__T_188_data = regfile[regfile__T_188_addr]; // @[dpath.scala 83:21:@2958.4]
  assign regfile__T_179_data = _T_364 ? alu_out : _T_370;
  assign regfile__T_179_addr = io_imem_resp_bits_data[11:7];
  assign regfile__T_179_mask = 1'h1;
  assign regfile__T_179_en = _T_175 & _T_177;
  assign regfile__T_181_data = io_ddpath_wdata;
  assign regfile__T_181_addr = io_ddpath_addr;
  assign regfile__T_181_mask = 1'h1;
  assign regfile__T_181_en = 1'h1;
  assign _T_153 = io_ctl_pc_sel == 3'h0; // @[dpath.scala 53:34:@2932.4]
  assign _T_154 = io_ctl_pc_sel == 3'h1; // @[dpath.scala 54:34:@2933.4]
  assign _T_155 = io_ctl_pc_sel == 3'h2; // @[dpath.scala 55:34:@2934.4]
  assign _T_156 = io_ctl_pc_sel == 3'h3; // @[dpath.scala 56:34:@2935.4]
  assign _T_157 = io_ctl_pc_sel == 3'h4; // @[dpath.scala 57:34:@2936.4]
  assign exception_target = csr_io_evec; // @[dpath.scala 49:31:@2931.4 dpath.scala 166:21:@3197.4]
  assign _T_167 = pc_reg + 32'h4; // @[dpath.scala 67:24:@2948.4]
  assign pc_plus4 = _T_167[31:0]; // @[dpath.scala 67:24:@2949.4]
  assign _T_158 = _T_157 ? exception_target : pc_plus4; // @[Mux.scala 61:16:@2937.4]
  assign rs1_addr = io_imem_resp_bits_data[19:15]; // @[dpath.scala 76:23:@2954.4]
  assign _T_183 = rs1_addr != 5'h0; // @[dpath.scala 97:33:@2973.4]
  assign rs1_data = _T_183 ? regfile__T_184_data : 32'h0; // @[dpath.scala 97:22:@2975.4]
  assign imm_i = io_imem_resp_bits_data[31:20]; // @[dpath.scala 102:20:@2979.4]
  assign _T_210 = imm_i[11]; // @[dpath.scala 110:38:@3001.4]
  assign _T_214 = _T_210 ? 20'hfffff : 20'h0; // @[Bitwise.scala 72:12:@3003.4]
  assign imm_i_sext = {_T_214,imm_i}; // @[Cat.scala 30:58:@3004.4]
  assign _T_299 = rs1_data + imm_i_sext; // @[dpath.scala 154:42:@3084.4]
  assign jump_reg_target = _T_299[31:0]; // @[dpath.scala 154:42:@3085.4]
  assign _T_159 = _T_156 ? jump_reg_target : _T_158; // @[Mux.scala 61:16:@2938.4]
  assign _T_198 = io_imem_resp_bits_data[31]; // @[dpath.scala 106:24:@2991.4]
  assign _T_199 = io_imem_resp_bits_data[19:12]; // @[dpath.scala 106:34:@2992.4]
  assign _T_203 = {_T_198,_T_199}; // @[Cat.scala 30:58:@2996.4]
  assign _T_200 = io_imem_resp_bits_data[20]; // @[dpath.scala 106:47:@2993.4]
  assign _T_201 = io_imem_resp_bits_data[30:21]; // @[dpath.scala 106:57:@2994.4]
  assign _T_202 = {_T_200,_T_201}; // @[Cat.scala 30:58:@2995.4]
  assign imm_j = {_T_203,_T_202}; // @[Cat.scala 30:58:@2997.4]
  assign _T_232 = imm_j[19]; // @[dpath.scala 114:38:@3016.4]
  assign _T_236 = _T_232 ? 11'h7ff : 11'h0; // @[Bitwise.scala 72:12:@3018.4]
  assign _T_238 = {_T_236,imm_j}; // @[Cat.scala 30:58:@3019.4]
  assign imm_j_sext = {_T_238,1'h0}; // @[Cat.scala 30:58:@3020.4]
  assign _T_297 = pc_reg + imm_j_sext; // @[dpath.scala 153:30:@3081.4]
  assign jmp_target = _T_297[31:0]; // @[dpath.scala 153:30:@3082.4]
  assign _T_160 = _T_155 ? jmp_target : _T_159; // @[Mux.scala 61:16:@2939.4]
  assign _T_193 = io_imem_resp_bits_data[7]; // @[dpath.scala 104:34:@2984.4]
  assign _T_197 = {_T_198,_T_193}; // @[Cat.scala 30:58:@2988.4]
  assign _T_194 = io_imem_resp_bits_data[30:25]; // @[dpath.scala 104:43:@2985.4]
  assign _T_195 = io_imem_resp_bits_data[11:8]; // @[dpath.scala 104:56:@2986.4]
  assign _T_196 = {_T_194,_T_195}; // @[Cat.scala 30:58:@2987.4]
  assign imm_b = {_T_197,_T_196}; // @[Cat.scala 30:58:@2989.4]
  assign _T_220 = imm_b[11]; // @[dpath.scala 112:38:@3009.4]
  assign _T_224 = _T_220 ? 19'h7ffff : 19'h0; // @[Bitwise.scala 72:12:@3011.4]
  assign _T_226 = {_T_224,imm_b}; // @[Cat.scala 30:58:@3012.4]
  assign imm_b_sext = {_T_226,1'h0}; // @[Cat.scala 30:58:@3013.4]
  assign _T_295 = pc_reg + imm_b_sext; // @[dpath.scala 152:30:@3078.4]
  assign br_target = _T_295[31:0]; // @[dpath.scala 152:30:@3079.4]
  assign _T_161 = _T_154 ? br_target : _T_160; // @[Mux.scala 61:16:@2940.4]
  assign pc_next = _T_153 ? pc_plus4 : _T_161; // @[Mux.scala 61:16:@2941.4]
  assign _T_165 = io_ctl_stall == 1'h0; // @[dpath.scala 62:10:@2944.4]
  assign _GEN_0 = _T_165 ? pc_next : pc_reg; // @[dpath.scala 63:4:@2945.4]
  assign rs2_addr = io_imem_resp_bits_data[24:20]; // @[dpath.scala 77:23:@2955.4]
  assign wb_addr = io_imem_resp_bits_data[11:7]; // @[dpath.scala 78:23:@2956.4]
  assign _T_174 = wb_addr != 5'h0; // @[dpath.scala 85:36:@2959.4]
  assign _T_175 = io_ctl_rf_wen & _T_174; // @[dpath.scala 85:24:@2960.4]
  assign _T_177 = io_ctl_exception == 1'h0; // @[dpath.scala 85:48:@2961.4]
  assign _T_364 = io_ctl_wb_sel == 2'h0; // @[dpath.scala 175:34:@3259.4]
  assign _T_255 = io_ctl_alu_fun == 4'h1; // @[dpath.scala 138:35:@3037.4]
  assign _T_240 = io_ctl_op1_sel == 2'h0; // @[dpath.scala 118:32:@3021.4]
  assign _T_241 = io_ctl_op1_sel == 2'h1; // @[dpath.scala 119:32:@3022.4]
  assign imm_u = io_imem_resp_bits_data[31:12]; // @[dpath.scala 105:20:@2990.4]
  assign imm_u_sext = {imm_u,12'h0}; // @[Cat.scala 30:58:@3015.4]
  assign _T_242 = io_ctl_op1_sel == 2'h2; // @[dpath.scala 120:32:@3023.4]
  assign imm_z = {27'h0,rs1_addr}; // @[Cat.scala 30:58:@3000.4]
  assign _T_243 = _T_242 ? imm_z : 32'h0; // @[Mux.scala 61:16:@3024.4]
  assign _T_244 = _T_241 ? imm_u_sext : _T_243; // @[Mux.scala 61:16:@3025.4]
  assign alu_op1 = _T_240 ? rs1_data : _T_244; // @[Mux.scala 61:16:@3026.4]
  assign _T_246 = io_ctl_op2_sel == 2'h0; // @[dpath.scala 124:32:@3027.4]
  assign _T_187 = rs2_addr != 5'h0; // @[dpath.scala 98:33:@2976.4]
  assign rs2_data = _T_187 ? regfile__T_188_data : 32'h0; // @[dpath.scala 98:22:@2978.4]
  assign _T_247 = io_ctl_op2_sel == 2'h3; // @[dpath.scala 125:32:@3028.4]
  assign _T_248 = io_ctl_op2_sel == 2'h1; // @[dpath.scala 126:32:@3029.4]
  assign _T_249 = io_ctl_op2_sel == 2'h2; // @[dpath.scala 127:32:@3030.4]
  assign _T_190 = io_imem_resp_bits_data[31:25]; // @[dpath.scala 103:24:@2980.4]
  assign imm_s = {_T_190,wb_addr}; // @[Cat.scala 30:58:@2982.4]
  assign _T_215 = imm_s[11]; // @[dpath.scala 111:38:@3005.4]
  assign _T_219 = _T_215 ? 20'hfffff : 20'h0; // @[Bitwise.scala 72:12:@3007.4]
  assign imm_s_sext = {_T_219,imm_s}; // @[Cat.scala 30:58:@3008.4]
  assign _T_250 = _T_249 ? imm_s_sext : 32'h0; // @[Mux.scala 61:16:@3031.4]
  assign _T_251 = _T_248 ? imm_i_sext : _T_250; // @[Mux.scala 61:16:@3032.4]
  assign _T_252 = _T_247 ? pc_reg : _T_251; // @[Mux.scala 61:16:@3033.4]
  assign alu_op2 = _T_246 ? rs2_data : _T_252; // @[Mux.scala 61:16:@3034.4]
  assign _T_256 = alu_op1 + alu_op2; // @[dpath.scala 138:61:@3038.4]
  assign _T_257 = _T_256[31:0]; // @[dpath.scala 138:61:@3039.4]
  assign _T_258 = io_ctl_alu_fun == 4'h2; // @[dpath.scala 139:35:@3040.4]
  assign _T_259 = alu_op1 - alu_op2; // @[dpath.scala 139:61:@3041.4]
  assign _T_260 = $unsigned(_T_259); // @[dpath.scala 139:61:@3042.4]
  assign _T_261 = _T_260[31:0]; // @[dpath.scala 139:61:@3043.4]
  assign _T_262 = io_ctl_alu_fun == 4'h6; // @[dpath.scala 140:35:@3044.4]
  assign _T_263 = alu_op1 & alu_op2; // @[dpath.scala 140:61:@3045.4]
  assign _T_264 = io_ctl_alu_fun == 4'h7; // @[dpath.scala 141:35:@3046.4]
  assign _T_265 = alu_op1 | alu_op2; // @[dpath.scala 141:61:@3047.4]
  assign _T_266 = io_ctl_alu_fun == 4'h8; // @[dpath.scala 142:35:@3048.4]
  assign _T_267 = alu_op1 ^ alu_op2; // @[dpath.scala 142:61:@3049.4]
  assign _T_268 = io_ctl_alu_fun == 4'h9; // @[dpath.scala 143:35:@3050.4]
  assign _T_269 = $signed(alu_op1); // @[dpath.scala 143:67:@3051.4]
  assign _T_270 = $signed(alu_op2); // @[dpath.scala 143:86:@3052.4]
  assign _T_271 = $signed(_T_269) < $signed(_T_270); // @[dpath.scala 143:70:@3053.4]
  assign _T_272 = io_ctl_alu_fun == 4'ha; // @[dpath.scala 144:35:@3054.4]
  assign _T_273 = alu_op1 < alu_op2; // @[dpath.scala 144:61:@3055.4]
  assign _T_274 = io_ctl_alu_fun == 4'h3; // @[dpath.scala 145:35:@3056.4]
  assign alu_shamt = alu_op2[4:0]; // @[dpath.scala 135:27:@3036.4]
  assign _GEN_25 = {{31'd0}, alu_op1}; // @[dpath.scala 145:62:@3057.4]
  assign _T_275 = _GEN_25 << alu_shamt; // @[dpath.scala 145:62:@3057.4]
  assign _T_276 = _T_275[31:0]; // @[dpath.scala 145:75:@3058.4]
  assign _T_277 = io_ctl_alu_fun == 4'h5; // @[dpath.scala 146:35:@3059.4]
  assign _T_279 = $signed(_T_269) >>> alu_shamt; // @[dpath.scala 146:70:@3061.4]
  assign _T_280 = $unsigned(_T_279); // @[dpath.scala 146:90:@3062.4]
  assign _T_281 = io_ctl_alu_fun == 4'h4; // @[dpath.scala 147:35:@3063.4]
  assign _T_282 = alu_op1 >> alu_shamt; // @[dpath.scala 147:61:@3064.4]
  assign _T_283 = io_ctl_alu_fun == 4'hb; // @[dpath.scala 148:35:@3065.4]
  assign _T_284 = _T_283 ? alu_op1 : 32'h0; // @[Mux.scala 61:16:@3066.4]
  assign _T_285 = _T_281 ? _T_282 : _T_284; // @[Mux.scala 61:16:@3067.4]
  assign _T_286 = _T_277 ? _T_280 : _T_285; // @[Mux.scala 61:16:@3068.4]
  assign _T_287 = _T_274 ? _T_276 : _T_286; // @[Mux.scala 61:16:@3069.4]
  assign _T_288 = _T_272 ? {{31'd0}, _T_273} : _T_287; // @[Mux.scala 61:16:@3070.4]
  assign _T_289 = _T_268 ? {{31'd0}, _T_271} : _T_288; // @[Mux.scala 61:16:@3071.4]
  assign _T_290 = _T_266 ? _T_267 : _T_289; // @[Mux.scala 61:16:@3072.4]
  assign _T_291 = _T_264 ? _T_265 : _T_290; // @[Mux.scala 61:16:@3073.4]
  assign _T_292 = _T_262 ? _T_263 : _T_291; // @[Mux.scala 61:16:@3074.4]
  assign _T_293 = _T_258 ? _T_261 : _T_292; // @[Mux.scala 61:16:@3075.4]
  assign alu_out = _T_255 ? _T_257 : _T_293; // @[Mux.scala 61:16:@3076.4]
  assign _T_365 = io_ctl_wb_sel == 2'h1; // @[dpath.scala 176:34:@3260.4]
  assign _T_366 = io_ctl_wb_sel == 2'h2; // @[dpath.scala 177:34:@3261.4]
  assign _T_367 = io_ctl_wb_sel == 2'h3; // @[dpath.scala 178:34:@3262.4]
  assign _T_368 = _T_367 ? csr_io_rw_rdata : alu_out; // @[Mux.scala 61:16:@3263.4]
  assign _T_369 = _T_366 ? pc_plus4 : _T_368; // @[Mux.scala 61:16:@3264.4]
  assign _T_370 = _T_365 ? io_dmem_resp_bits_data : _T_369; // @[Mux.scala 61:16:@3265.4]
  assign wb_data = _T_364 ? alu_out : _T_370; // @[Mux.scala 61:16:@3266.4]
  assign _T_373 = $signed(rs1_data); // @[dpath.scala 185:37:@3271.4]
  assign _T_374 = $signed(rs2_data); // @[dpath.scala 185:57:@3272.4]
  assign _T_377 = csr_io_time; // @[dpath.scala 197:20:@3279.4]
  assign _T_380 = io_ctl_rf_wen ? 8'h57 : 8'h5f; // @[dpath.scala 200:12:@3280.4]
  assign _T_383 = csr_io_exception ? 8'h45 : 8'h20; // @[dpath.scala 203:12:@3281.4]
  assign _T_386 = io_ctl_stall ? 8'h73 : 8'h20; // @[dpath.scala 208:12:@3282.4]
  assign _T_403 = _T_153 ? 8'h20 : 8'h3f; // @[dpath.scala 213:13:@3288.4]
  assign _T_404 = _T_157 ? 8'h58 : _T_403; // @[dpath.scala 212:13:@3289.4]
  assign _T_405 = _T_156 ? 8'h4b : _T_404; // @[dpath.scala 211:13:@3290.4]
  assign _T_406 = _T_155 ? 8'h4a : _T_405; // @[dpath.scala 210:13:@3291.4]
  assign _T_407 = _T_154 ? 8'h42 : _T_406; // @[dpath.scala 209:12:@3292.4]
  assign _T_410 = reset == 1'h0; // @[dpath.scala 196:10:@3294.4]
  assign io_ddpath_rdata = regfile__T_180_data; // @[dpath.scala 91:20:@2968.4]
  assign io_imem_req_bits_addr = pc_reg; // @[dpath.scala 70:26:@2951.4]
  assign io_dmem_req_bits_addr = _T_255 ? _T_257 : _T_293; // @[dpath.scala 190:27:@3277.4]
  assign io_dmem_req_bits_data = _T_187 ? regfile__T_188_data : 32'h0; // @[dpath.scala 191:26:@3278.4]
  assign io_dat_inst = io_imem_resp_bits_data; // @[dpath.scala 183:18:@3268.4]
  assign io_dat_br_eq = rs1_data == rs2_data; // @[dpath.scala 184:18:@3270.4]
  assign io_dat_br_lt = $signed(_T_373) < $signed(_T_374); // @[dpath.scala 185:18:@3274.4]
  assign io_dat_br_ltu = rs1_data < rs2_data; // @[dpath.scala 186:18:@3276.4]
  assign io_dat_csr_eret = csr_io_eret; // @[dpath.scala 168:20:@3198.4]
  assign csr_clock = clock; // @[:@3088.4]
  assign csr_reset = reset; // @[:@3089.4]
  assign csr_io_rw_cmd = io_ctl_csr_cmd; // @[dpath.scala 160:20:@3191.4]
  assign csr_io_rw_wdata = _T_255 ? _T_257 : _T_293; // @[dpath.scala 161:20:@3192.4]
  assign csr_io_decode_csr = io_imem_resp_bits_data[31:20]; // @[dpath.scala 159:22:@3190.4]
  assign csr_io_exception = io_ctl_exception; // @[dpath.scala 164:21:@3195.4]
  assign csr_io_retire = io_ctl_stall == 1'h0; // @[dpath.scala 163:21:@3194.4]
  assign csr_io_pc = pc_reg; // @[dpath.scala 165:21:@3196.4]
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
  _RAND_0 = {1{`RANDOM}};
  `ifdef RANDOMIZE_MEM_INIT
  for (initvar = 0; initvar < 32; initvar = initvar+1)
    regfile[initvar] = _RAND_0[31:0];
  `endif // RANDOMIZE_MEM_INIT
  `ifdef RANDOMIZE_REG_INIT
  _RAND_1 = {1{`RANDOM}};
  pc_reg = _RAND_1[31:0];
  `endif // RANDOMIZE_REG_INIT
  end
`endif // RANDOMIZE
  always @(posedge clock) begin
    if(regfile__T_179_en & regfile__T_179_mask) begin
      regfile[regfile__T_179_addr] <= regfile__T_179_data; // @[dpath.scala 83:21:@2958.4]
    end
    if(regfile__T_181_en & regfile__T_181_mask) begin
      regfile[regfile__T_181_addr] <= regfile__T_181_data; // @[dpath.scala 83:21:@2958.4]
    end
    if (reset) begin
      pc_reg <= 32'h80000000;
    end else begin
      if (_T_165) begin
        if (_T_153) begin
          pc_reg <= pc_plus4;
        end else begin
          if (_T_154) begin
            pc_reg <= br_target;
          end else begin
            if (_T_155) begin
              pc_reg <= jmp_target;
            end else begin
              if (_T_156) begin
                pc_reg <= jump_reg_target;
              end else begin
                if (_T_157) begin
                  pc_reg <= exception_target;
                end else begin
                  pc_reg <= pc_plus4;
                end
              end
            end
          end
        end
      end
    end
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_410) begin
          $fwrite(32'h80000002,"Cyc= %d Op1=[0x%x] Op2=[0x%x] W[%c,%d= 0x%x] %c Mem[%d: R:0x%x W:0x%x] PC= 0x%x %c%c DASM(%x)\n",_T_377,alu_op1,alu_op2,_T_380,wb_addr,wb_data,_T_383,io_ctl_wb_sel,io_dmem_resp_bits_data,io_dmem_req_bits_data,pc_reg,_T_386,_T_407,io_imem_resp_bits_data); // @[dpath.scala 196:10:@3296.6]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
  end
endmodule
module Core( // @[:@3299.2]
  input         clock, // @[:@3300.4]
  input         reset, // @[:@3301.4]
  output [31:0] io_imem_req_bits_addr, // @[:@3302.4]
  input  [31:0] io_imem_resp_bits_data, // @[:@3302.4]
  output        io_dmem_req_valid, // @[:@3302.4]
  output [31:0] io_dmem_req_bits_addr, // @[:@3302.4]
  output [31:0] io_dmem_req_bits_data, // @[:@3302.4]
  output        io_dmem_req_bits_fcn, // @[:@3302.4]
  output [2:0]  io_dmem_req_bits_typ, // @[:@3302.4]
  input         io_dmem_resp_valid, // @[:@3302.4]
  input  [31:0] io_dmem_resp_bits_data, // @[:@3302.4]
  input  [4:0]  io_ddpath_addr, // @[:@3302.4]
  input  [31:0] io_ddpath_wdata, // @[:@3302.4]
  output [31:0] io_ddpath_rdata // @[:@3302.4]
);
  wire  c_io_dmem_req_valid; // @[core.scala 37:18:@3327.4]
  wire  c_io_dmem_req_bits_fcn; // @[core.scala 37:18:@3327.4]
  wire [2:0] c_io_dmem_req_bits_typ; // @[core.scala 37:18:@3327.4]
  wire  c_io_dmem_resp_valid; // @[core.scala 37:18:@3327.4]
  wire [31:0] c_io_dat_inst; // @[core.scala 37:18:@3327.4]
  wire  c_io_dat_br_eq; // @[core.scala 37:18:@3327.4]
  wire  c_io_dat_br_lt; // @[core.scala 37:18:@3327.4]
  wire  c_io_dat_br_ltu; // @[core.scala 37:18:@3327.4]
  wire  c_io_dat_csr_eret; // @[core.scala 37:18:@3327.4]
  wire  c_io_ctl_stall /*verilator public_flat*/; // @[core.scala 37:18:@3327.4]
  wire [2:0] c_io_ctl_pc_sel; // @[core.scala 37:18:@3327.4]
  wire [1:0] c_io_ctl_op1_sel; // @[core.scala 37:18:@3327.4]
  wire [1:0] c_io_ctl_op2_sel; // @[core.scala 37:18:@3327.4]
  wire [3:0] c_io_ctl_alu_fun; // @[core.scala 37:18:@3327.4]
  wire [1:0] c_io_ctl_wb_sel; // @[core.scala 37:18:@3327.4]
  wire  c_io_ctl_rf_wen; // @[core.scala 37:18:@3327.4]
  wire [2:0] c_io_ctl_csr_cmd; // @[core.scala 37:18:@3327.4]
  wire  c_io_ctl_exception; // @[core.scala 37:18:@3327.4]
  wire  d_clock; // @[core.scala 38:18:@3330.4]
  wire  d_reset; // @[core.scala 38:18:@3330.4]
  wire [4:0] d_io_ddpath_addr; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_ddpath_wdata; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_ddpath_rdata; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_imem_req_bits_addr; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_imem_resp_bits_data; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_dmem_req_bits_addr; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_dmem_req_bits_data; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_dmem_resp_bits_data; // @[core.scala 38:18:@3330.4]
  wire  d_io_ctl_stall; // @[core.scala 38:18:@3330.4]
  wire [2:0] d_io_ctl_pc_sel; // @[core.scala 38:18:@3330.4]
  wire [1:0] d_io_ctl_op1_sel; // @[core.scala 38:18:@3330.4]
  wire [1:0] d_io_ctl_op2_sel; // @[core.scala 38:18:@3330.4]
  wire [3:0] d_io_ctl_alu_fun; // @[core.scala 38:18:@3330.4]
  wire [1:0] d_io_ctl_wb_sel; // @[core.scala 38:18:@3330.4]
  wire  d_io_ctl_rf_wen; // @[core.scala 38:18:@3330.4]
  wire [2:0] d_io_ctl_csr_cmd; // @[core.scala 38:18:@3330.4]
  wire  d_io_ctl_exception; // @[core.scala 38:18:@3330.4]
  wire [31:0] d_io_dat_inst; // @[core.scala 38:18:@3330.4]
  wire  d_io_dat_br_eq; // @[core.scala 38:18:@3330.4]
  wire  d_io_dat_br_lt; // @[core.scala 38:18:@3330.4]
  wire  d_io_dat_br_ltu; // @[core.scala 38:18:@3330.4]
  wire  d_io_dat_csr_eret; // @[core.scala 38:18:@3330.4]
  CtlPath c ( // @[core.scala 37:18:@3327.4]
    .io_dmem_req_valid(c_io_dmem_req_valid),
    .io_dmem_req_bits_fcn(c_io_dmem_req_bits_fcn),
    .io_dmem_req_bits_typ(c_io_dmem_req_bits_typ),
    .io_dmem_resp_valid(c_io_dmem_resp_valid),
    .io_dat_inst(c_io_dat_inst),
    .io_dat_br_eq(c_io_dat_br_eq),
    .io_dat_br_lt(c_io_dat_br_lt),
    .io_dat_br_ltu(c_io_dat_br_ltu),
    .io_dat_csr_eret(c_io_dat_csr_eret),
    .io_ctl_stall(c_io_ctl_stall),
    .io_ctl_pc_sel(c_io_ctl_pc_sel),
    .io_ctl_op1_sel(c_io_ctl_op1_sel),
    .io_ctl_op2_sel(c_io_ctl_op2_sel),
    .io_ctl_alu_fun(c_io_ctl_alu_fun),
    .io_ctl_wb_sel(c_io_ctl_wb_sel),
    .io_ctl_rf_wen(c_io_ctl_rf_wen),
    .io_ctl_csr_cmd(c_io_ctl_csr_cmd),
    .io_ctl_exception(c_io_ctl_exception)
  );
  DatPath d ( // @[core.scala 38:18:@3330.4]
    .clock(d_clock),
    .reset(d_reset),
    .io_ddpath_addr(d_io_ddpath_addr),
    .io_ddpath_wdata(d_io_ddpath_wdata),
    .io_ddpath_rdata(d_io_ddpath_rdata),
    .io_imem_req_bits_addr(d_io_imem_req_bits_addr),
    .io_imem_resp_bits_data(d_io_imem_resp_bits_data),
    .io_dmem_req_bits_addr(d_io_dmem_req_bits_addr),
    .io_dmem_req_bits_data(d_io_dmem_req_bits_data),
    .io_dmem_resp_bits_data(d_io_dmem_resp_bits_data),
    .io_ctl_stall(d_io_ctl_stall),
    .io_ctl_pc_sel(d_io_ctl_pc_sel),
    .io_ctl_op1_sel(d_io_ctl_op1_sel),
    .io_ctl_op2_sel(d_io_ctl_op2_sel),
    .io_ctl_alu_fun(d_io_ctl_alu_fun),
    .io_ctl_wb_sel(d_io_ctl_wb_sel),
    .io_ctl_rf_wen(d_io_ctl_rf_wen),
    .io_ctl_csr_cmd(d_io_ctl_csr_cmd),
    .io_ctl_exception(d_io_ctl_exception),
    .io_dat_inst(d_io_dat_inst),
    .io_dat_br_eq(d_io_dat_br_eq),
    .io_dat_br_lt(d_io_dat_br_lt),
    .io_dat_br_ltu(d_io_dat_br_ltu),
    .io_dat_csr_eret(d_io_dat_csr_eret)
  );
  assign io_imem_req_bits_addr = d_io_imem_req_bits_addr; // @[core.scala 42:11:@3352.4 core.scala 43:11:@3360.4]
  assign io_dmem_req_valid = c_io_dmem_req_valid; // @[core.scala 45:11:@3369.4 core.scala 46:11:@3377.4 core.scala 47:21:@3379.4]
  assign io_dmem_req_bits_addr = d_io_dmem_req_bits_addr; // @[core.scala 45:11:@3368.4 core.scala 46:11:@3376.4]
  assign io_dmem_req_bits_data = d_io_dmem_req_bits_data; // @[core.scala 45:11:@3367.4 core.scala 46:11:@3375.4]
  assign io_dmem_req_bits_fcn = c_io_dmem_req_bits_fcn; // @[core.scala 45:11:@3366.4 core.scala 46:11:@3374.4 core.scala 49:24:@3381.4]
  assign io_dmem_req_bits_typ = c_io_dmem_req_bits_typ; // @[core.scala 45:11:@3365.4 core.scala 46:11:@3373.4 core.scala 48:24:@3380.4]
  assign io_ddpath_rdata = d_io_ddpath_rdata; // @[core.scala 51:15:@3383.4]
  assign c_io_dmem_resp_valid = io_dmem_resp_valid; // @[core.scala 45:11:@3364.4]
  assign c_io_dat_inst = d_io_dat_inst; // @[core.scala 40:13:@3346.4]
  assign c_io_dat_br_eq = d_io_dat_br_eq; // @[core.scala 40:13:@3345.4]
  assign c_io_dat_br_lt = d_io_dat_br_lt; // @[core.scala 40:13:@3344.4]
  assign c_io_dat_br_ltu = d_io_dat_br_ltu; // @[core.scala 40:13:@3343.4]
  assign c_io_dat_csr_eret = d_io_dat_csr_eret; // @[core.scala 40:13:@3342.4]
  assign d_clock = clock; // @[:@3331.4]
  assign d_reset = reset; // @[:@3332.4]
  assign d_io_ddpath_addr = io_ddpath_addr; // @[core.scala 51:15:@3386.4]
  assign d_io_ddpath_wdata = io_ddpath_wdata; // @[core.scala 51:15:@3385.4]
  assign d_io_imem_resp_bits_data = io_imem_resp_bits_data; // @[core.scala 43:11:@3355.4]
  assign d_io_dmem_resp_bits_data = io_dmem_resp_bits_data; // @[core.scala 46:11:@3371.4]
  assign d_io_ctl_stall = c_io_ctl_stall; // @[core.scala 39:13:@3341.4]
  assign d_io_ctl_pc_sel = c_io_ctl_pc_sel; // @[core.scala 39:13:@3340.4]
  assign d_io_ctl_op1_sel = c_io_ctl_op1_sel; // @[core.scala 39:13:@3339.4]
  assign d_io_ctl_op2_sel = c_io_ctl_op2_sel; // @[core.scala 39:13:@3338.4]
  assign d_io_ctl_alu_fun = c_io_ctl_alu_fun; // @[core.scala 39:13:@3337.4]
  assign d_io_ctl_wb_sel = c_io_ctl_wb_sel; // @[core.scala 39:13:@3336.4]
  assign d_io_ctl_rf_wen = c_io_ctl_rf_wen; // @[core.scala 39:13:@3335.4]
  assign d_io_ctl_csr_cmd = c_io_ctl_csr_cmd; // @[core.scala 39:13:@3334.4]
  assign d_io_ctl_exception = c_io_ctl_exception; // @[core.scala 39:13:@3333.4]
endmodule
module AsyncScratchPadMemory( // @[:@3399.2]
  input         clock, // @[:@3400.4]
  input         io_core_ports_0_req_valid /*verilator public_flat*/, // @[:@3402.4]
  input  [31:0] io_core_ports_0_req_bits_addr, // @[:@3402.4]
  input  [31:0] io_core_ports_0_req_bits_data, // @[:@3402.4]
  input         io_core_ports_0_req_bits_fcn, // @[:@3402.4]
  input  [2:0]  io_core_ports_0_req_bits_typ, // @[:@3402.4]
  output        io_core_ports_0_resp_valid /*verilator public_flat*/, // @[:@3402.4]
  output [31:0] io_core_ports_0_resp_bits_data, // @[:@3402.4]
  input  [31:0] io_core_ports_1_req_bits_addr, // @[:@3402.4]
  output [31:0] io_core_ports_1_resp_bits_data, // @[:@3402.4]
  input         io_debug_port_req_valid, // @[:@3402.4]
  input  [31:0] io_debug_port_req_bits_addr, // @[:@3402.4]
  input  [31:0] io_debug_port_req_bits_data, // @[:@3402.4]
  input         io_debug_port_req_bits_fcn, // @[:@3402.4]
  output        io_debug_port_resp_valid, // @[:@3402.4]
  output [31:0] io_debug_port_resp_bits_data // @[:@3402.4]
);
  wire  async_data_clk; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_hr_addr; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_hr_data; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_dw_addr /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_dw_data /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire [3:0] async_data_dw_mask /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire  async_data_dw_en /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_hw_addr; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_hw_data; // @[memory.scala 151:27:@3404.4]
  wire [3:0] async_data_hw_mask; // @[memory.scala 151:27:@3404.4]
  wire  async_data_hw_en; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_dataInstr_0_addr /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_dataInstr_0_data /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_dataInstr_1_addr /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire [31:0] async_data_dataInstr_1_data /*verilator public_flat*/; // @[memory.scala 151:27:@3404.4]
  wire  _T_267; // @[memory.scala 166:17:@3417.4]
  wire  _T_268; // @[memory.scala 166:52:@3418.4]
  wire [23:0] _T_272; // @[Bitwise.scala 72:12:@3420.4]
  wire [7:0] _T_273; // @[memory.scala 166:67:@3421.4]
  wire [31:0] _T_274; // @[Cat.scala 30:58:@3422.4]
  wire  _T_275; // @[memory.scala 167:17:@3423.4]
  wire  _T_276; // @[memory.scala 167:52:@3424.4]
  wire [15:0] _T_280; // @[Bitwise.scala 72:12:@3426.4]
  wire [15:0] _T_281; // @[memory.scala 167:68:@3427.4]
  wire [31:0] _T_282; // @[Cat.scala 30:58:@3428.4]
  wire  _T_283; // @[memory.scala 168:17:@3429.4]
  wire [31:0] _T_290; // @[Cat.scala 30:58:@3432.4]
  wire  _T_291; // @[memory.scala 169:17:@3433.4]
  wire [31:0] _T_298; // @[Cat.scala 30:58:@3436.4]
  wire [31:0] _T_299; // @[Mux.scala 61:16:@3437.4]
  wire [31:0] _T_300; // @[Mux.scala 61:16:@3438.4]
  wire [31:0] _T_301; // @[Mux.scala 61:16:@3439.4]
  wire [1:0] _T_309; // @[memory.scala 174:80:@3448.6]
  wire [4:0] _GEN_6; // @[memory.scala 174:86:@3449.6]
  wire [4:0] _T_310; // @[memory.scala 174:86:@3449.6]
  wire [62:0] _GEN_7; // @[memory.scala 174:67:@3450.6]
  wire [62:0] _T_311; // @[memory.scala 174:67:@3450.6]
  wire [29:0] _T_312; // @[memory.scala 175:45:@3452.6]
  wire [31:0] _T_314; // @[Cat.scala 30:58:@3453.6]
  wire [3:0] _T_318; // @[memory.scala 176:58:@3457.6]
  wire [4:0] _T_322; // @[memory.scala 177:57:@3460.6]
  wire [4:0] _T_324; // @[memory.scala 177:34:@3461.6]
  wire [4:0] _T_325; // @[memory.scala 176:35:@3462.6]
  AsyncReadMem async_data ( // @[memory.scala 151:27:@3404.4]
    .clk(async_data_clk),
    .hr_addr(async_data_hr_addr),
    .hr_data(async_data_hr_data),
    .dw_addr(async_data_dw_addr),
    .dw_data(async_data_dw_data),
    .dw_mask(async_data_dw_mask),
    .dw_en(async_data_dw_en),
    .hw_addr(async_data_hw_addr),
    .hw_data(async_data_hw_data),
    .hw_mask(async_data_hw_mask),
    .hw_en(async_data_hw_en),
    .dataInstr_0_addr(async_data_dataInstr_0_addr),
    .dataInstr_0_data(async_data_dataInstr_0_data),
    .dataInstr_1_addr(async_data_dataInstr_1_addr),
    .dataInstr_1_data(async_data_dataInstr_1_data)
  );
  assign _T_267 = io_core_ports_0_req_bits_typ == 3'h1; // @[memory.scala 166:17:@3417.4]
  assign _T_268 = async_data_dataInstr_0_data[7]; // @[memory.scala 166:52:@3418.4]
  assign _T_272 = _T_268 ? 24'hffffff : 24'h0; // @[Bitwise.scala 72:12:@3420.4]
  assign _T_273 = async_data_dataInstr_0_data[7:0]; // @[memory.scala 166:67:@3421.4]
  assign _T_274 = {_T_272,_T_273}; // @[Cat.scala 30:58:@3422.4]
  assign _T_275 = io_core_ports_0_req_bits_typ == 3'h2; // @[memory.scala 167:17:@3423.4]
  assign _T_276 = async_data_dataInstr_0_data[15]; // @[memory.scala 167:52:@3424.4]
  assign _T_280 = _T_276 ? 16'hffff : 16'h0; // @[Bitwise.scala 72:12:@3426.4]
  assign _T_281 = async_data_dataInstr_0_data[15:0]; // @[memory.scala 167:68:@3427.4]
  assign _T_282 = {_T_280,_T_281}; // @[Cat.scala 30:58:@3428.4]
  assign _T_283 = io_core_ports_0_req_bits_typ == 3'h5; // @[memory.scala 168:17:@3429.4]
  assign _T_290 = {24'h0,_T_273}; // @[Cat.scala 30:58:@3432.4]
  assign _T_291 = io_core_ports_0_req_bits_typ == 3'h6; // @[memory.scala 169:17:@3433.4]
  assign _T_298 = {16'h0,_T_281}; // @[Cat.scala 30:58:@3436.4]
  assign _T_299 = _T_291 ? _T_298 : async_data_dataInstr_0_data; // @[Mux.scala 61:16:@3437.4]
  assign _T_300 = _T_283 ? _T_290 : _T_299; // @[Mux.scala 61:16:@3438.4]
  assign _T_301 = _T_275 ? _T_282 : _T_300; // @[Mux.scala 61:16:@3439.4]
  assign _T_309 = io_core_ports_0_req_bits_addr[1:0]; // @[memory.scala 174:80:@3448.6]
  assign _GEN_6 = {{3'd0}, _T_309}; // @[memory.scala 174:86:@3449.6]
  assign _T_310 = _GEN_6 << 3; // @[memory.scala 174:86:@3449.6]
  assign _GEN_7 = {{31'd0}, io_core_ports_0_req_bits_data}; // @[memory.scala 174:67:@3450.6]
  assign _T_311 = _GEN_7 << _T_310; // @[memory.scala 174:67:@3450.6]
  assign _T_312 = io_core_ports_0_req_bits_addr[31:2]; // @[memory.scala 175:45:@3452.6]
  assign _T_314 = {_T_312,2'h0}; // @[Cat.scala 30:58:@3453.6]
  assign _T_318 = 4'h1 << _T_309; // @[memory.scala 176:58:@3457.6]
  assign _T_322 = 5'h3 << _T_309; // @[memory.scala 177:57:@3460.6]
  assign _T_324 = _T_275 ? _T_322 : 5'hf; // @[memory.scala 177:34:@3461.6]
  assign _T_325 = _T_267 ? {{1'd0}, _T_318} : _T_324; // @[memory.scala 176:35:@3462.6]
  assign io_core_ports_0_resp_valid = io_core_ports_0_req_valid; // @[memory.scala 155:35:@3411.4]
  assign io_core_ports_0_resp_bits_data = _T_267 ? _T_274 : _T_301; // @[memory.scala 165:40:@3441.4]
  assign io_core_ports_1_resp_bits_data = async_data_dataInstr_1_data; // @[memory.scala 183:43:@3465.4]
  assign io_debug_port_resp_valid = io_debug_port_req_valid; // @[memory.scala 189:29:@3467.4]
  assign io_debug_port_resp_bits_data = async_data_hr_data; // @[memory.scala 192:33:@3469.4]
  assign async_data_clk = clock; // @[memory.scala 152:22:@3410.4]
  assign async_data_hr_addr = io_debug_port_req_bits_addr[31:0]; // @[memory.scala 191:26:@3468.4]
  assign async_data_dw_addr = _T_314[31:0]; // @[memory.scala 175:29:@3454.6]
  assign async_data_dw_data = _T_311[31:0]; // @[memory.scala 174:29:@3451.6]
  assign async_data_dw_mask = _T_325[3:0]; // @[memory.scala 176:29:@3463.6]
  assign async_data_dw_en = io_core_ports_0_req_bits_fcn; // @[memory.scala 171:24:@3444.4]
  assign async_data_hw_addr = io_debug_port_req_bits_addr[31:0]; // @[memory.scala 196:29:@3476.6]
  assign async_data_hw_data = io_debug_port_req_bits_data; // @[memory.scala 197:29:@3477.6]
  assign async_data_hw_mask = 4'hf; // @[memory.scala 198:29:@3478.6]
  assign async_data_hw_en = io_debug_port_req_bits_fcn; // @[memory.scala 193:24:@3472.4]
  assign async_data_dataInstr_0_addr = io_core_ports_0_req_bits_addr[31:0]; // @[memory.scala 157:39:@3413.4]
  assign async_data_dataInstr_1_addr = io_core_ports_1_req_bits_addr[31:0]; // @[memory.scala 157:39:@3416.4]
endmodule
module SodorTile( // @[:@3481.2]
  input         clock, // @[:@3482.4]
  input         reset, // @[:@3483.4]
  output        io_dmi_req_ready, // @[:@3484.4]
  input         io_dmi_req_valid, // @[:@3484.4]
  input  [1:0]  io_dmi_req_bits_op, // @[:@3484.4]
  input  [6:0]  io_dmi_req_bits_addr, // @[:@3484.4]
  input  [31:0] io_dmi_req_bits_data, // @[:@3484.4]
  output        io_dmi_resp_valid, // @[:@3484.4]
  output [31:0] io_dmi_resp_bits_data // @[:@3484.4]
);
  wire  debug_clock; // @[tile.scala 25:22:@3486.4]
  wire  debug_reset; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_dmi_req_ready; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_dmi_req_valid; // @[tile.scala 25:22:@3486.4]
  wire [1:0] debug_io_dmi_req_bits_op; // @[tile.scala 25:22:@3486.4]
  wire [6:0] debug_io_dmi_req_bits_addr; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_dmi_req_bits_data; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_dmi_resp_valid; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_dmi_resp_bits_data; // @[tile.scala 25:22:@3486.4]
  wire [4:0] debug_io_ddpath_addr; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_ddpath_wdata; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_ddpath_rdata; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_debugmem_req_valid; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_debugmem_req_bits_addr; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_debugmem_req_bits_data; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_debugmem_req_bits_fcn; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_debugmem_resp_valid; // @[tile.scala 25:22:@3486.4]
  wire [31:0] debug_io_debugmem_resp_bits_data; // @[tile.scala 25:22:@3486.4]
  wire  debug_io_resetcore; // @[tile.scala 25:22:@3486.4]
  wire  core_clock; // @[tile.scala 26:23:@3489.4]
  wire  core_reset; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_imem_req_bits_addr; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_imem_resp_bits_data; // @[tile.scala 26:23:@3489.4]
  wire  core_io_dmem_req_valid; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_dmem_req_bits_addr; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_dmem_req_bits_data; // @[tile.scala 26:23:@3489.4]
  wire  core_io_dmem_req_bits_fcn; // @[tile.scala 26:23:@3489.4]
  wire [2:0] core_io_dmem_req_bits_typ; // @[tile.scala 26:23:@3489.4]
  wire  core_io_dmem_resp_valid; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_dmem_resp_bits_data; // @[tile.scala 26:23:@3489.4]
  wire [4:0] core_io_ddpath_addr; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_ddpath_wdata; // @[tile.scala 26:23:@3489.4]
  wire [31:0] core_io_ddpath_rdata; // @[tile.scala 26:23:@3489.4]
  wire  memory_clock; // @[tile.scala 28:23:@3515.4]
  wire  memory_io_core_ports_0_req_valid; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_core_ports_0_req_bits_addr; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_core_ports_0_req_bits_data; // @[tile.scala 28:23:@3515.4]
  wire  memory_io_core_ports_0_req_bits_fcn; // @[tile.scala 28:23:@3515.4]
  wire [2:0] memory_io_core_ports_0_req_bits_typ; // @[tile.scala 28:23:@3515.4]
  wire  memory_io_core_ports_0_resp_valid; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_core_ports_0_resp_bits_data; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_core_ports_1_req_bits_addr; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_core_ports_1_resp_bits_data; // @[tile.scala 28:23:@3515.4]
  wire  memory_io_debug_port_req_valid; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_debug_port_req_bits_addr; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_debug_port_req_bits_data; // @[tile.scala 28:23:@3515.4]
  wire  memory_io_debug_port_req_bits_fcn; // @[tile.scala 28:23:@3515.4]
  wire  memory_io_debug_port_resp_valid; // @[tile.scala 28:23:@3515.4]
  wire [31:0] memory_io_debug_port_resp_bits_data; // @[tile.scala 28:23:@3515.4]
  DebugModule debug ( // @[tile.scala 25:22:@3486.4]
    .clock(debug_clock),
    .reset(debug_reset),
    .io_dmi_req_ready(debug_io_dmi_req_ready),
    .io_dmi_req_valid(debug_io_dmi_req_valid),
    .io_dmi_req_bits_op(debug_io_dmi_req_bits_op),
    .io_dmi_req_bits_addr(debug_io_dmi_req_bits_addr),
    .io_dmi_req_bits_data(debug_io_dmi_req_bits_data),
    .io_dmi_resp_valid(debug_io_dmi_resp_valid),
    .io_dmi_resp_bits_data(debug_io_dmi_resp_bits_data),
    .io_ddpath_addr(debug_io_ddpath_addr),
    .io_ddpath_wdata(debug_io_ddpath_wdata),
    .io_ddpath_rdata(debug_io_ddpath_rdata),
    .io_debugmem_req_valid(debug_io_debugmem_req_valid),
    .io_debugmem_req_bits_addr(debug_io_debugmem_req_bits_addr),
    .io_debugmem_req_bits_data(debug_io_debugmem_req_bits_data),
    .io_debugmem_req_bits_fcn(debug_io_debugmem_req_bits_fcn),
    .io_debugmem_resp_valid(debug_io_debugmem_resp_valid),
    .io_debugmem_resp_bits_data(debug_io_debugmem_resp_bits_data),
    .io_resetcore(debug_io_resetcore)
  );
  Core core ( // @[tile.scala 26:23:@3489.4]
    .clock(core_clock),
    .reset(core_reset),
    .io_imem_req_bits_addr(core_io_imem_req_bits_addr),
    .io_imem_resp_bits_data(core_io_imem_resp_bits_data),
    .io_dmem_req_valid(core_io_dmem_req_valid),
    .io_dmem_req_bits_addr(core_io_dmem_req_bits_addr),
    .io_dmem_req_bits_data(core_io_dmem_req_bits_data),
    .io_dmem_req_bits_fcn(core_io_dmem_req_bits_fcn),
    .io_dmem_req_bits_typ(core_io_dmem_req_bits_typ),
    .io_dmem_resp_valid(core_io_dmem_resp_valid),
    .io_dmem_resp_bits_data(core_io_dmem_resp_bits_data),
    .io_ddpath_addr(core_io_ddpath_addr),
    .io_ddpath_wdata(core_io_ddpath_wdata),
    .io_ddpath_rdata(core_io_ddpath_rdata)
  );
  AsyncScratchPadMemory memory ( // @[tile.scala 28:23:@3515.4]
    .clock(memory_clock),
    .io_core_ports_0_req_valid(memory_io_core_ports_0_req_valid),
    .io_core_ports_0_req_bits_addr(memory_io_core_ports_0_req_bits_addr),
    .io_core_ports_0_req_bits_data(memory_io_core_ports_0_req_bits_data),
    .io_core_ports_0_req_bits_fcn(memory_io_core_ports_0_req_bits_fcn),
    .io_core_ports_0_req_bits_typ(memory_io_core_ports_0_req_bits_typ),
    .io_core_ports_0_resp_valid(memory_io_core_ports_0_resp_valid),
    .io_core_ports_0_resp_bits_data(memory_io_core_ports_0_resp_bits_data),
    .io_core_ports_1_req_bits_addr(memory_io_core_ports_1_req_bits_addr),
    .io_core_ports_1_resp_bits_data(memory_io_core_ports_1_resp_bits_data),
    .io_debug_port_req_valid(memory_io_debug_port_req_valid),
    .io_debug_port_req_bits_addr(memory_io_debug_port_req_bits_addr),
    .io_debug_port_req_bits_data(memory_io_debug_port_req_bits_data),
    .io_debug_port_req_bits_fcn(memory_io_debug_port_req_bits_fcn),
    .io_debug_port_resp_valid(memory_io_debug_port_resp_valid),
    .io_debug_port_resp_bits_data(memory_io_debug_port_resp_bits_data)
  );
  assign io_dmi_req_ready = debug_io_dmi_req_ready; // @[tile.scala 35:17:@3559.4]
  assign io_dmi_resp_valid = debug_io_dmi_resp_valid; // @[tile.scala 35:17:@3553.4]
  assign io_dmi_resp_bits_data = debug_io_dmi_resp_bits_data; // @[tile.scala 35:17:@3552.4]
  assign debug_clock = clock; // @[:@3487.4]
  assign debug_reset = reset; // @[:@3488.4]
  assign debug_io_dmi_req_valid = io_dmi_req_valid; // @[tile.scala 35:17:@3558.4]
  assign debug_io_dmi_req_bits_op = io_dmi_req_bits_op; // @[tile.scala 35:17:@3557.4]
  assign debug_io_dmi_req_bits_addr = io_dmi_req_bits_addr; // @[tile.scala 35:17:@3556.4]
  assign debug_io_dmi_req_bits_data = io_dmi_req_bits_data; // @[tile.scala 35:17:@3555.4]
  assign debug_io_ddpath_rdata = core_io_ddpath_rdata; // @[tile.scala 33:20:@3546.4]
  assign debug_io_debugmem_resp_valid = memory_io_debug_port_resp_valid; // @[tile.scala 31:22:@3535.4]
  assign debug_io_debugmem_resp_bits_data = memory_io_debug_port_resp_bits_data; // @[tile.scala 31:22:@3534.4]
  assign core_clock = clock; // @[:@3490.4]
  assign core_reset = reset; // @[:@3491.4 tile.scala 32:15:@3544.4]
  assign core_io_imem_resp_bits_data = memory_io_core_ports_1_resp_bits_data; // @[tile.scala 30:17:@3526.4]
  assign core_io_dmem_resp_valid = memory_io_core_ports_0_resp_valid; // @[tile.scala 29:17:@3519.4]
  assign core_io_dmem_resp_bits_data = memory_io_core_ports_0_resp_bits_data; // @[tile.scala 29:17:@3518.4]
  assign core_io_ddpath_addr = debug_io_ddpath_addr; // @[tile.scala 33:20:@3549.4]
  assign core_io_ddpath_wdata = debug_io_ddpath_wdata; // @[tile.scala 33:20:@3548.4]
  assign memory_clock = clock; // @[:@3516.4]
  assign memory_io_core_ports_0_req_valid = core_io_dmem_req_valid; // @[tile.scala 29:17:@3524.4]
  assign memory_io_core_ports_0_req_bits_addr = core_io_dmem_req_bits_addr; // @[tile.scala 29:17:@3523.4]
  assign memory_io_core_ports_0_req_bits_data = core_io_dmem_req_bits_data; // @[tile.scala 29:17:@3522.4]
  assign memory_io_core_ports_0_req_bits_fcn = core_io_dmem_req_bits_fcn; // @[tile.scala 29:17:@3521.4]
  assign memory_io_core_ports_0_req_bits_typ = core_io_dmem_req_bits_typ; // @[tile.scala 29:17:@3520.4]
  assign memory_io_core_ports_1_req_bits_addr = core_io_imem_req_bits_addr; // @[tile.scala 30:17:@3531.4]
  assign memory_io_debug_port_req_valid = debug_io_debugmem_req_valid; // @[tile.scala 31:22:@3540.4]
  assign memory_io_debug_port_req_bits_addr = debug_io_debugmem_req_bits_addr; // @[tile.scala 31:22:@3539.4]
  assign memory_io_debug_port_req_bits_data = debug_io_debugmem_req_bits_data; // @[tile.scala 31:22:@3538.4]
  assign memory_io_debug_port_req_bits_fcn = debug_io_debugmem_req_bits_fcn; // @[tile.scala 31:22:@3537.4]
endmodule
module Top( // @[:@3570.2]
  input   clock, // @[:@3571.4]
  input   reset, // @[:@3572.4]
  output  io_success // @[:@3573.4]
);
  wire  tile_clock; // @[top.scala 20:21:@3575.4]
  wire  tile_reset; // @[top.scala 20:21:@3575.4]
  wire  tile_io_dmi_req_ready; // @[top.scala 20:21:@3575.4]
  wire  tile_io_dmi_req_valid; // @[top.scala 20:21:@3575.4]
  wire [1:0] tile_io_dmi_req_bits_op; // @[top.scala 20:21:@3575.4]
  wire [6:0] tile_io_dmi_req_bits_addr; // @[top.scala 20:21:@3575.4]
  wire [31:0] tile_io_dmi_req_bits_data; // @[top.scala 20:21:@3575.4]
  wire  tile_io_dmi_resp_valid; // @[top.scala 20:21:@3575.4]
  wire [31:0] tile_io_dmi_resp_bits_data; // @[top.scala 20:21:@3575.4]
  wire [31:0] SimDTM_exit; // @[top.scala 21:20:@3578.4]
  wire  SimDTM_debug_req_ready; // @[top.scala 21:20:@3578.4]
  wire  SimDTM_debug_req_valid; // @[top.scala 21:20:@3578.4]
  wire [1:0] SimDTM_debug_req_bits_op; // @[top.scala 21:20:@3578.4]
  wire [6:0] SimDTM_debug_req_bits_addr; // @[top.scala 21:20:@3578.4]
  wire [31:0] SimDTM_debug_req_bits_data; // @[top.scala 21:20:@3578.4]
  wire  SimDTM_debug_resp_ready; // @[top.scala 21:20:@3578.4]
  wire  SimDTM_debug_resp_valid; // @[top.scala 21:20:@3578.4]
  wire [31:0] SimDTM_debug_resp_bits_data; // @[top.scala 21:20:@3578.4]
  wire [1:0] SimDTM_debug_resp_bits_resp; // @[top.scala 21:20:@3578.4]
  wire  SimDTM_reset; // @[top.scala 21:20:@3578.4]
  wire  SimDTM_clk; // @[top.scala 21:20:@3578.4]
  wire  _T_11; // @[debug.scala 79:19:@3597.4]
  wire [31:0] _T_13; // @[debug.scala 80:59:@3599.6]
  wire  _T_16; // @[debug.scala 80:13:@3601.6]
  SodorTile tile ( // @[top.scala 20:21:@3575.4]
    .clock(tile_clock),
    .reset(tile_reset),
    .io_dmi_req_ready(tile_io_dmi_req_ready),
    .io_dmi_req_valid(tile_io_dmi_req_valid),
    .io_dmi_req_bits_op(tile_io_dmi_req_bits_op),
    .io_dmi_req_bits_addr(tile_io_dmi_req_bits_addr),
    .io_dmi_req_bits_data(tile_io_dmi_req_bits_data),
    .io_dmi_resp_valid(tile_io_dmi_resp_valid),
    .io_dmi_resp_bits_data(tile_io_dmi_resp_bits_data)
  );
  SimDTM SimDTM ( // @[top.scala 21:20:@3578.4]
    .exit(SimDTM_exit),
    .debug_req_ready(SimDTM_debug_req_ready),
    .debug_req_valid(SimDTM_debug_req_valid),
    .debug_req_bits_op(SimDTM_debug_req_bits_op),
    .debug_req_bits_addr(SimDTM_debug_req_bits_addr),
    .debug_req_bits_data(SimDTM_debug_req_bits_data),
    .debug_resp_ready(SimDTM_debug_resp_ready),
    .debug_resp_valid(SimDTM_debug_resp_valid),
    .debug_resp_bits_data(SimDTM_debug_resp_bits_data),
    .debug_resp_bits_resp(SimDTM_debug_resp_bits_resp),
    .reset(SimDTM_reset),
    .clk(SimDTM_clk)
  );
  assign _T_11 = SimDTM_exit >= 32'h2; // @[debug.scala 79:19:@3597.4]
  assign _T_13 = SimDTM_exit >> 1'h1; // @[debug.scala 80:59:@3599.6]
  assign _T_16 = reset == 1'h0; // @[debug.scala 80:13:@3601.6]
  assign io_success = SimDTM_exit == 32'h1; // @[debug.scala 78:15:@3596.4]
  assign tile_clock = clock; // @[:@3576.4]
  assign tile_reset = reset; // @[:@3577.4]
  assign tile_io_dmi_req_valid = SimDTM_debug_req_valid; // @[debug.scala 76:11:@3593.4]
  assign tile_io_dmi_req_bits_op = SimDTM_debug_req_bits_op; // @[debug.scala 76:11:@3592.4]
  assign tile_io_dmi_req_bits_addr = SimDTM_debug_req_bits_addr; // @[debug.scala 76:11:@3591.4]
  assign tile_io_dmi_req_bits_data = SimDTM_debug_req_bits_data; // @[debug.scala 76:11:@3590.4]
  assign SimDTM_debug_req_ready = tile_io_dmi_req_ready; // @[debug.scala 76:11:@3594.4]
  assign SimDTM_debug_resp_valid = tile_io_dmi_resp_valid; // @[debug.scala 76:11:@3588.4]
  assign SimDTM_debug_resp_bits_data = tile_io_dmi_resp_bits_data; // @[debug.scala 76:11:@3587.4]
  assign SimDTM_debug_resp_bits_resp = 2'h0; // @[debug.scala 76:11:@3586.4]
  assign SimDTM_reset = reset; // @[debug.scala 75:14:@3585.4]
  assign SimDTM_clk = clock; // @[debug.scala 74:12:@3584.4]
  always @(posedge clock) begin
    `ifndef SYNTHESIS
    `ifdef PRINTF_COND
      if (`PRINTF_COND) begin
    `endif
        if (_T_11 & _T_16) begin
          $fwrite(32'h80000002,"*** FAILED *** (exit code = %d)\n",_T_13); // @[debug.scala 80:13:@3603.8]
        end
    `ifdef PRINTF_COND
      end
    `endif
    `endif // SYNTHESIS
  end
endmodule
