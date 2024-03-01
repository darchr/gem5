
#!/bin/bash

# $1 is the class of the NPB to run

if [ $1 != 'C'] && [ $1 != 'D']
then
    echo "Run with C or D Class"
    exit
fi

bms=(bt cg dc ep ft is lu mg sp ua)

if [! -d checkpoints-npb]; then
     mkdir -p checkpoints-npb;
fi

for bm in "${bms[@]}"
do
echo $bm
build/RISCV/gem5.opt -re --outdir=$bm.timing Octopi-cache/riscv-2channel-1ccd-checkpoint-timing.py --benchmark $bm --size $1 --ckpt_path checkpoints-npb/$bm.timing/$bm &
done
