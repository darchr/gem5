from cluster import (
    Cluster,
    RemoteMemory,
)
from remote_x86board import RemoteMemoryX86Board

from gem5.coherence_protocol import CoherenceProtocol
from gem5.components.boards.abstract_board import AbstractBoard
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
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

board1 = RemoteMemoryX86Board(
    clk_freq="3GHz",
    processor=SimpleProcessor(
        cpu_type=CPUTypes.ATOMIC,
        num_cores=1,
        isa=ISA.X86,
    ),
    cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
        l1d_size="32kB",
        l1i_size="32kB",
        l2_size="256kB",
    ),
    memory=SingleChannelDDR3_1600(size="2GiB"),
)

board2 = RemoteMemoryX86Board(
    clk_freq="3GHz",
    processor=SimpleProcessor(
        cpu_type=CPUTypes.ATOMIC,
        num_cores=1,
        isa=ISA.X86,
    ),
    cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
        l1d_size="32kB",
        l1i_size="32kB",
        l2_size="256kB",
    ),
    memory=SingleChannelDDR3_1600(size="2GiB"),
)

# board1.set_se_binary_workload(binary=obtain_resource("x86-hello64-static"))
# board2.set_se_binary_workload(binary=obtain_resource("x86-hello64-static"))

# board1.set_workload(obtain_resource("x86-ubuntu-24.04-boot-no-systemd"))
# board2.set_workload(obtain_resource("x86-ubuntu-24.04-boot-no-systemd"))

# Board 1 will write "Hello world!" and read it back
command1 = "echo '12345' | sudo -S ./mount.sh; ./test-read-write"

# Board 2 will wait for 1 second, then read what was written (hopefully, "hello world")
command2 = "echo '12345' | sudo -S ./mount.sh; sleep 1; ./test-read"

board1.set_kernel_disk_workload(
    # kernel=obtain_resource("x86-linux-kernel-6.8.0-35-generic"),
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
    readfile_contents=command1,
)
board2.set_kernel_disk_workload(
    # kernel=obtain_resource("x86-linux-kernel-6.8.0-35-generic"),
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
    readfile_contents=command2,
)

board1.append_kernel_arg("memmap=128M!2G")
board1.append_kernel_arg("no_systemd=true")
board2.append_kernel_arg("memmap=128M!2G")
board2.append_kernel_arg("no_systemd=true")

cluster = Cluster(
    boards=[board1, board2],
    remote_memory=RemoteMemory(size="1GiB", start="2GiB"),
)


def on_exit():
    print("done booting kernel board 1")
    yield False
    print("done booting kernel board 2")
    yield False

    print("Exited for 'after boot' exit in board 1")
    yield False
    print("Exited for 'after boot' exit in board 2")
    yield False

    print("Finished simulation board 1")
    yield False
    print("Finished simulation board 2")
    yield True


simulator = Simulator(
    board=cluster,
    on_exit_event={
        ExitEvent.EXIT: on_exit(),
    },
)
simulator.run()
