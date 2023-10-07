from gem5.utils.multiprocessing import Process, Pool
import os
from riscvmatched_multiprocessing import (
    create_board_configurations,
    run_one_configuration,
)
from time import sleep
from post_process import postprocess

if __name__ == "__m5_main__":
    board_configurations = create_board_configurations()
    if "GEM5_CONFIG_CLUSTERS" in os.environ:
        num_clusters = len(board_configurations) // int(
            os.environ["GEM5_CONFIG_CLUSTERS"]
        )
    else:
        # Default to 1 cluster
        num_clusters = len(board_configurations) // 1

    board_configurations_clusters = []
    for i in range(num_clusters):
        board_configurations_clusters.append([])
    for i in range(len(board_configurations)):
        board_configurations_clusters[i % num_clusters].append(
            board_configurations[i]
        )

    for config_cluster in board_configurations_clusters:
        processes = []
        for config in config_cluster:
            print(config[1].get_id())
            print(config[0])
            folder_name = (
                config[1].get_id()
                + "_"
                + config[0][0].split(".")[3].split(" = ")[0]
                + "_"
                + config[0][0].split(" = ")[1]
            )
            if len(processes) >= num_clusters:
                for p in processes:
                    if not p.is_alive():
                        processes.remove(p)
                sleep(10)
            p = Process(
                target=run_one_configuration, args=(config,), name=folder_name
            )
            p.start()
            processes.append(p)

        while processes:
            for p in processes:
                if not p.is_alive():
                    processes.remove(p)
            sleep(10)

    postprocess()
