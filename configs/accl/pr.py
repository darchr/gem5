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
    argparser.add_argument("iterations", type=int)
    argparser.add_argument("alpha", type=float)
    argparser.add_argument("--num_nodes", type=int, default=1)
    argparser.add_argument("--error_threshold", type=float, default=0.0)
    argparser.add_argument(
        "--simple",
        dest="simple",
        action="store_const",
        const=True,
        default=False,
        help="Use simple memory for vertex",
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
        args.iterations,
        args.alpha,
        args.num_nodes,
        args.error_threshold,
        args.simple,
        args.sample,
        args.verify,
    )


if __name__ == "__m5_main__":
    (
        num_gpts,
        num_registers,
        cache_size,
        graph,
        iterations,
        alpha,
        num_nodes,
        error_threshold,
        simple,
        sample,
        verify,
    ) = get_inputs()

    print(f"error_threshold: {error_threshold}")

    if simple:
        from sega_simple import SEGA
    else:
        from sega import SEGA
    system = SEGA(num_gpts, num_registers, cache_size, graph)
    root = Root(full_system=False, system=system)

    m5.instantiate()

    system.set_bsp_mode()
    system.create_pop_count_directory(64)
    system.create_pr_workload(num_nodes, alpha)
    if sample:
        while True:
            exit_event = m5.simulate(100000000)
            print(
                f"Exited simulation at tick {m5.curTick()} "
                + f"because {exit_event.getCause()}"
            )
            m5.stats.dump()
            m5.stats.reset()
            if exit_event.getCause() != "simulate() limit reached":
                break
    else:
        iteration = 0
        while iteration < iterations:
            exit_event = m5.simulate()
            print(
                f"Exited simulation at tick {m5.curTick()} "
                + f"because {exit_event.getCause()}"
            )
            iteration += 1
            print(f"error: {system.get_pr_error()}")
            if system.get_pr_error() < error_threshold:
                break
            if system.work_count() == 0:
                break
    print(f"#iterations: {iteration}")
    if verify:
        system.print_answer()
