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

"""
A run script for running the parsec benchmark suite in gem5.

On the first run of this script it will download the disk image for parsec
and Linux kernel version 5.4.
The disk image is about 8 GB so this can take 10-15 minutes on a fast
connection.


"""

from components_library.motherboards.x86_motherboard import X86Motherboard
import m5
from m5.objects import Root

from components_library.motherboards.simple_motherboard import (
    SimpleMotherboard,
)
from components_library.cachehierarchies.private_l1_private_2_cache_hierarchy \
    import PrivateL1PrivateL2CacheHierarchy
from components_library.cachehierarchies.no_cache import NoCache
from components_library.memory.ddr3_1600_8x8 import DDR3_1600_8x8
from components_library.processors.simple_processor import SimpleProcessor
from components_library.processors.cpu_types import CPUTypes

import os
import subprocess
import gzip
import time
import shutil


cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size="32kB",
    l1i_size="32kB",
    l2_size="256kB",
)

# For an even simpler setup, have no cache at all!
# cache_hierarchy = NoCache()

# Warning!!! This must be kept at 3GB for now. X86Motherboard does not support
# anything else right now!
memory = DDR3_1600_8x8(size="3GB")

processor = SimpleProcessor(cpu_type=CPUTypes.TIMING, num_cores=1)

motherboard = X86Motherboard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

thispath = os.path.dirname(os.path.realpath(__file__))

kernel_url = (
    "http://dist.gem5.org/dist/v21-0/kernels/x86/static/vmlinux-5.4.49"
)
kernel_path = os.path.join(thispath, "vmlinux-5.4.49")
if not os.path.exists(kernel_path):
    subprocess.run(["wget", kernel_url])

parsec_img_url = (
    "http://dist.gem5.org/dist/v21-0/images/x86/ubuntu-18-04/parsec.img.gz"
)
parsec_img_path_gz = os.path.join(thispath, "parsec.img.gz")
parsec_img_path = os.path.join(thispath, "parsec.img")

if not os.path.exists(parsec_img_path):
    subprocess.run(["wget", parsec_img_url])
    with gzip.open(parsec_img_path_gz, "rb") as f:
        with open(parsec_img_path, "wb") as o:
            shutil.copyfileobj(f, o)

# Example command to run blackscholes with simsmall.
command = "cd /home/gem5/parsec-benchmark\n"
command += "source env.sh\n"
command += "parsecmgmt -a run -p blackscholes "
command += "-c gcc-hooks -i simsmall -n {}\n".format(processor.get_num_cores())
command += "sleep 5 \n"
command += "m5 exit \n"

motherboard.set_workload(
    kernel=kernel_path, disk_image=parsec_img_path, command=command
)

print("Running with ISA: " + motherboard.get_runtime_isa().name)
print(
    "Running with protocol: "
    + motherboard.get_runtime_coherence_protocol().name
)
print()

motherboard.get_system_simobject().exit_on_work_items = True

root = Root(
    full_system=True,
    system=motherboard.get_system_simobject(),
    sim_quantum=int(1e9), # Needed if running in KVM mode
)

m5.instantiate()

global_start = time.time()

print("Beginning simulation!")
exit_event = m5.simulate()

if exit_event.getCause() == "workbegin":
    # Reached the start of ROI
    # start of ROI is marked by an
    # m5_work_begin() call
    print("Resetting stats at the start of ROI!")
    m5.stats.dump()
    m5.stats.reset()
    start_tick = m5.curTick()
    # switching to timing cpu if argument cpu == timing
else:
    print("Unexpected termination of simulation!")
    print(f"Exiting @ tick {m5.curTick()} because { exit_event.getCause()}")
    m5.stats.dump()
    exit(1)

# Simulate the ROI
exit_event = m5.simulate()

# Reached the end of ROI
# Finish executing the benchmark with kvm cpu
if exit_event.getCause() == "workend":
    # Reached the end of ROI
    # end of ROI is marked by an
    # m5_work_end() call
    print("Dump stats at the end of the ROI!")
    m5.stats.dump()
    end_tick = m5.curTick()
else:
    print("Unexpected termination of simulation!")
    print()
    m5.stats.dump()
    end_tick = m5.curTick()
    exit(1)

# Run the rest of the workload until m5 exit
# NOTE: you can remove this to save time since it's after the ROI
exit_event = m5.simulate()

print("Done with the simulation")
print()
print("Performance statistics:")

print(f"Simulated time in ROI: {((end_tick-start_tick)/1e12):.2f}s")
print(f"Ran a total of {m5.curTick()/1e12} simulated seconds")
print(
    f"Total wallclock time: {time.time()-global_start:.2f}s, "
    f"{(time.time()-global_start)/60:.2f} min"
)
