
#!/bin/bash

# $1 is the class of the NPB to run

if [ $1 != 'C'] && [ $1 != 'D']
then
    echo "Run with C or D Class"
    exit
fi

bms=(bt cg dc ep ft is lu mg sp ua)

if [! -d /projects/gem5/npb-checkpoints]; then
     mkdir -p /projects/gem5/npb-checkpoints;
fi

for bm in "${bms[@]}"
do
echo $bm
M5_OVERRIDE_PY_SOURCE=TRUE build/RISCV/gem5.opt -re --outdir=/projects/gem5/npb-checkpoints/results/$bm.$1.timing Octopi-cache/riscv-2channel-1ccd-checkpoint-timing-npb-c.py --benchmark $bm --size $1 --ckpt_path /projects/gem5/npb-checkpoints/$bm.$1.timing/$bm.$1 &
done
