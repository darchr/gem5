from typing import List, Tuple, Dict, Any, Optional, Union
from gem5.resources.resource import BinaryResource
from gem5.prebuilt.riscvmatched.riscvmatched_board import RISCVMatchedBoard


def get_microbenchmarks() -> List[str]:
    from gem5.resources.resource import obtain_resource

    # TODO: this will be replaced with the suite later

    microbenchmarks = [
        obtain_resource("riscv-cca-run"),
        obtain_resource("riscv-cce-run"),
        obtain_resource("riscv-dpt-run"),
    ]

    return microbenchmarks


def create_board_configurations():
    from get_parameter import get_processed_parameters

    parameter_configurations = []

    for parameter in get_processed_parameters():

        microbenchmarks = get_microbenchmarks()

        config = [f"{parameter[0]} = {parameter[1]}"]

        for microbenchmark in microbenchmarks:
            parameter_configurations.append([config, microbenchmark])

    return parameter_configurations


def run_one_configuration(board_configuration):

    from gem5.simulate.simulator import Simulator
    from gem5.prebuilt.riscvmatched.riscvmatched_board import RISCVMatchedBoard
    from gem5.isas import ISA
    from gem5.utils.requires import requires

    requires(isa_required=ISA.RISCV)

    # print(board_configuration[0])
    # print(board_configuration[1])

    board = RISCVMatchedBoard()
    board.apply_config(board_configuration[0])

    board.set_workload(board_configuration[1])
    simulator = Simulator(board)
    simulator.run()
