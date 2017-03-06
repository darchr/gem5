
import m5
from m5.objects import *
from m5.util import convert
from m5.util import convert
import x86
from fs_tools import *
from caches import *
from cpu import HighPerformanceCPU

SimpleOpts.add_option("--no_host_parallel", default=False,
                action="store_true",
                help="Do NOT run gem5 on multiple host threads (kvm only)")

class MySystem(LinuxX86System):

    SimpleOpts.add_option("--mem_type", default="ddr3",
                 help="Type of memory to simulate. " +
                      "[ddr3, ddr4, hbm, ideal, inf]")

    SimpleOpts.add_option("--cpus", default=8, type="int",
                          help="Number of CPUs in the system")

    _infrastructure = '/p/multifacet/infrastructure/'
    SimpleOpts.add_option("--second_disk",
      default=_infrastructure + 'gem5_system_files/disks/linux-bigswap2.img',
      help="The second disk image to mount (/dev/hdb)")

    def __init__(self, opts, no_kvm=False):
        super(MySystem, self).__init__()
        self._opts = opts
        self._no_kvm = no_kvm

        self._host_parallel = not self._opts.no_host_parallel

        # Set up the clock domain and the voltage domain
        self.clk_domain = SrcClockDomain()
        self.clk_domain.clock = '3GHz'
        self.clk_domain.voltage_domain = VoltageDomain()

        mem_size = '8GB'
        self.mem_ranges = [AddrRange('100MB'), # For kernel
                           AddrRange(0xC0000000, size=0x100000), # For I/0
                           AddrRange(Addr('4GB'), size = mem_size) # All data
                           ]

        # Create the main memory bus
        # This connects to main memory
        self.membus = SystemXBar(width = 64) # 64-byte width
        self.membus.badaddr_responder = BadAddr()
        self.membus.default = Self.badaddr_responder.pio

        # Set up the system port for functional access from the simulator
        self.system_port = self.membus.slave

        x86.init_fs(self, self.membus, self._opts.cpus)

        # Replace these paths with the path to your disk images.
        # The first disk is the root disk. The second could be used for swap
        # or anything else.
        self.setDiskImages('gem5_system_files/disks/ubuntu-12.04.img',
                            opts.second_disk)

        # Change this path to point to the kernel you want to use
        self.kernel = 'gem5_system_files/binaries/x86_64-vmlinux-3.0.68.smp'

        # Options specified on the kernel command line
        boot_options = ['earlyprintk=ttyS0', 'console=ttyS0', 'lpj=7999923',
                        'root=/dev/hda1']
        self.boot_osflags = ' '.join(boot_options)

        # Create the CPUs for our system.
        self.createCPU()

        # Create the cache heirarchy for the system.
        self.createCacheHierarchy()

        # Set up the interrupt controllers for the system (x86 specific)
        self.setupInterrupts()

        # I'm interested in the performance of different memory technologies,
        # so select the technoology by a command line flag.
        if self._opts.mem_type == "ddr3":
            self.createMemoryControllersDDR3()
        elif self._opts.mem_type == "ddr4":
            self.createMemoryControllersDDR4()
        elif self._opts.mem_type == "hbm":
            self.createMemoryControllersHBM()
        elif self._opts.mem_type == "ideal":
            self.createMemoryControllersIdeal()
        elif self._opts.mem_type == "inf":
            self.createMemoryControllersInf()
        else:
            fatal("Invalid memory type: %s." % opts.mem_type)

        if self._host_parallel:
            # To get the KVM CPUs to run on different host CPUs
            # Specify a different event queue for each CPU
            for i,cpu in enumerate(self.cpu):
                for obj in cpu.descendants():
                    obj.eventq_index = 0
                cpu.eventq_index = i + 1

    def getHostParallel(self):
        return self._host_parallel

    def totalInsts(self):
        return sum([cpu.totalInsts() for cpu in self.cpu])

    def createCPU(self):
        if self._no_kvm:
            self.cpu = [AtomicSimpleCPU(cpu_id = i, switched_out = False)
                              for i in range(self._opts.cpus)]
        else:
            # Note KVM needs a VM and atomic_noncaching
            self.cpu = [X86KvmCPU(cpu_id = i)
                        for i in range(self._opts.cpus)]
            self.vm = KvmVM()
            self.mem_mode = 'atomic_noncaching'

            self.atomicCpu = [AtomicSimpleCPU(cpu_id = i,
                                              switched_out = True)
                              for i in range(self._opts.cpus)]

        self.timingCpu = [HighPerformanceCPU(cpu_id = i,
                                             switched_out = True)
                   for i in range(self._opts.cpus)]

    def switchCpus(self, old, new):
        assert(new[0].switchedOut())
        m5.switchCpus(self, zip(old, new))

    def setDiskImages(self, img_path_1, img_path_2):
        disk0 = CowDisk(img_path_1)
        disk2 = CowDisk(img_path_2)
        self.pc.south_bridge.ide.disks = [disk0, disk2]

    def createCacheHierarchy(self):
        # Create an L3 cache (with crossbar)
        self.l3bus = L2XBar(width = 64,
                            snoop_filter = SnoopFilter(max_capacity='32MB'))

        for cpu in self.cpu:
            # Create a memory bus, a coherent crossbar, in this case
            cpu.l2bus = L2XBar()

            # Create an L1 instruction and data cache
            cpu.icache = L1ICache(self._opts)
            cpu.dcache = L1DCache(self._opts)
            cpu.mmucache = MMUCache()

            # Connect the instruction and data caches to the CPU
            cpu.icache.connectCPU(cpu)
            cpu.dcache.connectCPU(cpu)
            cpu.mmucache.connectCPU(cpu)

            # Hook the CPU ports up to the l2bus
            cpu.icache.connectBus(cpu.l2bus)
            cpu.dcache.connectBus(cpu.l2bus)
            cpu.mmucache.connectBus(cpu.l2bus)

            # Create an L2 cache and connect it to the l2bus
            cpu.l2cache = L2Cache(self._opts)
            cpu.l2cache.connectCPUSideBus(cpu.l2bus)

            # Connect the L2 cache to the L3 bus
            cpu.l2cache.connectMemSideBus(self.l3bus)

        self.l3cache = BankedL3Cache(self._opts)
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

    def createMemoryControllersDDR4(self):
        self._createMemoryControllers(2, DDR4_2400_x64)

    def createMemoryControllersDDR3(self):
        self._createMemoryControllers(2, DDR3_1600_x64)

    def createMemoryControllersHBM(self):
        self._createMemoryControllers(16, HBM_1000_4H_x64)

    def createSimpleMemories(self, bandwidth, latency):
        kernel_controller = SimpleMemory(range = self.mem_ranges[0],
                                         latency = latency,
                                         bandwidth = bandwidth)

        memory_controller = SimpleMemory(range = self.mem_ranges[-1],
                                         latency = latency,
                                         bandwidth = bandwidth)

        return [kernel_controller, memory_controller]

    def createMemoryControllersIdeal(self):
        # Assume 16 HBM controller + 2 DDR 4 controllers + 1 NVM controller
        # 16 * 8 + 2 * 25.6 + 20 = 200 GB/s
        # Min latency (page hit): tCL + tBURST
        self.mem_cntrls = self.createSimpleMemories('200GB/s', '20ns')
        for mc in self.mem_cntrls:
            mc.port = self.membus.master

    def createMemoryControllersInf(self):
        self.mem_cntrls = self.createSimpleMemories('0GB/s', '1ns')
        for mc in self.mem_cntrls:
            mc.port = self.membus.master

    def _createMemoryControllers(self, num, cls):
        kernel_controller = self._createKernelMemoryController(cls)

        ranges = self._getInterleaveRanges(self.mem_ranges[-1], num, 7, 20)

        self.random_bridges = [RandomDelayBridge() for i in range(num)]
        for bridge in self.random_bridges:
            bridge.slave = self.membus.master

        self.mem_cntrls = [
            cls(range = ranges[i],
                port = self.random_bridges[i].master,
                channels = num)
            for i in range(num)
        ] + [kernel_controller]

    def _createKernelMemoryController(self, cls):
        return cls(range = self.mem_ranges[0],
                   port = self.membus.master,
                   channels = 1)

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
