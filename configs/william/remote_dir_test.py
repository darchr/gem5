import importlib
import importlib.util
import os

import gem5

print("gem5 __path__:", list(getattr(gem5, "__path__", [])))

for p in getattr(gem5, "__path__", []):
    probe = os.path.join(
        p, "components/cachehierarchies/chi/chi_l3_remote_dir.py"
    )
    print("exists in", p, "→", os.path.exists(probe))

print(
    "find_spec:",
    importlib.util.find_spec(
        "gem5.components.cachehierarchies.chi.chi_l3_remote_dir"
    ),
)
# import sys
# print("sys.path:\n ", "\n  ".join(sys.path))
# try:
#     import gem5
#     print("gem5 at:", gem5.__file__)
#     from importlib import import_module
#     m = import_module("gem5.components.cachehierarchies.chi.chi_l3_remote_dir")
#     print("Loaded:", m.__file__)
# except Exception as e:
#     import traceback; traceback.print_exc()

# # run_linear_gen.py
# #
# # Minimal gem5 stdlib config using a Linear traffic generator.
# # - Classic memory system (no caches)
# # - Single DDR4 channel
# # - One generator core issuing strided (linear) accesses

# from argparse import ArgumentParser

# from gem5.components.boards.simple_board import SimpleBoard
# from gem5.components.cachehierarchies.classic.no_cache import NoCache
# from gem5.components.memory.single_channel import SingleChannelDDR4_2400
# from gem5.components.processors.linear_generator import LinearGenerator
# from gem5.simulate.simulator import Simulator

# def parse_args():
#     p = ArgumentParser(description="Linear traffic generator with stdlib")
#     p.add_argument("--size", default="1GB",
#                    help="Total DRAM size (e.g., 1GB, 512MB)")
#     p.add_argument("--duration", default="5ms",
#                    help="How long the generator should run (e.g., 5ms, 1ms)")
#     p.add_argument("--start-addr", default="0x00000000",
#                    help="Start address for the generator")
#     p.add_argument("--end-addr", default="0x08000000",
#                    help="End (exclusive) address; generator wraps at this bound")
#     p.add_argument("--request-size", default="64B",
#                    help="Access size of each request (e.g., 64B)")
#     p.add_argument("--stride", default="64B",
#                    help="Stride for the linear generator (e.g., 64B)")
#     p.add_argument("--max-outstanding", type=int, default=128,
#                    help="Max outstanding requests per core")
#     p.add_argument("--num-cores", type=int, default=1,
#                    help="Number of generator cores")
#     p.add_argument("--clock", default="2GHz",
#                    help="Board clock frequency")
#     p.add_argument("--maxticks", type=int, default=100000000000,
#                    help="Optional global max ticks (0 = run until generators finish)")
#     return p.parse_args()


# args = parse_args()
# print("Starting linear generator with the following args:")
# print(args)

# # Classic memory: no caches keeps the example simple and isolates DRAM behavior.
# cache_hierarchy = NoCache()
# memory = SingleChannelDDR4_2400(size=args.size)

# # Linear (strided) traffic generator(s).
# #
# # Notes:
# # - The stdlib LinearGenerator models a predictable, sequential access pattern.
# # - `duration` is wall clock simulated time the generator will actively issue.
# # - `start_addr`..`end_addr` defines the working set window.
# processor = LinearGenerator(
#     num_cores=args.num_cores,
#     duration=args.duration,
#     min_addr=args.start_addr,
#     max_addr=args.end_addr,
#     rate="100GiB/s",
#     rd_perc=100,
#     data_limit=0,
# )

# # Simple board ties together clock, processor, caches, and memory.
# board = SimpleBoard(
#     clk_freq=args.clock,
#     processor=processor,
#     memory=memory,
#     cache_hierarchy=cache_hierarchy,
# )

# # Build and run the simulation. If --maxticks is 0, we let generators decide stop time.
# sim = Simulator(board=board)
# sim.run()

# print("Done.")
