
import m5
from m5.objects import *

class Memory(SubSystem):

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


    def connectCPU(self, cpu):
        cpu.icache_port = self.membus.slave
        cpu.dcache_port = self.membus.slave

        # create the interrupt controller for the CPU and connect to the membus
        cpu.createInterruptController()
        if m5.defines.buildEnv['TARGET_ISA'] == "x86":
            cpu.interrupts[0].pio = self.membus.master
            cpu.interrupts[0].int_master = self.membus.slave
            cpu.interrupts[0].int_slave = self.membus.master

    def connectSysPort(self, system):
        # Connect the system up to the membus
        system.system_port = self.membus.slave

    def willHotSwap(self, other):
        self.membus.willHotSwap(other.membus)
        for ctrl in other.mem_ctrls:
            ctrl.unplugged = True
            ctrl.mirror_memory = True
        other.internalConnect()

    def unplug(self):
        self.membus.unplug()

    def plugIn(self, old):
        self.membus.plugIn(old.membus)

    def internalConnect(self):
        for ctrl in self.mem_ctrls:
            self.membus.master = ctrl.port

class DDR3(Memory):
    def __init__(self, mem_ranges):
        super(DDR3, self).__init__()

        self.membus = SystemXBar()

        # Create a DDR3 memory controller and connect it to the membus
        rng = self._getInterleaveRanges(mem_ranges[0], 2, 7, 20)
        self.mem_ctrls = [DDR3_1600_8x8(channels = 2) for i in range(2)]
        self.mem_ctrls[0].range = rng[0]
        self.mem_ctrls[1] = DDR3_1600_8x8(channels = 2)
        self.mem_ctrls[1].range = rng[1]

class DDR4(Memory):
    def __init__(self, mem_ranges):
        super(DDR4, self).__init__()

        self.membus = SystemXBar()

        self.mem_ctrls = [DDR4_2400_16x4()]
        self.mem_ctrls[0].range = mem_ranges[0]

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.cpu = TimingSimpleCPU()

system.mem = DDR3(system.mem_ranges)
system.mem.connectSysPort(system)
system.mem.connectCPU(system.cpu)

system.mem2 = DDR4(system.mem_ranges)

system.mem.willHotSwap(system.mem2)

system.mem.internalConnect()


# Create a process for a simple "Hello World" application
process = Process()
# Set the command
isa = str(m5.defines.buildEnv['TARGET_ISA']).lower()
process.cmd = ['tests/test-progs/hello/bin/' + isa + '/linux/hello']
# Set the cpu to use the process as its workload and create thread contexts
system.cpu.workload = process
system.cpu.createThreads()

# set up the root SimObject and start the simulation
root = Root(full_system = False, system = system)
# instantiate all of the objects we've created above
m5.instantiate()
print "successfully initialized"

system.mem.unplug()
print "successfully disconnected the ports"

system.mem2.plugIn(system.mem)
print "successfully reconnected the ports"

m5.simulate(100000)
print "successfully simulated mem2"

m5.drain()

system.mem2.unplug()
print "successfully disconnected the ports"

system.mem.plugIn(system.mem2)
print "successfully reconnected the ports"

m5.simulate()
print "successfully simulated mem"
