# Running tests on the RISCV Matched Board with a Plackett-Burman Design

This document describes the scripts for testing and the process followed to perform tests.

## Part 1: Methodology

For this investigation, we used the methodology outlined in the paper "A Statistically Rigorous Approach for Improving Simulation Methodology" by Yi et al. The methodology uses a Plackett Burman design, which is a matrix containing only 1s and -1s, and whose dimensions are divisible by 4.

This matrix can also be multiplied by a -1 to form a "foldover matrix", thus doubling the number of rows/potential configurations.

Each row of the matrix represents a configuration of the system, and each column represents one of the parameters being varied. A -1 represents a value of a parameter that is slightly below the range of standard values, and a +1 represents a value that is slightly higher.

The number of columns in the matrix must be greater than the number of parameters. For the RISCV Matched Board, we used a matrix size of 16, as we had a total of 13 parameters. These are listed in `nopipe_params.json`. The first value (index 0) was the value from the original RISCV Matched Board code, while the second value (index 1) was an arbitrarily chosen larger value.

## Part 2: Tools
### Creating a Configured JSON

The first step was to make a script that would take as input:
1. a JSON file with a list of high and low values for each parameter, and
2. a row of the PB design,

and output a JSON with only one value for each parameter. We used `nopipe_params.json` as the input JSON file, and initially used a 16 by 16 Hadamard matrix generated in MATLAB to get rows of the PB design. We were not sure if all Hadamard matrices could be used as PB designs, so we later used a size 16 PB design from the original paper on PB designs.

We used the script `json_maker.py` to generate the JSONs.

### Using the Configured JSON to Parameterize the RISCV Matched Board

The RISCV Matched Board code was modified to be able to take in a JSON, convert it to a Python dictionary, then set the values of its parameters to the values from the JSON.

The files are found in src/python/gem5/prebuild/riscvmatched.

### Running the benchmarks

The benchmarks were run by using the bash scripts `pb_matched_run.sh` and `microbench_pb_run.sh`. `pb_matched_run.sh` was used for riscv-print-this, riscv-bubblesort, riscv-floatmm, and riscv-hello, while `microbench_pb_run.sh` was used to run a number of microbenchmarks.

`pb_matched_run.sh` loops over all combinations of row (0-15) and foldover (0 or 1) for a given benchmark, while `microbench_pb_run.sh` loops over all microbenchmarks for a given row and foldover.

The output files are put into either matched_board_tests_MATLAB or matched_board_tests_paper. Within these directories, the files are sorted by benchmark, then by the row, foldover, and matrix size. The name format for these subdirectories is foldover_row_matrixsize. For example, the directory 0_6_16 corresponds to the config json that was generated with row index 6 of the original (not the foldover) PB design.

Some of the configurations under matched_board_tests_MATLAB have multiple runs, while all of the runs under matched_board_tests_paper have only one run. When running the simulations the first time, we wanted to see if there were any variations in simulation time between different runs of the same system. We did not observe any variation, so we only did one run for each system the second time.
