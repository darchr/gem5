# Copyright (c) 2017 Jason Lowe-Power
# All rights reserved.
#
# Copyright (c) 2025 Regents of the University of California
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

from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.SimObject import (
    PyBindMethod,
    SimObject,
)


class RemoteMemoryPermissions(ClockedPermission):
    """
    This is a simple SimObject that sits between the LLC and the remote memory
    controller that is responsible for performing checks with physical
    addressess. This is based on the DeACT's access-control design.

    Here are a couple of notes to understand the reimplementations of their
    design:
    1. DeACT is based on Gen-Z interconnect where hosts let a virtual address
        to a remote memory location (R.V.A) go out of the host. The R.V.A is
        translated to a P.A outside the system, which substantially increases
        the translation latency. They refer to this design as the I-FAM.
    2. CXL on the other hand ditched this design and opted for the E-FAM-like
        design, where hosts translates V.A to remote physical addresses (RPA),
        eliminating the large latency.
    3. DeACT today cannot be implemented 1-1 on CXL due to the differences in
        assumptions.

    We extented ClockedPermission to implement a flat table structure for the
    access control per 4 KiB page. While this design works flawlessly (in
    theory) for pooled memory. However, permission tables needs to either have
    the same permission per 1 GiB (as per the paper) or the table needs to be
    replicated N times, where N is the number opf participant hosts.

    Observation here is that there not every page needs to have a different
    permission per host, but pages can be grouped together for a set of hosts.
    This will be defined as the "context" in our design.

    @params
    :: basic connections ::
    cpu_side_port: The CPU side port that receives requests
    mem_side_port: The mem side port that sends requests downstream

    :: timing :: (pretty much for MMP)
    hit_latency: hit time for access control data in the DeACT$
    miss_latency: should be the same as the remote memory latency. This
                should be ignored when scheduing the lookup event (ideally).

    cache_size: size of the DeACT cache. The table is small so a small cache
                should not be a problem
    cache_policy: There are some basic caching policies implemented in this
                version.

    addr_range: TO enable the traffic generator to send requests to the same
                memory device with a dedicated memory range.
    traffic_side_port: The traffic port that receives requests.
    """

    type = "RemoteMemoryPermission"
    cxx_header = "new/remote_memory_permission.hh"
    cxx_class = "gem5::RemoteMemoryPermission"

    # Receives request
    cpu_side_ports = VectorResponsePort("Response side port, sends requests")
    mem_side_port = RequestPort("Reqeust side port, receives requests")

    # We need to define a range on where the permission tables are stored in
    # the main memory. By default, the first 1 GiB after 4 GiB is fixed for the
    # permission table. The table is indexed by a binary search on the address
    # and the table can grow. Each entry has a start address (64 bits), size
    # (64 bits) and permissions (2 bits). The maximum size of the table if
    # each 4 KiB page on a 1 GiB system can be 32.5 MiB
    addr_range = Param.AddrRange(
        AddrRange(0x100000000, 0x140000000), "MMP table location"
    )

    # We need a toggle function to enable or disable MMP checks
    enable_permission_check = Param.Bool(
        True,
        "To enable or disable \
                                                        permission checks.",
    )

    # TODO
    # Need to add a port to connect this Object to the traffic generator with
    # Non-overlapping region with the operating system but muist be backed by
    # the same memory controller to understand the memory scheduling overhead.
    # traffic_side_port = ResponsePort("CPU side port for permission check")

    # So the check will happen with this latency before letting this address
    # go to the main memory.
    creation_latency = Param.Tick(
        25,
        "latency to create a new entry in the \
                                        permission table.",
    )
    hit_latency = Param.Tick(10, "Latency to forward packets")
    miss_latency = Param.Tick(50, "This must be a variable latency.")

    # For the MMP cache, there needs to be a size and the caching policy.
    # FIXME
    cache_size = Param.Int(1024, "Size of the MMP cache")
    cache_policy = Param.String(
        "lru",
        "caching policy of the MMP cache. \
                            Must be lru, mru random.",
    )

    # need to define the size of the memory
    total_memory_size = Param.Int(0x0, "Size of the memory")

    # Need to define a segment size for which default permissions are defined
    segment_size = Param.Int(4096, "By default, the segment is of 4 KiB")

    # Should we add a bandwidth to this simobject? Ideally no, this should be
    # an infinite bandwidth connection where the checks happen very fast
    # without queuing or buffering.

    # Adding more parameters to enable disaggregated memory info
    host_id = Param.Int(0, "Host ID, if simulating CXL-like system")

    # Adding params for enabling interrupts. If there is a write to this addr,
    # the OS should trap that as an interrupt. Ideally we want an mwait
    # instruction to have the minimal overhead.
    interrupt_addr = Param.Addr(
        0xC0000000,
        "Default interrupt address for \
                        X86 systems. The OS needs to monitor this address \
                        as well",
    )

    # What else do we need?
