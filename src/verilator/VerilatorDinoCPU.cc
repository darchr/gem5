#include "VerilatorDinoCPU.hh"
#include "base/logging.hh"
#include "sim/sim_exit.hh"

VerilatorDinoCPU::VerilatorDinoCPU(VerilatorDinoCPUParams *params) :
    SimObject(params),
    event([this]{updateCycle();}, params -> name),
    latency(params -> latency),
    designStages(params -> stages)
{
}

VerilatorDinoCPU*
VerilatorODinoCPUParams::create()
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
        exitSimLoop("Done Simulating", 1 /);
    }

    DPRINTF(Verilator, "INSTRUCTION FETCH?\n");

    verilatorMem.doFetch();
    verilatorMem.doMem();

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
    DPRINTF(Verilator, "STARTING UP SODOR\n");
    reset(designStages);

    DPRINTF(Verilator, "SCHEDULING FIRST TICK IN %d\n", latency);
    schedule(event, latency);
}

