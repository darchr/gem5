# -*- coding: utf-8 -*-
# Copyright (c) 2018 The Regents of the University of California
# All Rights Reserved.
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

"""
Main system for all SE experiments.

This contains the default caches and memory that will be shared between all
of the experiments.

The system does not have a CPU model, this must be specified by a subclass.
"""

import m5
from m5.objects import *

class L1Cache(Cache):
    """Simple L1 Cache with default values"""

    assoc = 8
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 16
    tgts_per_mshr = 20

    def connectBus(self, bus):
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus.slave

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU-side port
           This must be defined in a subclass"""
        raise NotImplementedError

class L1ICache(L1Cache):
    """Simple L1 instruction cache with default values"""

    # Set the default size
    size = '32kB'

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU icache port"""
        self.cpu_side = cpu.icache_port

class L1DCache(L1Cache):
    """Simple L1 data cache with default values"""

    # Set the default size
    size = '32kB'

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU dcache port"""
        self.cpu_side = cpu.dcache_port

class L2Cache(Cache):
    """Simple L2 Cache with default values"""

    # Default parameters
    size = '512kB'
    assoc = 16
    tag_latency = 10
    data_latency = 10
    response_latency = 1
    mshrs = 20
    tgts_per_mshr = 12

    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.master

    def connectMemSideBus(self, bus):
        self.mem_side = bus.slave


class BaseTestSystem(System):
    """Base class for all test systems.

    Each test system must set its own CPU model by setting the CPUModel
    attribute.
    """

    # This *must* be overridden by a subclass
    _CPUModel = BaseCPU()

    def __init__(self):
        super(BaseTestSystem, self).__init__()
        self.clk_domain = SrcClockDomain(clock = "3GHz",
                                         voltage_domain = VoltageDomain())

        self.mem_mode = 'timing'
        self.mem_ranges = [AddrRange('512MB')]

        self.cpu = self._CPUModel()

        self.cpu.l1d = L1DCache()
        self.cpu.l1i = L1ICache()
        self.l1_to_l2 = L2XBar()

        self.l2cache = L2Cache()
        self.membus = SystemXBar()

        self.cpu.l1d.connectCPU(self.cpu)
        self.cpu.l1d.connectBus(self.l1_to_l2)
        self.cpu.l1i.connectCPU(self.cpu)
        self.cpu.l1i.connectBus(self.l1_to_l2)

        self.l2cache.connectCPUSideBus(self.l1_to_l2)
        self.l2cache.connectMemSideBus(self.membus)

        self.mem_ctrl = DDR4_2400_16x4()
        self.mem_ctrl.range = self.mem_ranges[0]
        self.mem_ctrl.port = self.membus.master

        self.cpu.createInterruptController()
        if m5.defines.buildEnv['TARGET_ISA'] == "x86":
            self.cpu.interrupts[0].pio = self.membus.master
            self.cpu.interrupts[0].int_master = self.membus.slave
            self.cpu.interrupts[0].int_slave = self.membus.master

        self.system_port = self.membus.slave

    def setTestBinary(self, binary_path):
        """Set up the SE process to execute the binary at binary_path"""
        from m5 import options
        output = os.path.join(options.outdir, 'stdout')

        self.cpu.workload = Process(output = output,
                                    cmd = binary_path)
        self.cpu.createThreads()

