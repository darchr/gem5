from cluster import (
    Cluster,
    RemoteMemory,
)
from remote_armboard import RemoteArmBoard

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

board1 = RemoteArmBoard(
    clk_freq="3GHz",
    processor=SimpleProcessor(
        cpu_type=CPUTypes.ATOMIC,
        num_cores=1,
        isa=ISA.ARM,
    ),
    cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
        l1d_size="32kB",
        l1i_size="32kB",
        l2_size="256kB",
    ),
    memory=DualChannelDDR4_2400(size="4GiB"),
    release=ArmDefaultRelease.for_kvm(),
    platform=VExpress_GEM5_V1(),
)

board2 = RemoteArmBoard(
    clk_freq="3GHz",
    processor=SimpleProcessor(
        cpu_type=CPUTypes.ATOMIC,
        num_cores=1,
        isa=ISA.ARM,
    ),
    cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(
        l1d_size="32kB",
        l1i_size="32kB",
        l2_size="256kB",
    ),
    memory=DualChannelDDR4_2400(size="4GiB"),
    release=ArmDefaultRelease.for_kvm(),
    platform=VExpress_GEM5_V1(),
)

# Board 1 will write "Hello world!" and read it back
command1 = "echo '12345' | sudo -S ./mount.sh; ./test-read-write"

# Board 2 will wait for 1 second, then read what was written (hopefully, "hello world")
command2 = "echo '12345' | sudo -S ./mount.sh; sleep 1; ./test-read"

board1.set_kernel_disk_workload(
    kernel=KernelResource("/home/jlp/Code/linux/vmlinux.arm"),
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
        "rw",
    ],
    # readfile_contents=command1,
)

board2.set_kernel_disk_workload(
    kernel=KernelResource("/home/jlp/Code/linux/vmlinux.arm"),
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
        "rw",
    ],
    # readfile_contents=command2,
)

# board1.append_kernel_arg("no_systemd=true")
board1.append_kernel_arg("interactive=true")
# board2.append_kernel_arg("no_systemd=true")
board2.append_kernel_arg("interactive=true")

cluster = Cluster(
    boards=[board1, board2],
    # Note: The arm board starts its memory at 2GiB, so we need to start above that
    remote_memory=RemoteMemory(size="1GiB", start="6GiB"),
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
