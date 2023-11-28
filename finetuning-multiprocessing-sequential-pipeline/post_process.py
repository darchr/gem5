import os
import math

BASELINE_DATAFRAME = None
CORRECT_PARAMETER_VALUES = None


def create_baseline_data():
    """
    Create a dictionary of baseline data.

    Returns:
    - baseline_data (dict): Dictionary of baseline data.
    """
    global BASELINE_DATAFRAME
    if BASELINE_DATAFRAME is None:
        baseline_data = {}
        with open("baseline.csv") as f:
            for line in f:
                values = line.strip().split(",")
                microbenchmark = values[0]
                try:
                    baseline_data[microbenchmark] = {
                        "Microbenchmark": microbenchmark,
                        "Instructions": int(values[1]),
                        "Cycles": int(values[2]),
                        "IPC": float(values[3]),
                    }
                except:
                    pass
        BASELINE_DATAFRAME = baseline_data
    return BASELINE_DATAFRAME


def mean_squared_log_error(gem5_ipc, hardware_ipc):
    """
    Calculate the Geometric Mean Squared Error of log(gem5IPC/hardwareIPC).

    Parameters:
    - gem5_ipc (list): List of gem5 IPC values.
    - hardware_ipc (list): List of corresponding hardware IPC values.

    Returns:
    - gmse (float): Geometric Mean Squared Error.
    """

    # Calculate the log ratios
    log_ratios = [
        math.log(gem5 / hardware)
        for gem5, hardware in zip(gem5_ipc, hardware_ipc)
    ]

    # Calculate the squared errors
    squared_errors = [(log_ratio) ** 2 for log_ratio in log_ratios]

    # Calculate the geometric mean of the squared errors
    gmse = math.exp(math.sqrt(sum(squared_errors) / len(squared_errors)))

    return gmse


def postprocess(stat, current_parameter=None):
    """
    Postprocess the data.
    It will find the value of the parameter that corresponds to the minimum mean squared log error.
    It considers all microbenchmarks.

    Parameters:
    - stat (str): Statistic to postprocess.
    - current_parameter (str): Current parameter to postprocess.
    """

    baseline_data = create_baseline_data()
    table = {}

    print(f"Current parameter: {current_parameter}")

    for root, dirs, files in os.walk("m5out"):
        for direc in dirs:
            for root2, dirs2, files2 in os.walk(os.path.join(root, direc)):
                parameter_value = root2.split("_")[-1]
                parameter = "_".join(root2.split("riscv_")[1].split("_")[:-1])
                microbenchmark = root2.split(".")[0].split("/")[-1]
                if (
                    current_parameter is not None
                    and parameter != current_parameter
                ):
                    continue
                if "m5out-done" in root2:
                    continue
                for file in files2:
                    if file == "stats.txt":
                        with open(os.path.join(root2, file)) as f:
                            stats = f.readlines()
                            (
                                instructions,
                                cycles,
                                sim_seconds,
                                branch_instructions,
                            ) = (0, 0, 0.0, 0)
                            for line in stats:
                                if line.strip():
                                    if (
                                        line.startswith(
                                            "board.processor.cores.core.commitStats0.numInsts"
                                        )
                                        and "NOP" not in line
                                    ):
                                        line = line.split()
                                        instructions = int(line[1])
                                    if "numCycles" in line:
                                        line = line.split()
                                        cycles = int(line[1])
                                    if "simSeconds" in line:
                                        line = line.split()
                                        sim_seconds = float(line[1])
                                    if (
                                        "board.processor.cores.core.branchPred.lookups"
                                        in line
                                    ):
                                        line = line.split()
                                        branch_instructions = int(line[1])
                            try:
                                ipc = instructions / cycles
                            except:
                                # ipc with 0 cycles is undefined but we can set it to an absurdly high value
                                ipc = float(1000000)

                        table[parameter + parameter_value + microbenchmark] = {
                            "Value": parameter_value,
                            "Microbenchmark": microbenchmark,
                            "Instructions": instructions,
                            "Cycles": cycles,
                            "IPC": ipc,
                            "Seconds": sim_seconds,
                            "Branches": branch_instructions,
                        }

    # Convert the table to a list of dictionaries
    data_list = list(table.values())

    # Reset the index and sort the list of dictionaries by "Value"
    data_list = sorted(data_list, key=lambda x: x["Value"])

    # Split the list into sublists based on unique "Value" entries
    dataframes = {}
    for entry in data_list:
        value = entry["Value"]
        if value not in dataframes:
            dataframes[value] = []
        dataframes[value].append(entry)

    merged_dataframes = []
    mean_squared_log_errors = []

    # Merge each sublist with the baseline data
    for value, data_list_value in dataframes.items():
        unique_microbenchmarks_dataframes_i = {
            entry["Microbenchmark"] for entry in data_list_value
        }
        unique_microbenchmarks_baseline = {
            entry["Microbenchmark"] for entry in baseline_data.values()
        }

        # Merge the list with the baseline list
        merged_data = []
        for entry in data_list_value:
            for baseline_entry in baseline_data.values():
                if entry["Microbenchmark"] == baseline_entry["Microbenchmark"]:
                    merged_entry = entry.copy()
                    merged_entry[stat + "_gem5"] = entry[stat]
                    merged_entry[stat + "_baseline"] = baseline_entry[stat]
                    merged_data.append(merged_entry)

        merged_dataframes.append(merged_data)

        unique_microbenchmarks_merged = {
            entry["Microbenchmark"] for entry in merged_data
        }
        left_out_microbenchmarks_dataframes_i = (
            unique_microbenchmarks_dataframes_i - unique_microbenchmarks_merged
        )
        left_out_microbenchmarks_baseline = (
            unique_microbenchmarks_baseline - unique_microbenchmarks_merged
        )

        print(
            f"Microbenchmarks left out from dataframes[i]: {left_out_microbenchmarks_dataframes_i}"
        )
        print(
            f"Microbenchmarks left out from baseline_dataframe: {left_out_microbenchmarks_baseline}"
        )

    # Calculate mean squared log errors
    for i, merged_data in enumerate(merged_dataframes):
        value = merged_data[0]["Value"]
        print(f"Value: {value}")
        mean_squared_log_errors.append(
            mean_squared_log_error(
                [entry[stat + "_gem5"] for entry in merged_data],
                [entry[stat + "_baseline"] for entry in merged_data],
            )
        )

    print(f"Mean squared log errors: {mean_squared_log_errors}")

    # Find the index of the minimum mean squared log error
    min_mean_squared_log_error_index = mean_squared_log_errors.index(
        min(mean_squared_log_errors)
    )
    print(
        f"Index of the minimum mean squared log error: {min_mean_squared_log_error_index}"
    )

    # Find the value of the parameter that corresponds to the minimum mean squared log error
    parameter_value = merged_dataframes[min_mean_squared_log_error_index][0][
        "Value"
    ]
    print(
        f"Parameter value that corresponds to the minimum mean squared log error: {parameter_value}"
    )
    global CORRECT_PARAMETER_VALUES
    if CORRECT_PARAMETER_VALUES is None:
        CORRECT_PARAMETER_VALUES = [parameter_value]
    else:
        CORRECT_PARAMETER_VALUES.append(parameter_value)

    cleanup(current_parameter=current_parameter)


def get_correct_parameter_values():
    """
    Get the correct parameter values that have been found so far.

    Returns:
    - correct_parameter_values (list): List of correct parameter values.
    """
    global CORRECT_PARAMETER_VALUES
    return CORRECT_PARAMETER_VALUES


def cleanup(current_parameter=None):
    """
    Cleanup the m5out folder.
    It will move all subfolders of m5out that contain the current parameter to m5out-done.

    Parameters:
    - current_parameter (str): Current parameter to postprocess.
    """

    # move all subfolders of m5out that contain the current parameter to m5out-done
    import shutil

    for root, dirs, files in os.walk("m5out"):
        for direc in dirs:
            if (
                current_parameter is not None
                and current_parameter not in direc
            ):
                continue
            shutil.move(
                os.path.join(root, direc), os.path.join("m5out-done", direc)
            )


if __name__ == "__main__":
    # was used for testing
    postprocess("IPC", "point_of_unification")
