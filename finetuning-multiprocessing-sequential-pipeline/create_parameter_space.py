import json
import math


def extract_json(filename):
    """
    Extract the JSON file.

    Parameters:
    - filename (str): Name of the JSON file.
    """

    with open(filename) as f:
        data = json.load(f)
    return data


def process_parameters(data):
    """
    Process the parameters.
    Depending on the step operation, it will generate a list of values for each parameter.

    Parameters:
    - data (dict): Dictionary of parameters, values, and step operations.

    Returns:
    - parameter_names (list): List of parameter names.
    - parameter_values (list): List of parameter values.
    """

    parameter_names = []
    parameter_values = []
    for parameter in data:
        if parameter["step"] == "add":
            values = list(
                range(parameter["min_value"], parameter["max_value"] + 1)
            )
        elif parameter["step"] == "multiply":
            values = [
                parameter["min_value"] * 2**i
                for i in range(
                    int(
                        math.log2(
                            parameter["max_value"] / parameter["min_value"]
                        )
                        + 1
                    )
                )
            ]
        elif parameter["step"] == "flip":
            values = [parameter["min_value"], parameter["max_value"]]
        elif parameter["step"] == None:
            values = []
        else:
            raise ValueError("Invalid step operation")
        parameter_names.append(parameter["parameter"])
        parameter_values.append(values)

    return parameter_names, parameter_values


def processed_parameters():
    """
    Process the parameters.
    Depending on the step operation, it will generate a list of values for each parameter.

    Returns:
    - parameter_names (list): List of parameter names.
    - parameter_values (list): List of parameter values.
    """

    data = extract_json("gem5-parameters.json")
    return process_parameters(data)


if __name__ == "__main__":
    # for testing
    data = extract_json("gem5-parameters.json")
    process_parameters(data)
