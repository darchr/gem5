import m5
import argparse
from m5.objects import *

"""
Usage:
------

```
./build/X86/gem5.opt \
    drtrace.py \
    --workload <benchmark_name> \
    --players <Number of players to use>
```
"""

parser = argparse.ArgumentParser(
    description="A script to run google traces."
)

benchmark_choices = ["charlie", "delta", "merced", "whiskey"]

parser.add_argument(
    "--workload",
    type=str,
    required=True,
    help="Input the benchmark program to execute.",
    choices=benchmark_choices,
)

parser.add_argument(
    "--players",
    type=int,
    required=True,
    help="Input the number of players to use.",
)

parser.add_argument(
    "--dram",
    type = str,
    help = "Memory device to use"
)

args = parser.parse_args()


MemTypes = {
    'ddr4_2400' : DDR4_2400_16x4,
    'hbm_2000' : HBM_2000_4H_1x64,
    'ddr5_8400' : DDR5_8400_4x8
}

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange("8GiB")]  # Create an address range

if args.dram == 'hbm_2000':
    system.mem_ctrl = HBMCtrl()
    system.mem_ctrl.dram = MemTypes[args.dram](range=AddrRange(start = '0', end = '8GiB', masks = [1 << 6], intlvMatch = 0))
    system.mem_ctrl.dram_2 = MemTypes[args.dram](range=AddrRange(start = '0', end = '8GiB', masks = [1 << 6], intlvMatch = 1))
else:
    system.mem_ctrl = MemCtrl()
    system.mem_ctrl.dram = MemTypes[args.dram](range=system.mem_ranges[0])

#system.mem_ctrl.dram.range = system.mem_ranges[0]

players = args.players

system.reader = DRTraceReader(
    directory="/projects/google-traces/{}/".format(args.workload), num_players=args.players
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
exit_event = m5.simulate(100000000000)
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
