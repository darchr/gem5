# Running tests on the RISCV Matched Board with a Plackett-Burman Design

This document describes the scripts for testing and the process followed to perform tests.

## Part 1: Methodology

For this investigation, we used the methodology outlined in the paper "A Statistically Rigorous Approach for Improving Simulation Methodology" by Yi et al. The methodology uses a Plackett Burman design, which is a matrix filled with 1s and -1s, and whose dimensions are divisible by 4. This matrix can also be folded over, which simply means that the values of the original matrix are all multiplied by -1.

Each row of the matrix represents a configuration of the system, and each column represents one of the parameters being varied. A -1 represents a value of a parameter that is slightly below the range of standard values, and a +1 represents a value that is slightly higher.

The number of columns in the matrix must be greater than the number of parameters. For the RISCV Matched Board, we used a matrix size of 16, as we had a total of 13 parameters. These are listed in `nopipe_params.json`. The first value (index 0) was the value from the original RISCV Matched Board code, while the second value (index 1) was an arbitrarily chosen larger value.

## Part 2: Tools
### Generating the PB design

Initially, we thought that we would have around 70-80 parameters. We were able to find several generators for PB designs, but these only covered PB designs up to size 48. As such, we had to find other methods. We used MATLAB to generate Hadamard matrices, which are


### Creating a Configured JSON

The first step was to make a script that would take as input:
1. a JSON file with a list of high and low values for each parameter, and
2. a row of the PB design,

and output a JSON with only one value for each parameter.
