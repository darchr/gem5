
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

from typing import Any, Dict, KeysView, List, Optional, Tuple, Union

from .statistic import Statistic
from .util import TimeConversion


class Model:

    type: Optional[str]
    timeConversion: Optional[TimeConversion]

    def __init__(self, type: Optional[str] = None,
                 timeConversion: Optional[TimeConversion] = None,
                 **kwargs: Dict[str, Union["Model",Statistic,List["Model"]]]):
        self.type = type
        self.timeConversion = timeConversion

        for key,value in kwargs.items():
            setattr(self, key, value)

    #def __init__(self, data: Dict[str, Any]):
    #    for prop in ('type', 'timeConversion'):
    #        if prop in data:
    #            setattr(self, prop, data.pop(prop))
    #        else:
    #            setattr(self, prop, None)

    #    for prop,value in data.items():
    #        if type(value) is list:
    #            setattr(self, prop, [Model.load(model) for model in value])
    #        else:
    #            try:
    #                setattr(self, prop, Statistic.load(value))
    #            except KeyError:
                    # If we can't load it as a statistic then it must be a
                    # model. Note that currently models can have *anything*, so
                    # this doesn't do any checking, it should always succeed
    #                setattr(self, prop, Model.load(value))

    @classmethod
    def load(cls, data: Dict[str,
                Union["Model",Statistic,List["Model"]]]) -> "Model":
        return cls(data)
