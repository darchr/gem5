import argparse
import math


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--factors",
        type=int,
        required=True,
        help="Number of parameters or factors you want to test. The output file will have 2^factors rows.",
    )
    args = parser.parse_args()

    with open(f"{args.factors}_full_factorial_matrix.txt", "w") as out:
        for i in range(0, 2**args.factors):
            temp = str(bin(i))[2:]
            temp = f"{temp:0>{args.factors}}"
            temp = " ".join(temp)
            out.write(temp)
            out.write("\n")


main()
