import os
import pandas as pd
import seaborn as sns
import numpy as np
import sys
import json
import matplotlib.pyplot as plt

# This function reads the file which contains the pb matrix for the
# parameter setup and returns that matrix in int array format
def read_matrix_file(file, size, param_num):
    columns = [[] * size for i in range(0, size)]
    lines = []
    with open(os.path.join(file), "r") as h_file:
        line = h_file.readlines()
        for i in range(0, param_num):
            for j in range(0, size):
                lines.append(int(line[j].split(",")[i].strip()))
            columns[i] = lines
            lines = []
    return columns

def create_foldover(pbcolumns, matrix_sz):
    fd_matrix_sz = matrix_sz * 2
    columns = []
    for i in range(0, fd_matrix_sz):
        if i < matrix_sz:
            foldover = np.array(pbcolumns[i])
            columns.append(foldover)
        else:
            foldover = np.array(pbcolumns[i - matrix_sz])
            foldover *= -1
            columns[i - matrix_sz] = np.append(
                columns[i - matrix_sz], foldover
            )
    return columns

# This Function reads the stats from the directory where the results of
# the simulation runs are contained and returns a matrix which contains
# the correspondent row of the pb matrix, the benchmark to which it
# belongs and the execution time it took to complete the simulation
def get_sim_stats(benchmark):
    sim_stats = []
    with open(f"benchmark_stats/{benchmark}.txt", "r") as grep_file:
        lines = grep_file.readlines()
        for i in range(0, len(lines)):
            sim_second = float(lines[i].split()[1])
            directory = lines[i].split()[0]
            tokens = directory.split("/")[-3:]
            benchmark = tokens[0]
            if len(tokens[1].split("_")) == 3:
                foldover, ph_row, size = tokens[1].split("_")
                row = int(foldover) * int(size) + int(ph_row)
            else:
                row = int(tokens[1].split("_")[0])
            sim_stats.append([row, benchmark, sim_second])
    return sorted(sim_stats, key=lambda sim_stats: sim_stats[0])
        
# This function calculates the efficiencies of each parameter given a
# specific benchmark
def calculate_eff(exec_time, columns, param_num):
    # Perform the dot product between the execution times and the setup
    # of the parameter (either 1 or 0)
    eff = []
    for i in range(0, param_num):
        dot = np.dot(columns[i], exec_time)
        eff.append(dot)
    return eff


def get_results(efficiencies, parameters, parameter_num):
    stats = []
    highest_value = 0
    for i in range(0, parameter_num):
        abs_value = (
            efficiencies[i] if efficiencies[i] >= 0 else efficiencies[i] * -1
        )
        highest_value = (
            abs_value if abs_value > highest_value else highest_value
        )
        stats.append([parameters[i], efficiencies[i], abs_value])

    for i in range(0, parameter_num):
        normalization = stats[i][2] / highest_value
        stats[i].append(normalization)

    return sorted(stats, key=lambda stats: stats[2], reverse=True)

def yates_alg(sim_stats, param_num, size):
    yates_array = [[] * size for i in range(0, size)]
    tmp = [[] * size for i in range(0, size)]
    exec_time = np.transpose(sim_stats)[2]
    exec_time = np.array(exec_time, dtype=float)
    with open(f"yates-order/{param_num}.json", "r") as yates_file:
        jsonData = json.load(yates_file)
        for i in range(0, len(jsonData)):
            # tmp = [row_num, exec_time]
            yates_array[i] = exec_time[int(jsonData[f'{i}'])]
            tmp[i] = (i, int(jsonData[f'{i}']), exec_time[int(jsonData[f'{i}'])])
    print(tmp)
    return yates_array

def full_runs(sim_stats, parameters, parameter_num):
    perf = []
    singles = []
    for current_run in range(0, len(sim_stats)):
        combination = []
        for current_parameter in range(0, parameter_num):
            if int(sim_stats[current_run][0]) & (2**current_parameter):
                # print(f"row: {int(sim_stats[current_run][0])} cp: {current_parameter} 2^cp: {2**current_parameter} &: {int(sim_stats[current_run][0]) & (2**current_parameter)}")
                combination.append(parameters[(parameter_num - 1) - current_parameter]) 
                # parameters are stored in opposite order of bits, ie msb is parameter[0]
        if not len(combination):
            singles.append([combination, int(sim_stats[current_run][0]), sim_stats[current_run][2]])
        elif len(combination) == 1:
            singles.append([combination, int(sim_stats[current_run][0]), sim_stats[current_run][2]])
        perf.append([combination, sim_stats[current_run][0], sim_stats[current_run][2]])
    sorted_singles = sorted(singles, key=lambda singles: singles[2])
    print(
        "\n".join([",".join([str(cell) for cell in row]) for row in sorted_singles])
    )
    print("-----------------------------------")
    return sorted(perf, key=lambda perf: perf[2], reverse=True)




if len(sys.argv) < 4:
    print(
        "Usage: python read_stats.py [format] [pbdir] [pbfile] [matrix_size]"
    )
    print("\n")
    print("0 for yates format, 1 for pb format")
    print("path: the path to the file where the pb matrix file is stored")
    print("matrix_size: The size of the matrix")
    print("benchmark: The benchmark to evaluate")
    print("\n")
else:
    pb = int(sys.argv[1])
    pbfile = str(sys.argv[2])
    matrix_size = int(sys.argv[3])
    benchmark = str(sys.argv[4])
    parameters = [
        "threadPolicy",
        "BP",
        "U74IntFU",
        "U74IntMulFU",
        "U74IntDivFU",
        "U74MemReadFU",
        "U74MemWriteFU",
        "l2_size",
        "l2_assoc",
        "l1dcache-response_latency",
        "l2cache-data_latency",
        "iptw_caches-size",
        "dptw_caches-size",
    ]
    parameter_num = len(parameters)

    sim_stats = get_sim_stats(benchmark)
    columns = read_matrix_file(pbfile, matrix_size, parameter_num)
    if(pb):
        # columns = create_foldover(columns, matrix_size)
        exec_time = np.transpose(sim_stats)[2]
        exec_time = np.array(exec_time, dtype=float)
    else:
        exec_time = np.array(yates_alg(sim_stats, parameter_num, matrix_size), dtype=float)
    efficiencies = calculate_eff(exec_time, columns, parameter_num)


    results = get_results(efficiencies, parameters, parameter_num)

    if(pb == 1):
        print("Parameter,Efficiency,Absolute_Value,Normalization")
        print(
            "\n".join([",".join([str(cell) for cell in row]) for row in results])
        )
        print("--------------------------------------------")
        combinations = full_runs(sim_stats, parameters, parameter_num)
        print("Combination,Row,Exec_Time")
        print(
            "\n".join([",".join([str(cell) for cell in row]) for row in combinations])
        )
    elif (pb == 2):
        # data = pd.DataFrame(sim_stats, columns=["Row", "Benchmark", "Execution Time"])
        # ax = sns.barplot(x="Row", y="Execution Time", data=data)
        # ax.get_legend()
        # plt.show()
        print(
            "\n".join([",".join([str(cell) for cell in row]) for row in sim_stats])
        )
    # data = pd.DataFrame(results, columns=["Parameter", "Efficiency", "Absolute Efficiency", "Normalization"])
    # data.head(64)
    
