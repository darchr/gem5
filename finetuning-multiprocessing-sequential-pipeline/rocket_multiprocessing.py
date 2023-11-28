from typing import List, Tuple, Dict, Any, Optional, Union
from gem5.resources.resource import BinaryResource


def get_microbenchmarks() -> List[str]:
    """
    Get the microbenchmarks.

    Returns:
    - microbenchmarks (list): List of microbenchmarks.
    """

    from gem5.resources.resource import obtain_resource
    from gem5.resources.resource import CustomResource

    # TODO: this will be replaced with the suite later

    microbenchmarks = [
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CCa.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CCe.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/DPT.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/DPTd.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/EM1.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/EM5.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MIM.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MIM2.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CS1.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CS3.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CRf.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/M_Dyn.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/ML2_BW_ld.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/ML2_BW_st.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/ML2_BW_ldst.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MIP.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/STL2.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MD.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MI.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CCl.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CCm.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CCh.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CCh_st.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/ED1.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/DP1f.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/STc.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/EI.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/DP1d.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CRd.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MC.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MCS.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/DPcvt.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CF1.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/STL2b.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/CRm.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/EF.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/ML2.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/ML2_st.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MM.riscv"
        ),
        CustomResource(
            local_path="/projects/gem5/microbench_vertical-bins/bin-m5ops/MM_st.riscv"
        ),
    ]

    return microbenchmarks


def extract_one_configuration(board_configuration):
    """
    Extract one board configuration.
    This function is used to extract one board configuration from one element of the list of board configurations.

    Parameters:
    - board_configuration (list): List of board configurations.

    Returns:
    - config (list): Particular config. [element 0 of board_configuration]
    - microbenchmark (gem5.resources.resource.BinaryResource): Particular Microbenchmark. [element 1 of board_configuration]
    """
    return board_configuration[0], board_configuration[1]


def create_board_configurations():
    """
    Create the board configurations.
    It will create a list of board configurations.
    Each board configuration is a list of tuples.
    Each tuple contains a list of configurations and a microbenchmark.

    Returns:
    - board_configurations (list): List of board configurations.
    """

    from create_parameter_space import processed_parameters

    microbenchmarks = get_microbenchmarks()
    parameters, values = processed_parameters()
    board_configurations = []

    for i in range(len(parameters)):
        board_configurations_per_parameter = []
        parameter = parameters[i]
        for j in range(len(values[i])):
            config = [f"{parameter} = {values[i][j]}"]
            for microbenchmark in microbenchmarks:
                board_configurations_per_parameter.append(
                    (config, microbenchmark)
                )
        board_configurations.append(board_configurations_per_parameter)
    return board_configurations


def run_one_configuration(
    board_configuration, correct_board_configurations=None
):
    """
    Run one board configuration, i.e., one parameter value and one microbenchmark.

    It also applies the previously computed correct board configurations if they are exist.

    Parameters:
    - board_configuration (list): List of board configurations.
    - correct_board_configurations (list): List of correct board configurations.
    """

    from gem5.simulate.simulator import Simulator
    from gem5.prebuilt.rocketchip.rocketchip_board import RocketBoard
    from gem5.isas import ISA
    from gem5.utils.requires import requires

    requires(isa_required=ISA.RISCV)

    config, microbenchmark = extract_one_configuration(board_configuration)

    board = RocketBoard(clk_freq="100MHz")

    if correct_board_configurations is not None:
        for correct_board_configuration in correct_board_configurations:
            board.apply_config(correct_board_configuration)

    board.apply_config(config)

    board.set_se_binary_workload(microbenchmark)
    simulator = Simulator(board)
    simulator.run()


if __name__ == "__m5_main__":
    # for testing
    from post_process import postprocess
    from run import create_correct_board_configurations

    print(create_correct_board_configurations())
    postprocess("IPC", "clusivity")
    print(create_correct_board_configurations())
    # postprocess("IPC", "point_of_coherency")
    # print(create_correct_board_configurations())
    # postprocess("IPC", "core.executeFuncUnits.funcUnits[0].opLat")
    # print(create_correct_board_configurations())
