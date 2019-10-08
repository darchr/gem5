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
  cyclesPassed = 0;
  void* ptr = aligned_alloc(128, sizeof(VDevice));
  top = new(ptr) VDevice();
}

void
VerilatorDriver::clockDevice(unsigned int numSigs, ...)
{
  va_list ap0;
  va_list ap1;

  va_start(ap0, numSigs);
  va_start(ap1, numSigs);

  //run the device under test here through verilator
  //when clock = 0 device state is set
  for ( int j = 0; j < numSigs; ++j){
    unsigned char * sig = va_arg(ap0, unsigned char *);
    *sig = 1;
  }

  top->eval();

  cyclesPassed += 1;

  //when clock = 1 device state is evaluated
  for ( int j = 0; j < numSigs; ++j){
    unsigned char * sig = va_arg(ap1, unsigned char *);
    *sig = 0;
  }
  top->eval();

  cyclesPassed += 1;

  va_end(ap0);
  va_end(ap1);
}

VDevice *
VerilatorDriver::getTopLevel()
{
  return top;
}

void
VerilatorDriver::reset(int resetCycles, char * fmt, unsigned int numSigs, ...)
{

  //if we are pipelining we want to run reset for the number
  //of stages we have
  for (int i = 0; i < resetCycles; ++i){
    //we pass in a variable number of signals needed to reset
    //the device
    va_list ap0;
    va_list ap1;

    va_start(ap0, numSigs);
    va_start(ap1, numSigs);
    //assert reset signals and starting clock signal
    for ( int j = 0; j < numSigs; ++j){
      switch(fmt[j]){
        case 0: {
          unsigned char * sig = va_arg(ap0, unsigned char *);
          *sig = 1;
          break;
        }
        case 1: {
          unsigned short * sig = va_arg(ap0, unsigned short *);
          *sig = 1;
          break;
        }
        case 2: {
          unsigned int * sig = va_arg(ap0, unsigned int *);
          *sig = 1;
          break;
        }
        case 3: {
          unsigned long * sig = va_arg(ap0, unsigned long *);
          *sig = 1;
          break;
        }
        default:
          break;
      }
    }
    top->eval();

    cyclesPassed += 1;

    //run verilator for this state

    //run verilator for rising edge state
    //deassert reset signals and starting clock signal
    for ( int j = 0; j < numSigs; ++j){
       switch(fmt[j]){
        case 0: {
          unsigned char * sig = va_arg(ap1, unsigned char *);
          *sig = 0;
          break;
        }
        case 1: {
          unsigned short * sig = va_arg(ap1, unsigned short *);
          *sig = 0;
          break;
        }
        case 2: {
          unsigned int * sig = va_arg(ap1, unsigned int *);
          *sig = 0;
          break;
        }
        case 3: {
          unsigned long * sig = va_arg(ap1, unsigned long *);
          *sig = 0;
          break;
        }
        default:
          break;
      }
    }
    top->eval();

    cyclesPassed += 1;

    va_end(ap0);
    va_end(ap1);
  }
}

bool
VerilatorDriver::isFinished(){
  return Verilated::gotFinish();
}

unsigned int
VerilatorDriver::getCyclesPassed(){
  return cyclesPassed;
}
