
#!/bin/bash

# $1 is the class of the NPB to run

if [ $1 != 'C'] && [ $1 != 'D']
then
    echo "Run with C or D Class"
    exit
fi

bms=(bt cg dc ep ft is lu mg sp ua)

if [! -d results_npb]; then
     mkdir -p results_npb;
fi

for bm in "${bms[@]}"
do
echo $bm
M5_OVERRIDE_PY_SOURCE=TRUE build/RISCV/gem5.opt --debug-flags=O3LooppointAnalysis -re --outdir=results_npb/$bm.$1.O3 Octopi-cache/restore-npb-gapbs-looppoint.py --benchmark $bm.$1 --size $1 --ckpt_path /projects/gem5/dramcache/jason-checkpoints/npb/c/$bm/ckpt &
done
