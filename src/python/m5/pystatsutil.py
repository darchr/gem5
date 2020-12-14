# Copyright (c) 2021 The Regents of The University of California
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

"""
This Python module serves as the bridge between the gem5 statistics exposed via
PyBind and the Python Stats model in `src/python/m5/pystats`.

This module allows translation from the gem5 statistics and the Python Stats
model and helper functions to obtain the Python JSON output of a simulation.
"""

from typing import IO
import json

from _m5.stats import *
from m5.objects import *
from m5.pystats.group import *
from m5.pystats.simstat import *
from m5.pystats.statistic import *
from m5.pystats.storagetype import *

def get_stats_group(group) -> Group:
    """
    Translates a gem5 object into a Group object. A Group object is a
    map/dictionary of labeled Statistic objects. Any gem5 object passed to this
    will have its ``getStats()`` and ``getStatGroups`` function called, and all
    the stats translated (inclusive of the stats further down the hierarchy).

    Parameters
    ----------
    group:
        The gem5 object to be translated to be a Group object. Typically this
        will be a gem5 SimObject.

    Returns
    -------
    Group
        The stats group object translated from the input gem5 object.
    """

    stats_dict = {}
    for stat in group.getStats():
        statistic = __get_statistic(stat)
        if statistic is not None:
            stats_dict[stat.name] = statistic

    for key in group.getStatGroups():
        stats_dict[key] = get_stats_group(group.getStatGroups()[key])

    return Group(**stats_dict)

def __get_statistic(statistic: Info) -> Optional[Statistic]:
    """
    Translates a _m5.stats.Info object into a Statistic object, to process
    statistics at the Python level.

    Parameters
    ----------
    statistic: _m5.stats.Info
        The m5.stats.Info object to be translated to a Statistic object.

    Returns
    -------
    Optional[Statistic]
        The Statistic object of the _m5.stats.Info object. Returns ``None``
        if parameter object cannot be translated.

    """

    assert(isinstance(statistic, Info))
    statistic.prepare()

    if isinstance(statistic, ScalarInfo):
        value = statistic.value()
        unit = None # TODO https://gem5.atlassian.net/browse/GEM5-850.
        description = statistic.desc
        # ScalarInfo uses the C++ `double`.
        datatype = StorageType["f64"]

        return Scalar(
                      value=value,
                      unit=unit,
                      description=description,
                      datatype=datatype,
                      )

    elif isinstance(statistic, DistInfo):
        unit = None # TODO https://gem5.atlassian.net/browse/GEM5-850.
        description = statistic.desc
        value = statistic.values()
        min = statistic.min()
        max = statistic.max()
        bin_size = statistic.bucket_size()
        num_bins = len(value)
        # DistInfo uses the C++ `double`.
        datatype = StorageType["f64"]

        return Distribution(
                            value=value,
                            min=min,
                            max=max,
                            numBins=num_bins,
                            binSize=bin_size,
                            unit=unit,
                            description=description,
                            datatype=datatype,
                            )

    elif isinstance(statistic, FormulaInfo):
        # We don't do anything with Formula's right now.
        # We may never do so, see https://gem5.atlassian.net/browse/GEM5-868.
        pass

    elif isinstance(statistic, VectorInfo):
        to_add = {}
        for index in range(statistic.size()):
            value = statistic.value()[index]
            unit = None # TODO https://gem5.atlassian.net/browse/GEM5-850.
            description = statistic.subdescs[index]
            # ScalarInfo uses the C++ `double`.
            datatype = StorageType["f64"]

            to_add[statistic.subnames[index]] = Scalar(
                                                      value=value,
                                                      unit=unit,
                                                      description=description,
                                                      datatype=datatype,
                                                      )

        return Vector(scalar_map=to_add)

    return None

def get_simstat(root: Root) -> SimStat:
    """
    This function will return the SimStat object for a simulation. From the
    SimStat object all stats within the current gem5 simulation are present.

    Parameters
    ----------
    root: Root
        The root of the simulation.

    Returns
    -------
    SimStat
        The SimStat Object of the current simulation.

    """

    from datetime import datetime
    creation_time = datetime.now()
    time_converstion = None # TODO
    final_tick = root.resolveStat("final_tick").value()
    sim_ticks = root.resolveStat("sim_ticks").value()
    simulated_begin_time = int(final_tick - sim_ticks)
    simulated_end_time = int(final_tick)

    stats_map = {}
    stats_map['system'] = get_stats_group(root.system)

    return SimStat(
                   creationTime=creation_time,
                   timeConversion=time_converstion,
                   simulatedBeginTime=simulated_begin_time,
                   simulatedEndTime=simulated_end_time,
                   **stats_map,
                  )

def dumps(root: Root, **kwargs) -> str:
    """
    This function mirrors the Python stdlib JSON module method ``json.dumps``.
    It is used to obtain the gem5 statistics output to a JSON string.

    Parameters
    ----------
    root: Root
        The root of the simulation.

    kwargs: Dict[str, Any]
        Additional parameters to be passed to the ``json.dumps`` method.

    Returns
    -------
    str
        A string of the gem5 Statistics in a JSON format.


    Usage Example
    -------------
    ``
    print(statshandler.dumps(root=root, indent=6))
    ``

    The above will print the simulation statistic JSON string. The indentation
    will be 6 (by default the indentation is 4).

    """

    # Setting the default indentation to something readable.
    if 'indent' not in kwargs:
        kwargs['indent'] = 4

    simstat = get_simstat(root=root)
    return json.dumps(obj=simstat.to_json_dict(), **kwargs)

def dump(root: Root, fp: IO[str], **kwargs) -> None:
    """
    This function mirrors the Python stdlib JSON module method ``json.dump``.
    The root of the simulation is passed, and the JSON is output to the
    specified.


    Parameters
    ----------
    root: Root
        The root of the simulation.

    fp: IO[str]
        The Text IO stream to output the JSON to.

    **kwargs:
        Additional parameters to be passed to the ``json.dump`` method.

    Usage
    -----
    ``
    with open("test.json") as f:
        statshandler.dump(root=root, fp=f, indent=6)
    ``

    The above will dump the json output to the 'test.json' file. The
    indentation will be of 6 (by default the indentation is 4).
    """

    # Setting the default indentation to something readable.
    if 'indent' not in kwargs:
        kwargs['indent'] = 4

    simstat = get_simstat(root=root)
    json.dump(obj=simstat.to_json_dict(), fp=fp, **kwargs)
