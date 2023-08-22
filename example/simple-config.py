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

"""
This gem5 configuration script runs the RISCVMatchedBoard in FS mode with a
an Ubuntu 20.04 image and calls m5 exit after the simulation has booted the OS.

Usage
---

```
scons build/RISCV/gem5.opt

./build/RISCV/gem5.opt configs/example/gem5_library/riscvmatched-fs.py
```
"""

from gem5.prebuilt.riscvmatched.riscvmatched_board import RISCVMatchedBoard
from gem5.utils.requires import requires
from gem5.isas import ISA
from gem5.simulate.simulator import Simulator
from gem5.resources.resource import obtain_resource, BinaryResource

import argparse

requires(isa_required=ISA.RISCV)


parser = argparse.ArgumentParser()
parser.add_argument(
    "--matrix_size",
    required=True,
    type=int,
    help="Select an integer value for the size of the Hadamard matrix/ PB Design to use.",
    choices=[4, 12, 16, 40, 48, 80],
)

parser.add_argument(
    "--foldover",
    required=True,
    help="If False, then use the original Hadamard matrix/PB design. If True, then use the opposite of the Hadamard matrix.",
    type=int,
    choices=[0, 1],
)

parser.add_argument(
    "--pb_row",
    required=True,
    type=int,
    help="Choose a row of the PB design to use to create a configuration json.",
)

parser.add_argument(
    "--benchmark",
    required=True,
    help="Choose a benchmark from gem5 Resources, or specify a filepath to a local benchmark.",
)


args = parser.parse_args()


# instantiate the riscv matched board with default parameters
board = RISCVMatchedBoard(
    clk_freq="1.2GHz",
    l2_size="2MB",
    is_fs=False,
    config_json=f"./configured_jsons/{args.foldover}_{args.pb_row}_{args.matrix_size}.json",
)

if args.benchmark == "riscv-bubblesort":
    board.set_se_binary_workload(
        binary=BinaryResource("./test-binaries/Bubblesort")
    )

elif args.benchmark == "riscv-floatmm":
    board.set_se_binary_workload(
        binary=BinaryResource("./test-binaries/FloatMM")
    )
else:
    board.set_se_binary_workload(
        binary=obtain_resource(resource_id=args.benchmark),
        arguments=["sample", "100"],
    )

simulator = Simulator(board=board)
simulator.run()
