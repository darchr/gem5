from gem5.components.boards.riscv_board import RiscvBoard
from gem5.components.memory.memory import ChanneledMemory
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.utils.requires import requires
from gem5.utils.override import overrides
from gem5.resources.resource import Resource, DiskImageResource
from gem5.simulate.simulator import Simulator
from gem5.components.memory import CascadeLakeCache, OracleCache, RamCache

from m5.objects import HBM_1000_4H_1x64

from components.Octopi import OctopiCache

requires(isa_required=ISA.RISCV)



import argparse
parser = argparse.ArgumentParser()
parser.add_argument("--command", type=str)
parser.add_argument("--num_ccxs", type=int, required=True)
parser.add_argument("--num_cores", type=int, required=True)
args = parser.parse_args()

num_ccxs = args.num_ccxs
num_cores = args.num_cores
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
    num_core_complexes = num_ccxs,
    is_fullsystem = True,
)

#memory = ChanneledMemory(
#    dram_interface_class = HBM_1000_4H_1x64,
#    num_channels = num_ccxs, # should equal to the number of core complexes
#    interleaving_size = 2**8,
#    size = "8GiB",
#    addr_mapping = None
#)

memory = RamCache()

processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING, isa=ISA.RISCV, num_cores=num_cores
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
    disk_image=DiskImageResource("/scr/hn/DISK_IMAGES/ubuntu-2204-riscv.img"),
    readfile_contents=f"{command}"
)

simulator = Simulator(board=board)
print("Beginning simulation!")
simulator.run()
