import traceback

import log
import helper

global_fixtures = []

class TestScheduleUnknown(Exception):
    pass

class Fixture(object):
    collector = helper.InstanceCollector()

    def __new__(klass, *args, **kwargs):
        obj = super(Fixture, klass).__new__(klass, *args, **kwargs)
        Fixture.collector.collect(obj)
        return obj

    def __init__(self, *args, **kwargs):
        self.name = kwargs.pop('name', self.__class__.__name__)
        self.init(*args, **kwargs)
            
    def skip(self, testitem):
        raise SkipException(self.name, testitem.metadata)

    def schedule_finalized(self, schedule):
        pass

    def init(self, *args, **kwargs):
        pass
    
    def setup(self, testitem):
        pass
    
    def teardown(self, testitem):
        pass


def globalfixture(fixture):
    '''Store the given fixture as a global fixture. Its setup() method 
    will be called before the first test is executed.
    '''
    global_fixtures.append(fixture)
