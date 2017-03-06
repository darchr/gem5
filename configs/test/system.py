
import os

import m5
from m5.objects import *
from m5.util import convert
from m5.util import convert
from caches import *
from dram_cache import DRAMCache

class MySystem(System):

    SimpleOpts.add_option("--readers", type = "int", default = 4,
                        help = "Number of CPUs with only reads (default: 4)")
    SimpleOpts.add_option("--writers", type = "int", default = 0,
                        help = "Number of CPUs with only writes (default: 0)")
    SimpleOpts.add_option("--mixers", type = "int", default = 0,
                        help = "Number of CPUs with both reads/writes (50-50)"
                        "(default: 0)")
    SimpleOpts.add_option("--small_perc", type = "float", default = 1.0,
                        help = "Percent of accesses that are to the small WS"
                        "(default: 1.0)")

    def __init__(self, opts):
        super(MySystem, self).__init__()

        # Set up the clock domain and the voltage domain
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '3GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        self.setupMemoryRanges()

        # Create the main memory bus
        # This connects to main memory
        self.membus = SystemXBar(width = 64) # 64-byte width
        self.membus.badaddr_responder = BadAddr()
        self.membus.default = Self.badaddr_responder.pio

        # Set up the system port for functional access from the simulator
        self.system_port = self.membus.slave

        # Create the CPUs for our system.
        self.createCPU(opts.readers, opts.writers, opts.mixers,
                       opts.small_perc)

        # Create the cache heirarchy for the system.
        self.createCacheHierarchy()

        # create the memory system
        self.createMemorySystem()

    def setupMemoryRanges(self):
        self.mem_ranges = [AddrRange('12GB')]

    def createMemorySystem(self):
        raise NotImplementedError('createMemorySystem is pure virtual')

    def getHostParallel(self):
        return self._host_parallel

    def totalInsts(self):
        return sum([cpu.totalInsts() for cpu in self.cpu])

    def createConfig(self, access_type, is_small, cpus, cpu_num):
        dirname = os.path.dirname(os.path.realpath(__file__))

        memory = 2**26 if is_small else 2**33
        memory /= cpus
        memory = (memory / 64) * 64 + 64
        offset = 2**30 if not is_small else 0
        mem_start = memory * cpu_num + offset
        mem_end = memory * (cpu_num + 1) - 64 + offset
        print "CPU", cpu_num, "Memory", memory, "start:", mem_start,
        print "end:", mem_end
        if access_type == 'read':
            perc_read = 100
        elif access_type == 'mix':
            perc_read = 50
        else:
            perc_read = 0
        config_text = """
        STATE 0 100000000 RANDOM %d %d %d 64 333 333 0
        INIT 0
        TRANSITION 0 0 1
        """ % (perc_read, mem_start, mem_end)
        name = 'small_' if is_small else 'large_'
        name += access_type + '_'
        name += str(cpu_num) + '.cfg'
        with open(dirname + '/configs/' + name, 'w') as f:
            f.write(config_text)

    def createMixConfig(self, access_type, small_perc, cpus, cpu_num):
        dirname = os.path.dirname(os.path.realpath(__file__))

        small_start, small_end = self.getMemoryRange(True, cpus, cpu_num)
        large_start, large_end = self.getMemoryRange(False, cpus, cpu_num)

        if access_type == 'read':
            perc_read = 100
        elif access_type == 'mix':
            perc_read = 50
        else:
            perc_read = 0

        # Time in state: 333*10,000 = 3,330,000
        config_text = """
        STATE 0 3330000 RANDOM %d %d %d 64 333 333 0
        STATE 1 3330000 RANDOM %d %d %d 64 333 333 0
        INIT 0
        TRANSITION 0 1 %f
        TRANSITION 0 0 %f
        TRANSITION 1 0 %f
        TRANSITION 1 1 %f
        """ % (perc_read, small_start, small_end,
               perc_read, large_start, large_end,
               1-small_perc, small_perc, small_perc, 1-small_perc)

        name = access_type + '_'
        name += str(cpu_num) + '.cfg'
        with open(dirname + '/configs/' + name, 'w') as f:
            f.write(config_text)

    def getMemoryRange(self, is_small, cpus, cpu_num):
        memory = 2**26 if is_small else 2**33
        memory /= cpus
        memory = (memory / 64) * 64 + 64
        offset = 2**30 if not is_small else 0
        mem_start = memory * cpu_num + offset
        mem_end = memory * (cpu_num + 1) - 64 + offset
        print "CPU", cpu_num, "Memory", memory, "start:", mem_start,
        print "end:", mem_end

        return mem_start, mem_end

    def createCPU(self, read, write, mix, small_perc):
        dirname = os.path.dirname(os.path.realpath(__file__))

        if read:
            for i in range(read):
                self.createMixConfig('read', small_perc, read+write+mix, i)
        if write:
            for i in range(read, read+write):
                self.createMixConfig('write', small_perc, read+write+mix, i)
        if mix:
            for i in range(read+write, read+write+mix):
                self.createMixConfig('mix', small_perc, read+write+mix, i)

        self.cpu = [TrafficGen(elastic_req = False,
                             config_file = dirname +
                                '/configs/read_%d.cfg' % (i))
                    for i in range(read)] + \
                   [TrafficGen(elastic_req = False,
                             config_file = dirname +
                                '/configs/write_%d.cfg' % (i))
                    for i in range(read, read+write)] + \
                   [TrafficGen(elastic_req = False,
                             config_file = dirname +
                                '/configs/mix_%d.cfg' % (i))
                    for i in range(read+write, read+write+mix)]

    def createCacheHierarchy(self):
        # Create an L3 cache (with crossbar)
        self.l3bus = L2XBar(width = 64,
                            snoop_filter = SnoopFilter(max_capacity='32MB'))

        for cpu in self.cpu:
            # Create a memory bus, a coherent crossbar, in this case
            cpu.l2bus = L2XBar()

            # Create an L1 instruction and data cache
            cpu.dcache = L1DCache()

            # Connect the instruction and data caches to the CPU
            cpu.dcache.cpu_side = cpu.port

            # Hook the CPU ports up to the l2bus
            cpu.dcache.connectBus(cpu.l2bus)

            # Create an L2 cache and connect it to the l2bus
            cpu.l2cache = L2Cache()
            cpu.l2cache.connectCPUSideBus(cpu.l2bus)

            # Connect the L2 cache to the L3 bus
            cpu.l2cache.connectMemSideBus(self.l3bus)

        self.l3cache = BankedL3Cache()
        self.l3cache.connectCPUSideBus(self.l3bus)

        # Connect the L3 cache to the membus
        self.l3cache.connectMemSideBus(self.membus)

    def setupInterrupts(self):
        for cpu in self.cpu:
            # create the interrupt controller CPU and connect to the membus
            cpu.createInterruptController()

            # For x86 only, connect interrupts to the memory
            # Note: these are directly connected to the memory bus and
            #       not cached
            cpu.interrupts[0].pio = self.membus.master
            cpu.interrupts[0].int_master = self.membus.slave
            cpu.interrupts[0].int_slave = self.membus.master

    def createMemoryControllersDDR4(self, bus):
        self._createMemoryControllers(2, DDR4_2400_x64, bu)

    def createMemoryControllersDDR3(self, bus):
        self._createMemoryControllers(2, DDR3_1600_x64, bus)

    def createMemoryControllersHBM(self, bus):
        self._createMemoryControllers(16, HBM_1000_4H_x64, bus)

    def createSimpleMemories(self, bandwidth, latency):
        memory_controller = SimpleMemory(range = self.mem_ranges,
                                         latency = latency,
                                         bandwidth = bandwidth)

        return [memory_controller]

    def createMemoryControllersIdeal(self, bus):
        # Assume 16 HBM controller + 2 DDR 4 controllers + 1 NVM controller
        # 16 * 8 + 2 * 25.6 + 20 = 200 GB/s
        # Min latency (page hit): tCL + tBURST
        self.mem_cntrls = self.createSimpleMemories('200GB/s', '20ns')
        for mc in self.mem_cntrls:
            mc.port = bus.master

    def createMemoryControllersInf(self, bus):
        self.mem_cntrls = self.createSimpleMemories('0GB/s', '1ns')
        for mc in self.mem_cntrls:
            mc.port = bus.master

    def _createMemoryControllers(self, num, cls, bus):
        ranges = self._getInterleaveRanges(self.mem_ranges[0], num, 7, 20)

        self.mem_cntrls = [
            cls(range = ranges[i],
                port = bus.master,
                channels = num)
            for i in range(num)
        ]

    def _getInterleaveRanges(self, rng, num, intlv_low_bit, xor_low_bit):
        from math import log
        bits = int(log(num, 2))
        if 2**bits != num:
            m5.fatal("Non-power of two number of memory controllers")

        intlv_bits = bits
        ranges = [
            AddrRange(start=rng.start,
                      end=rng.end,
                      intlvHighBit = intlv_low_bit + intlv_bits - 1,
                      xorHighBit = xor_low_bit + intlv_bits - 1,
                      intlvBits = intlv_bits,
                      intlvMatch = i)
                for i in range(num)
            ]

        return ranges
