#ifndef __CPU_PROBES_LOOPPOINT_HH__
#define __CPU_PROBES_LOOPPOINT_HH__

#include "cpu/base.hh"
#include "params/LoopPoint.hh"
#include "sim/probe/probe.hh"

namespace gem5
{

class LoopPoint : public ProbeListenerObject
{
    public:
        LoopPoint(const LoopPointParams &params);
        virtual ~LoopPoint();
        virtual void init();
        virtual void regProbeListeners();
        void check_pc(const Addr&);

    private:
        const Addr targetPC;
        const int targetCount;
        BaseCPU *cpuptr;
        int counter;

};

}

#endif // __CPU_PROBES_LOOPPOINT_HH__
