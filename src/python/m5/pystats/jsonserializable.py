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

from datetime import datetime
from typing import Dict, List, Union, Any

from .storagetype import StorageType

class JsonSerializable:
    """
    Classes which inherit from JsonSerializable can be translated into JSON
    using Python's json package.

    Usage
    -----
    ``
    import json

    print(json.dumps(serializable_object.to_json_dict()))
    ``
    """

    def to_json_dict(self) -> Dict:
        """
        Translates the current object into a dictionary which may be utilized
        by the Python stdlib JSON package.

        Returns
        -------
        Dict
            The dictionary which may be utlized by the python stdlib JSON
            package.
        """

        model_dct = {}
        for key, value in self.__dict__.items():
            new_value = self.__process_value(value)
            model_dct[key] = new_value
        return model_dct

    def __process_value(self, value: Any) -> Union[str,int,float,Dict,List]:
        """
        A private function used to translate values into a value which can be
        handled by the Python stdlib JSON package.

        Parameters
        ----------
        value: Any
            The value to be translated.

        Returns
        -------
        Union[str,int,float,Dict,List]
            A value which can be handled by the Python stdlib JSON package.
        """

        if isinstance(value, JsonSerializable):
            return value.to_json_dict()
        elif isinstance(value, (str, int, float)):
            return value
        elif isinstance(value, datetime):
            return value.replace(microsecond=0).isoformat()
        elif isinstance(value, list):
            return [self.__process_value(v) for v in value]
        elif isinstance(value, StorageType):
            return str(value.name)

        return None