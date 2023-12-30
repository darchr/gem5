from rocket_multiprocessing import (
    create_board_configurations,
    run_one_configuration,
)
from time import sleep
from gem5.utils.multiprocessing import Process, Pool
from post_process import postprocess


def yield_board_configurations():
    """
    Generator for board configurations for one parameter.

    Returns:
    - board_configurations (list): List of board configurations for one parameter.
    """
    yield from create_board_configurations()


def create_correct_board_configurations():
    """
    Create correct board configurations.
    This uses the previously computed correct board configurations.

    Returns:
    - board_configurations (list): List of board configurations.
    """

    from create_parameter_space import processed_parameters
    from post_process import get_correct_parameter_values

    parameters, _ = processed_parameters()
    board_configurations = []
    correct_values = get_correct_parameter_values()
    if correct_values is None:
        return None
    for i in range(len(correct_values)):
        parameter = parameters[i]
        config = [f"{parameter} = {correct_values[i]}"]
        board_configurations.append(config)
    print(board_configurations)
    return board_configurations


def extract_one_configuration(board_configuration):
    """
    Extract one board configuration, i.e., one parameter value and one microbenchmark.

    Parameters:
    - board_configuration (list): List of board configurations.

    Returns:
    - config (list): Particular config. [element 0 of board_configuration]
    - microbenchmark (gem5.resources.resource.BinaryResource): Particular Microbenchmark. [element 1 of board_configuration]
    """
    return board_configuration[0], board_configuration[1]


def print_one_configuration(board_configuration):
    """
    Test function to print one board configuration.

    Parameters:
    - board_configuration (list): List of board configurations.
    """
    config, microbenchmark = extract_one_configuration(board_configuration)
    print(config)
    print(microbenchmark.get_id())


if __name__ == "__m5_main__":
    """
    Main function.

    This function will:
    - Create the board configurations.
    - Run the board configurations using gem5's multiprocessing.
    - Post-process the results.
    - Repeat for all parameters.
    - Write the correct board configurations to a file.
    """

    for board_configurations_per_parameter in yield_board_configurations():
        correct_board_configurations = create_correct_board_configurations()
        processes = []
        for board_configuration in board_configurations_per_parameter:
            folder_name = (
                board_configuration[1].get_id()
                + "_"
                + board_configuration[0][0].split(" = ")[0]
                + "_"
                + board_configuration[0][0].split(" = ")[1]
            )
            parameter = board_configuration[0][0].split(" = ")[0]
            print(folder_name)
            while len(processes) > 20:
                for process in processes:
                    if not process.is_alive():
                        print(
                            "Process for folder {} completed".format(
                                folder_name
                            )
                        )
                        processes.remove(process)
                sleep(10)
            process = Process(
                target=run_one_configuration,
                args=(board_configuration, correct_board_configurations),
                name=folder_name,
            )
            process.start()
            processes.append(process)

        while processes:
            for process in processes:
                if not process.is_alive():
                    print(f"Process for folder {folder_name} completed")
                    processes.remove(process)
            sleep(10)

        if len(processes) == 0:
            print("All processes completed")
            postprocess("IPC", parameter)
        print(f"Finished processing parameter {parameter}")

    for (
        board_configurations_per_parameter
    ) in create_correct_board_configurations():
        # write to file
        with open("correct_board_configurations.txt", "a") as f:
            f.write(str(board_configurations_per_parameter) + "\n")
