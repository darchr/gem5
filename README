## Analyzing Google Traces in gem5

This branch contains necessary code and scripts to run google workload traces with
gem5. These changes will eventually move to the mainline gem5 repo.

Follow the following steps to run google traces with gem5:

Compile gem5.

Download Google Workload Traces from [here](https://dynamorio.org/google_workload_traces.html).

In the current folder there are two example scripts that can be used to run google traces.

- [drtrace.py](./drtrace.py): This is a normal python script that imports all gem5 objects to manually set up a target system to simulate. `DRTraceReader` and `DRTracePlayer` are the trace reader and player objects that are used in this script. Example command:

```sh
build/X86/gem5.opt drtrace.py --path <path of folder containing all traces> --workload <benchmark_name, options: charlie, delta, merced, whiskey> --players <number of trace players to use> --dram <DRAM device to use, options:ddr4_2400, hbm_2000, ddr5_8400>
```

- [drtrace-stdlib.py](./drtrace-stdlib.py): This script uses gem5 standard library components to set-up a system to simulate. `DRTracePlayerGenerator` is the standard library component that creates a trace reader/player system. The arguments this class accepts include: `num_cores` `max_ipc`, and `max_outstanding_reqs`. Example command:

```sh
build/X86/gem5.opt drtrace-stdlib.py --path <path of folder containing all traces> --workload <benchmark_name, options: charlie, delta, merced, whiskey> --players <number of trace players to use>  --ruby <Use classic or ruby caches, options: 0,1>
```

## Default gem5 README

This is the gem5 simulator.

The main website can be found at http://www.gem5.org

A good starting point is http://www.gem5.org/about, and for
more information about building the simulator and getting started
please see http://www.gem5.org/documentation and
http://www.gem5.org/documentation/learning_gem5/introduction.

To build gem5, you will need the following software: g++ or clang,
Python (gem5 links in the Python interpreter), SCons, zlib, m4, and lastly
protobuf if you want trace capture and playback support. Please see
http://www.gem5.org/documentation/general_docs/building for more details
concerning the minimum versions of these tools.

Once you have all dependencies resolved, type 'scons
build/<CONFIG>/gem5.opt' where CONFIG is one of the options in build_opts like
ARM, NULL, MIPS, POWER, SPARC, X86, Garnet_standalone, etc. This will build an
optimized version of the gem5 binary (gem5.opt) with the the specified
configuration. See http://www.gem5.org/documentation/general_docs/building for
more details and options.

The main source tree includes these subdirectories:
   - build_opts: pre-made default configurations for gem5
   - build_tools: tools used internally by gem5's build process.
   - configs: example simulation configuration scripts
   - ext: less-common external packages needed to build gem5
   - include: include files for use in other programs
   - site_scons: modular components of the build system
   - src: source code of the gem5 simulator
   - system: source for some optional system software for simulated systems
   - tests: regression tests
   - util: useful utility programs and files

To run full-system simulations, you may need compiled system firmware, kernel
binaries and one or more disk images, depending on gem5's configuration and
what type of workload you're trying to run. Many of those resources can be
downloaded from http://resources.gem5.org, and/or from the git repository here:
https://gem5.googlesource.com/public/gem5-resources/

If you have questions, please send mail to gem5-users@gem5.org

Enjoy using gem5 and please share your modifications and extensions.
