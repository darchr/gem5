# Copyright (c) 2021 The Regents of the University of California
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

from enum import (
    Enum,
    unique,
)
from pathlib import Path
from typing import (
    Generator,
    Optional,
    Union,
)

import m5.stats
from m5 import (
    MaxTick,
    curTick,
)
from m5.ticks import fromSeconds
from m5.util import warn
from m5.util.convert import toLatency

from gem5.resources.looppoint import Looppoint

from ..components.processors.abstract_processor import AbstractProcessor
from ..components.processors.spatter_gen import SpatterGenerator
from ..components.processors.switchable_processor import SwitchableProcessor
from ..resources.resource import SimpointResource

"""
In this package we store generators for simulation exit events.
"""


@unique
class SimStep(Enum):
    STOP = 0
    REMAINING_TIME = 2**63 - 1

    @classmethod
    def convert_to_ticks(cls, time):
        # --- Legacy MPI-gem5 compatibility ---
        # Old generators return True → exit; False/None → continue
        if time is True:
            return 0  # STOP
        if time is False or time is None:
            return MaxTick - curTick()  # Continue

        # --- New gem5 behavior ---
        if isinstance(time, SimStep):
            if time == SimStep.STOP:
                return 0
            if time == SimStep.REMAINING_TIME:
                return MaxTick - curTick()
        elif isinstance(time, str):
            return fromSeconds(toLatency(time))

        raise TypeError(f"Unsupported SimStep type: {time}")

    # @classmethod
    # def convert_to_ticks(cls, time: Union["SimStep", str]) -> int:
    #     if isinstance(time, SimStep):
    #         if time == SimStep.STOP:
    #             return 0
    #         if time == SimStep.REMAINING_TIME:
    #             return MaxTick - curTick()
    #     elif isinstance(time, str):
    #         return fromSeconds(toLatency(time))


def warn_default_decorator(gen: Generator, type: str, effect: str):
    """A decortator for generators which will print a warning that it is a
    default generator.
    """

    def wrapped_generator(*args, **kw_args):
        warn(
            f"No behavior was set by the user for {type}."
            f" Default behavior is {effect}."
        )
        yield from gen(*args, **kw_args)

    return wrapped_generator


def exit_generator():
    """
    A default generator for an exit event. It will return ``True``, indicating that
    the Simulator run loop should exit.
    """
    while True:
        yield True


def switch_generator(processor: AbstractProcessor):
    """
    A default generator for a switch exit event. If the processor is a
    SwitchableProcessor, this generator will switch it. Otherwise nothing will
    happen.
    """
    is_switchable = isinstance(processor, SwitchableProcessor)
    while True:
        if is_switchable:
            yield processor.switch()
        else:
            yield False


def dump_reset_generator():
    """
    A generator for doing statstic dump and reset. It will reset the simulation
    statistics and then dump simulation statistics.

    The Simulation run loop will continue after executing the behavior of the
    generator.
    """
    while True:
        m5.stats.dump()
        m5.stats.reset()
        yield False


def save_checkpoint_generator(checkpoint_dir: Optional[Path] = None):
    """
    A generator for taking a checkpoint. It will take a checkpoint with the
    input path and the current simulation ``Ticks``.

    The Simulation run loop will continue after executing the behavior of the
    generator.
    """
    if not checkpoint_dir:
        from m5 import options

        checkpoint_dir = Path(options.outdir)
    while True:
        m5.checkpoint((checkpoint_dir / f"cpt.{str(m5.curTick())}").as_posix())
        yield False


def reset_stats_generator():
    """
    This generator resets the stats every time it is called. It does not dump
    the stats before resetting them.
    """
    while True:
        m5.stats.reset()
        yield False


def dump_stats_generator():
    """
    This generator dumps the stats every time it is called.
    """
    while True:
        m5.stats.dump()
        yield False


def skip_generator():
    """
    This generator does nothing when on the exit event.

    The simulation will continue after this generator.
    """
    while True:
        yield False


def simpoints_save_checkpoint_generator(
    checkpoint_dir: Path, simpoint: SimpointResource
):
    """
    A generator for taking multiple checkpoints for SimPoints. It will save the
    checkpoints in the ``checkpoint_dir`` path with the SimPoints' index.
    The Simulation run loop will continue after executing the behavior of the
    generator until all the SimPoints in the ``simpoint_list`` has taken a
    checkpoint.
    """
    simpoint_list = simpoint.get_simpoint_start_insts()
    count = 0
    last_start = -1
    while True:
        m5.checkpoint((checkpoint_dir / f"cpt.SimPoint{count}").as_posix())
        last_start = simpoint_list[count]
        count += 1
        # When the next SimPoint starting instruction is the same as the last
        # one, it will take a checkpoint for it with index+1. Because of there
        # are cases that the warmup length is larger than multiple SimPoints
        # starting instructions, then they might cause duplicates in the
        # simpoint_start_ints.
        while (
            count < len(simpoint_list) and last_start == simpoint_list[count]
        ):
            m5.checkpoint((checkpoint_dir / f"cpt.SimPoint{count}").as_posix())
            last_start = simpoint_list[count]
            count += 1
        # When there are remaining SimPoints in the list, let the Simulation
        # loop continues, otherwise, exit the Simulation loop.
        if count < len(simpoint_list):
            yield False
        else:
            yield True


def looppoint_save_checkpoint_generator(
    checkpoint_dir: Path,
    looppoint: Looppoint,
    update_relatives: bool = True,
    exit_when_empty: bool = True,
):
    """
    A generator for taking a checkpoint for LoopPoint. It will save the
    checkpoints in the checkpoint_dir path with the Region id.

    (i.e. "cpt.Region10) It only takes a checkpoint if the current PC Count
    pair is a significant PC Count Pair. This is determined in the LoopPoint
    module. The simulation loop continues after exiting this generator.

    :param checkpoint_dir: Where to save the checkpoints.
    :param loopoint: The LoopPoint object used in the configuration script
    :param update_relative: If the generator should update the relative count
                            information in the output json file, then it should
                            be ``True``. It is default as ``True``.
    :param exit_when_empty: If the generator should exit the simulation loop if
                            all PC paris have been discovered, then it should be
                            ``True``. It is default as ``True``.
    """
    if exit_when_empty:
        total_pairs = len(looppoint.get_targets())
    else:
        total_pairs = -1
        # it will never equal to 0 if exit_when_empty is false

    while total_pairs != 0:
        region = looppoint.get_current_region()
        # if it is a significant PC Count pair, then the get_current_region()
        # will return an integer greater than 0. By significant PC Count pair,
        # it means the PC Count pair that indicates where to take the
        # checkpoint at. This is determined in the LoopPoint module.
        if region:
            if update_relatives:
                looppoint.update_relatives_counts()
            m5.checkpoint((checkpoint_dir / f"cpt.Region{region}").as_posix())
        total_pairs -= 1
        yield False

    yield True


def spatter_exit_generator(spatter_gen: SpatterGenerator):
    while True:
        assert isinstance(spatter_gen, SpatterGenerator)
        yield from spatter_gen.handle_spatter_exit()


# Copyright (c) 2021 The Regents of the University of California
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

# from enum import Enum, unique
# from pathlib import Path
# from typing import Generator, Optional, Union

# import m5.stats
# from m5 import MaxTick, curTick
# from m5.ticks import fromSeconds
# from m5.util import warn
# from m5.util.convert import toLatency

# from gem5.resources.looppoint import Looppoint

# from ..components.processors.abstract_processor import AbstractProcessor
# from ..components.processors.spatter_gen import SpatterGenerator
# from ..components.processors.switchable_processor import SwitchableProcessor
# from ..resources.resource import SimpointResource

# """
# In this package we store generators for simulation exit events.
# """


# @unique
# class SimStep(Enum):
#     STOP = 0
#     REMAINING_TIME = 2**63 - 1

#     @classmethod
#     def convert_to_ticks(cls, time: Union["SimStep", str]) -> int:
#         if isinstance(time, SimStep):
#             if time == SimStep.STOP:
#                 return 0
#             if time == SimStep.REMAINING_TIME:
#                 return MaxTick - curTick()
#         elif isinstance(time, str):
#             return fromSeconds(toLatency(time))


# def warn_default_decorator(gen: Generator, type: str, effect: str):
#     """A decortator for generators which will print a warning that it is a
#     default generator.
#     """

#     def wrapped_generator(*args, **kw_args):
#         warn(
#             f"No behavior was set by the user for {type}."
#             f" Default behavior is {effect}."
#         )
#         yield from gen(*args, **kw_args)

#     return wrapped_generator


# def exit_generator():
#     """
#     A default generator for an exit event. It will return ``True``, indicating that
#     the Simulator run loop should exit.
#     """
#     while True:
#         yield SimStep.STOP


# def switch_generator(processor: AbstractProcessor):
#     """
#     A default generator for a switch exit event. If the processor is a
#     SwitchableProcessor, this generator will switch it. Otherwise nothing will
#     happen.
#     """
#     is_switchable = isinstance(processor, SwitchableProcessor)
#     while True:
#         if is_switchable:
#             yield processor.switch()
#         else:
#             yield SimStep.REMAINING_TIME


# def dump_reset_generator():
#     """
#     A generator for doing statstic dump and reset. It will reset the simulation
#     statistics and then dump simulation statistics.

#     The Simulation run loop will continue after executing the behavior of the
#     generator.
#     """
#     while True:
#         m5.stats.dump()
#         m5.stats.reset()
#         yield SimStep.REMAINING_TIME


# def save_checkpoint_generator(checkpoint_dir: Optional[Path] = None):
#     """
#     A generator for taking a checkpoint. It will take a checkpoint with the
#     input path and the current simulation ``Ticks``.

#     The Simulation run loop will continue after executing the behavior of the
#     generator.
#     """
#     if not checkpoint_dir:
#         from m5 import options

#         checkpoint_dir = Path(options.outdir)
#     while True:
#         m5.checkpoint((checkpoint_dir / f"cpt.{str(m5.curTick())}").as_posix())
#         yield SimStep.REMAINING_TIME


# def reset_stats_generator():
#     """
#     This generator resets the stats every time it is called. It does not dump
#     the stats before resetting them.
#     """
#     while True:
#         m5.stats.reset()
#         yield SimStep.REMAINING_TIME


# def dump_stats_generator():
#     """
#     This generator dumps the stats every time it is called.
#     """
#     while True:
#         m5.stats.dump()
#         yield SimStep.REMAINING_TIME


# def skip_generator():
#     """
#     This generator does nothing when on the exit event.

#     The simulation will continue after this generator.
#     """
#     while True:
#         yield SimStep.REMAINING_TIME


# def simpoints_save_checkpoint_generator(
#     checkpoint_dir: Path, simpoint: SimpointResource
# ):
#     """
#     A generator for taking multiple checkpoints for SimPoints. It will save the
#     checkpoints in the ``checkpoint_dir`` path with the SimPoints' index.
#     The Simulation run loop will continue after executing the behavior of the
#     generator until all the SimPoints in the ``simpoint_list`` has taken a
#     checkpoint.
#     """
#     simpoint_list = simpoint.get_simpoint_start_insts()
#     count = 0
#     last_start = -1
#     while True:
#         m5.checkpoint((checkpoint_dir / f"cpt.SimPoint{count}").as_posix())
#         last_start = simpoint_list[count]
#         count += 1
#         # When the next SimPoint starting instruction is the same as the last
#         # one, it will take a checkpoint for it with index+1. Because of there
#         # are cases that the warmup length is larger than multiple SimPoints
#         # starting instructions, then they might cause duplicates in the
#         # simpoint_start_ints.
#         while (
#             count < len(simpoint_list) and last_start == simpoint_list[count]
#         ):
#             m5.checkpoint((checkpoint_dir / f"cpt.SimPoint{count}").as_posix())
#             last_start = simpoint_list[count]
#             count += 1
#         # When there are remaining SimPoints in the list, let the Simulation
#         # loop continues, otherwise, exit the Simulation loop.
#         if count < len(simpoint_list):
#             yield SimStep.REMAINING_TIME
#         else:
#             yield SimStep.STOP


# def looppoint_save_checkpoint_generator(
#     checkpoint_dir: Path,
#     looppoint: Looppoint,
#     update_relatives: bool = True,
#     exit_when_empty: bool = True,
# ):
#     """
#     A generator for taking a checkpoint for LoopPoint. It will save the
#     checkpoints in the checkpoint_dir path with the Region id.

#     (i.e. "cpt.Region10) It only takes a checkpoint if the current PC Count
#     pair is a significant PC Count Pair. This is determined in the LoopPoint
#     module. The simulation loop continues after exiting this generator.

#     :param checkpoint_dir: Where to save the checkpoints.
#     :param loopoint: The LoopPoint object used in the configuration script
#     :param update_relative: If the generator should update the relative count
#                             information in the output json file, then it should
#                             be ``True``. It is default as ``True``.
#     :param exit_when_empty: If the generator should exit the simulation loop if
#                             all PC paris have been discovered, then it should be
#                             ``True``. It is default as ``True``.
#     """
#     if exit_when_empty:
#         total_pairs = len(looppoint.get_targets())
#     else:
#         total_pairs = -1
#         # it will never equal to 0 if exit_when_empty is false

#     while total_pairs != 0:
#         region = looppoint.get_current_region()
#         # if it is a significant PC Count pair, then the get_current_region()
#         # will return an integer greater than 0. By significant PC Count pair,
#         # it means the PC Count pair that indicates where to take the
#         # checkpoint at. This is determined in the LoopPoint module.
#         if region:
#             if update_relatives:
#                 looppoint.update_relatives_counts()
#             m5.checkpoint((checkpoint_dir / f"cpt.Region{region}").as_posix())
#         total_pairs -= 1
#         yield SimStep.REMAINING_TIME

#     yield SimStep.STOP


# def spatter_exit_generator(spatter_gen: SpatterGenerator):
#     while True:
#         assert isinstance(spatter_gen, SpatterGenerator)
#         yield from spatter_gen.handle_spatter_exit()
