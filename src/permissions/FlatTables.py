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

import os

from m5.objects.ClockedObject import ClockedObject
from m5.params import *
from m5.proxy import *
from m5.SimObject import (
    PyBindMethod,
    SimObject,
)

# It might be better to extend this class to implement space-control instead of
# putting lipstick to add more features.


class FlatTables(ClockedObject):
    """
    This is a simple SimObject that sits between the LLC and the memory
    controller that is responsible for performing checks with physical
    addressess. In this version of the SimObject, it can be used to estimate
    the performance overhead of Mondrian memory protection.

    With certain extensions, this can also be used as a reverse mapping table
    for permission checks.

    TODO:
    We need a traffic generator if we don't want the operating system to manage
    the stuff this SimObject does. Also that is a better design without the OS
    getting involved for security reasons.

    @params
    :: basic connections ::
    cpu_side_port: The CPU side port that receives requests
    mem_side_port: The mem side port that sends requests downstream

    :: timing :: (pretty much for MMP)
    hit_latency:
    miss_latency:

    cache_size: size of the MMP cache. The table is small so a small cache
                should not be a problem
    cache_policy: There are some basic caching policies implemented in this
                version.

    addr_range: TO enable the traffic generator to send requests to the same
                memory device with a dedicated memory range.
    traffic_side_port: The traffic port that receives requests.
    """

    type = "FlatTables"
    cxx_header = "permissions/flat_tables.hh"
    cxx_class = "gem5::FlatTables"

    # Receives request
    cpu_side_ports = VectorResponsePort("Response side port, sends requests")
    mem_side_port = RequestPort("Reqeust side port, receives requests")

    # We need to define a name for the exact permissions to simulate
    model_name = Param.String("flat-table", "The user needs to provide"
                    " the name of the model they want to simulate: "
                    " flat-tables, deact (larger flat table entries),")

    # TODO: Marked for deletion
    
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
        "To enable or disable permission checks.",
    )

    # Even if there is an OS driver that sets up the permissions, there will be
    # a need in the hardware to redirect all memory requests to a permission
    # region.

    # A final boolean is required to enable or disable dedicated permission
    # caching. Make sure to set this range as uncacheable in the config script.
    use_dedicated_caching = Param.Bool(
        True,
        "To enable dedicated permission caching.",
    )

    # To make sure that the table actually exists in the memory, a base address
    # is needed. The default address is hardcoded into X86's IO range.
    permission_base_addr = Param.Addr(
        0xC0000000,
        "Base of the permission table.",
    )

    # To perform binary lookup or linear lookup, we need to know the number
    # of entries if the permission are not maintained per segment.
    number_of_entries = Param.Unsigned(
        0,
        "Total number of variable permission table entries.",
    )

    binary_search = Param.Bool(
        True,
        "Assume that the permission table is sorted.",
    )

    permission_entry_size = Param.Unsigned(
        32,
        "Size of a permission table entry.",
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

    # Modeling the cache correctly. This is lg(total_Cache_entries)
    cache_lookup_latency = Param.Tick(5, "Latency to lookup an entry")
    cache_entry_creation_latency = Param.Tick(1, "Cache entry creation"
                                                " latency")

    # need to specify the start of the remote memory
    remote_memory_start = Param.UInt64(0x0, "remote memory start address")

    # need to define the size of the memory
    total_memory_size = Param.UInt64(0x0, "Size of the memory")

    # local memory sizes are needed for mondrain as it does permission checks
    # for the entire memory range. For the hardcoded x86 board, the memory
    # cannot start at 0x0.
    local_memory_start = Param.UInt64(0x0, "remote memory start address")
    local_memory_end = Param.UInt64(0x0, "remote memory end address")


    # For the MMP cache, there needs to be a size and the caching policy.
    # FIXME
    cache_size = Param.UInt64(1024, "Size of the MMP cache in Bytes")
    cache_policy = Param.String(
        "lru",
        "caching policy of the MMP cache. \
                            Must be lru, mru random.",
    )

    # Need to define a segment size for which default permissions are defined
    segment_size = Param.UInt64(4096, "By default, the segment is of 4 KiB")

    # Should we add a bandwidth to this simobject? Ideally no, this should be
    # an infinite bandwidth connection where the checks happen very fast
    # without queuing or buffering.

    # Adding more parameters to enable disaggregated memory info.
    host_id = Param.Int(-1, "Host ID, if simulating CXL-like system")

    # Adding params for enabling interrupts. If there is a write to this addr,
    # the OS should trap that as an interrupt. Ideally we want an mwait
    # instruction to have the minimal overhead.
    interrupt_addr = Param.Addr(
        0xC0000000,
        "Default interrupt address for \
                        X86 systems. The OS needs to monitor this address \
                        as well",
    )

    # We need to write the permission table as a JSON entry for the SimObject
    # to be able to cache these entries. Permission packet and the actual
    # memory packets are not blocking, meaning that a permission packet is
    # generated for a given memory request and the permission packet is
    # scheduled first, and, then the actual memory packet is sent to the memory
    # before the permission packet response is received from the memory for
    # performance purposes. The actual check is assumed to be done at the
    # recvTimingResp. But for the timing model to work correctly, a lazy
    # implementation follows where the permission entries are also entered as a
    # input JSON file for the permission cache to work. Flat table papers don't
    # have this problem.
    # host_permission_table_json = Param.String(os.path.join(os.getcwd(), ""),
    #                                     "Path to the permission table JSON")

    # What else do we need?
    mshr_count = Param.Unsigned(512, "Number of MSHR registers to keep a track \
                                of all the outgoing packets.")
