#ifndef __CPU_PROBES_LOOPPOINTMANAGER_HH__
#define __CPU_PROBES_LOOPPOINTMANAGER_HH__

#include <unordered_map>

#include "cpu/base.hh"
#include "params/LoopPointManager.hh"
#include "sim/probe/probe.hh"
#include "base/output.hh"

namespace gem5
{

class LoopPointManager : public SimObject
{
    public:
        LoopPointManager(const LoopPointManagerParams &params);
        virtual ~LoopPointManager();
        virtual void init();
        bool check_count(uint64_t);

    private:
        // const std::vector<int> targetCount;
        // std::vector<int> counter;
        OutputStream *info;
        std::unordered_map<uint64_t, std::vector<int>> targetCount;
        std::unordered_map<uint64_t,int> counter;
};

}

#endif // __CPU_PROBES_LOOPPOINTMANAGER_HH__
