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
#include "verilator_dino_cpu.hh"

//setup design params
VerilatorDinoCPU::VerilatorDinoCPU(VerilatorDinoCPUParams *params) :
  DrivenObject(params),
  event([this]{updateCycle();}, params->name),
  system(params->dinocpu_sys)
{
  top = driver.getTopLevel();
}

//creates object for gem5 to use
VerilatorDinoCPU*
VerilatorDinoCPUParams::create()
{
  //verilator has weird alignment issue for generated code
  void* ptr = aligned_alloc(128, sizeof(VerilatorDinoCPU));
  return new(ptr) VerilatorDinoCPU(this);
}

void
VerilatorDinoCPU::updateCycle()
{

  DPRINTF(Verilator, "\n\nCLOCKING DEVICE\n");

  driver.clockDevice(1, &(top->clock));

  //schedule another instruction fetch if verilator is not done
  if (!driver.isFinished()){
    schedule(event, nextCycle());
  }
}

void
VerilatorDinoCPU::reset(int resetCycles)
{
  DPRINTF(Verilator, "RESETING FOR %d CYCLES\n", resetCycles);

  char fmt[1] = {0};

  //reset
  top->reset = 1;
  driver.reset(resetCycles, fmt, 1, &(top->clock));

  //done reseting
  top->reset = 0;

  DPRINTF(Verilator, "DONE RESETING\n");
}

void
VerilatorDinoCPU::startup()
{
  DPRINTF(Verilator, "STARTING UP DINOCPU\n");
  reset(resetCycles);

  //lets fetch an instruction before doing anything
  DPRINTF(Verilator, "SCHEDULING FIRST TICK \n");
  schedule(event, nextCycle());
}

