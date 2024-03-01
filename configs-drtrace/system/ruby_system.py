# -*- coding: utf-8 -*-
# Copyright (c) 2016 Jason Lowe-Power
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

import m5
from m5.objects import *


class MyRubySystem(System):

    def __init__(self, mem_sys, num_cpus, assoc, dcache_size, main_mem_size, policy, is_link, link_lat, opts, restore=False):
        super(MyRubySystem, self).__init__()
        self._opts = opts

        # Set up the clock domain and the voltage domain
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '5GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.mem_ranges = [AddrRange(Addr(main_mem_size))]

        # self.intrctrl = IntrControl()
        self._createMemoryControllers(assoc, dcache_size, policy, is_link, link_lat)

        # Create the cache hierarchy for the system.
        if mem_sys == 'MI_example':
            from .MI_example_caches import MIExampleSystem
            self.caches = MIExampleSystem()
        elif mem_sys == 'MESI_Two_Level':
            from .MESI_Two_Level import MESITwoLevelCache
            self.caches = MESITwoLevelCache()
        elif mem_sys == 'MOESI_CMP_directory':
            from .MOESI_CMP_directory import MOESICMPDirCache
            self.caches = MOESICMPDirCache()

        self.reader = DRTraceReader(
            directory=f"/{opts.path}/{opts.workload}/", num_players=num_cpus
        )

        self.players = [
            DRTracePlayer(
                reader=self.reader,
                send_data=True,
                compress_address_range=self.mem_ranges[0],
            )
            for _ in range(num_cpus)
        ]

        self.caches.setup(self, self.players, self.mem_ctrl)

        self.caches.access_backing_store = True
        self.caches.phys_mem = SimpleMemory(range=self.mem_ranges[0],
                                           in_addr_map=False)


    def _createMemoryControllers(self, assoc, dcache_size, policy, is_link, link_lat):

        self.mem_ctrl = PolicyManager(range=self.mem_ranges[0], kvm_map=False)
        self.mem_ctrl.static_frontend_latency = "10ns"
        self.mem_ctrl.static_backend_latency = "10ns"

        self.mem_ctrl.loc_mem_policy = policy

        self.mem_ctrl.assoc = assoc

        # self.mem_ctrl.bypass_dcache = True

        # TDRAM cache
        self.loc_mem_ctrl = MemCtrl()
        self.loc_mem_ctrl.consider_oldest_write = True
        self.loc_mem_ctrl.oldest_write_age_threshold = 5000000
        self.loc_mem_ctrl.dram = TDRAM_32(range=self.mem_ranges[0], in_addr_map=False, kvm_map=False)


        self.mem_ctrl.loc_mem = self.loc_mem_ctrl.dram
        self.loc_mem_ctrl.static_frontend_latency = "1ns"
        self.loc_mem_ctrl.static_backend_latency = "1ns"
        self.loc_mem_ctrl.static_frontend_latency_tc = "0ns"
        self.loc_mem_ctrl.static_backend_latency_tc = "0ns"

        # main memory
        self.far_mem_ctrl = MemCtrl()
        self.far_mem_ctrl.dram = DDR4_2400_16x4(range=self.mem_ranges[0], in_addr_map=False, kvm_map=False)
        self.far_mem_ctrl.static_frontend_latency = "1ns"
        self.far_mem_ctrl.static_backend_latency = "1ns"

        self.loc_mem_ctrl.port = self.mem_ctrl.loc_req_port

        self.far_mem_ctrl.port = self.mem_ctrl.far_req_port

        self.mem_ctrl.orb_max_size = 128
        self.mem_ctrl.dram_cache_size = dcache_size

        self.loc_mem_ctrl.dram.read_buffer_size = 64
        self.loc_mem_ctrl.dram.write_buffer_size = 64

        self.far_mem_ctrl.dram.read_buffer_size = 64
        self.far_mem_ctrl.dram.write_buffer_size = 64

