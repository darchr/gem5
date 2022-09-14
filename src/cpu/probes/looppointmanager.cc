#include "cpu/probes/looppointmanager.hh"

namespace gem5
{
LoopPointManager::LoopPointManager(const LoopPointManagerParams &p)
    : SimObject(p),
    targetCount(p.target_count),
    counter(0)
{
}

LoopPointManager::~LoopPointManager()
{}

void
LoopPointManager::init()
{}

bool
LoopPointManager::check_count()
{
    counter += 1;
    if (counter >= targetCount)
        return true;
    else
        return false;
}


}
