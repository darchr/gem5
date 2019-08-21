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

//gem5 model includes
#include "verilator_driver.hh"

//setup design params
VerilatorDriver::VerilatorDriver( )
{
}

void
VerilatorDriver::clockDevice(unsigned int numSigs, ...)
{
  va_list ap;

  va_start(ap, numSigs);

  //run the device under test here through verilator
  //when clock = 0 device state is set
  for ( int j = 0; j < numSigs; ++j){
    unsigned char * sig = va_arg(ap, unsigned char *);
    *sig = 0;
  }

  top.eval();
  //when clock = 1 device state is evaluated
  for ( int j = 0; j < numSigs; ++j){
    unsigned char * sig = va_arg(ap, unsigned char *);
    *sig = 1;
  }
  top.eval();

  cyclesPassed += 1;

  va_end(ap);
}

VNV_nvdla *
VerilatorDriver::getTopLevel()
{
  return &top;
}

void
VerilatorDriver::reset(int resetCycles, char * fmt, unsigned int numSigs, ...)
{
  //we pass in a variable number of signals needed to reset
  //the device
  va_list ap;

  va_start(ap, numSigs);
  //if we are pipelining we want to run reset for the number
  //of stages we have
  for (int i = 0; i < resetCycles; ++i){

    //assert reset signals and starting clock signal
    for ( int j = 0; j < numSigs; ++j){
      switch(fmt[j]){
        case 0: {
          unsigned char * sig = va_arg(ap, unsigned char *);
          *sig = 0;
          break;
        }
        case 1: {
          unsigned short * sig = va_arg(ap, unsigned short *);
          *sig = 0;
          break;
        }
        case 2: {
          unsigned int * sig = va_arg(ap, unsigned int *);
          *sig = 0;
          break;
        }
        case 3: {
          unsigned long * sig = va_arg(ap, unsigned long *);
          *sig = 0;
          break;
        }
        default:
          break;
      }
    }
    top.eval();
    //run verilator for this state

    //run verilator for rising edge state
    //deassert reset signals and starting clock signal
    for ( int j = 0; j < numSigs; ++j){
       switch(fmt[j]){
        case 0: {
          unsigned char * sig = va_arg(ap, unsigned char *);
          *sig = 1;
          break;
        }
        case 1: {
          unsigned short * sig = va_arg(ap, unsigned short *);
          *sig = 1;
          break;
        }
        case 2: {
          unsigned int * sig = va_arg(ap, unsigned int *);
          *sig = 1;
          break;
        }
        case 3: {
          unsigned long * sig = va_arg(ap, unsigned long *);
          *sig = 1;
          break;
        }
        default:
          break;
      }
    }
    top.eval();
  }
  va_end(ap);
}

bool
VerilatorDriver::isFinished(){
  return Verilated::gotFinish();
}

unsigned int
VerilatorDriver::getCyclesPassed(){
  return cyclesPassed;
}
