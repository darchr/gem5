
#ifndef __VERILATOR_VERILATOR_OBJECT_HH__
#define __VERILATOR_VERILATOR_OBJECT_HH__

#define VM_TRACE 0
#define VL_THREADED 0

#include "VTop.h"
#include "VerilatorMemBlackBox.hh"
#include "params/VerilatorDinoCPU.hh"
#include "sim/sim_object.hh"

class VerilatorDinoCPU
{
    private:
        void updateCycle();

        VTop top;
        VerilatorMemBlackBox verilatorMem;
        EventFunctionWrapper event;
        Tick latency;
        int designStages;
        int cyclesPassed;

    public:
        VerilatorDinoCPUt(VerilatorDinoCPUParams *p);
        ~VerilatorDinoCPU();
        void reset(int resetCycles);
        void startup();
};
#endif
