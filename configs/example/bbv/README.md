---
layout: documentation
title: gem5 Simpoints
parent: gem5-standard-library
doc: gem5 documentation
permalink: gem5/configs/example/bbv/README.md
author: Georgy Zaets
---

## `Understanding SimPoints and Basic Block Vectors within gem5`

_______________________________________________________________________________________

Generation of `Basic Block Vectors` and `SimPoints` within `gem5` for simulator acceleration


A more complete description of the `SimPoints` method can be found in the papers “Basic Block Distribution Analysis to Find Periodic Behavior and Simulation Points in Applications” and "Automatically Characterizing Large Scale Program Behavior" by T. Sherwood, E. Perelman, G. Hamerly, and B. Calder.


In computer architecture research, running a benchmark on a cycle-level simulator can drastically slow down a program's runtime compared to a native hardware system, making it take days, weeks, or even longer to run full benchmarks. By utilizing `SimPoints` this can be reduced significantly, while still retaining reasonable accuracy.


Using `SimPoints` allows us to accelerate architectural simulations by only running a small portion of a program and then extrapolating total behavior from this small portion. `SimPoints` samples programs with a variety of features like IPC, branch prediction, cache hit/miss rates and obtains a phase analysis of the program. Most programs exhibit phase-based behavior, which means that at various times during execution a program will encounter intervals of time where the code behaves similarly to another interval within the same program. By detecting these intervals and clustering them together, we can generate an approximation of the total program by only simulating the minimum amount of representative intervals, and then scaling up the results accordingly to get accurate simulation data.


The interval sizes used to create `SimPoints` are passed in the program in a form of basic block vectors, usually 10 and 100 million execution instructions in length.  Each simulation point is provided as the number of the intervals since the beginning of the execution. A basic block is a linear section of code with one entry and one exit point. A `basic block vector`, also known as a `BBV`, is a list of basic blocks entered during program execution, and a count of how many times each basic block was run. Basic blocks contain the information `SimPoints` needs to do the necessary analysis and clustering.


`gem5’s` integrated `BBV` generator is a tool that produces basic block vectors for use with the `SimPoints` analysis tool. A user-defined `BBV` config script creates frequency vectors for each basic block of execution in the program. Choosing the interval length for the profile is important, since this is assumed to be the length of a single simulation point in the rest of the analysis below. For example, if you set the interval length 10 million, then each simulation point is calculated assuming you will simulate each point for 10 million instructions. MAKE SURE THAT THE `BBV` FILE WAS GENERATED IN THE SAME ISA BUILD AS THE ISA OF THE COMPILED EXECUTABLE BENCHMARK FILE!!! 


A configuration script is required in order to generate a `BBV` file. A sample configuration file has been provided. You can find it here:


```sh
/configs/example/bbv/bbv_generate.py
```


 In order to generate `basic blocks` and their appropriate `basic block vectors,` the config script is required to have the following format:

```python
processor = SimpleProcessor(
    cpu_type=CPUTypes.ATOMIC,
    isa=ISA.X86, #MAKE SURE THAT THE `BBV` FILE WAS GENERATED IN THE SAME ISA AS THE ISA OF THE FILE!!! 
    num_cores=1, # SimPoints only works with one core
)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

processor.cores[0].core.addSimPointProbe(10000000) #Probe for SimPoints

board.set_se_binary_workload(
    binary = CustomResource("/home/usr1/mibench/automotive/basicmath/basicmath_largeX86") #EDIT THIS PATH FOR RIGHT COMPILED FILE LOCATION
)
```


The Probe for SimPoints looks for occurences of entrance and exit points within the machine code of the executed program. In other words, this probe finds and generates the `basic blocks` that later get inserted into the appropriate `basic block vectors.` If the entry point of the code block has already appeared in the current or previous vetor, then the frequency of the `basic block` gets increased by 1.


The output .bb / .bbz file will appear in the following format in the m5out directory:


```sh
T:1:1 :2:6 :3:10 :4:28 :5:2 :6:5
T:78:452837 :88:329336 :265:817060 :273:27578
T:78:449515 :88:326920 :265:816820 :273:27248
```


Each new interval/basic block vector starts with an identifying letter ‘T.’ The vectors consist of basic block identifiers and frequency pairs, with a single pair for a unique basic block that was entered and exited during program’s execution. The `format for each basic block` is as follows: a basic block identifier, colon, and the frequency of access of that basic block (entry amount multiplied by the number of instructions in the block). The frequency count is multiplied by the number of instructions that are in the basic block, in order to weigh the count so that instructions in small basic blocks aren't counted as more important than instructions in large basic blocks. The pairs are separated from each other by a space.


The `SimPoints` 3.2 program only processes lines that start with a "T". All other lines are ignored. Traditionally comments are indicated by starting a line with a "#" character. Some other `BBV` generation tools, such as PinPoints, generate lines beginning with letters other than "T" to indicate more information about the program being run. We do not generate these, as the `SimPoints` utility ignores them.


After you have the basic block vector file, you will pass it to the `SimPoints` generator (currently, `gem5` works with SimPoint 3.2). This will perform the clustering analysis and choose the minimal number of simulation points to best represent the full execution of the program. You will need to provide the `BBV` file which contains multiple basic block vectors and the maximum number of simulation points you wish to extract. 


The generator produces two files: resulting `SimPoints` and their weights. Once a set of `SimPoints` and their respective weights have been collected, they can be used to quickly simulate parts of a program to represent the entire execution. 


The final step in using `SimPoints` is to combine the simulation results to estimate the full execution. To combine the `SimPoints`, each point first needs to be weighted by its corresponding weight in the .weights file.  Each weight in the .weights file contains a weight for every simulation point. The weight represents the percent of overall executed instructions each simulation point represents. The weight for a simulation point is the total instructions executed by all of the intervals in that simulation point's cluster divided by the total number of instructions executed for the input pair.


After getting all the files, the simpint output file looks like this:


```sh
3 0
123 1
345 2
```


Each line represents a `SimPoint` taken. Within each line, the number on the left means the cluster number of this `SimPoint`, the number on the right is the `SimPoint` index. For example, `Simpoint` 0 corresponds to 3rd interval and if each vector consists of 10,000,000 instructions, then the instruction is at 3 * 10,000,000.


Sample weights output file data is provided below:


```sh
0.480122 0
0.275229 1
0.244648 2
```


The weight output file has a similar format except the interval number changes to a weight number. The weights sum up to 1 and each weight tells you how important this `Simpoint` is. The `SimPoints` are used with their according weights to compute the weighted average for a given metric.


After getting the `SimPoint` file and the Weights file, the next step is to use those files to take checkpoints using `gem5’s` checkpointing function. Input the `SimPoint` files paths for the `SimPoint` config file. Refer to the format of the `SimPoints` class when providing SimPoints and their weights in the config file:


```sh
/src/python/gem5/utils/simpoint.py
```


A config file for taking Checkpoints from `SimPoints` can be found here:


```sh
/configs/example/gem5_library/checkpoints/simpoints-se-checkpoint.py
```


You can manually fill in the SimPoints and their appropriate weights, by identifying `simpoint_list` and `weight_list` or you can enter in the direct file paths to them. You must change the paths for the `simpoint_file_path` and `weight_file_path` in the format of the `SimPoint` Class Constructor:

```python
def __init__(
        self,
        simpoint_interval: int,
        simpoint_file_path: Path = None,
        weight_file_path: Path = None,
        simpoint_list: List[int] = None,
        weight_list: List[int] = None,
        warmup_interval: int = 0,
    ) -> None:
```


For example, if you wish to enter the `SimPoints` and their Weights manually, the config file should look something like this:


```sh
simpoint = SimPoint(
    simpoint_list=[2, 3, 5, 15],
    weight_list=[0.1, 0.2, 0.4, 0.3],
    simpoint_interval=1000000,
    warmup_interval=1000000,
)
```


The Process of taking checkpoints happens here in the Config script:


```python
simulator = Simulator(
    board=board,
    on_exit_event={
        # using the SimPoints event generator in the standard library to take
        # checkpoints
        ExitEvent.SIMPOINT_BEGIN: save_checkpoint_generator(dir)
    },
)
```


The exit event generator takes checkpoints whenever the `SimPoint` is found. To learn more about taking checkpoints and restoring them within gem5, refer to the Checkpoint gem5 documentation located here:


```sh
/documentation/general_docs/checkpoints/
```


Remember, make sure that the ISA of this config file is the same ISA as the program that you are generating `SimPoints` for!
