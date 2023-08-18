FOLDOVER=1
ROW=15
SIZE=16
FILE="nopipe_params.json"
BENCHMARK="riscv-hello"
#"riscv-ccm"


echo "Now running: foldover = ${FOLDOVER}, PB matrix row = ${ROW}, PB matrix size = ${SIZE}"

echo "Foldover = ${FOLDOVER}, so the row number in the foldover matrix is" $((($SIZE*$FOLDOVER)+$ROW))

#check if directory exists for
if [ ! -d ./matched_board_tests/$BENCHMARK ]; then
    mkdir ./matched_board_tests/$BENCHMARK
fi

if [ ! -d ./matched_board_tests/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE} ]; then
    mkdir ./matched_board_tests/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}
fi

#generate configured json
python3 json_maker.py --foldover=${FOLDOVER} --pb_row=${ROW} --matrix_size=${SIZE}  --file=${FILE}  --nums="one"


for i in {1..3}
do
    if [ ! -d ./matched_board_tests/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}/${i} ]; then
        mkdir ./matched_board_tests/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}/${i}
    fi

    # run simulation. Redirect terminal output
    /scr/bees/pb-test-board/build/RISCV/gem5.opt --outdir=matched_board_tests/${BENCHMARK}/${FOLDOVER}_${ROW}_${SIZE}/${i} simple-config.py --benchmark=${BENCHMARK} --foldover=$FOLDOVER --pb_row=$ROW --matrix_size=$SIZE  > ./matched_board_tests/${BENCHMARK}/${FOLDOVER}_${ROW}_${SIZE}/${i}/terminal.txt

done

# echo test_disk_2 $BENCHMARK $SYNTH $SIZE $START_PROC $SWITCH_PROC >> arg_combos.txt

# mv ./x86-gapbs-2204-23gem5/stats.txt ./x86-gapbs-2204-23gem5/test_disk_2_${BENCHMARK}_${SYNTH}_${SIZE}_${START_PROC}_${SWITCH_PROC}_stats.txt

# touch ./x86-gapbs-2204-23gem5/test_disk_2_${BENCHMARK}_${SYNTH}_${SIZE}_${START_PROC}_${SWITCH_PROC}_m5term.txt
