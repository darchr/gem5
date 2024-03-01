
#!/bin/bash

# $1 is the size of the gapbs to run

if [ $1 != '22'] && [ $1 != '25']
then
    echo "Run with different size"
    exit
fi

bms=(bc bfs cc pr tc sssp)

if [! -d checkpoints-gapbs]; then
     mkdir -p checkpoints-gapbs;
fi

for bm in "${bms[@]}"
do
echo $bm
build/RISCV/gem5.opt -re --outdir=$bm.timing Octopi-cache/riscv-2channel-1ccd-checkpoint-timing-gapbs.py --benchmark $bm --size $1 --ckpt_path checkpoints-gapbs/$bm.timing/$bm &
done
