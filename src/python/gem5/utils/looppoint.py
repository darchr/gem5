# Copyright (c) 2022 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


from m5.util import fatal
from m5.params import PcCountPair
from pathlib import Path
from typing import List, Dict
from gem5.components.processors.abstract_processor import AbstractProcessor
from m5.objects import PcCountTrackerManager
import csv
import re
import json

class LoopPoint:
    
    def __init__(
        self,
        targets: List[PcCountPair],
        regions: Dict[PcCountPair,int],
        json_file: Dict[int, Dict]
    ) -> None:
        
        self._manager = PcCountTrackerManager()
        self._manager.targets = targets
        self._targets = targets
        self._regions = regions
        self._json_file = json_file
        
    def setup_processor(
        self,
        processor: AbstractProcessor,
    ) -> None:
        for core in processor.get_cores():
            core.add_pc_tracker_probe(self._targets, self._manager)
            
    def update_relatives(self) -> None:
        current_pair = self._manager.get_current_pc_count_pair()
        temp_pair = PcCountPair(current_pair.getPC(), current_pair.getCount())
        if(temp_pair in self._regions):
            rid = self._regions[temp_pair]
            if("warmup" in region):
                region = self._json_file[rid]["simulation"]
                start = region["start"]["pc"]
                temp = region["start"]["global"] - self._manager.get_pc_count(start)
                self._json_file[rid]["simulation"]["start"]["relative"] = int(temp)
                end = region["end"]["pc"]
                temp = region["end"]["global"] - self._manager.get_pc_count(end)
                self._json_file[rid]["simulation"]["end"]["relative"] = int(temp)
                
                
    def output_json_file(
        self,
        input_indent: int = 4
        ) -> Dict[int, Dict]:
        with open("LoopPoint.json", "w") as file:
            json.dump(self._json_file, file, indent=input_indent)
    
    def get_current_region(self) -> int:
        current_pair = self._manager.get_current_pc_count_pair()
        temp_pair = PcCountPair(current_pair.getPC(), current_pair.getCount())
        if(temp_pair in self._regions):
            return self._regions[temp_pair]
        return -1
    
    def get_current_pair(self) -> PcCountPair:
        return self._manager.get_current_pc_count_pair()
    
    def get_regions(self) -> Dict[PcCountPair,int]:
        return self._regions
    
    def get_targets(self) ->  List[PcCountPair]:
        return self._targets
                

class LoopPointCheckpoint(LoopPoint):
    def __init__(
        self, 
        LoopPointFilePath: Path,
        if_csv: bool = True
        ) -> None:
        
        _json_file = {}
        _targets = []    
        _region_id = {}
         
        if(if_csv):
            self.profile_csv(LoopPointFilePath, _targets, _json_file, _region_id)
            
        super().__init__(
            _targets,
            _region_id,
            _json_file,
        )
            
    def profile_csv(
        self,
        looppoint_file_path: Path,
        targets: List[PcCountPair],
        json_file: Dict[int, Dict],
        region_id: Dict[PcCountPair, int],
    ) -> None:
        
        with open(looppoint_file_path, newline="") as csvfile:
            spamreader = csv.reader(csvfile, delimiter=" ", quotechar="|")
            for row in spamreader:
                if len(row) > 1:
                    if row[0] == "cluster":
                        line = row[4].split(",")
                        start = PcCountPair(int(line[3],16),int(line[6]))
                        end = PcCountPair(int(line[7],16),int(line[10]))
                        if(int(line[2]) in json_file):
                            json_file[int(line[2])]["simulation"]={"start": {"pc": int(line[3],16)}}
                        else:
                            json_file[int(line[2])] = {"simulation": {"start": {"pc": int(line[3],16)}}}
                        json_file[int(line[2])]["simulation"]["start"]["global"] = int(line[6])
                        json_file[int(line[2])]["simulation"]["end"] = {"pc" : int(line[7],16)}
                        json_file[int(line[2])]["simulation"]["end"]["global"] = int(line[10])
                        targets.append(start)
                        targets.append(end)
                    elif row[0] == "Warmup":
                        line = row[3].split(",")
                        start = PcCountPair(int(line[3],16),int(line[6]))
                        end = PcCountPair(int(line[7],16),int(line[10]))
                        if(int(line[0]) in json_file):
                            json_file[int(line[0])]["warmup"] = {"start": {"pc": int(line[3],16)}}
                        else:
                            json_file[int(line[0])] = {"warmup": {"start": {"pc": int(line[3],16)}}}
                        json_file[int(line[0])]["warmup"]["start"]["count"] = int(line[6])
                        json_file[int(line[0])]["warmup"]["end"] = {"pc": int(line[7],16)}
                        json_file[int(line[0])]["warmup"]["end"]["count"] = int(line[10])
                        targets.append(start)
                        targets.append(end)
        
        for rid, region in json_file.items():
            if("warmup" in region):
                start = PcCountPair(region["warmup"]["start"]["pc"],
                                    region["warmup"]["start"]["count"])
            else:
                start = PcCountPair(region["simulation"]["start"]["pc"],
                                    region["simulation"]["start"]["global"])
            region_id[start] = rid
        