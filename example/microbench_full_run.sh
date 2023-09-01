# Shell script for running the full set of microbenchmarks.
# Options are:
# FOLDOVER = [0,1]. Whether the Hadamard matrix is the original, or if it is the foldover matrix (whose values are multiplied by -1).
# ROW = [0 .. 15]. Row of the Hadamard matrix.
# SIZE = 16, or some other size of hadamard matrix, provided that you have a high/low parameter json
# FILE = the high/low json, which has a list of a low and high value for each parameter. The number of parameters in the high/low json should be less than SIZE.

# FOLDOVER=0
# ROW=5
SIZE=13
MAX_IND=$(($SIZE - 1))
FILE="nopipe_params.json"
LAUNCH_CTR=0
BATCH_CTR=0
PIDS=()
# Go through each microbenchmark

# for FOLDOVER in 0 1;
# do


# Check if directory exists. If not, make the directory.
if [ ! -d ./matched_board_tests_full ]; then
    mkdir ./matched_board_tests_full
fi

# Check if directory exists. If not, make the directory.
if [ ! -d ./matched_board_tests_full/microbench ]; then
    mkdir ./matched_board_tests_full/microbench
fi

for ROW in $(seq 0 $((2**$SIZE)))
do

for benchmark in riscv-cca riscv-cce riscv-cch riscv-cch-st riscv-cci riscv-ccm riscv-crd riscv-crf riscv-cs1 riscv-cs3 riscv-dp1d riscv-dp1f riscv-dpcvt riscv-dpt riscv-dptd riscv-ed1 riscv-ef riscv-ei riscv-em1 riscv-em5 riscv-m-dyn riscv-mc riscv-mcs riscv-md riscv-mi riscv-mim riscv-mim2 riscv-mip riscv-ml2-bw-ld riscv-ml2-bw-ldst riscv-ml2-bw-st riscv-stc riscv-stl2;
do

# for benchmark in riscv-cca;
# do

#echo "Now running: foldover = ${FOLDOVER}, PB matrix row = ${ROW}, PB matrix size = ${SIZE}"

#echo "Foldover = ${FOLDOVER}, so the row number in the foldover matrix is" $((($SIZE*$FOLDOVER)+$ROW))



if [ ! -d ./matched_board_tests_full/microbench/$benchmark ]; then
    mkdir ./matched_board_tests_full/microbench/$benchmark
fi

if [ ! -d ./matched_board_tests_full/microbench/$benchmark/${ROW}_${SIZE} ]; then
    mkdir ./matched_board_tests_full/microbench/$benchmark/${ROW}_${SIZE}
fi

#generate configured json
python3 json_maker.py --foldover=0 --row=${ROW} --matrix_size=${SIZE}  --file=${FILE}  --nums="one" --mode="full"

# Allows you to do several runs
# for i in {1..3}
# do
    # Check if directory exists. If not, make the directory.
    # if [ ! -d ./matched_board_tests_full/microbench/$benchmark/${FOLDOVER}_${ROW}_${SIZE}/${i} ]; then
    #     mkdir ./matched_board_tests_full/microbench/$benchmark/${FOLDOVER}_${ROW}_${SIZE}/${i}
    # fi

    # run simulation. Redirect terminal output to the output/stats directory.
    /scr/bees/gem5/build/RISCV/gem5.fast --outdir=matched_board_tests_full/microbench/${benchmark}/${ROW}_${SIZE}/${i} simple-config.py --benchmark=${benchmark} --foldover=0 --row=$ROW --matrix_size=$SIZE --mode=full > ./matched_board_tests_full/microbench/${benchmark}/${ROW}_${SIZE}/${i}/terminal.txt &
    echo "Simulation for row ${ROW} launched."
    echo "Simulation for row ${ROW}, benchmark ${benchmark} launched." >> matched_board_tests_full/microbench/run_log.txt
    ((LAUNCH_CTR++))
    echo "Launch counter:" $LAUNCH_CTR
    PIDS+=($!)
    echo $PIDS
    if [[ $LAUNCH_CTR -eq 100 ]]; then
        for pid in ${PIDS[@]}; do
            wait $pid
        done
        #wait ${PIDS[@]}
        echo "Batch ${BATCH_CTR} of simulations complete" >> matched_board_tests_full/microbench/run_log.txt
        ((BATCH_CTR++))
        PIDS=()
        LAUNCH_CTR=0
    fi

# done  # i loop

done    # benchmark loop
done    # row loop
# done    # foldover loop
