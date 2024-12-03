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

import m5
import argparse
import json

from m5.objects import *
from sega import SEGA


def get_inputs():
    argparser = argparse.ArgumentParser()
    argparser.add_argument("num_gpts", type=int)
    argparser.add_argument("cache_size", type=str)
    argparser.add_argument("graph", type=str)
    argparser.add_argument(
        "--tile",
        dest="tile",
        action="store_const",
        const=True,
        default=False,
        help="Whether to use temporal partitioning",
    )
    argparser.add_argument(
        "--sample",
        dest="sample",
        action="store_const",
        const=True,
        default=False,
        help="Sample sim stats every 100us",
    )
    argparser.add_argument(
        "--verify",
        dest="verify",
        action="store_const",
        const=True,
        default=False,
        help="Print final answer",
    )

    args = argparser.parse_args()

    return (
        args.num_gpts,
        args.cache_size,
        args.graph,
        args.tile,
        args.sample,
        args.verify,
    )


if __name__ == "__m5_main__":
    (
        num_gpts,
        cache_size,
        graph,
        tile,
        sample,
        verify,
    ) = get_inputs()

    system = SEGA(num_gpts, cache_size, graph)
    if tile:
        system.set_aux_images(f"{graph}/mirrors", f"{graph}/mirrors_map")

    root = Root(full_system=False, system=system)

    m5.instantiate()

    if tile:
        system.set_pg_mode()
    else:
        system.set_async_mode()

    # Vector multiplicand
    vector = [1.2, 4.2, 3.0, 2.1]

    system.create_pop_count_directory(32)
    system.create_spmv_workload(vector)

    if sample:
        while True:
            exit_event = m5.simulate(50000000)
            print(
                f"Exited simulation at tick {m5.curTick()} "
                + f"because {exit_event.getCause()}"
            )
            if exit_event.getCause() == "simulate() limit reached":
                m5.stats.dump()
                m5.stats.reset()
            elif exit_event.getCause() == "Done with all the slices.":
                break
            elif exit_event.getCause() == "no update left to process.":
                break
    else:
        while True:
            exit_event = m5.simulate()
            print(
                f"Exited simulation at tick {m5.curTick()} "
                + f"because {exit_event.getCause()}"
            )
            if exit_event.getCause() == "Done with all the slices.":
                break
            if exit_event.getCause() == "no update left to process.":
                break

    if verify:
        system.print_answer()
