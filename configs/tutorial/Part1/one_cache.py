
""" Cache with options for a simple gem5 configuration script

This file contains L1 I/D cache to be used in the simple
gem5 configuration script. It uses the SimpleOpts wrapper to set up command
line options from each individual class.
"""

from __future__ import print_function
from __future__ import absolute_import

import m5
from m5.objects import Cache

# Add the common scripts to our path
m5.util.addToPath('../../')

#from common import SimpleOpts

# Some specific options for caches
# For all options see src/mem/cache/BaseCache.py

class L1Cache(Cache):
    """Simple L1 Cache with default values"""

    assoc = 2
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 4
    tgts_per_mshr = 20

    def __init__(self, options=None):
        super(L1Cache, self).__init__()
        pass

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
    size = '16kB'

    def __init__(self, options=None):
        super(L1ICache, self).__init__(options)
        if not options or not options.l1i_size:
            return
        self.size = options.l1i_size

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU icache port"""
        self.cpu_side = cpu.icache_port
    def connectMemSideBus(self, bus):
        self.mem_side = bus.slave

class L1DCache(L1Cache):
    """Simple L1 data cache with default values"""

    # Set the default size
    size = '64kB'

    def __init__(self, options=None):
        super(L1DCache, self).__init__(options)
        if not options or not options.l1d_size:
            return
        self.size = options.l1d_size

    def connectCPU(self, cpu):
        """Connect this cache's port to a CPU dcache port"""
        self.cpu_side = cpu.dcache_port

    def connectMemSideBus(self, bus):
        self.mem_side = bus.slave
