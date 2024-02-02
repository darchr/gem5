import m5
from m5.util import *
from m5.util.convert import *
from m5.objects import *

from caches import *

import argparse

parser = argparse.ArgumentParser()

parser.add_argument('clock_speed', type = str,
                    help = '''clock speed''')

parser.add_argument('workload', type = str,
                    help = '''short for short twitter, long for long twitter''')

# parser.add_argument('edge_mem_type', type = str,
#                     help = '''edge memory model to simulate''')

# parser.add_argument('vertex_mem_type', type = str,
#                     help = '''vertex memory model to simulate''')

parser.add_argument('num_edge_GiB', type = int, default = 8,
                    help = 'number of GiB in the memory system, \
                    could only be a multiple of 2 must be at least 4, e.g.  4, 6, 8, ..')

parser.add_argument('num_vertex_GiB', type = int, default = 2,
                    help = 'number of GiB in the memory system, \
                    could only be a multiple of 1 must be at least 2, e.g. 2, 3, 4, 5, ..')

parser.add_argument('batch', type = int, default = 0,
                    help = '''does the consumer read from the message queue in batches? true is yes''')

parser.add_argument('cpu_type', type = str, default = "timing",
                    help = '''timing or O3''')
# parser.add_argument('num_cores', type = int, default = 3,
#                     help = 'number of cores in the system, must be at least 3, must be an odd number')

# parser.add_argument('graph_file', type = str, default = 'facebook_combined.txt','
#                     help = 'name of the graph file to be used Examples are facebook_combined.txt , soc-Slashdot0902')


options = parser.parse_args()


num_cores = 17 # Needs to be odd number!!
edge_mem_type = "DDR4_2400_16x4"

vertex_mem_type = "HBM_2000_4H_1x64" # "DDR"

num_edge_gib = options.num_edge_GiB # default is 8
num_vertex_gib = options.num_vertex_GiB # default is 2

mem_size = str(num_edge_gib + num_vertex_gib) + "GiB"
# mem_size = "8GiB"

print("total mem size: " + mem_size)
print("num_edge_gib: " + str(num_edge_gib) + "GiB num_vertex_gib: " + str(num_vertex_gib) + "GiB")

# num_edge_mem_ctrls = 6
num_edge_mem_ctrls = int(num_edge_gib/2) # assumes 2 GiB per memory controller
num_vertex_mem_ctrls = int(num_vertex_gib/1) # assumes 1 GiB per memory controller


# const uint64_t buffer_addr = 0x100000000; // change buffer_addr to MessageQueues[], add 4096 to each message queue
# const uint64_t EL_addr = 0x600000000;
# const uint64_t VL_addr = 0x800000000;
# const uint64_t initalized_addr = 0x200000000;
# const uint64_t finished_addr = 0x300000000;
# const uint64_t finished_flag = 0x310000000;
# const uint64_t activeList_addr = 0x400000000;

# EL_vaddr = 0x200000000 # Make sure this is the same as in consumer.cpp, counter.cpp, and generator.cpp

EL_vaddr = 0x600000000 # Make sure this is the same as in consumer.cpp, counter.cpp, and generator.cpp
EL_paddr = 2 << 31# 4GiB   # need to update in graph_init
# VL_vaddr = 0x300000000
VL_vaddr = 0x2000000000
VL_paddr = num_edge_gib * gibi # 6GiB 11 << 31 Change this to be vertex memory base
# VL_paddr = 6442450944 # 6GiB 11 << 31 Change this to be vertex memory base

active_list_size = 65536  # 64KiB 

# num_vertex_mem_ctrls = 2


# initialization_vaddr = 0x400000000
initialization_vaddr = 0x200000000
initialization_paddr = (1 << 25) #1 << 32 # 4GiB
# finished_vaddr = 0x500000000 # 101 << 32
finished_vaddr = 0x300000000 # 
finished_paddr = (1 << 25) + 4096 # 

print("finished_paddr = ", finished_paddr)
# active_list_vaddr = 0x400000000
# active_list_paddr = 1 << 31 #2 GiB

# finished_flag_vaddr = 0x510000000
finished_flag_vaddr = 0x310000000
finished_flag_paddr = (1 << 25) - 4096 # 1GiB - 4KiB
# message queues are odd numbered addresses of finished flag
msg_queue_vaddr = 0x100000000
msg_queue_paddr = (num_edge_gib + num_vertex_gib) * gibi# 1 << 33 # 8GiB
print("msg_queue_paddr", msg_queue_paddr)
active_list_vaddr = 0x400000000
active_list_paddr = msg_queue_paddr + (4*gibi)
print("active_list_paddr", active_list_paddr)






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
# system.clk_domain.clock = '1GHz'
system.clk_domain.clock = options.clock_speed + 'GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange(mem_size)]

system.membus = SystemXBar(width=64)

if options.cpu_type == "timing":
    system.cpu = [X86TimingSimpleCPU() for i in range(num_cores)]
elif options.cpu_type == "O3":
    system.cpu = [X86O3CPU() for i in range(num_cores)]
else:
    print("Invalid cpu type")
    exit()
# system.cpu = [ARMTimingSimpleCPU() for i in range(num_cores)]



# soc-Epinions1.txt
# /data/graph_cache/real/twitter/sorted_graph.txt
# system.graphInitializer = GraphInit(graph_file="facebook_combined.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)
if options.workload == "short":
    system.graphInitializer = GraphInit(graph_file="twitter_sorted_done.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)

if options.workload == "long":
    system.graphInitializer = GraphInit(graph_file="/data/graph_cache/real/twitter/sorted_graph.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)

if options.workload == "fb":
    system.graphInitializer = GraphInit(graph_file="facebook_combined.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)
# system.graphInitializer = GraphInit(graph_file="soc-Epinions.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)
# system.graphInitializer = GraphInit(graph_file="/data/graph_cache/real/twitter/sorted_graph.txt", EL_addr=EL_paddr, VL_addr=VL_paddr)


# need to edit memory ranges
system.test_xbar = [IOXBar(width=64) for i in range(num_cores)] # xbar for each core between dcache and core
system.mem_bridge = [Bridge(delay='1ns', ranges=system.mem_ranges[0]) for i in range(num_cores)] # bridge for each core between dcache and membus

# bridge for each core between dcache and membus
system.xbar2xbar_bridge = [Bridge(delay='.1ns', ranges=AddrRange(start=mem_size, size=str((2147483648) + (4096 * int(num_cores/2))) + "B")) for i in range(num_cores)] 
system.xbar2al_bridge = [Bridge(delay='.1ns', ranges=AddrRange(start=active_list_paddr, size=str((2147483648) + (4096 * int(num_cores/2))) + "B"), req_size=1024, resp_size=1024) for i in range(num_cores)]
system.mq_xbar = IOXBar(width=64) # xbar for all message queues
system.al_xbar = IOXBar(width=64) # xbar for all active lists

# excess bridges
system.mq_bridge =[Bridge(delay='.1ns', ranges=AddrRange(start=mem_size, size="4KiB")) for i in range(int(num_cores/2))] # needs adjustig
system.al_bridge = [Bridge(delay='.1ns', ranges=AddrRange(start=active_list_paddr, size="64KiB"),req_size=1024, resp_size=1024) for i in range(int(num_cores/2))] # needs adjustig

mq_range = mq_range_base
# excess bridges
for bridge in system.mq_bridge:
    bridge.ranges = AddrRange(start=str(mq_range), size="4KiB")
    mq_range += 4096

al_range = active_list_paddr
for bridge in system.al_bridge:
    bridge.ranges = AddrRange(start=str(al_range), size="64KiB")
    al_range += 65536

system.icache = [L1ICache() for i in range(num_cores)]
system.dcache = [L1DCache() for i in range(num_cores)]

for j in range(num_cores):
    system.cpu[j].icache_port = system.icache[j].cpu_side
    system.icache[j].mem_side = system.membus.cpu_side_ports
    system.cpu[j].dcache_port = system.test_xbar[j].cpu_side_ports

    system.test_xbar[j].mem_side_ports = system.mem_bridge[j].cpu_side_port
    system.test_xbar[j].mem_side_ports = system.xbar2xbar_bridge[j].cpu_side_port
    system.test_xbar[j].mem_side_ports = system.xbar2al_bridge[j].cpu_side_port

    system.xbar2xbar_bridge[j].mem_side_port = system.mq_xbar.cpu_side_ports
    system.xbar2al_bridge[j].mem_side_port = system.al_xbar.cpu_side_ports

    system.mem_bridge[j].mem_side_port = system.dcache[j].cpu_side
    system.dcache[j].mem_side = system.membus.cpu_side_ports


# these bridges might be unnecessary
# excess bridges
for j in range(int(num_cores/2)): # num_cores/2 is the number of consumers
    system.mq_bridge[j].cpu_side_port = system.mq_xbar.mem_side_ports
    system.al_bridge[j].cpu_side_port = system.al_xbar.mem_side_ports
    
    # system.msg_queues[j].cpu_side = system.mq_xbar.mem_side_ports
    # system.active_lists[j].cpu_side = system.al_xbar.mem_side_ports
    
for j in range(num_cores):
    system.cpu[j].createInterruptController()
    # Note: Next 3 lines are x86 specific
    system.cpu[j].interrupts[0].pio = system.membus.mem_side_ports
    system.cpu[j].interrupts[0].int_requestor = system.membus.cpu_side_ports
    system.cpu[j].interrupts[0].int_responder = system.membus.mem_side_ports


system.msg_queues = [MessageQueue(myRange=AddrRange(start=str(0x200000000 + (i*4096)) + "B", size="4KiB"), queueSize=640000, finished_addr=(finished_paddr +(2*i) + 1)) for i in range(int(num_cores/2))]
system.active_lists = [ActiveList(myRange=AddrRange(start=str(active_list_paddr + (65536*i)) + "B", size="64KiB"), queueSize=640000, finished_addr=(finished_paddr +(2*i))) for i in range(int(num_cores/2))]


for j in range(int(num_cores/2)):
    # excess bridges
    system.msg_queues[j].cpu_side = system.mq_bridge[j].mem_side_port
    system.msg_queues[j].myRange = AddrRange(start=str(mq_range_base + (4096*j)) + "B", size="4KiB")
    # only include below line if messageQueue is writing to finished
    system.msg_queues[j].mem_side = system.membus.cpu_side_ports    

    system.active_lists[j].cpu_side = system.al_bridge[j].mem_side_port
    system.active_lists[j].mem_side = system.membus.cpu_side_ports    



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
# if options.batch is 1:
#     print("if")
#     binary2 = "configs/William/graph_src/consumer_batch"
# else:
#     print("else")
binary2 = "configs/William/graph_src/consumer_al"
# binary2 = "configs/William/graph_src/consumer_al"
binary3 = "configs/William/graph_src/generator_al"

# bring to Jason's attention?
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

print("VL_paddr - EL_paddr", VL_paddr - EL_paddr)
print("type(VL_paddr - EL_paddr)", type(VL_paddr - EL_paddr))
EL_size = VL_paddr - EL_paddr
max_int = 2**30

print(type(EL_vaddr+(5*max_int)), type(EL_paddr+(5*max_int)), type(max_int))
for my_process in process:

    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=int(VL_paddr-EL_paddr), cacheable=True) # EL mapping  changing paddr to 1<<30 broke it
    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=VL_paddr-EL_paddr, cacheable=False) # EL mapping  changing paddr to 1<<30 broke it
    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=3000000*8*2, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=4*gibi , cacheable=False) # EL mapping  Twitter Graph 3,000,000 edges



    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=3000000*8*2, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=max_int, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    my_process.map(vaddr=EL_vaddr+max_int, paddr=EL_paddr+max_int, size=max_int, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    my_process.map(vaddr=EL_vaddr+(2*max_int), paddr=EL_paddr+(2*max_int), size=max_int, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    my_process.map(vaddr=EL_vaddr+(3*max_int), paddr=EL_paddr+(3*max_int), size=max_int, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    my_process.map(vaddr=EL_vaddr+(4*max_int), paddr=EL_paddr+(4*max_int), size=max_int, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2
    my_process.map(vaddr=EL_vaddr+(5*max_int), paddr=EL_paddr+(5*max_int), size=max_int, cacheable=True) # EL mapping  Twitter Graph 3,000,000 edges 3000000*8*2

    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=4096 * 1024*16*2 , cacheable=False) # EL mapping  FB_Graph
    # # my_process.map(vaddr=VL_vaddr, paddr=VL_paddr, size=int(num_vertex_gib*gibi), cacheable=True) # VL mapping
    # my_process.map(vaddr=EL_vaddr, paddr=EL_paddr, size=EL_size , cacheable=False) # EL mapping  changing paddr to 1<<30 broke it
    
    # my_process.map(vaddr=VL_vaddr, paddr=VL_paddr, size=9000000*32*8, cacheable=True) # VL mapping 90,000 Vertices
    my_process.map(vaddr=VL_vaddr, paddr=VL_paddr, size=max_int, cacheable=True) # VL mapping 90,000 Vertices
    my_process.map(vaddr=VL_vaddr+max_int, paddr=VL_paddr+max_int, size=max_int, cacheable=True) # VL mapping 90,000 Vertices
    my_process.map(vaddr=VL_vaddr+(2*max_int), paddr=VL_paddr+(2*max_int), size=max_int, cacheable=True) # VL mapping 90,000 Vertices
    my_process.map(vaddr=VL_vaddr+(3*max_int), paddr=VL_paddr+(3*max_int), size=max_int, cacheable=True) # VL mapping 90,000 Vertices
    my_process.map(vaddr=VL_vaddr+(4*max_int), paddr=VL_paddr+(4*max_int), size=max_int, cacheable=True) # VL mapping 90,000 Vertices


    # my_process.map(vaddr=VL_vaddr, paddr=VL_paddr, size=4096 * 512 * 2, cacheable=False) # VL mapping
    # my_process.map(vaddr=0x400000000, paddr=(1 << 25), size=4096, cacheable=False) #  initialized variable mapping working
    my_process.map(vaddr=initialization_vaddr, paddr=initialization_paddr, size=4096, cacheable=False) #  initialized variable mapping

    my_process.map(vaddr=finished_vaddr, paddr=finished_paddr, size=4096, cacheable=False) #  Mapping of finished variabel
    # my_process.map(vaddr=0x600000000, paddr=(1 << 25) + 8192, size=65536 * int(num_cores/2), cacheable=True) #  ActiveList addr mapping
    my_process.map(vaddr=active_list_vaddr, paddr=active_list_paddr, size=65536 * int(num_cores/2), cacheable=False) #  ActiveList addr mapping shouldn't be cacheable?
    # my_process.map(vaddr=0x600000000, paddr=active_list_base, size=65536 * int(num_cores/2), cacheable=True) #  ActiveList addr mapping shouldn't be cacheable?
    my_process.map(vaddr=finished_flag_vaddr, paddr=finished_flag_paddr, size=4096, cacheable=False) #  Mapping of finished flag
    my_process.map(vaddr=msg_queue_vaddr, paddr=msg_queue_paddr, size=4096 * int(num_cores/2), cacheable=False) # 1<<33 is 8Gib, msg queue mapping

print("Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ {m5.curTick()} because {exit_event.getCause()}")