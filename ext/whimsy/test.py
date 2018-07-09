import functools

import config
import helper
import runner as runner_mod
import state
import uid

class TestCase(object):
    fixtures = tuple()
    runner = runner_mod.TestRunner
    collector = helper.InstanceCollector()

    def __new__(klass, *args, **kwargs):
        obj = super(TestCase, klass).__new__(klass, *args, **kwargs)
        TestCase.collector.collect(obj)
        return obj

    def __init__(self, *args, **kwargs):
        self.fixtures = list(self.fixtures)
        self.name = kwargs.pop('name', self.__class__.__name__)
        self.init(*args, **kwargs)

    def init(self, *args, **kwargs):
        pass

class TestFunction(TestCase):
    def __init__(self, function, name=None, *args, **kwargs):
        if name is None:
            name = test.__name__
        self.test_function = function
        TestCase.__init__(self, name=name, *args, **kwargs)

    def test(self, *args, **kwargs):
        self.test_function(*args, **kwargs)

# TODO Change the decorator to make this easier to create copy tests.
# Good way to do so might be return by reference.
def testfunction(f):
    testcase = TestCase(name=f.__name__)
    testcase.test = f
    return f

def testfunction(function=None, name=None, fixtures=None):
    '''
    A decorator used to wrap a function as a TestFunction.
    '''
    def testfunctiondecorator(function):
        '''Decorator used to mark a function as a test case.'''
        TestFunction(function, name=name, fixtures=fixtures)
        return function
    if function is not None:
        return testfunctiondecorator(function)
    else:
        return testfunctiondecorator
class TestApplication(TestCase):
    def init(self, filename):
        # TODO Save current file being loaded path in order to properly resolve the filename path.
        self.filename = filename
    
    def test(self, test_parameters):
        #TODO
        pass

def test_application(name, filename):
    return TestApplication(filename, name=name)