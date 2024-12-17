import argparse

from cluster import (
    Cluster,
    RemoteMemory,
)
from host_x86board import HostX86Board

import m5

from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.cachehierarchies.ruby.mesi_two_level_cache_hierarchy import (
    MESITwoLevelCacheHierarchy,
)
from gem5.components.memory.single_channel import SingleChannelDDR3_1600
from gem5.components.processors.cpu_types import CPUTypes
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.isas import ISA
from gem5.resources.resource import (
    DiskImageResource,
    KernelResource,
    obtain_resource,
)
from gem5.simulate.exit_event import ExitEvent
from gem5.simulate.simulator import Simulator

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("--num-boards", type=int, default=2)
arg_parser.add_argument("--parallel", action="store_true", default=False)
args = arg_parser.parse_args()


def get_board():
    board = HostX86Board(
        clk_freq="3GHz",
        processor=SimpleProcessor(
            cpu_type=CPUTypes.TIMING,
            num_cores=1,
            isa=ISA.X86,
        ),
        cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
            l1d_size="2MiB",
            l1i_size="2MiB",
            l2_size="64MiB",
        ),
        memory=SingleChannelDDR3_1600(size="2GiB"),
    )
    return board


# Board 1 will write "Hello world!" and read it back
writer_command = [
    "echo '12345' | sudo -S ./mount.sh;",
    "dmesg",
    "sleep 1;",
    "gem5-bridge dumpresetstats;",
    "gem5-bridge exit;",
    "./test-read-write",
    "dmesg",
]

# other boards will wait for 1 second,
# then read what was written (hopefully, "hello world")
reader_command = [
    "echo '12345' | sudo -S ./mount.sh;",
    "sleep 2;",
    "gem5-bridge dumpresetstats;",
    "gem5-bridge exit;",
    "./test-read",
]

boards = [get_board() for _ in range(args.num_boards)]

workload0 = obtain_resource("x86-ubuntu-24.04-disagg")
workload0.set_parameter("readfile_contents", " ".join(writer_command))
boards[0].set_workload(workload0)


for board in boards[1:]:
    workload1 = obtain_resource("x86-ubuntu-24.04-disagg")
    workload1.set_parameter("readfile_contents", " ".join(reader_command))
    board.set_workload(workload1)


cluster = Cluster(
    boards=boards,
    remote_memory=RemoteMemory(size="1GiB", start="2GiB"),
    parallel=args.parallel,
)


def on_exit():
    for i in range(len(boards)):
        print(f"Exited for 'before boot' exit in board {i}")
        yield False

    for i in range(len(boards)):
        print(f"Exited for 'after boot' exit in board {i}")
        yield False

    for i in range(len(boards)):
        yield False

    for i in range(len(boards) - 1):
        print(f"finished board {i}")
        yield False

    yield True


simulator = Simulator(
    board=cluster,
    on_exit_event={
        ExitEvent.EXIT: on_exit(),
    },
)
simulator.run()
