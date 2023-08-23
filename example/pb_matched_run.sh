
SIZE=40
ROW=1
FILE="edited_params.json"
FOLDOVER=0

#echo "Now running: benchmark=${BENCHMARK}, synthetic=${SYNTH}, and size=${SIZE}"


echo "Now running: PB matrix size = ${SIZE}, PB matrix row = ${ROW}, foldover = ${FOLDOVER}"

echo "Foldover = ${FOLDOVER}, so the row number in the foldover matrix is" $((($SIZE*$FOLDOVER)+$ROW))

#generate configured json
python3 json_maker.py --matrix_size=${SIZE} --pb_row=${ROW} --file=${FILE} --foldover=${FOLDOVER} --nums="one"
# run simulation. Redirect terminal output
/scr/bees/pb-test-board/build/RISCV/gem5.opt --outdir=matched_board_tests simple-config.py --matrix_size=$SIZE --pb_row=$ROW --foldover=$FOLDOVER  > ./matched_board_tests/${SIZE}_${ROW}_${FOLDOVER}_terminal.txt




# echo test_disk_2 $BENCHMARK $SYNTH $SIZE $START_PROC $SWITCH_PROC >> arg_combos.txt

# mv ./x86-gapbs-2204-23gem5/stats.txt ./x86-gapbs-2204-23gem5/test_disk_2_${BENCHMARK}_${SYNTH}_${SIZE}_${START_PROC}_${SWITCH_PROC}_stats.txt

# touch ./x86-gapbs-2204-23gem5/test_disk_2_${BENCHMARK}_${SYNTH}_${SIZE}_${START_PROC}_${SWITCH_PROC}_m5term.txt
