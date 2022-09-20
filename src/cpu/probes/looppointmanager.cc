/*
 * Copyright (c) 2022 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

void
LoopPointManager::check_count(Addr pc)
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
            *info->stream() << curTick() << " : " << pc << " : " << count;
            *info->stream() << " \n "; 
            exitSimLoopNow("simpoint starting point found");
        }
    }
}


}
