import json
import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--matrix_size",
        required=True,
        type=int,
        help="Select an integer value for the size of the experiment design to use. If using a PB Design, options are some multiples of 4.",
    )
    parser.add_argument(
        "--row",
        # required=True,
        type=int,
        help="Choose a row of the experimental design matrix/file to use to create a configuration json.",
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
        help="Choose to generate all jsons for a given PB design, or only one json. If choosing to generate only one, then --row and --foldover must be specified.",
        choices=["one", "all"],
    )
    parser.add_argument(
        "--mode",
        required=True,
        help="Choose between PB (Plackett-Burman design) or full (full factorial design)",
        choices=["PB", "full"],
    )

    args = parser.parse_args()

    # If you want to generate all JSONs at once. Currently unused.
    if args.nums == "all":
        if args.mode == "PB":
            for foldover in [0, 1]:
                for row in range(0, args.matrix_size):
                    file_maker(
                        matrix_size=args.matrix_size,
                        row=row,
                        in_file=args.file,
                        foldover=foldover,
                        mode="PB",
                    )
        if args.mode == "full":
            for row in range(0, 2**args.matrix_size):
                file_maker(
                    matrix_size=args.matrix_size,
                    row=row,
                    in_file=args.file,
                    foldover=0,
                    mode="full",
                )

    else:
        if args.mode == "PB":
            file_maker(  # Generate one JSON at a time
                matrix_size=args.matrix_size,
                row=args.row,
                in_file=args.file,
                foldover=args.foldover,
                mode=args.mode,
            )
        elif args.mode == "full":
            file_maker(
                matrix_size=args.matrix_size,
                row=args.row,
                in_file=args.file,
                foldover="0",
                mode=args.mode,
            )


def file_maker(matrix_size, row, in_file, foldover, mode):
    if mode == "PB":
        with open(f"./pb-designs/PB_{matrix_size}.txt", "r") as matrix_file:
            all_run_settings = []
            # Read in the txt file with the Plackett Burman design
            for line in matrix_file:
                line = line.split(sep=",")
                line[-1] = line[-1].replace("\n", "")
                all_run_settings.append(line)

            # If the row is part of the foldover matrix, change "1"s to "-1"s and vice versa
            fold_row = all_run_settings[row]
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
            f"./configured_jsons/{foldover}_{row}_{matrix_size}.json", "w"
        ) as configured_json:
            # Get the JSON with a high and low value for each parameter
            high_low = json.load(high_low_json)
            # Call a function to assign a high or low value for each parameter based on the values in fold_row
            configured_dict = configure_dictionary(high_low, fold_row, mode)
            # Write the resulting dictionary to a JSON file
            configured_json.write(json.dumps(configured_dict[0]))

    elif mode == "full":
        with open(
            f"{matrix_size}_full_factorial_matrix.txt", "r"
        ) as matrix_file:
            all_run_settings = []
            # Read in the txt file with the full design
            for line in matrix_file:
                line = line.split(sep=" ")
                line[-1] = line[-1].replace("\n", "")
                all_run_settings.append(line)

            # for i, line in enumerate(all_run_settings):
            #     print(i)
            with open(in_file, "r") as high_low_json, open(
                f"./configured_jsons/full/{row}_{matrix_size}.json", "w"
            ) as configured_json:
                # Get the JSON with a high and low value for each parameter
                high_low = json.load(high_low_json)
                # Call a function to assign a high or low value for each parameter based on the values in fold_row
                configured_dict = configure_dictionary(
                    high_low, all_run_settings[row], mode
                )
                # Write the resulting dictionary to a JSON file
                configured_json.write(json.dumps(configured_dict[0]))

            # If the row is part of the foldover matrix, change "1"s to "-1"s and vice versa
            # fold_row = all_run_settings[row]
            # if foldover == 1:
            #     for i, num in enumerate(fold_row):
            #         if num == "1":
            #             fold_row[i] = "-1"
            #         elif num == "-1":
            #             fold_row[i] = "1"
            #         else:
            #             print(
            #                 "Error: value that is not 1 or -1 encountered in run_settings"
            #             )
            # print(fold_row)


def configure_dictionary(high_low_dict, run_settings, mode):
    ret_dict = {}
    settings_index = 0
    for param, value in high_low_dict.items():
        # If the parameter has a dictionary as value, call the function recursively.
        if isinstance(value, dict):
            print(param)
            run_settings_slice = run_settings[settings_index:]
            # Use a temporary variable b/c the function returns 2 values
            unpack = configure_dictionary(value, run_settings_slice, mode)
            ret_dict[param] = unpack[0]
            settings_index += unpack[1]
        # Base case
        elif isinstance(value, list):
            if mode == "PB":
                if run_settings[settings_index] == "-1":
                    ret_dict[param] = value[0]
                elif run_settings[settings_index] == "1":
                    ret_dict[param] = value[1]
                else:
                    print(
                        "Error: value that is not 1 or -1 encountered in run_settings"
                    )
            elif mode == "full":
                if run_settings[settings_index] == "0":
                    ret_dict[param] = value[0]
                elif run_settings[settings_index] == "1":
                    ret_dict[param] = value[1]
                else:
                    print(
                        "Error: value that is not 0 or 1 encountered in run_settings"
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
