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
    """
    This LoopPoint class is used to manage the information needed for LoopPoint
    in workload
    """

    def __init__(
        self,
        targets: List[PcCountPair],
        regions: Dict[PcCountPair, int],
        json_file: Dict[int, Dict],
    ) -> None:
        """
        :param targets: a list of PcCountPair that are used to generate exit
        event at when the PcCountTrackerManager encounter this PcCountPair in
        execution
        :param regions: a dictionary used to find the corresponding region id
        for the significant PcCountPair. This is mainly used to ensure
        checkpoints are taken in the correct PcCountPair or relative counts are
        updated at the correct count
        :param json_file: all the LoopPoint data including relative counts and
        multiplier are stored in this parameter. It can be outputted as a json
        file.
        """

        self._manager = PcCountTrackerManager()
        self._manager.targets = targets
        self._targets = targets
        self._regions = regions
        self._json_file = json_file

    def setup_processor(
        self,
        processor: AbstractProcessor,
    ) -> None:
        """
        This function is used to setup a PC tracker in all the cores and
        connect all the tracker to the PC tracker manager to perform
        multithread PC tracking
        :param processor: the processor used in the simulation configuration
        """
        for core in processor.get_cores():
            core.add_pc_tracker_probe(self._targets, self._manager)

    def update_relatives(self) -> None:
        """
        This function is used to update the relative count for restore used.
        The new relative count will be stored in the _json_file and can be
        outputted into a json file by calling the output_json_file function.
        """
        current_pair = self._manager.get_current_pc_count_pair()
        temp_pair = PcCountPair(current_pair.getPC(), current_pair.getCount())
        if temp_pair in self._regions:
            rid = self._regions[temp_pair]
            region = self._json_file[rid]["simulation"]
            if "warmup" in self._json_file[rid]:
                # if this region has a warmup interval,
                # then update the relative count for the
                # start of the simulation region
                start = region["start"]["pc"]
                temp = region["start"]["global"] - self._manager.get_pc_count(
                    start
                )
                self._json_file[rid]["simulation"]["start"]["relative"] = int(
                    temp
                )
            end = region["end"]["pc"]
            temp = region["end"]["global"] - self._manager.get_pc_count(end)
            self._json_file[rid]["simulation"]["end"]["relative"] = int(temp)

    def output_json_file(
        self, input_indent: int = 4, filename: str = "LoopPoint.json"
    ) -> Dict[int, Dict]:
        """
        This function is used to output the _json_file into a json file
        :param input_indent: the indent value of the json file
        :param filename: the name of the output file
        """
        with open(filename, "w") as file:
            json.dump(self._json_file, file, indent=input_indent)

    def get_current_region(self) -> int:
        """
        This function returns the region id if the current PC Count pair is
        significant(e.x. beginning of the checkpoint), otherwise, it returns
        a '-1' to indicate the current PC Count pair is not significant
        """
        current_pair = self._manager.get_current_pc_count_pair()
        temp_pair = PcCountPair(current_pair.getPC(), current_pair.getCount())
        if temp_pair in self._regions:
            return self._regions[temp_pair]
        return -1

    def get_current_pair(self) -> PcCountPair:
        """
        This function returns the current PC Count pair
        """
        return self._manager.get_current_pc_count_pair()

    def get_regions(self) -> Dict[PcCountPair, int]:
        """
        This function returns the complete dictionary of _regions
        """
        return self._regions

    def get_targets(self) -> List[PcCountPair]:
        """
        This function returns the complete list of _targets
        """
        return self._targets


class LoopPointCheckpoint(LoopPoint):
    def __init__(self, looppoint_file: Path, if_csv: bool) -> None:
        """
        This class is specifically designed to take in the LoopPoint data file
        and generate the information needed to take checkpoints for LoopPoint
        regions(warmup region+simulation region)
        :param looppoint_file: the director of the LoopPoint data file
        :param if_csv: if the file is a csv file, then it is True. If the file
        is a json file, then it is False
        """

        _json_file = {}
        _targets = []
        _region_id = {}

        if if_csv:
            self.profile_csv(looppoint_file, _targets, _json_file, _region_id)
        else:
            self.profile_json(looppoint_file, _targets, _json_file, _region_id)

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
        """
        This function profiles the csv LoopPoint data file into three variables
        to take correct checkpoints for LoopPoint
        :param looppoint_file_path: the director of the LoopPoint data file
        :param targets: a list of PcCountPair
        :param json_file: a dictionary for all the LoopPoint data
        :param region_id: a dictionary for all the significant PcCountPair and
        its corresponding region id
        """

        # This section is hard-coded to parse the data in the csv file.
        # The csv file is assumed to have a constant format.
        with open(looppoint_file_path, newline="") as csvfile:
            spamreader = csv.reader(csvfile, delimiter=" ", quotechar="|")
            for row in spamreader:
                if len(row) > 1:
                    if row[0] == "cluster":
                        # if it is a simulation region
                        line = row[4].split(",")
                        start = PcCountPair(int(line[3], 16), int(line[6]))
                        end = PcCountPair(int(line[7], 16), int(line[10]))
                        if int(line[2]) in json_file:
                            #  if this region was created in the json_file
                            json_file[int(line[2])]["simulation"] = {
                                "start": {"pc": int(line[3], 16)}
                            }
                        else:
                            json_file[int(line[2])] = {
                                "simulation": {
                                    "start": {"pc": int(line[3], 16)}
                                }
                            }
                        json_file[int(line[2])]["simulation"]["start"][
                            "global"
                        ] = int(line[6])
                        json_file[int(line[2])]["simulation"]["end"] = {
                            "pc": int(line[7], 16)
                        }
                        json_file[int(line[2])]["simulation"]["end"][
                            "global"
                        ] = int(line[10])
                        json_file[int(line[2])]["multiplier"] = float(line[14])
                        targets.append(start)
                        targets.append(end)
                        # store all the PC Count pairs from the file to the
                        # targets list
                    elif row[0] == "Warmup":
                        line = row[3].split(",")
                        start = PcCountPair(int(line[3], 16), int(line[6]))
                        end = PcCountPair(int(line[7], 16), int(line[10]))
                        if int(line[0]) in json_file:
                            json_file[int(line[0])]["warmup"] = {
                                "start": {"pc": int(line[3], 16)}
                            }
                        else:
                            json_file[int(line[0])] = {
                                "warmup": {"start": {"pc": int(line[3], 16)}}
                            }
                        json_file[int(line[0])]["warmup"]["start"][
                            "count"
                        ] = int(line[6])
                        json_file[int(line[0])]["warmup"]["end"] = {
                            "pc": int(line[7], 16)
                        }
                        json_file[int(line[0])]["warmup"]["end"][
                            "count"
                        ] = int(line[10])
                        targets.append(start)
                        targets.append(end)
                        # store all the PC Count pairs from the file to the
                        # targets list

        for rid, region in json_file.items():
            # this loop iterates all the regions and find the significant PC
            # Count pair for the region
            if "warmup" in region:
                # if the region has a warmup interval, then the checkpoint
                # should be taken at the start of the warmup interval
                start = PcCountPair(
                    region["warmup"]["start"]["pc"],
                    region["warmup"]["start"]["count"],
                )
            else:
                # if the region does not have a warmup interval, then the
                # checkpoint should be taken at the start of the simulation
                # region
                start = PcCountPair(
                    region["simulation"]["start"]["pc"],
                    region["simulation"]["start"]["global"],
                )
            region_id[start] = rid

    def profile_json(
        self,
        looppoint_file_path: Path,
        targets: List[PcCountPair],
        json_file: Dict[int, Dict],
        region_id: Dict[PcCountPair, int],
    ) -> None:
        """
        This function profiles the json LoopPoint data file into three
        variables to take correct checkpoints for LoopPoint
        :param looppoint_file_path: the director of the LoopPoint data file
        :param targets: a list of PcCountPair
        :param json_file: a dictionary for all the LoopPoint data
        :param region_id: a dictionary for all the significant PcCountPair and
        its corresponding region id
        """

        with open(looppoint_file_path) as file:
            json_file = json.load(file)
            # load all json information into the json_file variable
            for rid, region in json_file.items():
                # iterates all regions
                sim_start = PcCountPair(
                    region["simulation"]["start"]["pc"],
                    region["simulation"]["start"]["global"],
                )
                targets.append(sim_start)
                # store all PC Count pairs in the file into targets list
                end = PcCountPair(
                    region["simulation"]["end"]["pc"],
                    region["simulation"]["end"]["global"],
                )
                targets.append(end)
                if "warmup" in region:
                    # if there is a warmup in the region, then the checkpoint
                    # should be taken at the start of the warmup interval
                    start = PcCountPair(
                        region["warmup"]["start"]["pc"],
                        region["warmup"]["start"]["count"],
                    )
                    targets.append(start)
                    end = PcCountPair(
                        region["warmup"]["end"]["pc"],
                        region["warmup"]["end"]["count"],
                    )
                    targets.append(end)
                else:
                    # if there is not a warmup interval in the region, then the
                    # checkpoint should be taken at the start of the simulation
                    # region
                    start = sim_start
                region_id[start] = rid


class LoopPointRestore(LoopPoint):
    def __init__(self, looppoint_file: Path, checkpoint_path: Path) -> None:
        """
        This class is specifically designed to take in the LoopPoint data file and
        generator information needed to restore a checkpoint taken by the
        LoopPointCheckPoint.
        :param looppoint_file: a json file generated by gem5 that has all the
        LoopPoint data information
        :param checkpoint_path: the director of the checkpoint taken by the gem5
        standard library looppoint_save_checkpoint_generator

        """

        _json_file = {}
        _targets = []
        _region_id = {}

        self.profile_restore(
            looppoint_file, checkpoint_path, _targets, _json_file, _region_id
        )

        super().__init__(
            _targets,
            _region_id,
            _json_file,
        )

    def profile_restore(
        self,
        looppoint_file_path: Path,
        checkpoint_dir: Path,
        targets: List[PcCountPair],
        json_file: Dict[int, Dict],
        region_id: Dict[PcCountPair, int],
    ) -> None:
        """
        This function is used to profile data from the LoopPoint data file to
        information needed to restore the LoopPoint checkpoint
        :param looppoint_file_path: the director of the LoopPoint data file
        :param targets: a list of PcCountPair
        :param json_file: a dictionary for all the LoopPoint data
        :param region_id: a dictionary for all the significant PcCountPair and
        its corresponding region id
        """
        regex = re.compile(r"cpt.Region([0-9]+)")
        rid = regex.findall(checkpoint_dir.as_posix())[0]
        # finds out the region id from the directory name
        with open(looppoint_file_path) as file:
            json_file = json.load(file)
            if rid not in json_file:
                # if the region id does not exist in the LoopPoint data file
                # raise a fatal message
                fatal(f"{rid} is not a valid region\n")
            region = json_file[rid]
            if "warmup" in region:
                if "relative" not in region["simulation"]["start"]:
                    # if there are not relative counts for the PC Count pair
                    # then it means there is not enough information to restore
                    # this checkpoint
                    fatal(f"region {rid} doesn't have relative count info\n")
                start = PcCountPair(
                    region["simulation"]["start"]["pc"],
                    region["simulation"]["start"]["relative"],
                )
                region_id[start] = rid
                targets.append(start)
            if "relative" not in region["simulation"]["end"]:
                fatal(f"region {rid} doesn't have relative count info\n")
            end = PcCountPair(
                region["simulation"]["end"]["pc"],
                region["simulation"]["end"]["relative"],
            )
            region_id[end] = rid
            targets.append(end)
