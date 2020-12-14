
from enum import Enum
from typing import Dict, Optional, Tuple, Union


class TimeConversion:

    scaleFactor: float
    description: Optional[str]

    def __init__(self, scaleFactor: float, description: Optional[str] = None):
        self.scaleFactor = scaleFactor
        self.description = description

    def __init__(self, data: Dict[str, Union[str,float]]):
        self.scaleFactor = data.pop('scaleFactor')
        if 'description' in data:
            self.description = data.pop('description')
        else:
            self.description = None

        for prop,value in data.items():
            setattr(self, prop, value)

    @classmethod
    def load(cls, data: Dict[str, Union[str,float]]) -> "TimeConversion":
        return cls(data)

class StorageType(Enum):
    u32: str = "u32"
    u64: str = "u64"
    s32: str = "s32"
    s64: str = "s64"
    f32: str = "f32"
    f64: str = "f64"

    mapping: Dict[str,"StorageType"] = {
        "u32": u32,
        "u64": u64,
        "s32": s32,
        "s64": s64,
        "f32": f32,
        "f64": f64,
    }

    def load(cls, data: str) -> "StorageType":
        return cls.mapping[data]
