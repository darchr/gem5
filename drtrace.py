import m5

from m5.objects import *

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange("8GiB")]  # Create an address range
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]

players = 16

system.reader = DRTraceReader(
    directory="/projects/google-traces/delta/", num_players=players
)
system.players = [
    DRTracePlayer(
        reader=system.reader,
        send_data=True,
        compress_address_range=system.mem_ranges[0],
    )
    for _ in range(players)
]

system.xbar = SystemXBar()
system.xbar.mem_side_ports = system.mem_ctrl.port

for player in system.players:
    player.max_ipc = 8
    player.max_outstanding_reqs = 8
    player.cache = Cache(
        size="32KiB",
        assoc=8,
        tag_latency=1,
        data_latency=1,
        response_latency=1,
        mshrs=16,
        tgts_per_mshr=8,
    )
    player.port = player.cache.cpu_side
    player.cache.mem_side = system.xbar.cpu_side_ports
root = Root(full_system=False, system=system)

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate(1000 * 1000000)
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
