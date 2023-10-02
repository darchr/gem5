from gem5.utils.multiprocessing import Process, Pool
import os
from riscvmatched_multiprocessing import (
    create_board_configurations,
    run_one_configuration,
)

if __name__ == "__m5_main__":
    board_configurations = create_board_configurations()
    if "GEM5_CONFIG_CLUSTERS" in os.environ:
        num_clusters = int(os.environ["GEM5_CONFIG_CLUSTERS"])
    else:
        num_clusters = len(board_configurations)

    board_configurations_clusters = []
    for i in range(num_clusters):
        board_configurations_clusters.append([])
    for i in range(len(board_configurations)):
        board_configurations_clusters[i % num_clusters].append(
            board_configurations[i]
        )

    processes = []
    for i in board_configurations_clusters:
        print(i[0][1].get_id())
        print(i[0][0])
        folder_name = (
            i[0][1].get_id()
            + "_"
            + i[0][0][0].split(".")[3].split(" = ")[0]
            + "_"
            + i[0][0][0].split(" = ")[1]
        )
        if len(processes) >= num_clusters:
            for p in processes:
                if not p.is_alive():
                    processes.remove(p)
            sleep(10)
        p = Process(target=run_one_configuration, args=(i,), name=folder_name)
        p.start()
        processes.append(p)

    while processes:
        for p in processes:
            if not p.is_alive():
                processes.remove(p)
        sleep(10)
