#ifndef __CPU_PROBES_LOOPPOINT_HH__
#define __CPU_PROBES_LOOPPOINT_HH__

#include "cpu/base.hh"
#include "cpu/probes/looppointmanager.hh"
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
        const std::vector<Addr> targetPC;
        BaseCPU *cpuptr;
        LoopPointManager *manager;

};

}

#endif // __CPU_PROBES_LOOPPOINT_HH__
