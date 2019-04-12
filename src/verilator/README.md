BASIC VERILATOR WRAPPER
This is a wrapper for verilator generated c++ code. The intent of this is to allow you to write verilog code and combine it with gem5 models for a robust and ACCURATE simulation of your hardware designs.

USAGE
I) Generating Necessary Files
  1. Write a verilog design. It is suggested to have your top level file named as Top.v
  2. Have some sort of black box that you want gem5 to model which exposes necessary wires
    (i.e. verilog cpu design, but use gem5's ddr model, need address, readoutput, etc)
  3. Install verilator. It is reccomended to use a package that comes with your linux distro
  4. Put all .v and/or /sv files in gem5/ext/verilator/
  5. Type the make command in that directory
  6. You should have a set of new files now.

II) Using The Generated Files
TODO AFTER WRAPER DESIGN IS GENERALIZED
