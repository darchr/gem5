# HiFive GEM5 Board Specs and Usage

## Specs for the HiFive Unmatched

### Cache Hierarchy
- L1D Size : 32kB
- L1D Associativity : 8
- L1I Size : 32kB
- L1I Associativity : 4
- L2 Size : 2MB
- L2 Associativity : 16
- MMU Size : 4kiB
- Branch Predictor:
    16 entry BTB, 3.6 kiB branch history table, 8 entry indirect jump target predictor, 6 entry RAS

### Memory
- 16GB DDR4 channeled subsystem
- Starting address : 0x80000000

### CPU
- Inherited from MinorCPU
- Latencies: Int inst. (3 cycles), Mul inst. (3 cycles), Mem inst. (3 cycles), Div inst. (6 cycles)

### Board
- Clock Freq.: 1.2 GHz (base: 1.0 GHz, upper limit: 1.5 GHz)


## Running the HiFive Unmatched Board in GEM5
The source code for the board can be found in *newboard/hifivenew/*. To run this
board in gem5, do the following:  

1. Copy the board directory into the gem5 source.
    ```sh
    cp -r newboard/hifivenew/ <path_to_gem5_source>/src/python/gem5/prebuilt/
    ```

2. Change the current working directory to the gem5 source directory.

3. In src/python/gem5/components/processors/cpu_types.py, create a new enum 
underneath the CPUTypes enum called CustomCPUTypes with the name of the U74 CPU.
    ```py
    class CustomCPUTypes(Enum):
        U74 = "u74"
    ```

4. In ```src/python/gem5/components/processors/abstract_core.py```: import
CustomCPUTypes and add **CustomCPUTypes.U74: "U74CPU",** to the
_cpu_types_string_map dictionary.
    ```py
    from .cpu_types import CPUTypes, CustomCPUTypes

    _cpu_types_string_map = {
            CPUTypes.ATOMIC : "AtomicSimpleCPU",
            CPUTypes.O3 : "O3CPU",
            CPUTypes.TIMING : "TimingSimpleCPU",
            CPUTypes.KVM : "KvmCPU",
            CPUTypes.MINOR : "MinorCPU",
            CustomCPUTypes.U74: "U74CPU",
        }
    ```

5. In ```src/python/gem5/components/processors/simple_processor.py```, import
CustomCPUTypes. In the **incoprorate_processor()** function, add 
CustomCPUTypes.U74 to the list of processors in the first if condition, 
alongside CPUTypes.MINOR.
    ```py
    from .cpu_types import CPUTypes, CustomCPUTypes

     # Set the memory mode.
    if self._cpu_type in (CPUTypes.TIMING, CPUTypes.O3, CPUTypes.MINOR,
    CustomCPUTypes.U74):
    ```

6. Copy and paste all imports and classes from
```<path_to_this_repo>/newboard/hifivenew/HiFiveCPU.py ```(exclude U74Processor class)
to the bottom of src/arch/riscv/RiscvCPU.py.

7. Compile the gem5 RISCV binary.
    ```sh
    scons build/RISCV/gem5.opt -j<threads>
    ```

8. Run the board's hello world SE mode simulation script.
    ```sh
    ./build/RISCV/gem5.opt src/python/gem5/prebuilt/hifivenew/HiFiveRun.py
    ```
    This should run a successful simulation and print "Hello world!".
    You can also run a custom RISCV binary by passing in the absolute path
    to the binary via the command line. Here is an example:
    ```sh
    ./build/RISCV/gem5.opt src/python/gem5/prebuilt/hifivenew/HiFiveRun.py --riscv_binary=/home/rangij/gem5/src/python/gem5/prebuilt/hifivenew/bins/CCa.RISCV
    ```