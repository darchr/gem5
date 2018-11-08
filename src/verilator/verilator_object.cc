#include "base/logging.hh"
#include "sim/sim_exit.hh"
#include "verilator_object.hh"

VerilatorObject::VerilatorObject(VerilatorObjectParams *params) :
    SimObject(params),
    event([this]{processEvent();}, params->name),
    maxCycles(params->cycles),
    latency(params->latency),
   designStages(params->stages),
   start(params->startTime),
    loadMem(params->memData.c_str()),
   objName(params->name)
{
    //send program or other data to fesvr to communicate
    //with design
    toDtm.push_back(loadMem);
    if (!toDtm.size()){
        //DPRINT
        fatal("No binary specified for emulator\n");
    }

    inform("Instantiated DTM.\n");
    //give fesvr list of things to load/parse
    dtm = new dtm_t(toDtm);
    if (!dtm){
        panic("Could not allocate dtm\n");
    }
}

VerilatorObject::~VerilatorObject()
{
    delete dtm;
}

VerilatorObject*
VerilatorObjectParams::create()
{
    VerilatorObject * vo = new VerilatorObject(this);
    return vo;
}

void
VerilatorObject::processEvent()
{
    //has verilator finished running?
    if (dtm->done() && dut.io_success && Verilated::gotFinish()){
        inform("Simulation has Completed\n");
        exitSimLoop("Done Simulating", dtm->exit_code());
    }

    //run the device under test here through verilator
    dut.clock = 0;
    dut.eval();
    dut.clock = 1;
    dut.eval();

    //check if we should stop
    cyclesPassed += 1;
    if (maxCycles != 0 && cyclesPassed == maxCycles){
        inform("Simulation Timed Out\n");
        exitSimLoop("Done Simulating", dtm->exit_code());
    }

    schedule(event, curTick() + latency);
}

void
VerilatorObject::reset(int cycles)
{
    //if we are pipelining we want to run reset for the number
    //of stages we have
    for (int i = 0; i < cycles; ++i){
        //set reset signal and starting clock signal
        dut.reset = 1;
        dut.clock = 0;
        //run verilator for this state
        dut.eval();
        //run verilator for rising edge state
        dut.clock = 1;
        dut.eval();
        //done reseting
        dut.reset = 0;
    }
}

void
VerilatorObject::startup()
{
    reset(designStages);
    schedule(event, latency);
}
