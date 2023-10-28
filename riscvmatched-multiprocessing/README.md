# Greedy Multiprocessing Pipeline for "Fine-tuning" a CPU Model

## Introduction

gem5's multiprocessing module is a powerful tool that allows users to run multiple gem5 simulations in parallel. This is especially useful when running a large number of simulations with different properties, as it allows users to run all of these simulations in parallel, rather than sequentially. This results in a significant reduction in the overall time required for the simulations. For example, a set of simulations that would normally take 1 to 2 months to complete sequentially can now be completed in just 1 month thanks to parallelization, effectively saving valuable time in the research or testing process.

This README will go over how to use the multiprocessing module to run multiple simulations in parallel, and how to use it to "fine-tune" a CPU model.

## Defining "Fine-tuning"

In this context, "fine-tuning" refers to the process of running multiple simulations with different CPU parameters in order to find the optimal set of parameters for a given workload.
For example, if we wanted to find the optimal L1 cache size for a given workload, we could run multiple simulations with different L1 cache sizes and compare the results to the baseline to find the optimal L1 cache size.

For more information on initial work into "fine-tuning" a CPU model for higher accuracy, please refer to [Validating Hardware and SimPoints with gem5: A RISC-V Board Case Study](https://www.gem5.org/assets/files/workshop-isca-2023/posters/validating-hardware-and-simpoints-with-gem5-poster.pdf).

## Goals of the Multiprocessing Pipeline

The pipeline that was built for "fine-tuning" a CPU model has the following goals:

1. To find the optimal set of parameters for a given workload without having to manually set the different parameters in each simulation.

2. To run all of the simulations in parallel to reduce the overall time required for the simulations.

3. To be able to easily add new parameters to the pipeline and define the relation between their lower and upper bounds without having to modify the code.

4. To be able to easily add new workloads to the pipeline.

5. To be able to define the number of simulations to be run per process pool without having to modify the code.

## Multiprocessing Pipeline

The multiprocessing pipeline consists of the following components:

1. A configuration file that defines the parameters to be tuned, their lower and upper bounds, and the relation between them.

2. A script that generates the different combinations of parameters to be used in the simulations.

3. A script that creates a 2D list of all the combinations of parameters and benchmarks to be used in the simulations, and defines the board, applies one combination of parameter and benchmark to the board, and runs the simulation.

4. A script that runs the simulations using the multiprocessing module.

5. A script that parses the results of the simulations and generates a JSON file containing the statistic of interest (in this case, IPC) for each combination of parameters and benchmark.

### Configuration File

The configuration file defines the parameters to be tuned, their lower and upper bounds, and the relation between them. It is a JSON file in which each parameter is defined as follows:

```json
{
    "parameter": "processor.cores[:].core.executeFuncUnits.funcUnits[0].opLat",
    "min_value": 1,
    "max_value": 3,
    "annotation": "Latency of the integer functional unit",
    "step": "add"
}
```

The fields in the JSON file are as follows:

1. `parameter`:
    The parameter to be tuned. This can be any parameter in the gem5 configuration file.
    For example, `processor.cores[:].core.executeFuncUnits.funcUnits[0].opLat` is the parameter for the latency of the integer functional unit.

    Caveat: The parameter must be defined in a way that allows it to be applied to the board using the `apply_config` function in `src/python/m5/SimObject.py`, found [here](https://github.com/gem5/gem5/blob/48a40cf2f5182a82de360b7efa497d82e06b1631/src/python/m5/SimObject.py#L1298). The best way to find the text to use for each parameter is to use the `enumerateParams()` function in a gem5 simulation, defined [here](https://github.com/gem5/gem5/blob/48a40cf2f5182a82de360b7efa497d82e06b1631/src/python/m5/params.py#L328) and look at the output.

2. `min_value`:
The minimum value of the parameter to be tuned.

3. `max_value`:
The maximum value of the parameter to be tuned.

4. `annotation`:
A description of the parameter to be tuned. This is not important to the pipeline, but is useful for documentation purposes.

5. `step`:
The step size to be used when generating the different combinations of parameters to be used in the simulations. This can be any of the following:

    - `add`:
    The step size is added to the minimum value until the maximum value is reached.

    - `multiply`:
    The step size is multiplied by 2 to the minimum value until the maximum value is reached (i.e. the step size is doubled each time).

    - `flip`:
    The step is an array of 2 values, where the first value is the `min_value` and the second value is the `max_value`. This is useful for parameters that have a boolean value, or a string that has 2 possible values.

### Define the Workloads to be Used

The workloads to be used in the simulations are defined in the `get_microbenchmarks()` function in `gem5/riscvmatched-multiprocessing/riscvmatched_multiprocessing.py`, found [here](insert).

Currently, this function supports the use of a list of WorkloadResource objects, which are defined in `src/python/gem5/resources/resource.py`, found [here](https://github.com/gem5/gem5/blob/f7ad8fe4350be7d4299b22d9030385e3f261c7d8/src/python/gem5/resources/resource.py#L745).

### Generate the Different Combinations of Parameters

The generation of the different combinations of parameters is done in the `create_board_configurations()` function in `gem5/riscvmatched-multiprocessing/riscvmatched_multiprocessing.py`, found [here](insert).

The function creates a 2D list of all the combinations of parameters and benchmarks to be used in the simulations.

### Define the Board

The board is defined in the `run_one_configuration()` function in `gem5/riscvmatched-multiprocessing/riscvmatched_multiprocessing.py`, found [here](insert).

This script applies one combination of parameter and benchmark to the RISCVMatched board, and runs the simulation.

### Run the Simulations

The simulations are run using the multiprocessing module in `gem5/riscvmatched-multiprocessing/run.py`, found [here](insert).

This file also defines a cluster of gem5 configurations to be run per process pool, through the environment variable `GEM5_CONFIGS_PER_CLUSTER`. This allows users to define the number of simulations to be run per process pool without having to modify the code.

### Parse the Results

The results of the simulations are parsed in the `postprocess()` function in `gem5/riscvmatched-multiprocessing/post_process.py`, found [here](insert).

The results are written to a JSON file containing the statistic of interest (in this case, IPC) for each combination of parameters and benchmark.
They should look like:

``` json
[
    {
        "parameter": "enableIdling",
        "values": [
            {
                "value": "True",
                "workloads": [
                    {
                        "workload": "riscv-cca-run",
                        "IPC": 1.2098081218798735
                    }
                ]
            },
            {
                "value": "False",
                "workloads": [
                    {
                        "workload": "riscv-cca-run",
                        "IPC": 1.2098131875912785
                    }
                ]
            }
        ]
    }
]
```

where `parameter` is the parameter that was tuned, `values` is a list of the different values of the parameter, `value` is the value of the parameter, `workloads` is a list of all the used WorkloadResources, `workload` is the specific WorkloadResource that was run, and `IPC` is the IPC of that WorkloadResource with that particular value of the parameter.

### Running the Pipeline

To run the pipeline, run the following command:

``` bash
    ../build/RISCV/gem5.opt run.py
```
