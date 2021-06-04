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
    yield tgen.createLinear(1000000,    # duration
                            0,          # min_addr
                            1856,      # max_adr
                            64,         # block_size
                            1000,       # min_period
                            1000,       # max_period
                            100,        # rd_perc
                            0)          # data_limit
    yield tgen.createExit(0)


root = Root(full_system=False, system=system)

m5.instantiate()
print("35 35 35 35")
system.generator.start(createLinearTraffic(system.generator))
print("37 37 37 37")
exit_event = m5.simulate()
print("39 39 39 39")
print('maryam: ', exit_event.getCause())
