# shell script for running individual benchmarks/benchmarks other than the full set of microbenchmarks
# options are:
# SIZE = 16, or some other size of hadamard matrix, provided that you have a high/low parameter json
# FILE = the high/low json. The number of parameters in the high/low json should be less than SIZE.
# BENCHMARK = binaries available in gem5 resources, or local resources

SIZE=16
MAX_IND=$(($SIZE - 1))
FILE="nopipe_params.json"
BENCHMARK="riscv-bubblesort"
#"riscv-ccm"

# Go through each foldover and row option. Foldover = [0,1], row = [0 ...  15]

for FOLDOVER in {0..1}
do
    for ROW in $(seq 0 ${MAX_IND})
    do
        echo "Now running: foldover = ${FOLDOVER}, PB matrix row = ${ROW}, PB matrix size = ${SIZE}"

        echo "Foldover = ${FOLDOVER}, so the row number in the foldover matrix is" $((($SIZE*$FOLDOVER)+$ROW))

        # Check if directory exists. If not, make the directory.
        if [ ! -d ./matched_board_tests_paper/$BENCHMARK ]; then
            mkdir ./matched_board_tests_paper/$BENCHMARK
        fi

        if [ ! -d ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE} ]; then
            mkdir ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}
        fi

        #generate configured json
        python3 json_maker.py --foldover=${FOLDOVER} --pb_row=${ROW} --matrix_size=${SIZE}  --file=${FILE}  --nums="one"


        #Allows you to do multiple runs on one benchmark
        # for i in {1..3}
        # do
        #     if [ ! -d ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}/${i} ]; then
        #         mkdir ./matched_board_tests_paper/$BENCHMARK/${FOLDOVER}_${ROW}_${SIZE}/${i}
        #     fi

            # run simulation. Redirect terminal output
            /scr/bees/pb-test-board/build/RISCV/gem5.opt --outdir=matched_board_tests_paper/${BENCHMARK}/${FOLDOVER}_${ROW}_${SIZE} simple-config.py --benchmark=${BENCHMARK} --foldover=$FOLDOVER --pb_row=$ROW --matrix_size=$SIZE  > ./matched_board_tests_paper/${BENCHMARK}/${FOLDOVER}_${ROW}_${SIZE}/terminal.txt
        # done   # i end

    done    # ROW end
done    #FOLDOVER end
