from typing import Any

from m5.objects import SimObject


class Modifiable:
    def __init__(self):
        self._modifiers = []

    def modify_part(self, part_name: str, new_part: Any, **params):
        raise NotImplementedError

    @classmethod
    def get_modifiables(cls):
        raise NotImplementedError

    def add_modifier(self, modifier):
        self._modifiers.append(modifier)

    def do_modify(self):
        for callable_object in self._modifiers:
            callable_object()
