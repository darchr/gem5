# -*- mode:python -*-

# Copyright (c) 2016 Mark D. Hill and David A. Wood
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
#
# Authors: Jason Lowe-Power

from m5.params import *
from m5.proxy import *

from MemObject import *
from DirtyList import *

class WritebackPolicy(Enum): vals = ['writeback', 'writethrough', 'adaptive']

class DRAMCacheCtrl(MemObject):
    type = "DRAMCacheCtrl"
    cxx_header = "mem/dram_cache/dram_cache_ctrl.hh"

    cpu_side = SlavePort("Upstream port closer to the CPU and/or device")
    mem_side = MasterPort("Downstream port closer to memory")

    invalidation_port = MasterPort("Port to invalidate the other caches")

    addr_ranges = VectorParam.AddrRange([AllMemory],
     "Address range for the CPU-side port (to allow striping)")

    max_outstanding = Param.Int(128, "Max outstanding reqeusts")

    size = Param.MemorySize("Capacity")

    banks = Param.Unsigned("Number of banks (or channels) for whole cache")

    block_size = Param.Unsigned(Parent.cache_line_size, "Block size of cache")

    system = Param.System(Parent.any, "System this cache is a part of")

    storage_ctrl = Param.DRAMCacheStorage("DRAM storage for the cache")

    dirty_list = Param.AbstractDirtyList("Tracks dirty lines")

    check_data = Param.Bool(False, "Checks whether the data is correct on"
        " every access. (Significant overhead)")

    # exit_when_warm = Param.Bool(False, "Exit when cache detects it's warm")
    # warm_threshold = Param.Int(20, "Warm when one cold miss per all misses")

    writeback_policy = Param.WritebackPolicy('writethrough', "Policy for"
        " writebacks. Note: writebacks or adaptive adds a dirty list check")

    send_backprobes = Param.Bool(False, "Send backprobes to chip on evictions")

    bank_number = Param.Int("Number of this bank (for addr offset)")

    dram_ctrl = Param.DRAMCtrl("The DRAM controller this sends misses to")

    def setStorageCtrl(self, ctrl_class):
        ctrl = ctrl_class()
        ctrl.range = int(self.size.getValue())

        self.storage_ctrl = ctrl

class DRAMCacheCtrlFOM(DRAMCacheCtrl):
    type = "DRAMCacheCtrlFOM"
    cxx_header = "mem/dram_cache/dram_cache_ctrl_fom.hh"
