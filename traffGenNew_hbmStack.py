from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.NVMInterface import *

args = argparse.ArgumentParser()

# build/NULL/gem5.opt --outdir=m5outHBMstack   traffGenNew_hbmStack.py  200  random  10000000000  12000   100
args.add_argument(
    "xbarLatency",
    type = int,
    help = "latency of crossbar front/resp"
)

args.add_argument(
    "traffic_mode",
    type = str,
    help = "Traffic type to use"
)

args.add_argument(
    "duration",
    type = int,
    help = "Duration of simulation"
)

args.add_argument(
    "inj_period",
    type = int,
    help = "Period to inject reqs"
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


system.membus0 = SystemXBar()
system.membus0.max_routing_table_size = 900000
system.membus1 = SystemXBar()
system.membus1.frontend_latency = options.xbarLatency
system.membus1.response_latency  = options.xbarLatency
system.membus1.max_routing_table_size = 900000

system.generator.port = system.membus0.cpu_side_ports

loc_ranges = ['0', '4GB', '8GB', '12GB', '16GB', '20GB', '24GB', '28GB', '32GB', '36GB', '40GB', '44GB', '48GB', '52GB', '56GB', '60GB', '64GB']

system.dcache_ctrl = [DCacheCtrl() for i in range(16)]

for i in range (0,16):
    #system.dcache_ctrl[i] = DCacheCtrl()
    system.dcache_ctrl[i].dram = HBM_1000_4H_1x128(range=AddrRange(start = loc_ranges[i], end = loc_ranges[i+1]), in_addr_map=False)
    system.dcache_ctrl[i].dram_cache_size = '128MiB'
    system.dcache_ctrl[i].always_hit = True
    system.dcache_ctrl[i].always_dirty = False
    system.membus0.mem_side_ports = system.dcache_ctrl[i].port
    system.membus1.cpu_side_ports = system.dcache_ctrl[i].req_port

far_ranges = ['0', '32GB', '64GB']

system.farMem_ctrl = [MemCtrl() for i in range(2)]

for i in range (0,2):
    #system.farMem_ctrl[i] = MemCtrl()
    system.farMem_ctrl[i].dram = DDR4_2400_16x4(range=AddrRange(start = far_ranges[i], end = far_ranges[i+1]))
    system.membus1.mem_side_ports = system.farMem_ctrl[i].port


def createRandomTraffic(tgen):
    yield tgen.createRandom(options.duration,         # duration
                            0,                        # min_addr
                            AddrRange('64GB').end,  # max_adr
                            64,                       # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)                        # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(options.duration,         # duration
                            0,                        # min_addr
                            AddrRange('64GB').end,  # max_adr
                            64,                       # block_size
                            options.inj_period,       # min_period
                            options.inj_period,       # max_period
                            options.rd_prct,          # rd_perc
                            0)             # data_limit
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