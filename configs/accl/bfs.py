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

from m5.objects import *


def get_inputs():
    argparser = argparse.ArgumentParser()
    argparser.add_argument("num_gpts", type=int)
    argparser.add_argument("num_registers", type=int)
    argparser.add_argument("cache_size", type=str)
    argparser.add_argument("graph", type=str)
    argparser.add_argument("init_addr", type=int)
    argparser.add_argument("init_value", type=int)
    argparser.add_argument(
        "--tile",
        dest="tile",
        action="store_const",
        const=True,
        default=False,
        help="Whether to use temporal partitioning",
    )
    argparser.add_argument(
        "--best",
        dest="best",
        action="store_const",
        const=True,
        default=False,
        help="Whether to use best update value for switching slices",
    )
    argparser.add_argument(
        "--visited",
        dest="visited",
        action="store_const",
        const=True,
        default=False,
        help="Use visitation version of BFS",
    )
    argparser.add_argument(
        "--simple",
        dest="simple",
        action="store_const",
        const=True,
        default=False,
        help="Use simple memory for vertex",
    )
    argparser.add_argument(
        "--dsimple",
        dest="dsimple",
        action="store_const",
        const=True,
        default=False,
        help="Use simple memory for both vertex and edge",
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
        args.num_registers,
        args.cache_size,
        args.graph,
        args.init_addr,
        args.init_value,
        args.tile,
        args.best,
        args.visited,
        args.simple,
        args.dsimple,
        args.sample,
        args.verify,
    )


if __name__ == "__m5_main__":
    (
        num_gpts,
        num_registers,
        cache_size,
        graph,
        init_addr,
        init_value,
        tile,
        best,
        visited,
        simple,
        dsimple,
        sample,
        verify,
    ) = get_inputs()

    if simple:
        if dsimple:
            raise ValueError("Can only pass either of --simple or --dsimple")
        from sega_simple import SEGA
    elif dsimple:
        if simple:
            raise ValueError("Can only pass either of --simple or --dsimple")
        from sega_double_simple import SEGA
    else:
        from sega import SEGA

    system = SEGA(num_gpts, num_registers, cache_size, graph)
    if tile:
        system.set_aux_images(f"{graph}/mirrors", f"{graph}/mirrors_map")

    if best:
        system.set_choose_best(True)

    root = Root(full_system=False, system=system)

    m5.instantiate()

    if tile:
        system.set_pg_mode()
    else:
        system.set_async_mode()

    system.create_pop_count_directory(32)
    if visited:
        system.create_bfs_visited_workload(init_addr, init_value)
    else:
        system.create_bfs_workload(init_addr, init_value)
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
