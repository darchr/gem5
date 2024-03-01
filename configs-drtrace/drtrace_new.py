import m5
import argparse
from m5.objects import *
from system import *

"""
Usage:
------

```
./build/X86/gem5.opt \
    drtrace.py \
    --path <path of folder containing all traces> \
    --workload <benchmark_name> \
    --players <Number of players to use> \
    --dram <DRAM device to use>
```
"""

parser = argparse.ArgumentParser(
    description="A script to run google traces."
)

benchmark_choices = ["charlie", "delta", "merced", "whiskey"]

parser.add_argument(
    "path",
    type=str,
    help="Main directory containing the traces.",
)

parser.add_argument(
    "workload",
    type=str,
    help="Input the benchmark program to execute.",
    choices=benchmark_choices,
)

parser.add_argument(
    "players",
    type=int,
    help="Input the number of players to use.",
)

parser.add_argument(
    "dcache_policy",
    type=str,
    help="The architecture of DRAM cache: "
    "CascadeLakeNoPartWrs, Oracle, BearWriteOpt, Rambus",
)
parser.add_argument(
    "assoc",
    type=int,
    help="THe associativity of the DRAM cache",
)
parser.add_argument(
    "dcache_size",
    type=str,
    help="The size of DRAM cache",
)
parser.add_argument(
    "main_mem_size",
    type=str,
    help="The size of main memory",
)
parser.add_argument(
    "is_link",
    type=int,
    help="whether to use a link for backing store or not",
)
parser.add_argument(
    "link_lat",
    type=str,
    help="latency of the link to backing store"
)

args = parser.parse_args()

#system = System()
system = MyRubySystem("MESI_Two_Level", args.players, args.assoc, args.dcache_size, args.main_mem_size, args.dcache_policy, args.is_link, args.link_lat, args)

root = Root(full_system=True, system=system)

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate(100000000000)
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
