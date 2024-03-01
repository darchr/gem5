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

args.add_argument(
    "end_address",
    type=str,
    help="end address",
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = PolicyManager(range=AddrRange('3GiB'))

system.mem_ctrl.orb_max_size = 128
system.mem_ctrl.static_frontend_latency = "10ns"
system.mem_ctrl.static_backend_latency = "10ns"
#system.mem_ctrl.bypass_dcache = True

system.loc_mem_ctrl = MemCtrl()
#system.loc_mem_ctrl.consider_oldest_write= True
system.loc_mem_ctrl.dram = TDRAM(range=AddrRange('3GiB'), in_addr_map=False, null=True)
system.mem_ctrl.loc_mem_policy = 'Rambus'

# system.loc_mem_ctrl = MemCtrl()
# #system.loc_mem_ctrl.consider_oldest_write= True
# system.loc_mem_ctrl.dram = TDRAM_32(range=AddrRange('3GiB'), in_addr_map=False, null=True)
# system.loc_mem_ctrl.dram.activation_limit = 8
# system.loc_mem_ctrl.dram.page_policy = "close_adaptive"
# system.mem_ctrl.loc_mem_policy = 'Rambus'
# system.loc_mem_ctrl.dram.read_buffer_size = 64
# system.loc_mem_ctrl.dram.write_buffer_size = 64

# system.loc_mem_ctrl = HBMCtrl()
# system.loc_mem_ctrl.dram =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '3GiB', masks = [1 << 6], intlvMatch = 0), in_addr_map=False, kvm_map=False, null=True)
# system.loc_mem_ctrl.dram_2 =  HBM_2000_4H_1x64(range=AddrRange(start = '0', end = '3GiB', masks = [1 << 6], intlvMatch = 1), in_addr_map=False, kvm_map=False, null=True)
# system.mem_ctrl.loc_mem_policy = 'CascadeLakeNoPartWrs'
# system.loc_mem_ctrl.dram.read_buffer_size = 32
# system.loc_mem_ctrl.dram.write_buffer_size = 32
# system.loc_mem_ctrl.dram_2.read_buffer_size = 32
# system.loc_mem_ctrl.dram_2.write_buffer_size = 32

system.mem_ctrl.loc_mem = system.loc_mem_ctrl.dram
system.loc_mem_ctrl.static_frontend_latency = "2ns"
system.loc_mem_ctrl.static_backend_latency = "2ns"
system.loc_mem_ctrl.static_frontend_latency_tc = "1ns"
system.loc_mem_ctrl.static_backend_latency_tc = "1ns"

system.far_mem_ctrl = MemCtrl()
system.far_mem_ctrl.dram = DDR4_2400_16x4(range=AddrRange('3GiB'),in_addr_map=False, null=True)
system.far_mem_ctrl.dram.read_buffer_size = 64
system.far_mem_ctrl.dram.write_buffer_size = 64
system.far_mem_ctrl.static_frontend_latency = "2ns"
system.far_mem_ctrl.static_backend_latency = "2ns"

system.mem_ctrl.dram_cache_size = "1GiB"

system.generator.port = system.mem_ctrl.port
system.loc_mem_ctrl.port = system.mem_ctrl.loc_req_port
system.far_mem_ctrl.port = system.mem_ctrl.far_req_port

def createRandomTraffic(tgen):
    yield tgen.createRandom(10000000000,            # duration
                            0,                      # min_addr
                            AddrRange(options.end_address).end,  # max_adr
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

# for testing checkpointing
# exit_event = m5.simulate(1000000000)
# print(f"Exit reason {exit_event.getCause()}")

# # print("Draining")
# # m5.drain()
# # print("Done draining!")
# m5.stats.dump()
# m5.checkpoint(m5.options.outdir + '/cpt-test')
# m5.stats.reset()

# system.generator.start(createLinearTraffic(system.generator))
# exit_event = m5.simulate()
# print(f"Exit reason {exit_event.getCause()}")
