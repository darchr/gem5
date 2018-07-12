import functools

import helper
import runner as runner_mod

class TestCase(object):
    '''
    Base class for all tests.

    ..note::
        The :func:`__new__` method enables collection of test cases, it must be called
        in order for test cases to be collected.
    
    ..note::
        To reduce test definition boilerplate, the :func:`init` method is forwarded
        all `*args` and `**kwargs`. This means derived classes can define init without 
        boilerplate super().__init__(*args, **kwargs).
    '''
    fixtures = tuple()
    # TODO, remove explicit dependency. Use the loader to set the default runner
    runner = runner_mod.TestRunner
    collector = helper.InstanceCollector()

    def __new__(cls, *args, **kwargs):
        obj = super(TestCase, cls).__new__(cls, *args, **kwargs)
        TestCase.collector.collect(obj)
        return obj

    def __init__(self, *args, **kwargs):
        self.fixtures = kwargs.pop('fixtures', getattr(self, 'fixtures', []))
        self.name = kwargs.pop('name', self.__class__.__name__)
        self.init(*args, **kwargs)

    def init(self, *args, **kwargs):
        pass

class TestFunction(TestCase):
    '''
    TestCase implementation which uses a callable object as a test.
    '''
    def init(self, function, name=None):
        if name is None:
            self.name = function.__name__
        self.test_function = function

    def test(self, *args, **kwargs):
        self.test_function(*args, **kwargs)

# TODO Change the decorator to make this easier to create copy tests.
# Good way to do so might be return by reference.
def testfunction(function=None, name=None, fixtures=tuple()):
    '''
    A decorator used to wrap a function as a TestFunction.
    '''
    def testfunctiondecorator(function):
        '''Decorator used to mark a function as a test case.'''
        kwargs = {}
        if name is not None:
            kwargs['name'] = name
        if fixtures is not None:
            kwargs['fixtures'] = fixtures
        TestFunction(function, **kwargs)
        return function
    if function is not None:
        return testfunctiondecorator(function)
    else:
        return testfunctiondecorator

# TODO
# class TestApplication(TestCase):
#     def init(self, filename):
#         # TODO Save current file being loaded path in order to properly resolve the filename path.
#         self.filename = filename
    
#     def test(self, test_parameters):
#         #TODO
#         pass

# def test_application(name, filename):
#     return TestApplication(filename, name=name)