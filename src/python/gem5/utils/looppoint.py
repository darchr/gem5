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
from m5.objects.LoopPoint import LoopPointManager
import csv
import re
        
        
class BaseLoopPoint:
    """
    A stdlib LoopPoint object that contains the information gather from the
    input files and use a LoopPointManager to connect them with the cores.
    """
    
    def __init__(
        self,
        checkpointPC: List[int],
        checkpointCount: List[int],
        relativePC: List[int],
        relativeCount: List[int],
        regionID: List[int]
    ) -> None:
        """ Stores the LoopPoint related information
        
        :param checkpointPC: A list of PC numbers that will raise an exit
        event at.
        :param checkpointCount: A list of targeting counts that will raise an
        exit event at.
        :param relativePC: A list of PC numbers that are related to the
        checkpoint PC and needed for outputting relative PC counts for
        restoring the LoopPoint checkpoints.
        :param relativeCount: A list of targeting counts for the relativePC. It
        is needed for outputing relative PC counts for restoring the LoopPoint
        checkpoints.
        :param reginID: The ID of the targeting region.
        """
        
        self._checkpointPC = checkpointPC
        self._checkpointCount = checkpointCount
        self._relativePC = relativePC
        self._relativeCount = relativeCount
        self._regionID = regionID
        
        self._looppointManager = LoopPointManager()
    
    def setup_cpu(
        self, 
        cpuList: List[AbstractCore], 
        ) -> None:
        """ Setup LoopPointManager and connect LoopPoint probes with the cores
        :param cpuList: A list of cores that will be used in the simulation
        """
        self._looppointManager.target_count = self._checkpointCount
        self._looppointManager.target_pc = self._checkpointPC
        self._looppointManager.region_id = self._regionID
        self._looppointManager.relative_pc = self._relativePC
        self._looppointManager.relative_count = self._relativeCount
        for core in cpuList:
            core.addLoopPointProbe(
                self._checkpointPC + self._relativePC, 
                self._looppointManager
            )
    
    
    def get_checkpointPC(self):
        return self._checkpointPC
    
    def get_checkpointCount(self):
        return self._checkpointCount
    
    def get_relativePC(self):
        return self._relativePC
    
    def get_relativeCount(self):
        return self._relativeCount
        

class LoopPointCheckpoint(BaseLoopPoint):
    
    def __init__(
        self,
        LoopPointFilePath: Path
    ) -> None:
        """ A subclass of BaseLoopPoint for profiling LoopPoint checkpoints
        :param LoopPointFilePath: The path to the LoopPoint file that contains
        all the LoopPoint region information
        """
        
        _checkpointPC = []
        _checkpointCount = []
        _relativePC = []
        _relativeCount = []
        _regionID = []
        self.simulationRegion = dict()
        self.warmupRegion = dict()
        self.max_regionid = 0
        self.profile_checkpoints(
            LoopPointFilePath,
            checkpointPC = _checkpointPC,
            checkpointCount = _checkpointCount,
            relativePC = _relativePC,
            relativeCount = _relativeCount,
            regionID = _regionID
        )
        super().__init__(
            checkpointPC=_checkpointPC ,
            checkpointCount=_checkpointCount ,
            relativePC = _relativePC,
            relativeCount = _relativeCount,
            regionID = _regionID
        )
        
    def profile_checkpoints(
        self,
        looppoint_file_path:Path,
        checkpointPC: List[int],
        checkpointCount: List[int],
        relativePC: List[int],
        relativeCount: List[int],
        regionID: List[int]
        ) -> None:
        """ 
        Profile LoopPoint checkpoint information from the input file
        """
        
        # read information from the file
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
        
        # Assign PC and its targeting counts to the corresponding lists
        for id in range(1,self.max_regionid+1):
            # The LoopPoint region id might not be continuous, so it needs to 
            # check if the region id was recorded in the simulationRegion or
            # not. If it was not, it means this region id is not valid. 
            if str(id) in self.simulationRegion:
                regionID.append(id)
                if str(id) in self.warmupRegion:
                    # Check if this simulation region has a warmup region.
                    # If it does, use the PC and the starting count of the warm
                    # up region as the checkpoint. The PC, starting
                    # count, and ending count of the simulation region will be
                    # the relatives to the checkpoint.
                    checkpointPC.append(self.warmupRegion[str(id)][0][0])
                    checkpointCount.append(self.warmupRegion[str(id)][0][1])
                    relativePC.append(self.simulationRegion[str(id)][0][0])
                    relativeCount.append(self.simulationRegion[str(id)][0][1])
                    relativePC.append(self.simulationRegion[str(id)][1][0])
                    relativeCount.append(self.simulationRegion[str(id)][1][1])
                else:
                    # If the simulation region doesn't have a warmup region,
                    # then the PC and starting count of the simulation region 
                    # will be the checkpoint. The PC and the ending count of
                    # this simulation region will be the relatives of the
                    # checkpoint.
                    checkpointPC.append(self.simulationRegion[str(id)][0][0])
                    checkpointCount.append(self.simulationRegion[str(id)][0][1])
                    relativePC.append(self.simulationRegion[str(id)][1][0])
                    relativeCount.append(self.simulationRegion[str(id)][1][1])
                    relativePC.append(0)
                    relativeCount.append(-1)
                    
    def get_simulationRegion_dict(self):
        return self.simulationRegion
    
    def get_warmupRegion_dict(self):
        return self.warmupRegion
    
    def get_max_regionid(self):
        return self.max_regionid
        
    
class LoopPointRestore(BaseLoopPoint):
    
    def __init__(
        self,
        LoopPointOutputFilePath: Path,
        CheckPointDir: Path
    ) -> None:
        """ A subclass of BaseLoopPoint for profiling LoopPoint restoring point
        :param LoopPointOutputFilePath: The path to the LoopPoint file
        outputted from the checkpoint simulation that contains all the
        information needed to restore a LoopPoint checkpoint.
        :param CheckPointDir: The path to the checkpoint directory.
        """
        
        _checkpointPC = []
        _checkpointCount = []
        _regionID = []
        self.profile_restore(
            LoopPointOutputFilePath, 
            CheckPointDir,
            checkpointPC = _checkpointPC,
            checkpointCount = _checkpointCount,
            regionID = _regionID
        )
        super().__init__(
            checkpointPC=_checkpointPC ,
            checkpointCount=_checkpointCount ,
            relativePC = [],
            relativeCount = [],
            regionID = _regionID
        )
    
    def profile_restore(
        self, 
        checkpoint_file_path:Path, 
        checkpoint_dir:Path,
        checkpointPC: List[int],
        checkpointCount: List[int],
        regionID: List[int]
        ) -> None:
        """ 
        Profile LoopPoint checkpoint information from the input file
        """
        regex = re.compile(r"cpt.([0-9]+)")
        # The checkpoint directory is named by the simulation Tick it was at
        # when the checkpoint was taken, so we can get the Tick number from the
        # checkpoint directory name.
        tick = regex.findall(checkpoint_dir.as_posix())[0]
        with open(checkpoint_file_path) as file:
            while True:
                line = file.readline()
                if not line:
                    # If the Tick number can't find a match in the output file
                    # it means the checkpoint and the output file doesn't match
                    fatal("checkpoint tick not found in checkpoint file")
                line = line.split(":")
                print(f"split:{line}\n")
                if line[0] == tick:
                    # If the Tick number is found, we can read the information
                    # needed to setup the restoring point and ending point
                    regionID.append(line[1])
                    # Because the simulation region might or might not have a 
                    # warmup region, there might only be one targeting point.
                    if(len(line)>6):
                        checkpointPC.append(line[4])
                        checkpointCount.append(line[5])
                    if(len(line)>8):
                        checkpointPC.append(line[6])
                        checkpointCount.append(line[7])
                    print(f"get target PC {checkpointPC}\n")
                    print(f"get target PC count {checkpointCount}\n")
                    break
                