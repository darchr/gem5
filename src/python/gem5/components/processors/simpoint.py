# Copyright (c) 2021 The Regents of the University of California
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
from typing import List, Tuple


class SimPoint:
    """
    This Simpoint class is used to manage the information needed for simpoint
    in workload

    """

    def __init__(
        self,
        simpoint_interval: int,
        simpoint_file_path: Path = None,
        weight_file_path: Path = None,
        simpoint_list: List[int] = None,
        weight_list: List[int] = None,
        # bbv_file_path: Path = None, will take in bbv and generate simpoints
        # with it in the future
    ) -> None:
        """
        :param simpoint_interval: the length of each simpoint interval
        :param simpoint_file_path: the path to the simpoint result file
        generated by Simpoint3.2 or gem5
        :param weight_file_path: the path to the weight result file generated
        by Simpoint3.2 or gem5

        :param simpoint_list: a list of simpoint starting instructions
        :param weight_list: a list of weights

        """
        self._simpoint_interval = simpoint_interval

        if simpoint_file_path is None or weight_file_path is None:
            if simpoint_list is None or weight_list is None:
                fatal("Please pass in file paths or lists for both simpoints "
                    "and weights.")
            else:
                self._simpoint_start_insts = \
                    list(inst * simpoint_interval for inst in simpoint_list)
                self._weight_list = weight_list
        else:
            # if passing in file paths then it calls the function to generate
            # simpoint_start_insts and weight list from the files
            self._simpoint_start_insts, self._weight_list  = \
                self.get_weights_and_simpoints_from_file(
                    simpoint_file_path,
                    weight_file_path
                    )

    def get_weights_and_simpoints_from_file(
        self,
        simpoint_path:Path,
        weight_path:Path,
        )-> Tuple[List[int],List[int]]:
        """
        This function takes in file paths and outputs a list of simpoint
        instruction starts and a list of weights
        """
        simpoint = []
        with open(simpoint_path) as simpoint_file, \
            open(weight_path) as weight_file:
            while True:
                line = simpoint_file.readline()
                if not line:
                    break
                interval = int(line.split(" ",1)[0])
                line = weight_file.readline()
                if not line:
                    fatal("not engough weights")
                weight = float(line.split(" ",1)[0])
                simpoint.append((interval,weight))
        simpoint.sort(key=lambda obj: obj[0])
        # use simpoint to sort
        simpoint_start_insts = []
        weight_list = []
        for start, weight in simpoint:
            simpoint_start_insts.append(start * self._simpoint_interval)
            weight_list.append(weight)
        return simpoint_start_insts, weight_list

    def get_simpoint_start_insts(self)->List[int]:
        return self._simpoint_start_insts

    def get_weight_list(self)->List[float]:
        return self._weight_list

    def get_simpoint_interval(self)->int:
        return self._simpoint_interval