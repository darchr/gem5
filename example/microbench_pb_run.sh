FOLDOVER=1
ROW=15
SIZE=16
FILE="nopipe_params.json"
#BENCHMARK="riscv-hello"
#"riscv-ccm"

for benchmark in riscv-cca riscv-cce riscv-cch riscv-cch-st riscv-cci riscv-ccm riscv-crd riscv-crf riscv-cs1 riscv-cs3 riscv-dp1d riscv-dp1f riscv-dpcvt riscv-dpt riscv-dptd riscv-ed1 riscv-ef riscv-ei riscv-em1 riscv-em5 riscv-m-dyn riscv-mc riscv-mcs riscv-md riscv-mi riscv-mim riscv-mim2 riscv-mip riscv-ml2-bw-ld riscv-ml2-bw-ldst riscv-ml2-bw-st riscv-stc riscv-stl2;
do
# for benchmark in "riscv-cca" "riscv-cce";
# do

echo "Now running: foldover = ${FOLDOVER}, PB matrix row = ${ROW}, PB matrix size = ${SIZE}"

echo "Foldover = ${FOLDOVER}, so the row number in the foldover matrix is" $((($SIZE*$FOLDOVER)+$ROW))

if [ ! -d ./matched_board_tests_paper/microbench ]; then
    mkdir ./matched_board_tests_paper/microbench
fi

#check if directory exists for
if [ ! -d ./matched_board_tests_paper/microbench/$benchmark ]; then
    mkdir ./matched_board_tests_paper/microbench/$benchmark
fi

if [ ! -d ./matched_board_tests_paper/microbench/$benchmark/${FOLDOVER}_${ROW}_${SIZE} ]; then
    mkdir ./matched_board_tests_paper/microbench/$benchmark/${FOLDOVER}_${ROW}_${SIZE}
fi

#generate configured json
# python3 json_maker.py --foldover=${FOLDOVER} --pb_row=${ROW} --matrix_size=${SIZE}  --file=${FILE}  --nums="one"


# for i in {1..3}
# do
    # if [ ! -d ./matched_board_tests_paper/microbench/$benchmark/${FOLDOVER}_${ROW}_${SIZE}/${i} ]; then
    #     mkdir ./matched_board_tests_paper/microbench/$benchmark/${FOLDOVER}_${ROW}_${SIZE}/${i}
    # fi

    # run simulation. Redirect terminal output
    /scr/bees/pb-test-board/build/RISCV/gem5.opt --outdir=matched_board_tests_paper/microbench/${benchmark}/${FOLDOVER}_${ROW}_${SIZE}/${i} simple-config.py --benchmark=${benchmark} --foldover=$FOLDOVER --pb_row=$ROW --matrix_size=$SIZE  > ./matched_board_tests_paper/microbench/${benchmark}/${FOLDOVER}_${ROW}_${SIZE}/${i}/terminal.txt

# done

# echo test_disk_2 $BENCHMARK $SYNTH $SIZE $START_PROC $SWITCH_PROC >> arg_combos.txt

# mv ./x86-gapbs-2204-23gem5/stats.txt ./x86-gapbs-2204-23gem5/test_disk_2_${BENCHMARK}_${SYNTH}_${SIZE}_${START_PROC}_${SWITCH_PROC}_stats.txt

# touch ./x86-gapbs-2204-23gem5/test_disk_2_${BENCHMARK}_${SYNTH}_${SIZE}_${START_PROC}_${SWITCH_PROC}_m5term.txt
done
