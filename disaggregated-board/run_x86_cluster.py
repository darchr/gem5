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
from gem5.components.processors.simple_switchable_processor import (
    SimpleSwitchableProcessor,
)
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
        processor=SimpleSwitchableProcessor(
            starting_core_type=CPUTypes.ATOMIC,
            switch_core_type=CPUTypes.TIMING,
            isa=ISA.X86,
            num_cores=1,
        ),
        cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
            l1d_size="32KiB",
            l1i_size="32KiB",
            l2_size="256KiB",
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

boards[0].set_kernel_disk_workload(
    kernel=KernelResource("/home/jlp/Code/linux/vmlinux.x86"),
    disk_image=DiskImageResource(
        "/home/jlp/Code/gem5/gem5-resources/src/add-dax/disk-image/x86-ubuntu-24-04-dax"
    ),
    kernel_args=[
        "earlyprintk=ttyS0",
        "console=ttyS0",
        "lpj=7999923",
        "root=/dev/sda2",
    ],
    readfile_contents=" ".join(writer_command),
)

for board in boards[1:]:
    board.set_kernel_disk_workload(
        kernel=KernelResource("/home/jlp/Code/linux/vmlinux.x86"),
        disk_image=DiskImageResource(
            "/home/jlp/Code/gem5/gem5-resources/src/add-dax/disk-image/x86-ubuntu-24-04-dax"
        ),
        kernel_args=[
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/sda2",
        ],
        readfile_contents=" ".join(reader_command),
    )


for board in boards:
    board.append_kernel_arg("memmap=1G!2G")
    board.append_kernel_arg("no_systemd=true")

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
        print(f"Switching cpu {i}")
        board.processor.switch()
        yield False

    for i in range(len(boards)):
        print("debug start")
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
