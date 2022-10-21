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
from pathlib import Path
from typing import List
from gem5.components.processors.abstract_core import AbstractCore
from m5.objects.LoopPointManager import LoopPointManager
import csv
import re
        
        
class BaseLoopPoints:
    
    def __init__(
        self,
        checkpointPC,
        checkpointCount,
        relativePC,
        relativeCount,
        regionID
    ) -> None:
        
        self.checkpointPC = checkpointPC
        self.checkpointCount = checkpointCount
        self.relativePC = relativePC
        self.relativeCount = relativeCount
        self.regionID = regionID
        
        self.looppointManager = LoopPointManager()
    
    def setup_cpu(
        self, 
        cpuList: List[AbstractCore], 
        ) -> None:
        self.looppointManager.target_count = self.checkpointCount
        self.looppointManager.target_pc = self.checkpointPC
        self.looppointManager.region_id = self.regionID
        self.looppointManager.relative_pc = self.relativePC
        self.looppointManager.relative_count = self.relativeCount
        self.looppointManager.setup(cpuList)
    
    
    def get_checkpointPC(self):
        return self.checkpointPC
    
    def get_checkpointCount(self):
        return self.checkpointCount
    
    def get_relativePC(self):
        return self.relativePC
    
    def get_relativeCount(self):
        return self.relativeCount
        

class LoopPointsCheckPoint(BaseLoopPoints):
    
    def __init__(
        self,
        LoopPointFilePath: Path
    ) -> None:
        
        self.checkpointPC = []
        self.checkpointCount = []
        self.relativePC = []
        self.relativeCount = []
        self.regionID = []
        self.simulationRegion = dict()
        self.warmupRegion = dict()
        self.max_regionid = 0
        self.profile_checkpoints(LoopPointFilePath)
        super().__init__(
            checkpointPC=self.checkpointPC ,
            checkpointCount=self.checkpointCount ,
            relativePC = self.relativePC,
            relativeCount = self.relativeCount,
            regionID = self.regionID
        )
        
    def profile_checkpoints(self, looppoint_file_path:Path):
        with open(looppoint_file_path, newline='') as csvfile:
            spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
            for row in spamreader:
                if(len(row)>1):
                    if row[0]=="cluster":
                        line = row[4].split(",")
                        self.simulationRegion[line[2]] = [(line[3], line[6]), (line[7],line[10])]
                        if(self.max_regionid<int(line[2])):
                            self.max_regionid = int(line[2])
                    elif row[0]=="Warmup":
                        line = row[3].split(",")
                        self.warmupRegion[line[0]] = [(line[3], line[6]), (line[7],line[10])]
                        
        for id in range(1,self.max_regionid+1):
            if str(id) in self.simulationRegion:
                self.regionID.append(id)
                if str(id) in self.warmupRegion:
                    self.checkpointPC.append(self.warmupRegion[str(id)][0][0])
                    self.checkpointCount.append(self.warmupRegion[str(id)][0][1])
                    self.relativePC.append(self.simulationRegion[str(id)][0][0])
                    self.relativeCount.append(self.simulationRegion[str(id)][0][1])
                    self.relativePC.append(self.simulationRegion[str(id)][1][0])
                    self.relativeCount.append(self.simulationRegion[str(id)][1][1])
                else:
                    self.checkpointPC.append(self.simulationRegion[str(id)][0][0])
                    self.checkpointCount.append(self.simulationRegion[str(id)][0][1])
                    self.relativePC.append(self.simulationRegion[str(id)][1][0])
                    self.relativeCount.append(self.simulationRegion[str(id)][1][1])
                    self.relativePC.append(0)
                    self.relativeCount.append(-1)
                    
    def get_simulationRegion_dict(self):
        return self.simulationRegion
    
    def get_warmupRegion_dict(self):
        return self.warmupRegion
    
    def get_max_regionid(self):
        return self.max_regionid
        
    
class LoopPointsRestore(BaseLoopPoints):
    
    def __init__(
        self,
        LoopPointOutputFilePath: Path,
        CheckPointDir: Path
    ) -> None:
        
        self.checkpointPC = []
        self.checkpointCount = []
        self.relativePC = []
        self.relativeCount = []
        self.regionID = []
        self.profile_restore(LoopPointOutputFilePath, CheckPointDir)
        super().__init__(
            checkpointPC=self.checkpointPC ,
            checkpointCount=self.checkpointCount ,
            relativePC = self.relativePC,
            relativeCount = self.relativeCount,
            regionID = self.regionID
        )
    
    def profile_restore(self, checkpoint_file_path:Path, checkpoint_dir:Path):
        regex = re.compile(r"cpt.([0-9]+)")
        tick = regex.findall(checkpoint_dir.as_posix())[0]
        print(f"looking for tick {tick}\n")
        with open(checkpoint_file_path) as file:
            while True:
                line = file.readline()
                if not line:
                    fatal("checkpoint tick not found in checkpoint file")
                line = line.split(":")
                print(f"split:{line}\n")
                if line[0] == tick:
                    self.regionID.append(line[1])
                    if(len(line)>6):
                        self.checkpointPC.append(line[4])
                        self.checkpointCount.append(line[5])
                    if(len(line)>8):
                        self.checkpointPC.append(line[6])
                        self.checkpointCount.append(line[7])
                    print(f"get target PC {self.checkpointPC}\n")
                    print(f"get target PC count {self.checkpointCount}\n")
                    break
                