# Copyright (c) 2020 The Regents of The University of California
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

import _m5.stats
import json
from datetime import datetime
# import all of the SimObjects
from m5.objects import *
from .model import *
from .simstat import *
from .statistic import *

def _get_model(simobj: SimObject):
    if hasattr(simobj, "type"):
        simobj_type = simobj.type
    else:
        simobj_type = None

    time_conversion = None # TODO: Not added yet

    # Start the dictionary by adding the statistics
    stats_dict = {}
    for stat in simobj.getStats():
        statistic = _get_statistic(stat)
        if statistic is not None:
            stats_dict[stat.name] = statistic

    #Then add the other models
    for key in simobj.getStatGroups():
        stats_dict[key] = _get_model(simobj.getStatGroups()[key])

    return Model(simobj_type, time_conversion, **stats_dict)

def _get_statistic(statistic):
    # Right now I'll just focus on scalars. # TODO
    if isinstance(statistic, _m5.stats.ScalarInfo):
        value = statistic.value()
        stat_type = "Scalar"
        unit = None # TODO
        description = statistic.desc
        datatype = None # TODO

        return Scalar(value, stat_type, unit, description, datatype)
    return None

def _get_simstat(root: Root):
    creation_time = datetime.now()
    time_converstion = None # TODO
    final_tick = root.resolveStat("final_tick").value()
    # The number of ticks simulated.
    sim_ticks = root.resolveStat("sim_ticks").value()
    simulated_begin_time = int(final_tick - sim_ticks)
    simulated_end_time = int(final_tick)

    stats_map = {}
    for key,value in root.getStatGroups().items():
        stats_map[key] = _get_model(value)

    return SimStat(creationTime=creation_time,
                   timeConversion=time_converstion,
                   simulatedBeginTime=simulated_begin_time,
                   simulatedEndTime=simulated_end_time,
                   **stats_map)

def get_json_str(root: Root, indent: Optional[int] = 4):
    simstat = _get_simstat(root)
    return json.dumps(simstat.to_json_dict(), indent=indent)

def json_to_file(root: Root, file: str):
    with open(file, "w") as out:
        out.write(get_json_str(root))
