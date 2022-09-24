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

from m5.params import NULL, UInt64
from m5.util import fatal
from pathlib import Path
from typing import List, Tuple
from m5.objects.LoopPointManager import LoopPointManager
import csv
import re

class LoopPoints:
    
    def __init__(
        self,
        looppoint_file_path: Path = NULL
    ) -> None:
        
        self.simulationRegion = dict()
        self.warmupRegion = dict()
        self.relativePC = []
        self.relativeCount = []
        self.checkpointPC = []
        self.checkpointCount = []
        
        self.max_regionid = 0   
        
        if(looppoint_file_path):
            self.profile_looppoints_checkpoints(looppoint_file_path)

    def get_simulationRegion_dict(self):
        return self.simulationRegion
    
    def get_warmupRegion_dict(self):
        return self.warmupRegion
    
    def get_max_regionid(self):
        return self.max_regionid
    
    def get_checkpointPC(self):
        return self.checkpointPC
    
    def get_checkpointCount(self):
        return self.checkpointCount
    
    def get_relativePC(self):
        return self.relativePC
    
    def profile_looppoints_checkpoints(self, looppoint_file_path:Path):
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
    
    def setup_checkpoints(self, cpuList):
        self.looppointManager = LoopPointManager()
        self.looppointManager.target_count = self.checkpointCount
        self.looppointManager.target_pc = self.checkpointPC
        self.looppointManager.relative_pc = self.relativePC
        self.looppointManager.relative_count = self.relativeCount
        self.looppointManager.setup(cpuList)
        
    
    def setup_restore(self, cpuList, checkpoint_file_path:Path, checkpoint_dir:Path):
        regex = re.compile(r"cpt.([0-9]+)")
        tick = regex.findall(checkpoint_dir.as_posix())
        self.looppointManager = LoopPointManager()
        with open(checkpoint_file_path) as file:
            while True:
                line = file.readline()
                if not line:
                    fatal("checkpoint tick not found in checkpoint file")
                tick = regex.findall(line)
                if line[0] == tick:
                    targetPC = list()
                    targetCount = list()
                    if(line.size()>5):
                        targetPC.append(line[3])
                        targetCount.append(line[4])
                    if(line.size()>7):
                        targetPC.append(line[5])
                        targetCount.append(line[6])
                    self.looppointManager.target_count = targetCount
                    self.looppointManager.target_pc = targetPC
                    self.looppointManager.setup(cpuList)
        