import os
import itertools

import config

class UID(object):
    sep = ':'
    type_idx, path_idx = range(2)

    def __init__(self, path, *args):
        self.path = path
        self.attributes = args
    
    def _path_to_str(self):
        return os.path.relpath(self.path,
                os.path.commonprefix((config.constants.testing_base,
                                      self.path)))

    @classmethod
    def uid_to_path(cls, uid):
        split_path = str(uid).split(cls.sep)[cls.path_idx]
        return os.path.join(config.constants.testing_base, split_path) 

    @classmethod
    def uid_to_class(cls, uid):
        return globals()[uid.split(cls.sep)[cls.type_idx]]

    @classmethod
    def from_suite(self, suite, filepath):
        return SuiteUID(filepath, suite.name)
    
    @classmethod
    def from_test(self, test, filepath):
        return TestUID(filepath, test.name, test.parent_suite.name)

    @classmethod
    def from_uid(cls, uid):
        args = uid.split(cls.sep)
        del args[cls.type_idx]
        return cls._uid_to_class(uid)(*args)

    def __str__(self):
        common_opts = {
            self.path_idx: self.path,
            self.type_idx: self.__class__.__name__
        }
        return self.sep.join(itertools.chain(
            [common_opts[0], common_opts[1]],
            self.attributes))

    def __hash__(self):
        return hash(str(self))

    def __eq__(self, other):
        return type(self) == type(other) and str(self) == str(other)


class TestUID(UID):
    def __init__(self, filename, test_name, suite_name):
        UID.__init__(self, filename, suite_name, test_name)

    @property
    def test(self):
        return self.attributes[1]

    @property
    def suite(self):
        return self.attributes[0]


class SuiteUID(UID):
    def __init__(self, filename, suite_name):
        UID.__init__(self, filename, suite_name)

    @property
    def suite(self):
        return self.attributes[0]