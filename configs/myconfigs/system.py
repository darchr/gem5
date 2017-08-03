from abc import abstractmethod

from caches import L1ICache, L1DCache, L2Cache

import m5
from m5.objects import *

class MemSubSystem(SubSystem):

    def __init__(self, cpus, mem_ranges, switched_out):
        super(MemSubSystem, self).__init__()
        self._mem_ranges = mem_ranges
        self._switched_out = switched_out

        self.membus = SystemXBar()

        self.connectMemSide()

        self.connectCPUSide(cpus)

    @abstractmethod
    def connectCPUSide(self, cpus):
        """Connect these caches to a set of CPUs. This assumes each CPU has an
           icache_port and dcache_port.
        """
        pass

    @abstractmethod
    def connectMemSide(self):
        """Connect the cache system to the memory (DRAM). Assumes there is a
           membus.
        """
        pass

    def connectX86Ints(self, interrupts):
        """Connect the x86 interrupts to the memory bus.
        """
        interrupts.pio = self.membus.master
        interrupts.int_master = self.membus.slave
        interrupts.int_slave = self.membus.master

    def connectSystemPort(self, system):
        """Connect the functional system port to the cache system. Assumes
           there is a system.system_port
           NOTE: Cannot pass ports as parameters!
        """
        system.system_port = self.membus.slave

class NoCaches(MemSubSystem):

    def __init__(self, num_cpus, mem_ranges, switched_out = False):
        super(NoCaches, self).__init__(num_cpus, mem_ranges, switched_out)

    def connectCPUSide(self, cpus):
        for cpu in cpus:
            cpu.icache_port = self.membus.slave
            cpu.dcache_port = self.membus.slave

    def connectMemSide(self):
        self.mem = SimpleMemory(range = self._mem_ranges[0])
        self.mem.port = self.membus.master

        if self._switched_out:
            self.mem.mirror_memory = True

class TwoLevel(MemSubSystem):

    def __init__(self, num_cpus, mem_ranges, switched_out = False):
        super(TwoLevel, self).__init__(num_cpus, mem_ranges, switched_out)

    def connectCaches(self):
        for icache, dcache in zip(self.icaches, self.dcaches):
            icache.connectBus(self.l2bus)
            dcache.connectBus(self.l2bus)

        self.l2cache.connectCPUSideBus(self.l2bus)
        self.l2cache.connectMemSideBus(self.membus)

    def createCaches(self, num_cpus):
        self.icaches = [L1ICache() for i in range(num_cpus)]
        self.dcaches = [L1DCache() for i in range(num_cpus)]

        self.l2cache = L2Cache()

        self.l2bus = L2XBar()

        self.connectCaches()

    def connectCPUSide(self, cpus):
        self.createCaches(len(cpus))

        for i,cpu in enumerate(cpus):
            self.icaches[i].connectCPU(cpu)
            self.dcaches[i].connectCPU(cpu)

    def createMemory(self):
        self.mem_ctrl = DDR3_1600_8x8()
        self.mem_ctrl.range = self._mem_ranges[0]

    def connectMemSide(self):
        self.createMemory()

        self.mem_ctrl.port = self.membus.master

        if self._switched_out:
            self.mem_ctrl.mirror_memory = True


class Config(SubSystem):

    def __init__(self, system, memcls, freq='1GHz', switched_out = False):
        super(Config, self).__init__()

        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = freq
        self.clk_domain.voltage_domain = VoltageDomain()

        self.cpu = TimingSimpleCPU(cpu_id = 0, switched_out = switched_out)

        self.mem_sys = memcls([self.cpu], system.mem_ranges, switched_out)

        # create the interrupt controller for the CPU and connect to the membus
        self.cpu.createInterruptController()

        if m5.defines.buildEnv['TARGET_ISA'] == "x86":
            self.mem_sys.connectX86Ints(self.cpu.interrupts[0])

        if not switched_out:
            self.mem_sys.connectSystemPort(system)

    def setFutureConfig(self, other):
        self.cpu.setFutureCPU(other.cpu)

    def setWorkload(self, process):
        # Set the cpu to use the process as its workload and create thread
        # contexts
        self.cpu.workload = process
        self.cpu.createThreads()
