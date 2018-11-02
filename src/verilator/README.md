Basic Verilator Wrapper
* ITop is intended as a wrapper for the top level module of your design
* IMem is intended as a wrapper for a blackbox verilog memory module
	-You should use IMem to forward all memory requests to gem5 rather than the c++ model
* Verilator Object is a basic implementation of ITop for the riscv sodor 1 stage core
* IMem is currently a work in progrress

Building
* First you must generate your verilator c++ models, and include them as needed
	-add flags to the SConscript to modify the compiler flags and include wha tyou need (.i.e. env.Append(CCFLAGS='-I ./<directory>'))
* Second you must run scons build/X86/gem5.opt

Using Verilator With Gem5
* You will run the application just as with any other gem5 code
* Inorder to interface your verilog/verilator design with gem5 you must first build your verilog design
and then set the output files to the include folder
* Inorder to build verilator support into gem5 you must have verilator installed and add the library path to your
SConscript
* Verilator will expose top level signals to C/C++ code in VTop.h
* You can expose more signals by adding /*verialtor public*/ next to any wire, reg, input, and output in the verilog code
* Only generated files are included in this repo because of licensing

Notes
* If you are having issues including verilator header files make sure that your enviornment variables are set correctly
* If after building the verilator wrapper into Gem5 you get linker errors make sure that you have the ncessary object files
  included or that you are building with the generated files from verilator
