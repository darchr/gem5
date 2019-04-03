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

#include "VerilatorDinoCPU.hh"
#include "base/logging.hh"
#include "debug/Verilator.hh"
#include "obj_dir/VTop_DualPortedMemory.h"
#include "obj_dir/VTop_DualPortedMemoryBlackBox.h"
#include "obj_dir/VTop_Top.h"
#include "sim/sim_exit.hh"

VerilatorDinoCPU::VerilatorDinoCPU(VerilatorDinoCPUParams *params) :
    ClockedObject(params),
    verilatorMem(params->verilatorMem),
    event([this]{updateCycle();}, params->name),
    clkperiod(params->clkperiod),
    designStages(params->stages)
{
    verilatorMem->blkbox = top.Top->mem->memory;
}

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

    DPRINTF(Verilator, "\nTICKING\n");
    //has verilator finished running?
    if (Verilated::gotFinish()){
        inform("Simulation has Completed\n");
        exitSimLoop("Done Simulating", 1) ;
    }

    DPRINTF(Verilator, "INSTRUCTION FETCH?\n");

    verilatorMem->doFetch();
    verilatorMem->doMem();

    //run the device under test here through verilator
    top.clock = 0;
    top.eval();
    top.clock = 1;
    top.eval();

    cyclesPassed += 1;

    schedule(event, nextCycle());
}


void
VerilatorDinoCPU::reset(int resetCycles)
{
    DPRINTF(Verilator, "RESETING FOR %d CYCLES\n", resetCycles);

    //if we are pipelining we want to run reset for the number
    //of stages we have
    for (int i = 0; i < resetCycles; ++i){
        //set reset signal and starting clock signal
        top.reset = 1;
        top.clock = 0;
        //run verilator for this state
        top.eval();
        //run verilator for rising edge state
        top.clock = 1;
        top.eval();
        //done reseting
        top.reset = 0;
    }
    DPRINTF(Verilator, "DONE RESETING\n");
}

void
VerilatorDinoCPU::startup()
{
    DPRINTF(Verilator, "STARTING UP DINOCPU\n");
    reset(designStages);

    DPRINTF(Verilator, "SCHEDULING FIRST TICK IN %d\n", clkperiod);
    schedule(event, clkperiod);
}

