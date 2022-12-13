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

#include "cpu/probes/pc_count_tracker_manager.hh"

namespace gem5
{
    PcCountTrackerManager::PcCountTrackerManager(
        const PcCountTrackerManagerParams &p) 
        : SimObject(p)
        {
            currentPair = PcCountPair(0,0);
            ifListNotEmpty = true;
            lastTick = 0;

            for (int i = 0 ; i < p.targets.size() ; i++) {
                counter.insert(std::make_pair(p.targets[i].getPC(),0));
                targetPair.insert(p.targets[i]);
            }
        }
    
    void 
    PcCountTrackerManager::check_count(Addr pc) {

        int count = ++counter.find(pc)->second;

        if(ifListNotEmpty) {

            if(targetPair.empty()) {
                if(curTick() > lastTick) {
                    DPRINTF(PcCountTracker,
                    "all targets are encountered. Last Tick:%i\n", lastTick);
                    exitSimLoop("reached the end of the PcCountPair list");
                    ifListNotEmpty = false;
                }
            }
            
            currentPair = PcCountPair(pc,count);
            if(targetPair.find(currentPair) != targetPair.end()) {

                DPRINTF(PcCountTracker,
                    "pc:%s encountered\n", 
                            currentPair.to_string());

                lastTick = curTick();
                exitSimLoopNow("simpoint starting point found");

                targetPair.erase(currentPair);
                DPRINTF(PcCountTracker,
                    "There are %i targets remained\n", targetPair.size());
            }
        }
    }

}