import json
import argparse


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--matrix_size",
        required=True,
        type=int,
        help="Select an integer value for the size of the Hadamard matrix/ PB Design to use.",
        choices=[4, 12, 48, 80],
    )
    parser.add_argument(
        "--run_setting_row",
        required=True,
        type=int,
        help="Choose a row of the PB design to use to create a configuration json.",
    )
    parser.add_argument(
        "--file",
        required=True,
        help="Select file to make a configuration json from.",
        choices=["example.json", "example_core.json"],
    )
    parser.add_argument(
        "--foldover",
        required=True,
        help="If False, then use the original Hadamard matrix/PB design. If True, then use the opposite of the Hadamard matrix.",
        type=bool,
    )
    args = parser.parse_args()

    with open(f"H_{args.matrix_size}.txt", "r") as pb_file:
        # with open("H_4.txt", "r") as pb_file:
        all_run_settings = []
        for line in pb_file:
            # temp = pb_file.readline()
            line = line.split(sep=",")
            line[-1] = line[-1].replace("\n", "")
            all_run_settings.append(line)
        # print(type(run_settings))
        # print(all_run_settings)
        print(all_run_settings[args.run_setting_row])

    with open(args.file, "r") as high_low_json, open(
        f"configured_{args.matrix_size}_{args.run_setting_row}.json", "w"
    ) as configured_json:
        # aa = high_low_json.readline()
        # print (aa)
        high_low = json.load(high_low_json)
        # print(high_low)
        # for i, param in enumerate(high_low):
        configured_dict = configure_dictionary(
            high_low, all_run_settings[args.run_setting_row], args.foldover
        )
        # configured_dict = configure_dictionary(test_dict, test_settings)
        # print(configured_dict)
        configured_json.write(json.dumps(configured_dict[0]))


def configure_dictionary(high_low_dict, run_settings, foldover):
    ret_dict = {}
    settings_index = 0
    for param, value in high_low_dict.items():
        if isinstance(value, dict):
            print(param)
            run_settings_slice = run_settings[settings_index:]
            # print("settings_index: ",settings_index)
            # print(run_settings_slice)
            # Use a temporary variable b/c the function returns 2 values
            unpack = configure_dictionary(value, run_settings_slice, foldover)
            ret_dict[param] = unpack[0]
            settings_index += unpack[1]
        elif isinstance(value, list):
            if run_settings[settings_index] == "-1":
                if foldover == False:
                    ret_dict[param] = value[0]
                else:
                    ret_dict[param] = value[1]
            elif run_settings[settings_index] == "1":
                if foldover == False:
                    ret_dict[param] = value[1]
                else:
                    ret_dict[param] = value[0]
                # ret_dict[param] = value[0]
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
