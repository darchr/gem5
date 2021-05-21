import m5
import argparse

from m5.objects import Root
from m5.util.convert import *
from m5.stats import gem5stats

parser = argparse.ArgumentParser()

parser.add_argument(
    "mem_type_N",
    type=str,
    help="type of NVM memory to use",
)

parser.add_argument(
    "mem_type_D",
    type=str,
    help="type of DRAM memory to use",
)

parser.add_argument(
    "num_chnls",
    type=int,
    default=1,
    help="number of channels in the memory system, \
          could only be a power of 2, e.g. 1, 2, 4, 8, ..",
)

parser.add_argument(
    "mode",
    type=str,
    help="type of traffic to be generated",
)

parser.add_argument(
    "duration",
    type=str,
    help="real time duration to generate traffic \
          e.g. 1s, 1ms, 1us, 1ns",
)

parser.add_argument(
    "injection_rate",
    type=int,
    help="The amount of traffic generated \
          by the traffic generator in GBps",
)

parser.add_argument(
    "rd_perc",
    type=int,
    help="Percentage of read request, \
          rd_perc = 100 - write requests percentage",
)

parser.add_argument(
    "data_limit",
    type=int,
    default=0
)

parser.add_argument(
    "--num_mshrs",
    type=int,
    default=4,
    help="number of miss status holding registers \
          in the cache",
)

parser.add_argument(
    "--num_gens",
    type=int,
    default=1,
    help="number of traffic generators, only used with cached_ruby",
)

options = parser.parse_args()
from system.CachelessSystem import CachelessSystem
system = CachelessSystem(options.mem_type_D,
            options.mem_type_N, options.num_chnls)

options.block_size = system.getCachelineSize(options.mem_type_D)
options.duration = int(toLatency(options.duration) * 1e12)
injection_period = int(
    (1e12 * options.block_size) / (options.injection_rate * 1073741824)
)
options.min_period = injection_period
options.max_period = injection_period

outdir = m5.options.outdir

root = Root(full_system=False, system=system)

m5.instantiate()

if options.mode == "linear":
    system.startLinearTraffic(options)
elif options.mode == "random":
    system.startRandomTraffic(options)
else:
    print("Traffic type not supported!")
    exit()

exit_event = m5.simulate()

simstat = gem5stats.get_simstat(root)
json_file_name = '{}/stats.json'.format(outdir)
with open(json_file_name, 'w') as f:
    simstat.dump(f)
