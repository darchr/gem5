#include "cpu/probes/looppointmanager.hh"

namespace gem5
{
LoopPointManager::LoopPointManager(const LoopPointManagerParams &p)
    : SimObject(p),
    targetCount(p.target_count)
{
    counter = std::vector<int>(targetCount.size(),0);
}

LoopPointManager::~LoopPointManager()
{}

void
LoopPointManager::init()
{}

bool
LoopPointManager::check_count(int index)
{
    counter[index] += 1;

    if (counter[index] == targetCount[index])
        return true;
    else
        return false;
}


}
