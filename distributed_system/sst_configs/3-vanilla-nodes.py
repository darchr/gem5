# Copyright (c) 2021-2024 The Regents of the University of California
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

import sst
import sys
import os

from sst import UnitAlgebra

cpu_clock_rate = "4GHz"
num_nodes = 3

for node_idx in range(num_nodes):
    gem5_node_params = {
        "frequency": cpu_clock_rate,
        "cmd": f"-re;--outdir=m5out_{node_idx};/workdir/gem5/distributed_system/gem5_node_configs/riscv-vanilla-system.py;",
        "debug_flags": "",
        "ports" : ""
    }

    gem5_node = sst.Component(f"gem5_node_{node_idx}", "gem5.gem5Component")
    gem5_node.addParams(gem5_node_params)
    gem5_node.setRank(node_idx, 0)

# enable Statistics
stat_params = { "rate" : "0ns" }
sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./sst-stats.txt"})
