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

from m5.params import UInt64
from m5.util import fatal
from pathlib import Path
from typing import List, Tuple
import csv

class LoopPoints:
    
    def __init__(
        self,
        looppoint_file_path: Path = None,
        checkpoint_info_file_path: Path = None
    ) -> None:
        
        self.simulation = dict()
        self.warmup = dict()
        self.relativePC = []
        self.relativeCount = []
        self.checkpointPC = []
        self.checkpointCount = []
        
        self.max_regionid = 0
    
        with open(looppoint_file_path, newline='') as csvfile:
            spamreader = csv.reader(csvfile, delimiter=' ', quotechar='|')
            for row in spamreader:
                if(len(row)>1) and ('#' not in row[0]):
                    if row[0]=="cluster":
                        line = row[4].split(",")
                        self.simulation[line[2]] = [(line[3], line[6]), (line[7],line[10])]
                        if(self.max_regionid<int(line[2])):
                            self.max_regionid = int(line[2])
                    elif row[0]=="Warmup":
                        line = row[3].split(",")
                        self.warmup[line[0]] = [(line[3], line[6]), (line[7],line[10])]
                        
        for id in self.max_regionid:
            if str(id) in self.simulation:
                if str(id) in self.warmup:
                    self.checkpointPC.append(self.warmup[str(id)][0][0])
                    self.checkpointCount.append(self.warmup[str(id)][0][1])
                    self.relativePC.append(self.simulation[str[id]][0][0])
                    self.relativePC.append(self.simulation[str[id]][1][0])
                else:
                    self.checkpointPC.append(self.simulation[str(id)][0][0])
                    self.checkpointCount.append(self.simulation[str(id)][0][1])
                    self.relativePC.append(self.simulation[str[id]][1][0])
                    self.relativePC.append(0)
                       

    def get_simulation_dict(self):
        return self.simulation
    
    def get_warmup_dict(self):
        return self.warmup
    
    def get_max_regionid(self):
        return self.max_regionid
    
    def get_checkpointPC(self):
        return self.checkpointPC
    
    def get_checkpointCount(self):
        return self.checkpointCount
    
    def get_relativePC(self):
        return self.relativePC