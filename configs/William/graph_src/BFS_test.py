import m5
from m5.objects import *

from caches import *

num_cores = 11

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('8GiB')]

system.membus = SystemXBar()

# system.cpu = X86TimingSimpleCPU()
system.cpu = [X86TimingSimpleCPU() for i in range(num_cores)]
# system.cpu[0] = X86AtomicSimpleCPU()

# system.icache = L1ICache()


system.test_xbar = IOXBar()
system.mem_bridge = Bridge(delay='1ns', ranges=system.mem_ranges[0])

system.mq_bridge =[Bridge(delay='1ns', ranges=AddrRange(start="8GiB", size="4KiB")) for i in range(int(num_cores/2))] # needs adjustig
mq_range = 8589934592
for bridge in system.mq_bridge:
    bridge.ranges = AddrRange(start=str(mq_range), size="4KiB")
    mq_range += 4096
# system.mq_bridge[1] = Bridge(delay='1ns', ranges=AddrRange(start="8589938688B", size="4KiB"))  # needs adjustig

system.icache = [L1ICache() for i in range(num_cores)]
system.dcache = L1DCache()

for j in range(num_cores):
    system.cpu[j].icache_port = system.icache[j].cpu_side
    system.icache[j].mem_side = system.membus.cpu_side_ports
    system.cpu[j].dcache_port = system.test_xbar.cpu_side_ports

system.test_xbar.mem_side_ports = system.mem_bridge.cpu_side_port
system.mem_bridge.mem_side_port = system.dcache.cpu_side

for j in range(int(num_cores/2)): # num_cores/2 is the number of consumers
    system.mq_bridge[j].cpu_side_port = system.test_xbar.mem_side_ports
    

system.dcache.mem_side = system.membus.cpu_side_ports


for j in range(num_cores):
    system.cpu[j].createInterruptController()
    # Note: Next 3 lines are x86 specific
    system.cpu[j].interrupts[0].pio = system.membus.mem_side_ports
    system.cpu[j].interrupts[0].int_requestor = system.membus.cpu_side_ports
    system.cpu[j].interrupts[0].int_responder = system.membus.mem_side_ports



system.msg_queues = [MessageQueue(myRange=AddrRange(start="8GiB", size="4KiB"), queueSize=64000) for i in range(int(num_cores/2))]
# system.msg_queues[1].myRange = AddrRange(start="8589938688B", size="4KiB")

msg_queue_base = 8589934592

for j in range(int(num_cores/2)):
    system.msg_queues[j].cpu_side = system.mq_bridge[j].mem_side_port
    system.msg_queues[j].myRange = AddrRange(start=str(msg_queue_base + (4096*j)) + "B", size="4KiB")



system.system_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports


binary = "configs/William/graph_src/counter"
binary2 = "configs/William/graph_src/consumer"
binary3 = "configs/William/graph_src/generator"

system.workload = SEWorkload.init_compatible(binary)
system.workload2 = SEWorkload.init_compatible(binary2)
system.workload3 = SEWorkload.init_compatible(binary3)

process = [Process() for i in range(num_cores)]
process[0].cmd = [binary, num_cores - 1] # should always be true

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
    
    my_process.map(vaddr=0x200000000, paddr=1 << 31, size=4096 * 1024*16*2 , cacheable=True) # EL mapping
    my_process.map(vaddr=0x300000000, paddr=(1 << 27) , size=4096 * 512 * 2, cacheable=True) # VL mapping
    my_process.map(vaddr=0x400000000, paddr=(1 << 25), size=4096, cacheable=False) #  initialized variable mapping
    my_process.map(vaddr=0x500000000, paddr=(1 << 25) + 4096, size=4096, cacheable=True) #  Mapping of finished variabel
    my_process.map(vaddr=0x600000000, paddr=(1 << 25) + 8192, size=4096 * int(num_cores/2), cacheable=True) #  ActiveList addr mapping
    my_process.map(vaddr=0x510000000, paddr=(1 << 25) - 4096, size=4096, cacheable=False) #  Mapping of finished flag

    my_process.map(vaddr=0x100000000, paddr=1 << 33, size=4096 * int(num_cores/2), cacheable=True) # 1<<33 is 8Gib, msg queue mapping

print("Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ {m5.curTick()} because {exit_event.getCause()}")