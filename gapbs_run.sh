
#!/bin/bash

# $1 is the size of the gapbs to run

if [ $1 != '22'] && [ $1 != '25']
then
    echo "Run with different size"
    exit
fi

bms=(bc bfs cc pr tc sssp)

if [! -d GAPBS_Base]; then
     mkdir -p GAPBS_Base;
fi

for bm in "${bms[@]}"
do
echo $bm
M5_OVERRIDE_PY_SOURCE=TRUE build/RISCV/gem5.opt -re --outdir=GAPBS_Base/$bm.22.O3 Octopi-cache/restore-npb-gapbs-looppoint.py --benchmark $bm --size $1 --ckpt_path /projects/gem5/dramcache/jason-checkpoints/gapbs/$1/$bm/ckpt &
done
