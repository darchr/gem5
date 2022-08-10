import argparse
from gem5.resources.resource import Resource, CustomResource
from gem5.simulate.simulator import Simulator
from hifive_board import HiFiveUnmatchedBoard
from typing import List

# collect optional CLI arg for RISCV binary to run
parser = argparse.ArgumentParser(description="Binary to run on system")
parser.add_argument(
    "--riscv_binary", type=str, help="The RISCV binary to execute on the CPU",
    default="riscv-hello",
)
parser.add_argument(
    "--argv", type=str, help="CLI argument to the binary",
    default=""
)
args = parser.parse_args()

board = HiFiveUnmatchedBoard()

# run default binary if no binary provided, otherwise run CLI provided resource
if args.riscv_binary == "riscv-hello":
    board.set_se_binary_workload(
        Resource(args.riscv_binary)
    )
else:
    board.set_se_binary_workload(
        CustomResource(args.riscv_binary),
        arguments=[args.argv]
    )

simulator = Simulator(board=board)
simulator.run()