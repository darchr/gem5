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
This gem5 configuation script runs a "hello world" binary on the
RISCVMatched prebuilt board found in src/python/gem5/prebuilt/riscvmatched/

Usage
-----

```
scons build/RISCV/gem5.opt
./build/RISCV/gem5.opt \
    configs/example/gem5_library/riscvmatched-hello.py
```
"""

import argparse
from time import sleep

from run_binary import run_binary

from gem5.isas import ISA
from gem5.prebuilt.riscvmatched.riscvmatched_board import RISCVMatchedBoard
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator
from gem5.utils.multiprocessing import (
    Pool,
    Process,
)
from gem5.utils.requires import requires

if __name__ == "__m5_main__":
    parser = argparse.ArgumentParser(
        description="Script with optional super parameter"
    )
    parser.add_argument("--super", help="Superconducting", default=False)

    args = parser.parse_args()

    # workloads = [obtain_resource("riscv-llvm-minisat-run"), obtain_resource("riscv-gapbs-bfs-run"), obtain_resource("riscv-gapbs-tc-run"), obtain_resource("riscv-npb-is-size-s-run"), obtain_resource("riscv-npb-lu-size-s-run"), obtain_resource("riscv-npb-cg-size-s-run"), obtain_resource("riscv-npb-bt-size-s-run"), obtain_resource("riscv-npb-ft-size-s-run"), obtain_resource("riscv-matrix-multiply-run")]

    workloads = obtain_resource("riscv-getting-started-benchmark-suite")
    # workloads = [obtain_resource("riscv-matrix-multiply-run")]

    processes = []
    for workload in workloads:
        print(workload.get_id())
        folder_name = workload.get_id()

        while len(processes) > 20:
            for process in processes:
                if not process.is_alive():
                    print(f"Process for folder {folder_name} completed")
                    processes.remove(process)
            sleep(10)
        process = Process(
            target=run_binary,
            args=(args.super, workload),
            name=folder_name,
        )
        process.start()
        processes.append(process)

    while processes:
        for process in processes:
            if not process.is_alive():
                print(f"Process for folder {folder_name} completed")
                processes.remove(process)
        sleep(5)

    if len(processes) == 0:
        print("All processes completed")
