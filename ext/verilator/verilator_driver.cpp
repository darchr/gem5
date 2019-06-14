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

//gem5 includes
#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "sim/sim_exit.hh"

//gem5 model includes
#include "verilator_driver.hh"

//setup design params
VerilatorDriver::VerilatorDriver( ) :
  event([this]{clockDevice();}),
{
}

void
VerilatorDinoCPU::clockDevice()
{

  DPRINTF(Verilator, "\n\nCLOCKING DEVICE\n");

  //run the device under test here through verilator
  //when clock = 0 device state is set
  top.clock = 0;
  top.eval();
  //when clock = 1 device state is evaluated
  top.clock = 1;
  top.eval();

  cyclesPassed += 1;

  //schedule another instruction fetch if verilator is not done
  if (!Verilated::gotFinish()){
    schedule(event, nextCycle());
  }
}

unsigned int
VerilatorDriver::getClockState()
{
  return top.clock;
}

void
VerilatorDriver::reset(int resetCycles)
{
  DPRINTF(Verilator, "RESETING FOR %d CYCLES\n", resetCycles);

  //if we are pipelining we want to run reset for the number
  //of stages we have
  top.reset = 1;
  for (int i = 0; i < resetCycles; ++i){
    //set reset signal and starting clock signal
    top.clock = 0;
    top.eval();
    //run verilator for this state
    //run verilator for rising edge state
    top.clock = 1;
    top.eval();
  }

  //done reseting
  top.reset = 0;

  DPRINTF(Verilator, "DONE RESETING\n");
}

void
VerilatorDriver::startup()
{
  DPRINTF(Verilator, "STARTING UP DINOCPU\n");
  reset(designStages);

  //lets fetch an instruction before doing anything
  DPRINTF(Verilator, "SCHEDULING FIRST TICK \n");
  schedule(event, nextCycle());
}

