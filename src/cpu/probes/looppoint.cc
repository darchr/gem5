#include "cpu/probes/looppoint.hh"

namespace gem5
{

LoopPoint::LoopPoint(const LoopPointParams &p)
    : ProbeListenerObject(p),
    targetPC(p.target_pc),
    cpuptr(p.core),
    lpmanager(p.LPmanager)
{
}

LoopPoint::~LoopPoint()
{}

void
LoopPoint::init()
{}

void
LoopPoint::regProbeListeners()
{
    typedef ProbeListenerArg<LoopPoint, Addr> LoopPointListener;
    listeners.push_back(new LoopPointListener(this, "RetiredInstsPC",
                                             &LoopPoint::check_pc));
}

void
LoopPoint::check_pc(const Addr& pc)
{
    if (pc==targetPC)
    {
        if (lpmanager->check_count())
            cpuptr->scheduleInstStop(0,1,"simpoint starting point found");
    }
}

}
