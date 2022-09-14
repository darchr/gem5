#ifndef __CPU_PROBES_LOOPPOINTMANAGER_HH__
#define __CPU_PROBES_LOOPPOINTMANAGER_HH__

#include "cpu/base.hh"
#include "params/LoopPointManager.hh"
#include "sim/probe/probe.hh"

namespace gem5
{

class LoopPointManager : public SimObject
{
    public:
        LoopPointManager(const LoopPointManagerParams &params);
        virtual ~LoopPointManager();
        virtual void init();
        bool check_count();

    private:
        const int targetCount;
        int counter;
};

}

#endif // __CPU_PROBES_LOOPPOINTMANAGER_HH__
