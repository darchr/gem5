#include "cpu/probes/looppointmanager.hh"

namespace gem5
{
LoopPointManager::LoopPointManager(const LoopPointManagerParams &p)
    : SimObject(p)
{
    for (int i = 0; i< p.target_pc.size(); i++)
    {
        auto map_itr = targetCount.find(p.target_pc[i]);
        if (map_itr == targetCount.end()){
            std::vector<int> pcCount = {p.target_count[i]};
            targetCount.insert(std::make_pair(p.target_pc[i], pcCount));
            counter.insert(std::make_pair(p.target_pc[i],0));
        } else {
            std::vector<int>& pcCount = map_itr->second;
            pcCount.push_back(p.target_count[i]);
        }
    }
    info = simout.create("LoopPointInfo.txt", false);
    if (!info)
        fatal("unable to open LoopPoint info txt");
    
}

LoopPointManager::~LoopPointManager()
{
    simout.close(info);
}

void
LoopPointManager::init()
{}

bool
LoopPointManager::check_count(uint64_t pc)
{
    auto count_itr = counter.find(pc);
    int& count = count_itr-> second;
    count += 1;
    auto target_itr = targetCount.find(pc);
    std::vector<int>& targetcount = target_itr->second;
    for (std::vector<int>::iterator iter = targetcount.begin(); iter < targetcount.end(); iter++)
    {
        if(*iter==count)
        {
            targetcount.erase(iter);
            *info->stream() << curTick() << " : " << pc << " : " << count << " \n "; 
            return true;
        }
    }
    
    return false;
}


}
