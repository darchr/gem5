
from typing import Any, Dict, Optional, Union

from .util import StorageType

class Statistic:
    """
    A simulator statistic.
    """

    value: Any
    type: Optional[str]
    unit: Optional[str]
    description: Optional[str]

    def __init__(self, value: Any, type: Optional[str] = None,
                 unit: Optional[str] = None,
                 description: Optional[str] = None):
        self.value = value
        self.type = type
        self.unit = unit
        self.description = description

    #def __init__(self, data: Dict[str, Any]):
     #   self.value = data['value']

        # Optional properties
     #   for prop in ('type', 'unit', 'description'):
     #       if prop in data:
     #           setattr(self, prop, data.pop(prop))
      #      else:
      #          setattr(self, prop, None)

    @classmethod
    def load(cls, data: Dict[str, Any]) -> "Statistic":
        return cls(data)


class Scalar(Statistic):

    value: Union[float, int]
    datatype: Optional[StorageType]

    def __init__(self, value: Any, type: Optional[str] = None,
                 unit: Optional[str] = None, description: Optional[str] = None,
                 datatype: Optional[StorageType] = None):
        super(Scalar, self).__init__(value, type, unit, description)

        if self.type is not None:
            assert(self.type == 'Scalar')
        else:
            self.type = 'Scalar'

        self.datatype = datatype

    #def __init__(self, data: Dict[str, Any]):
    #    super(Scalar, self).__init__(data)

     #   for prop in ('datatype'):
     #       if prop in data:
      #          setattr(self, prop, data.pop(prop))
      #      else:
#                setattr(self, prop, None)

    @classmethod
    def load(cls, data: Dict[str, Any]) -> "Scalar":
        return cls(data)
