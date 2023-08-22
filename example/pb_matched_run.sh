# shell script for running individual benchmarksl/benchmarks other than the full set of microbenchmarks
# options are:
# FOLDOVER = [0,1]. Whether the Hadamard matrix is the original, or if it is multiplied by -1.
# ROW = [0 .. 15]. Row of the Hadamard matrix.
# SIZE = 16, or some other size of hadamard matrix, provided that you have a high/low parameter json
# FILE = the high/low json. The number of parameters in the high/low json should be less than SIZE.
# BENCHMARK = binaries available in gem5 resources, or local resources

#FOLDOVER=0
#ROW=7
SIZE=16
MAX_IND=$(($SIZE - 1))
FILE="nopipe_params.json"
BENCHMARK="riscv-bubblesort"
#"riscv-ccm"

for FOLDOVER in {0..1}
do
    for ROW in $(seq 0 ${MAX_IND})
    do
        echo "Now running: foldover = ${FOLDOVER}, PB matrix row = ${ROW}, PB matrix size = ${SIZE}"

        echo "Foldover = ${FOLDOVER}, so the row number in the foldover matrix is" $((($SIZE*$FOLDOVER)+$ROW))

        #check if directory exists for
        if [ ! -d ./matched_board_tests_paper/$BENCHMARK ]; then
            mkdir ./matched_board_tests_paper/$BENCHMARK
        fi

        if [ ! -d ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE} ]; then
            mkdir ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}
        fi

        #generate configured json
        python3 json_maker.py --foldover=${FOLDOVER} --pb_row=${ROW} --matrix_size=${SIZE}  --file=${FILE}  --nums="one"


        # for i in {1..3}
        # do
        #     if [ ! -d ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}/${i} ]; then
        #         mkdir ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}/${i}
        #     fi

            # run simulation. Redirect terminal output
            /scr/bees/pb-test-board/build/RISCV/gem5.opt --outdir=matched_board_tests_paper/${BENCHMARK}/${FOLDOVER}_${ROW}_${SIZE} simple-config.py --benchmark=${BENCHMARK} --foldover=$FOLDOVER --pb_row=$ROW --matrix_size=$SIZE  > ./matched_board_tests_paper/${BENCHMARK}/${FOLDOVER}_${ROW}_${SIZE}/terminal.txt
# done

    done
done
