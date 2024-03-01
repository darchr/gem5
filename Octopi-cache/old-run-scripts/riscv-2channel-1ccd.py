from gem5.components.boards.riscv_board import RiscvBoard
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.utils.requires import requires
from gem5.utils.override import overrides
from gem5.resources.resource import Resource, DiskImageResource
from gem5.simulate.simulator import Simulator
from gem5.components.processors.simple_switchable_processor import (
    SimpleSwitchableProcessor,
)
from gem5.components.memory import CascadeLakeCache, OracleCache, RamCache
import m5

from m5.objects import HBM_1000_4H_1x64

from components.Octopi import OctopiCache

requires(isa_required=ISA.RISCV)

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--command", type=str)
parser.add_argument("--checkpoint-path", type=str)

args = parser.parse_args()

num_ccds = 1
num_cores = 8
command = args.command

cache_hierarchy = OctopiCache(
    l1i_size  = "32KiB",
    l1i_assoc = 8,
    l1d_size  = "32KiB",
    l1d_assoc = 8,
    l2_size  = "512KiB",
    l2_assoc = 8,
    l3_size = "32MiB",
    l3_assoc = 32,
    num_core_complexes = 1,
    is_fullsystem = True,
)
memory = RamCache()

"""
memory = ChanneledMemory(
    dram_interface_class = HBM_1000_4H_1x64,
    num_channels = 2,
    interleaving_size = 2**8,
    size = "64GiB",
    addr_mapping = None
)
"""

processor = SimpleSwitchableProcessor(
    starting_core_type=CPUTypes.TIMING,
    switch_core_type=CPUTypes.TIMING, # TODO
    isa=ISA.RISCV,
    num_cores=num_cores
)

class HighPerformanceRiscvBoard(RiscvBoard):
    @overrides(RiscvBoard)
    def get_default_kernel_args(self):
        return [
            "earlyprintk=ttyS0",
            "console=ttyS0",
            "lpj=7999923",
            "root=/dev/vda1",
            "init=/root/init.sh",
            "rw",
        ]

# Setup the board.
board = HighPerformanceRiscvBoard(
    clk_freq="4GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

# Set the Full System workload.
board.set_kernel_disk_workload(
    kernel=Resource("riscv-bootloader-vmlinux-5.10"),
    #disk_image=DiskImageResource(
    #    "/projects/gem5/npb-gapbs-riscv.img", root_partition="1"
    #),
    disk_image=DiskImageResource("/scr/hn/DISK_IMAGES/ubuntu-2204-riscv.img"),
    readfile_contents=f"{command}"
)

m5.scheduleTickExitFromCurrent(1000000000)

simulator = Simulator(board=board)
print("Beginning simulation!")
simulator.run()
print("We are exiting because : ")
print(simulator.get_last_exit_event_cause())
simulator.save_checkpoint(args.checkpoint_path)
