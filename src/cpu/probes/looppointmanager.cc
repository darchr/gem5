#include "cpu/probes/looppointmanager.hh"

namespace gem5
{
LoopPointManager::LoopPointManager(const LoopPointManagerParams &p)
    : SimObject(p),
    targetCount(p.target_count)
{
    counter = std::vector<int>{};
}

LoopPointManager::~LoopPointManager()
{}

void
LoopPointManager::init()
{}

bool
LoopPointManager::check_count(int cpuID)
{
    while ((cpuID+1) > counter.size())
        counter.push_back(0);
    counter[cpuID] += 1;
    int sum  = 0;
    for (int i =0; i<counter.size(); i++)
    {
        sum += counter[i];
    }
    if (sum >= targetCount[0])
    {
        for (int i =0; i<counter.size(); i++)
        {
            printf("counter[%i]: %i \n", i,counter[i]);
        }
        return true;
    }
    else
        return false;
}


}
