# Copyright (c) 2025 The Regents of the University of California
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

from m5.params import *
from m5.SimObject import SimObject


class DualPort(SimObject):
    type = "DualPort"
    cxx_header = "new/dual_port.hh"
    cxx_class = "gem5::DualPort"

    cpu_side_port = ResponsePort("CPU side port, receives requests")
    mem_side_port = RequestPort("Memory side port, sends requests")

    # We need to define a range on where the permission tables are stored in
    # the main memory. By default, the first 1 GiB after 4 GiB is fixed for the
    # permission table. The table is indexed by a binary search on the address
    # and the table can grow. Each entry has a start address (64 bits), size
    # (64 bits) and permissions (2 bits). The maximum size of the table if
    # each 4 KiB page on a 1 GiB system can be 32.5 MiB
    addr_range = Param.AddrRange(AddrRange(0x100000000, 0x140000000), "table location")

    # TODO
    # Need to add a port to connect this Object to the traffic generator with
    # Non-overlapping region with the operating system but muist be backed by
    # the same memory controller to understand the memory scheduling overhead.
    # traffic_side_port = ResponsePort("CPU side port for permission check")
    
    # So the check will happen with this latency before letting this address
    # go to the main memory.
    # hit_latency = Param.Tick(10, "Latency to forward packets")
    # miss_latency = Param.Tick(50, "This must be a variable latency.")
