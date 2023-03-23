from m5.objects import *
import m5
import argparse
from m5.objects.DRAMInterface import *
from m5.objects.DRAMAlloyInterface import *
from m5.objects.NVMInterface import *
from math import log

def interleave_addresses(dram_class, num_chnl, intlv_size, start, size):
    # if dram_class.addr_mapping == "RoRaBaChCo":
    #     rowbuffer_size = (
    #         dram_class.device_rowbuffer_size.value
    #         * dram_class.devices_per_rank.value
    #     )
    #     intlv_low_bit = log(rowbuffer_size, 2)
    # elif dram_class.addr_mapping in ["RoRaBaCoCh", "RoCoRaBaCh"]:
    #     intlv_low_bit = log(intlv_size, 2)
    # else:
    #     raise ValueError(
    #         "Only these address mappings are supported: "
    #         "RoRaBaChCo, RoRaBaCoCh, RoCoRaBaCh"
    #     )

    # assert dram_class.addr_mapping == 'RoRaBaCoCh'

    intlv_low_bit = log(intlv_size, 2)
    intlv_bits = log(num_chnl, 2)
    mask_list = []

    for ib in range(int(intlv_bits)):
        mask_list.append(1 << int(ib + intlv_low_bit))

    # for interleaving across pseudo channels (at 64B currently)
    mask_list.insert(0, 1 << 6)
    ret_list = []
    for i in range(num_chnl):
        ret_list.append(AddrRange(
            start=start,
            size=size,
            masks=mask_list,
            intlvMatch=(i << 1) | 0,
        ))
        ret_list.append(AddrRange(
            start=start,
            size=size,
            masks=mask_list,
            intlvMatch=(i << 1) | 1,
        ))
    return ret_list

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
    "hit",
    type=int,
    help="Read Percentage",
)

args.add_argument(
    "dirty",
    type=int,
    help="Read Percentage",
)

options = args.parse_args()

system = System()
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "5GHz"
system.clk_domain.voltage_domain = VoltageDomain()
system.mem_mode = 'timing'

system.generator = PyTrafficGen()

system.mem_ctrl = PolicyManager(range=AddrRange('3GiB'))
system.mem_ctrl.tRP = '14.16ns'
system.mem_ctrl.tRCD_RD = '14.16ns'
system.mem_ctrl.tRL = '14.16ns'

system.membusPolManLocMem = L2XBar(width=64)
# system.membusPolManLocMem.frontend_latency = options.xbarLatency
# system.membusPolManLocMem.response_latency  = options.xbarLatency
# system.membusPolManLocMem.max_routing_table_size = 900000
system.membusPolManLocMem.cpu_side_ports = system.mem_ctrl.loc_req_port


# loc_ranges = ['0', '384MiB', '768MiB', '1152MiB', '1536MiB', '1920MiB', '2304MiB', '2688MiB', '3072MiB']

system.loc_mem_ctrlrs = [HBMCtrl() for i in range(8)]

loc_ranges = interleave_addresses(HBM_2000_4H_1x64, 8, 128, 0, '3GiB')

for i in range (0,8):
    system.loc_mem_ctrlrs[i].dram = HBM_2000_4H_1x64(range=loc_ranges[2*i], in_addr_map=False, null=True)
    system.loc_mem_ctrlrs[i].dram_2 = HBM_2000_4H_1x64(range=loc_ranges[2*i+1], in_addr_map=False, null=True)
    system.loc_mem_ctrlrs[i].port = system.membusPolManLocMem.mem_side_ports
    # system.loc_mem_ctrlrs.dram.read_buffer_size = 4
    # system.loc_mem_ctrlrs.dram.write_buffer_size = 4
    # system.loc_mem_ctrlrs.dram_2.read_buffer_size = 4
    # system.loc_mem_ctrlrs.dram_2.write_buffer_size = 4

# main memory
system.far_mem_ctrl = MemCtrl()
system.far_mem_ctrl.dram = DDR4_2400_16x4(range=AddrRange('3GiB'), in_addr_map=False, null=True)
#system.far_mem_ctrl.port = system.mem_ctrl.far_req_port
system.membusPolManFarMem = L2XBar(width=64)
system.membusPolManFarMem.cpu_side_ports = system.mem_ctrl.far_req_port
system.membusPolManFarMem.mem_side_ports = system.far_mem_ctrl.port
system.far_mem_ctrl.dram.read_buffer_size = 128
system.far_mem_ctrl.dram.write_buffer_size = 128

system.mem_ctrl.orb_max_size = 256
system.mem_ctrl.dram_cache_size = "128MiB"

if options.hit==1 :
    system.mem_ctrl.always_hit = True
else:
    system.mem_ctrl.always_hit = False

if options.dirty==1 :
    system.mem_ctrl.always_dirty = True
else:
    system.mem_ctrl.always_dirty = False

system.generator.port = system.mem_ctrl.port

def createRandomTraffic(tgen):
    yield tgen.createRandom(10000000000,            # duration
                            0,                      # min_addr
                            AddrRange('3GiB').end,  # max_adr
                            64,                     # block_size
                            10,                   # min_period
                            10,                   # max_period
                            options.rd_prct,        # rd_perc
                            0)                      # data_limit
    yield tgen.createExit(0)

def createLinearTraffic(tgen):
    yield tgen.createLinear(2000000000,            # duration
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


