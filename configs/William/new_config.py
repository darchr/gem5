import m5
from m5.objects import *

from caches import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("256MB")]
system.membus = SystemXBar()

# system.cpu = X86TimingSimpleCPU()
system.cpu = [X86TimingSimpleCPU() for i in range(2)]

# system.icache = L1ICache()
system.dcache = L1DCache()

system.icache = [L1ICache() for i in range(2)]
# system.dcache = [L1DCache() for i in range(2)]

# system.cpu.icache_port = system.icache.cpu_side
system.cpu[0].icache_port = system.icache[0].cpu_side
system.cpu[1].icache_port = system.icache[1].cpu_side


system.test_xbar = IOXBar()
system.mem_bridge = Bridge(delay="1ns", ranges=system.mem_ranges[0])

# system.cpu.dcache_port = system.test_xbar.cpu_side_ports

system.cpu[0].dcache_port = system.test_xbar.cpu_side_ports
system.cpu[1].dcache_port = system.test_xbar.cpu_side_ports

system.test_xbar.mem_side_ports = system.mem_bridge.cpu_side_port
system.mem_bridge.mem_side_port = system.dcache.cpu_side
# system.mem_bridge.mem_side_port = system.dcache[0].cpu_side
# system.mem_bridge.mem_side_port = system.dcache[1].cpu_side

msg_queue_range = AddrRange(start="1GiB", size="4KiB")
system.mq_bridge = Bridge(delay="1ns", ranges=msg_queue_range)
system.mq_bridge.cpu_side_port = system.test_xbar.mem_side_ports


# system.icache.mem_side = system.membus.cpu_side_ports
system.dcache.mem_side = system.membus.cpu_side_ports

# system.cpu.createInterruptController()

system.icache[0].mem_side = system.membus.cpu_side_ports
# system.dcache[0].mem_side = system.membus.cpu_side_ports
system.icache[1].mem_side = system.membus.cpu_side_ports
# system.dcache[1].mem_side = system.membus.cpu_side_ports

system.cpu[0].createInterruptController()
system.cpu[1].createInterruptController()

# Note: Next 3 lines are x86 specific
# system.cpu.interrupts[0].pio = system.membus.mem_side_ports
# system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
# system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.cpu[0].interrupts[0].pio = system.membus.mem_side_ports
system.cpu[0].interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu[0].interrupts[0].int_responder = system.membus.mem_side_ports
system.cpu[1].interrupts[0].pio = system.membus.mem_side_ports
system.cpu[1].interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu[1].interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

system.msg_queue = MessageQueue(myRange=msg_queue_range, queueSize=18)
system.msg_queue.cpu_side = system.mq_bridge.mem_side_port

# binary = "tests/test-progs/hello/bin/x86/linux/hello"
# binary = "configs/William/mapped_queue"
binary = "configs/William/graph_test"
binary2 = "configs/William/consumer_test"

system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]

process2 = Process()
process2.cmd = [binary2]
process2.pid = 200
# system.cpu.workload = process
# system.cpu.createThreads()

system.cpu[0].workload = process
system.cpu[0].createThreads()

system.cpu[1].workload = process2
system.cpu[1].createThreads()

root = Root(full_system=False, system=system)
m5.instantiate()
process.map(vaddr=0x100000000, paddr=1 << 30, size=4096, cacheable=True)
process.map(vaddr=0x100000000 + 4096, paddr=(1 << 27) , size=4096, cacheable=True)

process2.map(vaddr=0x100000000, paddr=1 << 30, size=4096, cacheable=True)
process2.map(vaddr=0x100000000 + 4096, paddr=(1 << 27) , size=4096, cacheable=True)

print("Beginning simulation!")
exit_event = m5.simulate()
print(f"Exiting @ {m5.curTick()} because {exit_event.getCause()}")
