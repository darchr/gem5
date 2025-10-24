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
from m5.proxy import *
from m5.SimObject import (
    PyBindMethod,
    SimObject,
)

# This class purely exists to make a dedicated hardware for space control that
# is based on the OS-level driver/library design.


class SpaceControl(ClockedPermission):
    """
    This is a simple extension of ClockedPermissions with a modified caching
    structure, that is the same as the MMIO object in the OS. It sits between
    the LLC and the memory
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

    type = "ClockedPermission"
    cxx_header = "new/clocked_permission.hh"
    cxx_class = "gem5::ClockedPermission"

    # When we don't have a flat-table for permissions lookup and need to
    # implement a multi-level page table, the permission cache needs to know
    # what to cache.
    permission_table_json = Param.String(
        "configs_kg/configs/space_control/permission_table.json",
        "The permission table in JSON format",
    )

    # the correct permission model to use. space-control, mondrian, deact
    # space-control is similar to mondrain but only checks for remote memory
    # range and the permission lookups are tiny.
    # Mondrain creates a 64 bit lookup for the start and end address. For host
    # and process isolation, the rest of the entry is 
    permission_model = Param.String(
        "space-control",
        "The permission model to use. Options are space-control, mondrian, deact"
    )
