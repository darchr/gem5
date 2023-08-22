import json
import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--matrix_size",
        required=True,
        type=int,
        help="Select an integer value for the size of the Hadamard matrix/ PB Design to use.",
        choices=[4, 12, 16, 40, 48, 80],
    )
    parser.add_argument(
        "--pb_row",
        # required=True,
        type=int,
        help="Choose a row of the PB design to use to create a configuration json.",
    )
    parser.add_argument(
        "--file",
        required=True,
        help="Select file to make a configuration json from.",
        choices=[
            "example.json",
            "example_core.json",
            "edited_params.json",
            "nopipe_params.json",
        ],
    )
    parser.add_argument(
        "--foldover",
        # required=True,
        help="If False, then use the original Hadamard matrix/PB design. If True, then use the opposite of the Hadamard matrix.",
        type=int,
        choices=[0, 1],
    )
    parser.add_argument(
        "--nums",
        required=True,
        help="Choose to generate all jsons for a given PB design, or only one json. If choosing to generate only one, then --pb_row and --foldover must be specified.",
        choices=["one", "all"],
    )

    args = parser.parse_args()

    # If you want to generate all JSONs at once. Currently unused.
    if args.nums == "all":
        for foldover in [0, 1]:
            for row in range(0, args.matrix_size):
                file_maker(
                    matrix_size=args.matrix_size,
                    pb_row=row,
                    in_file=args.file,
                    foldover=foldover,
                )
    else:
        file_maker(  # Generate one JSON at a time
            matrix_size=args.matrix_size,
            pb_row=args.pb_row,
            in_file=args.file,
            foldover=args.foldover,
        )


def file_maker(matrix_size, pb_row, in_file, foldover):
    with open(f"./pb-designs/PB_{matrix_size}.txt", "r") as pb_file:
        all_run_settings = []
        # Read in the txt file with the Plackett Burman design
        for line in pb_file:
            line = line.split(sep=",")
            line[-1] = line[-1].replace("\n", "")
            all_run_settings.append(line)

        # If the row is part of the foldover matrix, change "1"s to "-1"s and vice versa
        fold_row = all_run_settings[pb_row]
        if foldover == 1:
            for i, num in enumerate(fold_row):
                if num == "1":
                    fold_row[i] = "-1"
                elif num == "-1":
                    fold_row[i] = "1"
                else:
                    print(
                        "Error: value that is not 1 or -1 encountered in run_settings"
                    )
        print(fold_row)

    with open(in_file, "r") as high_low_json, open(
        f"./configured_jsons/{foldover}_{pb_row}_{matrix_size}.json", "w"
    ) as configured_json:
        # Get the JSON with a high and low value for each parameter
        high_low = json.load(high_low_json)
        # Call a function to assign a high or low value for each parameter based on the values in fold_row
        configured_dict = configure_dictionary(high_low, fold_row)
        # Write the resulting dictionary to a JSON file
        configured_json.write(json.dumps(configured_dict[0]))


def configure_dictionary(high_low_dict, run_settings):
    ret_dict = {}
    settings_index = 0
    for param, value in high_low_dict.items():
        # If the parameter has a dictionary as value, call the function recursively.
        if isinstance(value, dict):
            print(param)
            run_settings_slice = run_settings[settings_index:]
            # Use a temporary variable b/c the function returns 2 values
            unpack = configure_dictionary(value, run_settings_slice)
            ret_dict[param] = unpack[0]
            settings_index += unpack[1]
        # Base case
        elif isinstance(value, list):
            if run_settings[settings_index] == "-1":
                ret_dict[param] = value[0]
            elif run_settings[settings_index] == "1":
                ret_dict[param] = value[1]
            else:
                print(
                    "Error: value that is not 1 or -1 encountered in run_settings"
                )
            settings_index += 1
        elif isinstance(value, bool):
            ret_dict[param] = value
            settings_index += 1

    return ret_dict, settings_index


main()


# test_dict= {
#     "one": ["1","4"],
#     "two": ["2","5"],
#     "three": ["3","6"],
#     "notes": {
#         "a" : ["flat", "sharp"],
#         "b" : ["flat", "sharp"],
#         "c" : ["flat", "sharp"],
#         "d" : ["flat", "sharp"],
#         "e" : ["flat", "sharp"],
#         "f" : ["flat", "sharp"],
#         "g" : ["flat", "sharp"]
#     }
# }

# test_settings = ["1","1","1","1","1","1","1","1","1","1"]
