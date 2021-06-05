import m5
import os
import configparser

from m5.objects import DRAMsim3, AddrRange

from ..boards.abstract_board import AbstractBoard
from .abstract_memory_system import AbstractMemorySystem

from typing import Optional, Tuple

"""
This function creates a config file that will be used to create a memory
controller of type DRAMSim3. It stores the config file in /tmp/ directory.

:param mem_type: The name for the type of the memory to be configured.
:param num_chnls: The number of channels to configure for the memory
"""
def config_ds3(mem_type, num_chnls):
    config = configparser.ConfigParser()
    thispath = os.path.dirname(os.path.realpath(__file__))
    input_file = os.path.join(
        thispath, "dramsim_3_mem_configs/" + mem_type + ".ini"
    )
    output_file = "/tmp/" + mem_type + "_chnls" + str(num_chnls) + ".ini"
    new_config = open(output_file, "w")
    config.read(input_file)
    config.set("system", "channels", str(num_chnls))
    config.write(new_config)
    new_config.close()
    return output_file, m5.options.outdir


class DRAMSim3MemCtrl(DRAMsim3):
    def __init__(self, mem_name, num_chnls):
        super(DS3MemCtrl, self).__init__()
        ini_path, outdir = config_ds3(mem_name, num_chnls)
        self.configFile = ini_path
        self.filePath = outdir


class SingleChannel(AbstractMemorySystem):
    def __init__(self, mem_type: str, size: Optional[str]):
        super(SingleChannel, self).__init__()
        self.mem_ctrl = DRAMSim3MemCtrl(mem_type, 1)
        if size:
            self.mem_ctrl.range = AddrRange(size)
        else:
            raise NotImplementedError("Currently not supporting default size!")

    @overrides(AbstractMemorySystem)
    def incorporate_memory(self, board: AbstractBoard) -> None:
        pass

    @overrides(AbstractMemorySystem)
    def get_mem_ports(self) -> Tuple[Sequence[AddrRange], Port]:
        return [(self.mem_ctrl.range, self.mem_ctrl.port)]

    @overrides(AbstractMemorySystem)
    def get_memory_controllers(self) -> List[MemCtrl]:
        return [self.mem_ctrl]

    @overrides(AbstractMemorySystem)
    def get_memory_ranges(self):
        return [self.mem_ctrl.range]


def SingleChannelDDR3_1600(
    size: Optional[str] = None,
) -> AbstractMemorySystem:
    return SingleChannel("DDR3_8Gb_x8_1600", "2048MB")


def SingleChannelDDR4_2400(
    size: Optional[str] = None,
) -> AbstractMemorySystem:
    return SingleChannel("DDR4_4Gb_x8_2400", "1024MB")


def SingleChannelLPDDR3_1600(
    size: Optional[str] = None,
) -> AbstractMemorySystem:
    return SingleChannel("LPDDR3_8Gb_x32_1600", "256MB")


def SingleChannelHBM(size: Optional[str] = None) -> AbstractMemorySystem:
    return SingleChannel("HBM1_4Gb_x128", "64MB")
