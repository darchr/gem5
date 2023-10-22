from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

args.add_argument(
    "associativity",
    type=int,
    help="associativity",
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "4GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = TrafficGen(config_file="state_machine")
system.generator.progress_check = "2ms"

system.mem_ctrl = PolicyManager(range=AddrRange('4GiB'))

system.mem_ctrl.orb_max_size = 8
system.mem_ctrl.assoc = options.associativity
system.mem_ctrl.static_frontend_latency = "10ns"
system.mem_ctrl.static_backend_latency = "10ns"

system.loc_mem_ctrl = MemCtrl()
system.loc_mem_ctrl.dram = TDRAM(range=AddrRange('4GiB'), in_addr_map=False, null=True)
system.mem_ctrl.loc_mem_policy = 'RambusTagProbOpt'

system.mem_ctrl.loc_mem = system.loc_mem_ctrl.dram
system.loc_mem_ctrl.dram.read_buffer_size = 4
system.loc_mem_ctrl.dram.write_buffer_size = 4
system.loc_mem_ctrl.static_frontend_latency = "1ns"
system.loc_mem_ctrl.static_backend_latency = "1ns"
system.loc_mem_ctrl.static_frontend_latency_tc = "0ns"
system.loc_mem_ctrl.static_backend_latency_tc = "0ns"
system.loc_mem_ctrl.consider_oldest_write = True
system.loc_mem_ctrl.oldest_write_age_threshold = 2500000
# system.loc_mem_ctrl.dram.tRLFAST = "32ns"
# system.loc_mem_ctrl.dram.tRCD_FAST = "32ns"

system.far_mem_ctrl = MemCtrl()
system.far_mem_ctrl.dram = DDR4_2400_16x4(range=AddrRange('4GiB'),in_addr_map=False, null=True)
system.far_mem_ctrl.dram.read_buffer_size = 4
system.far_mem_ctrl.dram.write_buffer_size = 4
system.far_mem_ctrl.static_frontend_latency = "1ns"
system.far_mem_ctrl.static_backend_latency = "1ns"

system.mem_ctrl.dram_cache_size = "16MiB"

system.generator.port = system.mem_ctrl.port
system.loc_mem_ctrl.port = system.mem_ctrl.loc_req_port
system.far_mem_ctrl.port = system.mem_ctrl.far_req_port

root = Root(full_system=False, system=system)

m5.instantiate()

exitSimCount = 0 

while True:
    exit_event = m5.simulate()
    print(f"Exit reason {exit_event.getCause()}")
    if exit_event.getCause().endswith("will terminate the simulation.\n") and exitSimCount == 0:
        print("here0")
        m5.stats.dump()
        m5.stats.reset()
        exitSimCount = exitSimCount + 1

    elif exit_event.getCause().endswith("will terminate the simulation.\n") and exitSimCount == 1:
        print("here1")
        break

print(f"Exit reason {exit_event.getCause()}")
print("here2")

