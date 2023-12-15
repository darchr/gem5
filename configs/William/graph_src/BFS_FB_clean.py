import m5
from m5.util import *
from m5.util.convert import *
from m5.objects import *

from caches import *

import argparse

parser = argparse.ArgumentParser()

parser.add_argument('edge_mem_type', type = str,
                    help = '''edge memory model to simulate''')

parser.add_argument('vertex_mem_type', type = str,
                    help = '''vertex memory model to simulate''')

parser.add_argument('num_edge_GiB', type = int, default = 4,
                    help = 'number of GiB in the memory system, \
                    could only be a multiple of 2 must be at least 4, e.g.  4, 6, 8, ..')

parser.add_argument('num_vertex_GiB', type = int, default = 4,
                    help = 'number of GiB in the memory system, \
                    could only be a multiple of 1 must be at least 2, e.g. 2, 3, 4, 5, ..')

parser.add_argument('num_cores', type = int, default = 3,
                    help = 'number of cores in the system, must be at least 3, must be an odd number')

parser.add_argument('graph_file', type = str, default = 'facebook_combined.txt','
                    help = 'name of the graph file to be used Examples are facebook_combined.txt , soc-Slashdot0902')


options = parser.parse_args()


num_cores = 11 # Needs to be odd number!!
edge_mem_type = "DDR4_2400_16x4"

num_edge_gib = 6
num_vertex_gib = 2

mem_size = str(num_edge_gib + num_vertex_gib) + "GiB"
# mem_size = "8GiB"

print("total mem size: " + mem_size)
print("num_edge_gib: " + str(num_edge_gib) + "GiB num_vertex_gib: " + str(num_vertex_gib) + "GiB")

# num_edge_mem_ctrls = 6
num_edge_mem_ctrls = int(num_edge_gib/2) # assumes 2 GiB per memory controller
num_vertex_mem_ctrls = int(num_vertex_gib/1) # assumes 1 GiB per memory controller

EL_vaddr = 0x200000000 # Make sure this is the same as in consumer.cpp, counter.cpp, and generator.cpp
EL_paddr = 2 << 31# 1 << 31 # 2147483648 2GiB   # need to update in graph_init
VL_vaddr = 0x300000000
VL_paddr = num_edge_gib * gibi # 6GiB 11 << 31 Change this to be vertex memory base
# VL_paddr = 6442450944 # 6GiB 11 << 31 Change this to be vertex memory base

vertex_mem_type = "ddr" #"HBM_2000_4H_1x64"
num_vertex_mem_ctrls = 2


initialization_vaddr = 0x400000000
initialization_paddr = (1 << 25)#1 << 32 # 4GiB
finished_vaddr = 0x500000000 # 101 << 32
finished_paddr = (1 << 25) + 4096 # 
active_list_vaddr = 0x600000000
active_list_paddr = 1 << 31 #7516192768 # 7GiB  111 << 30
finished_flag_vaddr = 0x510000000
finished_flag_paddr = (1 << 25) - 4096 # 1GiB - 4KiB
msg_queue_vaddr = 0x100000000
msg_queue_paddr = 1 << 33 # 8GiB





active_list_base = toInteger(mem_size, "bytes", "GiB") * gibi + toInteger("1GiB", "bytes", "GiB") * gibi # must always be higher than mem_size
mq_range_base = toInteger(mem_size, "bytes", "GiB") * gibi#, metric_prefixes)#, int)

# each msg_queue gets a 4KiB range so that it has its own page
mq_ranges = [AddrRange(start=str(mq_range_base + (4096*i)) + "B", size="4KiB") for i in range(int(num_cores/2))] 

# each active list gets 64 KiB range which is used as a circular buffer
active_list_ranges = [AddrRange(start=str(active_list_base + (65536*i)) + "B", size="64KiB") for i in range(int(num_cores/2))]


num_mem_ctrls = num_edge_mem_ctrls + num_vertex_mem_ctrls
mem_range_base = 0
# mem_ctrl_size = int((toInteger(mem_size, "bytes", "GiB")*gibi) / num_mem_ctrls) # 2147483648  8589934592
edge_ctrl_size = int((num_edge_gib*gibi) / num_edge_mem_ctrls) # 2147483648  8589934592
vertex_ctrl_size = int((num_vertex_gib*gibi) / num_vertex_mem_ctrls) # can use this to change sizes of edge memory vs vertex memory

# vertex_ctrl_size = int((toInteger(mem_size, "bytes", "GiB")*gibi) / num_mem_ctrls) # can use this to change sizes of edge memory vs vertex memory

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange(mem_size)]

system.membus = SystemXBar()

system.cpu = [X86TimingSimpleCPU() for i in range(num_cores)]

system.graphInitializer = GraphInit(graph_file="facebook_combined.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)

# need to edit memory ranges
system.test_xbar = [IOXBar() for i in range(num_cores)] # xbar for each core between dcache and core
system.mem_bridge = [Bridge(delay='1ns', ranges=system.mem_ranges[0]) for i in range(num_cores)] # bridge for each core between dcache and membus

# bridge for each core between dcache and membus
system.xbar2xbar_bridge = [Bridge(delay='1ns', ranges=AddrRange(start=mem_size, size=str((2147483648) + (4096 * int(num_cores/2))) + "B")) for i in range(num_cores)] 
system.mq_xbar = IOXBar() # xbar for all message queues

system.mq_bridge =[Bridge(delay='1ns', ranges=AddrRange(start=mem_size, size="4KiB")) for i in range(int(num_cores/2))] # needs adjustig
mq_range = mq_range_base
for bridge in system.mq_bridge:
    bridge.ranges = AddrRange(start=str(mq_range), size="4KiB")
    mq_range += 4096

system.icache = [L1ICache() for i in range(num_cores)]
system.dcache = [L1DCache() for i in range(num_cores)]

for j in range(num_cores):
    system.cpu[j].icache_port = system.icache[j].cpu_side
    system.icache[j].mem_side = system.membus.cpu_side_ports
    system.cpu[j].dcache_port = system.test_xbar[j].cpu_side_ports

    system.test_xbar[j].mem_side_ports = system.mem_bridge[j].cpu_side_port
    system.test_xbar[j].mem_side_ports = system.xbar2xbar_bridge[j].cpu_side_port

    system.xbar2xbar_bridge[j].mem_side_port = system.mq_xbar.cpu_side_ports

    system.mem_bridge[j].mem_side_port = system.dcache[j].cpu_side
    system.dcache[j].mem_side = system.membus.cpu_side_ports


for j in range(int(num_cores/2)): # num_cores/2 is the number of consumers
    system.mq_bridge[j].cpu_side_port = system.mq_xbar.mem_side_ports
    
for j in range(num_cores):
    system.cpu[j].createInterruptController()
    # Note: Next 3 lines are x86 specific
    system.cpu[j].interrupts[0].pio = system.membus.mem_side_ports
    system.cpu[j].interrupts[0].int_requestor = system.membus.cpu_side_ports
    system.cpu[j].interrupts[0].int_responder = system.membus.mem_side_ports


system.msg_queues = [MessageQueue(myRange=AddrRange(start=str(0x200000000 + (i*4096)) + "B", size="4KiB"), queueSize=64000) for i in range(int(num_cores/2))]


for j in range(int(num_cores/2)):
    system.msg_queues[j].cpu_side = system.mq_bridge[j].mem_side_port
    system.msg_queues[j].myRange = AddrRange(start=str(mq_range_base + (4096*j)) + "B", size="4KiB")

system.system_port = system.membus.cpu_side_ports

system.graphInitializer.mirrors_map_mem = system.membus.cpu_side_ports


system.mem_ctrls = [MemCtrl() for i in range(num_mem_ctrls)]

for i in range(num_edge_mem_ctrls):
    system.mem_ctrls[i].dram = DDR4_2400_16x4()
    # system.mem_ctrls[i].dram.range = AddrRange(start=str(mem_range_base) + "B", size=str(mem_ctrl_size) + "B")
    system.mem_ctrls[i].dram.range = AddrRange(start=str(mem_range_base) + "B", size=str(edge_ctrl_size) + "B")
    system.mem_ctrls[i].port = system.membus.mem_side_ports
    
    print("Edge " + str(i) + " mem start: " + str(mem_range_base) + "B")
    mem_range_base += edge_ctrl_size
    



# Vertex Memory Controllers
for i in range(num_edge_mem_ctrls, num_mem_ctrls):
    if(vertex_mem_type == "HBM_2000_4H_1x64"):
        system.mem_ctrls[i].dram = HBM_2000_4H_1x64()
    else:
        system.mem_ctrls[i].dram = DDR4_2400_16x4()
    # system.mem_ctrls[i].dram.range = AddrRange(start=str(mem_range_base) + "B", size=str(mem_ctrl_size) + "B")
    system.mem_ctrls[i].dram.range = AddrRange(start=str(mem_range_base) + "B", size=str(vertex_ctrl_size) + "B")
    system.mem_ctrls[i].port = system.membus.mem_side_ports
    print("Vertex " + str(i) +  " mem start: " + str(mem_range_base) + "B")
    mem_range_base += vertex_ctrl_size


binary = "configs/William/graph_src/counter"
binary2 = "configs/William/graph_src/consumer"
binary3 = "configs/William/graph_src/generator"

system.workload = SEWorkload.init_compatible(binary)
system.workload2 = SEWorkload.init_compatible(binary2)
system.workload3 = SEWorkload.init_compatible(binary3)

process = [Process() for i in range(num_cores)]
process[0].cmd = [binary, str(num_cores - 1)] # should always be true

for i in range(1, int(num_cores/2) + 1):
    process[(2*i) - 1].cmd = [binary2, str(i - 1), str(int(num_cores/2))]
    process[(2*i) - 1].pid = 101 + i
    process[2*i].cmd = [binary3, str(i - 1), str(int(num_cores/2))]
    process[2*i].pid = 301 + i


for i in range(num_cores):
    system.cpu[i].workload = process[i]
    system.cpu[i].createThreads()



root = Root(full_system=False, system=system)
m5.instantiate()

for my_process in process:

    my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=4096 * 1024*16*2 , cacheable=True) # EL mapping  changing paddr to 1<<30 broke it
    # my_process.map(vaddr=0x300000000, paddr=(1 << 27) , size=4096 * 512 * 2, cacheable=True) # VL mapping
    # my_process.map(vaddr=0x300000000, paddr= 1 << 31 , size=4096 * 512 * 2, cacheable=True) # VL mapping
    my_process.map(vaddr=VL_vaddr, paddr=VL_paddr, size=4096 * 512 * 2, cacheable=True) # VL mapping
    # my_process.map(vaddr=0x400000000, paddr=(1 << 25), size=4096, cacheable=False) #  initialized variable mapping working
    my_process.map(vaddr=initialization_vaddr, paddr=initialization_paddr, size=4096, cacheable=False) #  initialized variable mapping

    my_process.map(vaddr=finished_vaddr, paddr=finished_paddr, size=4096, cacheable=False) #  Mapping of finished variabel
    # my_process.map(vaddr=0x600000000, paddr=(1 << 25) + 8192, size=65536 * int(num_cores/2), cacheable=True) #  ActiveList addr mapping
    my_process.map(vaddr=active_list_vaddr, paddr=active_list_paddr, size=65536 * int(num_cores/2), cacheable=True) #  ActiveList addr mapping shouldn't be cacheable?
    # my_process.map(vaddr=0x600000000, paddr=active_list_base, size=65536 * int(num_cores/2), cacheable=True) #  ActiveList addr mapping shouldn't be cacheable?
    my_process.map(vaddr=finished_flag_vaddr, paddr=finished_flag_paddr, size=4096, cacheable=False) #  Mapping of finished flag
    my_process.map(vaddr=msg_queue_vaddr, paddr=msg_queue_paddr, size=4096 * int(num_cores/2), cacheable=True) # 1<<33 is 8Gib, msg queue mapping

print("Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ {m5.curTick()} because {exit_event.getCause()}")