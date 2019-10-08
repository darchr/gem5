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

//verilator design includes

//general includes
#include <cstdarg>

#define VDevice VTop

class VDevice;
//Wrapper for verilator generated code. Clocks the device
class VerilatorDriver{
  
  private:
    //count how many cycles we have run
    unsigned int cyclesPassed;

    //Our verilator design
    VDevice * top;
  public:
    VerilatorDriver();

/* clocks the device
    * PARAMS: number of clocks to set,
    * signal pointer list
    * DESCRIPTION: Because verilator top level designs can drastically vary
    * this function is designed to generalize the clocking process. This
    * currently assumes a clock is a vluint_8 type. It will count the number
    * of cycles passed.
    * */
    void clockDevice(unsigned int numSigs, ...);

    VDevice * getTopLevel();

    //is the verilated device finished with execution?
    bool isFinished();

    //get the number of elapsed cycles
    unsigned int getCyclesPassed();

    /* reset the device for a # of cycles
    * PARAMS: number of cycles to reset,
    * specifies signal types to interpret
    * (unsigned char 0, unsigned short 1, unsigned int 2, unsigned long 3),
    * number of signal pointers passed,
    * signal pointer list
    * DESCRIPTION: Because verilator top level designs can drastically vary
    * this function is designed to generalize the reset process. You must
    * specify how many cycles to reset, the type of signals you are passing,
    * the number of signals you are passing and then the signals
    * themselves in the order specified by the input format
    * */
    void reset(int resetCycles, char * fmt, unsigned int numSigs, ...);
};
