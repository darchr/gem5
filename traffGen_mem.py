from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

args.add_argument(
    "traffic_mode",
    type = str,
    help = "Traffic type to use"
)

args.add_argument(
    "rd_prct",
    type=int,
    help="Read Percentage",
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = TDRAM_32(range=AddrRange('3GiB'))
system.mem_ctrl.dram.activation_limit = 8
system.mem_ctrl.dram.addr_mapping = 'RoCoRaBaCh'

# DDR4 Alloy
# system.mem_ctrl.dram =  DDR4_2400_16x4(range=AddrRange('3GiB'))
# system.mem_ctrl.dram.burst_length = 10
# system.mem_ctrl.dram.tBURST = "4.165ns"
# system.mem_ctrl.dram.tCCD_L = "5ns" "6.25ns"
# system.mem_ctrl.dram.is_alloy = True

# HBM2 1 PC Alloy
# system.mem_ctrl.dram = HBM_2000_4H_1x64(range=AddrRange('3GiB'))
# system.mem_ctrl.dram.burst_length = 10
# system.mem_ctrl.dram.tBURST = "5ns"
# #system.mem_ctrl.dram.tCCD_L = "7ns"
# system.mem_ctrl.dram.is_alloy = True


system.generator.port = system.mem_ctrl.port

def createRandomTraffic(tgen):
    yield tgen.createRandom(10000000000,            # duration
                            0,                      # min_addr
                            AddrRange('3GiB').end,  # max_adr
                            64,                     # block_size
                            1000,                   # min_period
                            1000,                   # max_period
                            options.rd_prct,        # rd_perc
                            0)                      # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(10000000000,            # duration
                            0,                      # min_addr
                            AddrRange('3GiB').end,  # max_adr
                            64,                     # block_size
                            1000,                   # min_period
                            1000,                   # max_period
                            options.rd_prct,        # rd_perc
                            0)                      # data_limit
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
print(f"Exit reason {exit_event.getCause()}")