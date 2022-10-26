# Copyright (c) 2022 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import argparse
import subprocess


def get_inputs():
    argparser = argparse.ArgumentParser()
    argparser.add_argument(
        "scale", type=int, help="The scale of the synth graph to generate."
    )
    argparser.add_argument(
        "deg",
        type=int,
        help="The average degree of the synth graph to generate.",
    )
    argparser.add_argument(
        "num_gpts",
        type=int,
        help="Number gpts to create synth graph binaries for.",
    )

    args = argparser.parse_args()
    return args.scale, args.deg, args.num_gpts


if __name__ == "__main__":
    scale, deg, num_gpts = get_inputs()

    base_dir = os.environ.get("GRAPH_DIR", default="/tmp")
    graph_gen = os.environ.get("GRAPH_GEN")
    graph_reader = os.environ.get("GRAPH_READER")
    graph_sorter = os.environ.get("GRAPH_SORTER")
    if graph_gen is None:
        raise ValueError(f"No value for $GRAPH_GEN.")
    if graph_reader is None:
        raise ValueError(f"No value for $GRAPH_READER.")
    if graph_sorter is None:
        raise ValueError(f"No value for $GRAPH_SORTER")

    graph_path = os.path.join(base_dir, f"graph_{scale}_{deg}")
    if not os.path.exists(graph_path):
        print(f"{graph_path} does not exist already.")
        os.mkdir(graph_path)
        print(f"Created {graph_path}")

    if not "graph.txt" in os.listdir(graph_path):
        print(f"graph.txt not found in {graph_path}")
        for delete in os.scandir(graph_path):
            os.remove(delete.path)
        print(f"Deleted everything in {graph_path}")
        subprocess.run(
            [
                f"{graph_gen}",
                f"{scale}",
                f"{deg}",
                f"{graph_path}/graph_unordered.txt",
            ]
        )
        print(f"Generated a graph with scale " f"{scale} and deg {deg}")
        subprocess.run(
            [
                "python",
                f"{graph_sorter}",
                f"{graph_path}/graph_unordered.txt",
                f"{graph_path}/graph.txt",
            ]
        )
        print(
            f"Sorted the graph here {graph_path}/graph_unordered.txt"
            f" and saved in {graph_path}/graph.txt"
        )
        subprocess.run(["rm", f"{graph_path}/graph_unordered.txt"])
        print(f"Deleted {graph_path}/graph_unordered.txt")

    if not "binaries" in os.listdir(graph_path):
        print(f"binaries directory not found in {graph_path}")
        os.mkdir(f"{graph_path}/binaries")
        print(f"Created {graph_path}/binaries")

    if not f"gpts_{num_gpts}" in os.listdir(f"{graph_path}/binaries"):
        print(f"gpts_{num_gpts} not found in {graph_path}/binaries")
        os.mkdir(f"{graph_path}/binaries/gpts_{num_gpts}")
        print(f"Created {graph_path}/binaries/gpts_{num_gpts}")

    expected_bins = ["vertices"] + [f"edgelist_{i}" for i in range(num_gpts)]
    if not all(
        [
            binary in os.listdir(f"{graph_path}/binaries/gpts_{num_gpts}")
            for binary in expected_bins
        ]
    ):
        print(
            f"Not all expected binaries found in {graph_path}/binaries/gpts_{num_gpts}"
        )
        for delete in os.scandir(f"{graph_path}/binaries/gpts_{num_gpts}"):
            os.remove(delete.path)
        print(
            f"Deleted all the files in {graph_path}/binaries/gpts_{num_gpts}"
        )
        subprocess.run(
            [
                f"{graph_reader}",
                f"{graph_path}/graph.txt",
                "false",
                f"{num_gpts}",
                "32",
                f"{graph_path}/binaries/gpts_{num_gpts}",
            ]
        )
        print(
            f"Created the graph binaries in "
            f"{graph_path}/binaries/gpts_{num_gpts}"
        )
