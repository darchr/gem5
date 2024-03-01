# Documentation on the methodology work for DRAM cache experiments

This is not a very detailed or formal document, but tries to at least point to
the relevant links or paths and important notes.

- DRAM cache component is part of the stdlib in this repo ("src/python/gem5/components/memory/dcache.py").
- Octopi-cache is a three level MESI cache hiearchy 


## gem5 scripts used to take checkpoints

- **NPB D Class:**  Octopi-cache/riscv-2channel-1ccd-checkpoint-timing.py

- **NPB C Class:** ctopi-cache/riscv-2channel-1ccd-checkpoint-timing.py

- **GAPBS (2^25 vertices):** Octopi-cache/riscv-2channel-1ccd-checkpoint-timing-gapbs.py


## Bash scripts used (will show the directory where the checkpoints and results are)

- **NPB:** `npb_checkpoint.sh` and `npb_checkpoint_c_class.sh`
- **GAPBS:** `gapbs_checkpoint.sh`

The gem5 directory I have been using is here: `/home/aakahlow/Dcache_Src/dramCacheController`.
D Class and GAPBS checkpoints will go there. The C class checkpoints will be stored in /projects (exact path in the npb_checkpoint_c_class.sh script).

## Looppoint stuff

- The branch with O3 loop point analysis stuff: https://github.com/darchr/gem5/commits/looppointAnalysis-Ayaz
- example.py script in the above branch is a modified version of Zhantong's example script to use looppoint analysis probe and print most recent PC + count.

