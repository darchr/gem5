import traceback

import helper
import log

global_fixtures = []

class Fixture(object):
    '''
    Base Class for a test Fixture.

    Fixtures are items which possibly require setup and/or tearing down after
    a TestCase, TestSuite, or the Library has completed.

    Fixtures are the prefered method of carrying incremental results or
    variables between TestCases in TestSuites. (Rather than using globals.)
    Using fixtures rather than globals ensures that state will be maintained
    when executing tests in parallel.

    .. note:: In order for Fixtures to be enumerated by the test system this
        class' :code:`__new__` method must be called.
    '''
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
    '''
    Store the given fixture as a global fixture. Its setup() method 
    will be called before the first test is executed.
    '''
    global_fixtures.append(fixture)
    return fixture
