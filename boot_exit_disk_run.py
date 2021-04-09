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

from components_library.motherboards.x86_motherboard import X86Motherboard
import m5
from m5.objects import Root

from components_library.motherboards.simple_motherboard \
    import SimpleMotherboard
from components_library.cachehierarchies.private_l1_private_2_cache_hierarchy \
    import PrivateL1PrivateL2CacheHierarchy
from components_library.cachehierarchies.no_cache import NoCache
from components_library.memory.ddr3_1600_8x8 import DDR3_1600_8x8
from components_library.processors.simple_processor import SimpleProcessor
from components_library.processors.cpu_types import CPUTypes

import os
import subprocess
import gzip
import shutil


cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(l1d_size = "32kB",
                                                   l1i_size = "32kB",
                                                   l2_size = "256kB",
                                                  )

# For an even simpler setup, have no cache at all!
#cache_hierarchy = NoCache()

memory = DDR3_1600_8x8(size="3GB")

processor = SimpleProcessor(cpu_type = CPUTypes.ATOMIC, num_cores=1)

motherboard = X86Motherboard(clk_freq="3GHz",
                                processor=processor,
                                memory=memory,
                                cache_hierarchy=cache_hierarchy,
                               )



thispath = os.path.dirname(os.path.realpath(__file__))

kernel_url = \
    "http://dist.gem5.org/dist/v21-0/kernels/x86/static/vmlinux-5.4.49"
kernel_path = os.path.join(thispath, 'vmlinux-5.4.49')
if not os.path.exists(kernel_path):
    subprocess.run(["wget", kernel_url])

boot_img_url = \
    "http://dist.gem5.org/dist/v21-0/images/x86/ubuntu-18-04/boot-exit.img.gz"
boot_img_path_gz = os.path.join(thispath, 'boot-exit.img.gz')
boot_img_path = os.path.join(thispath, 'boot-exit.img')

if not os.path.exists(boot_img_path):
    subprocess.run(["wget", boot_img_url])
    with gzip.open(boot_img_path_gz, "rb") as f:
        with open(boot_img_path, 'wb') as o:
            shutil.copyfileobj(f,o)

command =  "m5 exit \n"

motherboard.set_workload(kernel = kernel_path, disk_image = boot_img_path,
                         command = command)

print("Running with ISA: " + motherboard.get_runtime_isa().name)
print("Running with protocol: " +
    motherboard.get_runtime_coherence_protocol().name)
print()

root = Root(full_system = True, system = motherboard.get_system_simobject())

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick %i because %s' % (m5.curTick(), exit_event.getCause()))