from pathlib import Path
import os
import json


def postprocess():
    if "OUT_DIR" in os.environ:
        # If OUT_DIR is set, use it
        m5out_folder = Path(os.environ["OUT_DIR"])
    else:
        # Otherwise, use the default
        m5out_folder = Path("m5out")

    # Create a dictionary to store results
    results = []

    for folder in os.listdir(m5out_folder):
        folder_path = os.path.join(m5out_folder, folder)
        if not os.path.isdir(folder_path):
            continue
        workload = folder.split("_")[0]
        parameter = folder.split("_")[1]
        value = folder.split("_")[2]

        stats_file = os.path.join(folder_path, "stats.txt")
        if not os.path.isfile(stats_file):
            continue

        with open(stats_file, "r") as f:
            stats = f.readlines()
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
            ipc = instructions / cycles

        # Create a dictionary for the value and workload
        value_dict = {"workload": workload, "IPC": ipc}

        # Check if the parameter already exists in results
        found_param = None
        for param_item in results:
            if param_item["parameter"] == parameter:
                found_param = param_item
                break

        if found_param:
            # Check if the value already exists in the parameter
            found_value = None
            for val_item in found_param["values"]:
                if val_item["value"] == value:
                    found_value = val_item
                    break

            if found_value:
                found_value["workloads"].append(value_dict)
            else:
                found_param["values"].append(
                    {"value": value, "workloads": [value_dict]}
                )
        else:
            param_dict = {
                "parameter": parameter,
                "values": [{"value": value, "workloads": [value_dict]}],
            }
            results.append(param_dict)

    # Write results to a json file
    with open("results.json", "w") as f:
        json.dump(results, f, indent=4)
