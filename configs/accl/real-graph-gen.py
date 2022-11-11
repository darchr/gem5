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
    argparser.add_argument("path", type=str, help="Path to the graph file.")
    argparser.add_argument(
        "num_gpts",
        type=int,
        help="Number gpts to create synth graph binaries for.",
    )

    args = argparser.parse_args()
    return args.path, args.num_gpts


if __name__ == "__main__":
    graph_path, num_gpts = get_inputs()

    graph_sorter = os.environ.get("GRAPH_SORTER")
    graph_reader = os.environ.get("GRAPH_READER")

    if graph_sorter is None:
        raise ValueError(f"No value for $GRAPH_SORTER.")
    if graph_reader is None:
        raise ValueError(f"No value for $GRAPH_READER.")

    if not os.path.exists(graph_path):
        raise ValueError(f"{graph_path} does not exist.")

    graph_dir = os.path.dirname(graph_path)
    sorted_graph = f"{graph_dir}/sorted_graph.txt"
    if not os.path.exists(sorted_graph):
        print(f"Sorting {graph_path} into {sorted_graph}.")
        subprocess.run(
            [
                "python",
                f"{graph_sorter}",
                f"{graph_path}",
                f"{sorted_graph}",
            ]
        )
    if not "binaries" in os.listdir(graph_dir):
        print(f"binaries directory not found in {graph_dir}")
        os.mkdir(f"{graph_dir}/binaries")
        print(f"Created {graph_dir}/binaries")

    if not f"gpts_{num_gpts}" in os.listdir(f"{graph_dir}/binaries"):
        print(f"gpts_{num_gpts} not found in {graph_dir}/binaries")
        os.mkdir(f"{graph_dir}/binaries/gpts_{num_gpts}")
        print(f"Created {graph_dir}/binaries/gpts_{num_gpts}")

    expected_bins = ["vertices"] + [f"edgelist_{i}" for i in range(num_gpts)]
    if not all(
        [
            binary in os.listdir(f"{graph_dir}/binaries/gpts_{num_gpts}")
            for binary in expected_bins
        ]
    ):
        print(
            f"Not all expected binaries found in {graph_path}/binaries/gpts_{num_gpts}"
        )
        for delete in os.scandir(f"{graph_dir}/binaries/gpts_{num_gpts}"):
            os.remove(delete.path)
        print(f"Deleted all the files in {graph_dir}/binaries/gpts_{num_gpts}")
        subprocess.run(
            [
                f"{graph_reader}",
                f"{sorted_graph}",
                "false",
                f"{num_gpts}",
                "32",
                f"{graph_dir}/binaries/gpts_{num_gpts}",
            ]
        )
        print(
            f"Created the graph binaries in "
            f"{graph_dir}/binaries/gpts_{num_gpts}"
        )
