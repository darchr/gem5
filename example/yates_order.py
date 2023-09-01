import argparse
import json


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--size",
        help="Input the number of parameters/factors.",
        type=int,
        required=True,
    )
    args = parser.parse_args()

    num_to_reversed = {}
    for i in range(0, 2**args.size):
        bin_str = f"{i:0>{args.size}b}"
        rev_str = bin_str[::-1]
        print(bin_str)
        print(rev_str)
        new_num = int(rev_str, 2)
        num_to_reversed[i] = new_num
    print(num_to_reversed)
    # return num_to_reversed
    with open(f"./yates-order/{args.size}.json", "w") as yates_json:
        yates_json.write(json.dumps(num_to_reversed))


main()
