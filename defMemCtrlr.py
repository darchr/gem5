from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

args.add_argument(
    "device",
    type = str,
    help = "Memory device to use as a dram cache (local memory)"
)

args.add_argument(
    "traffic_mode",
    type = str,
    help = "Traffic type to use"
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = eval(options.device)(range=AddrRange('8GB'))

system.mem_ranges = [AddrRange('8GB')]

system.membus = SystemXBar()
system.membus.cpu_side_ports = system.generator.port
system.mem_ctrl.port = system.membus.mem_side_ports
system.membus.frontend_latency = 100 #options.xbarLatency
system.membus.response_latency  = 100 #options.xbarLatency

def createRandomTraffic(tgen):
    yield tgen.createRandom(1000000000,   # duration
                            0,              # min_addr
                            AddrRange("1GB").end,  # max_adr
                            64,             # block_size
                            1000,       # min_period
                            1000,       # max_period
                            0,          # rd_perc
                            0)              # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(0,   # duration
                            0,              # min_addr
                            AddrRange("1GB").end,  # max_adr
                            64,             # block_size
                            1000,   # min_period
                            1000,   # max_period
                            100,       # rd_perc
                            104857600)              # data_limit
    yield tgen.createExit(0)


root = Root(full_system=False, system=system)

m5.instantiate()

if options.traffic_mode == 'linear':
    system.generator.start(createLinearTraffic(system.generator))
elif options.traffic_mode == 'random':
    system.generator.start(createRandomTraffic(system.generator))
else:
    print('Wrong traffic type! Exiting!')
    exit()

exit_event = m5.simulate()