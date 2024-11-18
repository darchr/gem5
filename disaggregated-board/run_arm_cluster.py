import argparse

from cluster import (
    Cluster,
    RemoteMemory,
)
from host_armboard import HostArmBoard

from m5.objects import (
    ArmDefaultRelease,
    VExpress_GEM5_Foundation,
    VExpress_GEM5_V1,
)

from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.memory.multi_channel import DualChannelDDR4_2400
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
arg_parser.add_argument("--num-boards", type=int, default=1)
arg_parser.add_argument("--parallel", action="store_true", default=False)
args = arg_parser.parse_args()


def get_board():
    board = HostArmBoard(
        clk_freq="3GHz",
        processor=SimpleProcessor(
            cpu_type=CPUTypes.ATOMIC,
            num_cores=2,
            isa=ISA.ARM,
        ),
        cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
            l1d_size="32KiB",
            l1i_size="32KiB",
            l2_size="256KiB",
        ),
        memory=DualChannelDDR4_2400(size="4GiB"),
        release=ArmDefaultRelease.for_kvm(),
        platform=VExpress_GEM5_V1(),
    )
    return board


# Board 1 will write "Hello world!" and read it back
writer_command = [
    "echo '12345' | sudo -S ./mount.sh;",
    "sleep 1;",
    "gem5-bridge dumpresetstats;",
    "./test-read-write",
]

# other boards will wait for 1 second,
# then read what was written (hopefully, "hello world")
reader_command = [
    "echo '12345' | sudo -S ./mount.sh;",
    "sleep 2;",
    "gem5-bridge dumpresetstats;",
    "./test-read",
]

boards = [get_board() for _ in range(args.num_boards)]

boards[0].set_kernel_disk_workload(
    kernel=KernelResource(
        "/home/lredivo/darchr/gem5-cxl/linux-6.10.11/vmlinux"
    ),
    disk_image=DiskImageResource(
        "/home/jlp/Code/gem5/gem5-resources/src/add-dax/disk-image-arm/arm-ubuntu-24-04-dax",
        root_partition="2",
    ),
    bootloader=obtain_resource("arm64-bootloader", resource_version="1.0.0"),
    kernel_args=[
        "console=ttyAMA0",
        "lpj=19988480",
        "norandmaps",
        "root={root_value}",
        "init=/bin/bash",
        "rw",
    ],
    # readfile_contents=" ".join(writer_command),
)

for board in boards[1:]:
    board.set_kernel_disk_workload(
        kernel=KernelResource(
            "/home/lredivo/darchr/gem5-cxl/linux-6.10.11/vmlinux"
        ),
        disk_image=DiskImageResource(
            "/home/jlp/Code/gem5/gem5-resources/src/add-dax/disk-image-arm/arm-ubuntu-24-04-dax",
            root_partition="2",
        ),
        bootloader=obtain_resource(
            "arm64-bootloader", resource_version="1.0.0"
        ),
        kernel_args=[
            "console=ttyAMA0",
            "lpj=19988480",
            "norandmaps",
            "root={root_value}",
            "init=/bin/bash",
            "rw",
        ],
        # readfile_contents=" ".join(reader_command),
    )

for board in boards:
    board.append_kernel_arg("no_systemd=true")
    board.append_kernel_arg("interactive=true")

cluster = Cluster(
    boards=boards,
    remote_memory=RemoteMemory(size="1GiB", start="6GiB"),
    parallel=args.parallel,
)


def on_exit():
    for i in range(len(boards)):
        print(f"Exited for 'before boot' exit in board {i}")
        yield False

    for i in range(len(boards)):
        print(f"Exited for 'after boot' exit in board {i}")
        yield False

    for i in range(len(boards) - 1):
        print(f"finished board {i}")
        yield False

    print(f"finished board {args.num_boards}")
    yield True


simulator = Simulator(
    board=cluster,
    on_exit_event={
        ExitEvent.EXIT: on_exit(),
    },
)
simulator.run()
