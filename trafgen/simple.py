from m5.objects import *
import m5

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8(range=AddrRange('1GB'), in_addr_map=False)
system.mem_ctrl.nvm = NVM_2400_1x64(range=AddrRange('1GB'))

system.mem_ranges = [AddrRange('1GB')]

system.generator.port = system.mem_ctrl.port

def createLinearTraffic(tgen):
    yield tgen.createLinear(1000000,
                            0,
                            1024,
                            64,
                            1000,
                            1000,
                            100, 0)
    yield tgen.createExit(0)


root = Root(full_system=False, system=system)

m5.instantiate()

system.generator.start(createLinearTraffic(system.generator))

m5.simulate()
