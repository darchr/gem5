
import datetime
from typing import Any, Dict, List, Optional, Union

from .model import Model
from .statistic import Statistic
from .util import TimeConversion


class SimStat:
    """Class for a set of statistics from a simulation.
    """

    creationTime: Optional[datetime.datetime]
    timeConversion: Optional[TimeConversion]
    simulatedBeginTime: Optional[Union[int, float]]
    simulatedEndTime: Optional[Union[int, float]]

    def __init__(self, creationTime: Optional[datetime.datetime],
                 timeConversion: Optional[TimeConversion],
                 simulatedBeginTime: Optional[Union[int, float]],
                 simulatedEndTime: Optional[Union[int, float]],
                 **kwargs: Dict[str, Union[Model,Statistic,List[Model]]]):
        self.creationTime = creationTime
        self.timeConversion = timeConversion
        self.simulatedBeginTime = simulatedBeginTime
        self.simulatedEndTime = simulatedEndTime

        for key,value in kwargs:
            setattr(self, key, value)

    def __init__(self, data: Dict[str, Any]):
        if 'creationTime' in data:
            self.creationTime = datetime.datetime(data.pop('creationTime'))
        else:
            self.creationTime = None

        if 'timeConversion' in data:
            self.timeConversion = TimeConversion.load(
                                    data.pop('timeConversion'))
        else:
            self.timeConversion = None

        for prop in ('simulatedBeginTime', 'simulatedEndTime'):
            if prop in data:
                setattr(self, prop, data.pop(prop))
            else:
                setattr(self, prop, None)

        for prop,value in data.items():
            if type(value) is list:
                setattr(self, prop, [Model.load(model) for model in value])
            else:
                try:
                    setattr(self, prop, Statistic.load(value))
                except KeyError:
                    # If we can't load it as a statistic then it must be a
                    # model Note that currently models can have *anything*, so
                    # this doesn't do any checking, it should always succeed
                    setattr(self, prop, Model.load(value))

    @classmethod
    def load(cls, data: Dict[str, Any]):
        return cls(data)
