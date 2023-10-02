import json
import math


def get_parameters_from_file():
    with open("gem5-parameters-test.json", "r") as f:
        parameters = json.load(f)
    return parameters


def get_processed_parameters():
    processed_parameters = process_parameters(get_parameters_from_file())
    return processed_parameters


def process_parameters(parameters):
    # depending on value of step,
    # create an array of values between
    # min_value and max_value
    processed_parameters = []
    for parameter in parameters:
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
        for value in values:
            processed_parameters.append((parameter["parameter"], value))
    return processed_parameters
