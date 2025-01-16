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
        processor=SimpleProcessor(
            cpu_type=CPUTypes.ATOMIC,
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


# Mount the dax device and then exit
command = [
    "echo '12345' | sudo -S ./mount.sh;",
    "sudo dmesg;",
    "sleep 1;",
    "gem5-bridge exit;",  # We are going to checkpoint here
    "./after_boot.sh;",  # Re-run after boot after the checkpoint
]

boards = [get_board() for _ in range(args.num_boards)]

for board in boards:
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
        readfile_contents=" ".join(command),
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
        yield False

    # Now all boards have finished booting.

    for i in range(len(boards) - 1):
        print(f"Exiting after mounting for board {i}")
        yield False

    print("Exiting after mounting for board {len(boards) - 1}")
    m5.checkpoint("ckpt_x86_cluster")

    yield True


simulator = Simulator(
    board=cluster,
    on_exit_event={
        ExitEvent.EXIT: on_exit(),
    },
)
simulator.run()
