import m5

from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange(0x800000000000)]  # Create an address range
system.memory = SimpleMemory(range=system.mem_ranges[0], null=True)

system.reader = DRTraceReader(
    directory="/projects/google-traces/delta/", num_players=1
)
system.player = DRTracePlayer(reader=system.reader)
system.player.max_ipc = 8
system.player.max_outstanding_reqs = 8
system.player.port = system.memory.port
root = Root(full_system=False, system=system)

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate(1000 * 1000000)
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
