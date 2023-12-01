import os
import sys


def create_dir(microbench):
    dir_name = "microbench_results" if microbench else "bench_results"
    err_file = ".get_all_mb_error" if microbench else ".get_all_error"
    os.system(f"touch {err_file}")
    ret_code = os.system(f"mkdir {dir_name} 2> {err_file}")

    if ret_code != 0:
        with open(err_file, "r") as e_file:
            line = e_file.readlines()[0]
            if (
                line
                != f"mkdir: cannot create directory ‘{dir_name}’: File exists\n"
            ):
                print("Could not create output directory")
                os.system(f"rm -rf {err_file}")
                exit
    os.system(f"rm -rf {err_file}")
    return f"{dir_name}/"


def run_command(
    directory, pb_directory, pb_file, benchmark, matrix_size, resultsdir
):
    os.system(
        f"python read_stats.py {directory} {pb_directory} {pb_file} {benchmark} {str(matrix_size)} > {resultsdir}{benchmark}.md"
    )
    return f"{resultsdir}{benchmark}.md"


def parse_results_file(file):
    results = []
    with open(file, "r") as h_file:
        lines = h_file.readlines()
        for line in range(1, len(lines)):
            parameter = lines[line].split(",")[0]
            eff = float(lines[line].split(",")[1])
            abs_v = float(lines[line].split(",")[2])
            norm = float(lines[line].split(",")[3])
            rank = line
            results.append([parameter, eff, abs_v, norm, rank])
    return sorted(results, key=lambda results: results[0])


directory = (
    "/home/lredivo/gem5-pb-test-board/example/matched_board_tests_paper/"
)
pb_directory = "./pb-designs"
pb_file = "PB_16.txt"
matrix_size = 16
skip = 0

bench_list = os.listdir(directory)
bench_list_len = len(bench_list)
micro_dir = directory + "microbench/"
micro_bench_list = os.listdir(micro_dir)
micro_bench_list_len = len(micro_bench_list)

param_num = 13

# [] * bench_list_len for i in range(0, bench_list_len)
benchmark_results = [["parameter"]]
micro_results = [["parameter"]]
for parameters in range(0, param_num):
    benchmark_results.append([])
    micro_results.append([])


for benchmark in range(0, bench_list_len):
    if bench_list[benchmark] == "microbench" and not skip:
        results_dir = create_dir(1)
        for micro_bench in range(0, micro_bench_list_len):
            file = run_command(
                directory,
                pb_directory,
                pb_file,
                micro_bench_list[micro_bench],
                matrix_size,
                results_dir,
            )
            results = parse_results_file(file)
            micro_results[0].append(micro_bench_list[micro_bench])
            for parameter in range(0, param_num):
                if len(micro_results[1]) == 0:
                    micro_results[parameter + 1].append(results[parameter][0])
                micro_results[parameter + 1].append(results[parameter][2])
    elif bench_list[benchmark] == "microbench" and skip:
        continue
    else:
        results_dir = create_dir(0)
        file = run_command(
            directory,
            pb_directory,
            pb_file,
            bench_list[benchmark],
            matrix_size,
            results_dir,
        )
        results = parse_results_file(file)
        benchmark_results[0].append(bench_list[benchmark])
        for parameter in range(0, param_num):
            if len(benchmark_results[1]) == 0:
                benchmark_results[parameter + 1].append(results[parameter][0])
            benchmark_results[parameter + 1].append(results[parameter][2])

print(
    "\n".join([",".join([str(cell) for cell in row]) for row in micro_results])
)
print("-----------------------")
print(
    "\n".join(
        [",".join([str(cell) for cell in row]) for row in benchmark_results]
    )
)
