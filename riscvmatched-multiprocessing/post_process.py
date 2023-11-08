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
                    if "simSeconds" in line:
                        line = line.split()
                        sim_seconds = float(line[1])
                    if "board.processor.cores.core.branchPred.lookups" in line:
                        line = line.split()
                        branch_instructions = int(line[1])
            ipc = instructions / cycles

        # Create a dictionary for the value and workload
        value_dict = {
            "workload": workload,
            "stats": {
                "Instructions": instructions,
                "Cycles": cycles,
                "IPC": instructions / cycles,
                "Seconds": sim_seconds,
                "Branches": branch_instructions,
            },
        }

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

    # go through the results and calculate the average
    for param in results:
        for value in param["values"]:
            total_instructions = 0
            total_cycles = 0
            total_seconds = 0
            total_branches = 0
            control_instructions = 0
            control_cycles = 0
            control_seconds = 0
            control_branches = 0
            control_length = 0
            execution_instructions = 0
            execution_cycles = 0
            execution_seconds = 0
            execution_branches = 0
            execution_length = 0
            memory_instructions = 0
            memory_cycles = 0
            memory_seconds = 0
            memory_branches = 0
            memory_length = 0
            data_instructions = 0
            data_cycles = 0
            data_seconds = 0
            data_branches = 0
            data_length = 0
            store_instructions = 0
            store_cycles = 0
            store_seconds = 0
            store_branches = 0
            store_length = 0
            for workload in value["workloads"]:
                workload_name = workload["workload"]
                if workload_name.split("-")[1].startswith("c"):
                    control_length += 1
                    control_instructions += workload["stats"]["Instructions"]
                    control_cycles += workload["stats"]["Cycles"]
                    control_seconds += workload["stats"]["Seconds"]
                    control_branches += workload["stats"]["Branches"]
                elif workload_name.split("-")[1].startswith("e"):
                    execution_length += 1
                    execution_instructions += workload["stats"]["Instructions"]
                    execution_cycles += workload["stats"]["Cycles"]
                    execution_seconds += workload["stats"]["Seconds"]
                    execution_branches += workload["stats"]["Branches"]
                elif workload_name.split("-")[1].startswith("m"):
                    memory_length += 1
                    memory_instructions += workload["stats"]["Instructions"]
                    memory_cycles += workload["stats"]["Cycles"]
                    memory_seconds += workload["stats"]["Seconds"]
                    memory_branches += workload["stats"]["Branches"]
                elif workload_name.split("-")[1].startswith("d"):
                    data_length += 1
                    data_instructions += workload["stats"]["Instructions"]
                    data_cycles += workload["stats"]["Cycles"]
                    data_seconds += workload["stats"]["Seconds"]
                    data_branches += workload["stats"]["Branches"]
                elif workload_name.split("-")[1].startswith("s"):
                    store_length += 1
                    store_instructions += workload["stats"]["Instructions"]
                    store_cycles += workload["stats"]["Cycles"]
                    store_seconds += workload["stats"]["Seconds"]
                    store_branches += workload["stats"]["Branches"]

                total_instructions += workload["stats"]["Instructions"]
                total_cycles += workload["stats"]["Cycles"]
                total_seconds += workload["stats"]["Seconds"]
                total_branches += workload["stats"]["Branches"]

            value["average"] = {
                "overall": {
                    "Instructions": total_instructions
                    / len(value["workloads"]),
                    "Cycles": total_cycles / len(value["workloads"]),
                    "IPC": total_instructions / total_cycles,
                    "Seconds": total_seconds / len(value["workloads"]),
                    "Branches": total_branches / len(value["workloads"]),
                }
            }
            # only write the values that are not zero
            if control_length != 0:
                value["average"]["control"] = {
                    "Instructions": control_instructions / control_length,
                    "Cycles": control_cycles / control_length,
                    "IPC": control_instructions / control_cycles,
                    "Seconds": control_seconds / control_length,
                    "Branches": control_branches / control_length,
                }
            if execution_length != 0:
                value["average"]["execution"] = {
                    "Instructions": execution_instructions / execution_length,
                    "Cycles": execution_cycles / execution_length,
                    "IPC": execution_instructions / execution_cycles,
                    "Seconds": execution_seconds / execution_length,
                    "Branches": execution_branches / execution_length,
                }
            if memory_length != 0:
                value["average"]["memory"] = {
                    "Instructions": memory_instructions / memory_length,
                    "Cycles": memory_cycles / memory_length,
                    "IPC": memory_instructions / memory_cycles,
                    "Seconds": memory_seconds / memory_length,
                    "Branches": memory_branches / memory_length,
                }
            if data_length != 0:
                value["average"]["data_dependency"] = {
                    "Instructions": data_instructions / data_length,
                    "Cycles": data_cycles / data_length,
                    "IPC": data_instructions / data_cycles,
                    "Seconds": data_seconds / data_length,
                    "Branches": data_branches / data_length,
                }
            if store_length != 0:
                value["average"]["store_intense"] = {
                    "Instructions": store_instructions / store_length,
                    "Cycles": store_cycles / store_length,
                    "IPC": store_instructions / store_cycles,
                    "Seconds": store_seconds / store_length,
                    "Branches": store_branches / store_length,
                }

    # Write results to a json file
    with open("results.json", "w") as f:
        json.dump(results, f, indent=4)
