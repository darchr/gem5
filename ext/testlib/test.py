from abc import ABCMeta, abstractmethod
from os import getcwd

from config import constants
from suite import TestList
from unittest import FunctionTestCase as _Ftc
from functools import partial

from uid import uid

def _steal_unittest_assertions(module):
    '''
    Attach all the unittest.TestCase assertion helpers to the given modules
    namespace.
    '''
    # Since unittest assertion helpers all need an instance to work, we
    # need to do some partial application with a wrapper function.
    fake_testcase = _Ftc(None)
    for item in dir(_Ftc):
        if item.startswith('assert'):
            module[item] = partial(getattr(_Ftc, item), fake_testcase)

# Export the unittest assertion helpers from this module.
_steal_unittest_assertions(globals())


class TestingException(Exception):
    '''Common ancestor for manual Testing Exceptions.'''
class TestFailException(TestingException):
    '''Signals that a test has failed.'''
class TestSkipException(TestingException):
    '''Signals that a test has been skipped.'''

def fail(message):
    '''Cause the current test to fail with the given message.'''
    raise TestFailException(message)

def skip(message):
    '''Cause the current test to skip with the given message.'''
    raise TestSkipException(message)

class TestCase(object):
    '''
    Abstract Base Class for test cases. All that's missing is
    a :meth:`__call__` implementation.

    Represents a single major item of assertion. (Yes that's vague.) Some
    examples are: Asserting that gem5 actually started, asserting that output
    from gem5 standard out matched a gold standard diff file. (See
    :mod:`whimsy.gem5.verifier` for some concrete examples.)

    .. warning:: Although this class is abstract its ``__new__`` method must be
        called by subclasses in order for them to be discovered by the
        :class:`whimsy.loader.TestLoader`.
    '''
    def __init__(self, name,
                 tags = {typ: set() for typ in constants.supported_tags},
                 fixtures = None, path = None):
        '''
        This must be called in subclasses for tests to be recognized by the
        test loader.

        Internally fixtures are stored as a dictionary with their name as the
        key. If multiple fixtures with the same name are provided, earlier
        onces will be overriden. Fixtures provided by containing suites will
        also be available. Fixtures with the same name in this testcase as the
        containing suite will overwrite suite level fixtures with our own.

        :param fixtures: An iterable container of :class:`Fixture` objects
        which are required by this test.

        :param tags: Iterable containg tags that this testcase will have (in
        addition to those in the containing suite).
        '''
        if fixtures is None:
            fixtures = {}
        elif not isinstance(fixtures, dict):
            fixtures = {fixture.name: fixture for fixture in fixtures}
        self.fixtures = fixtures

        self.tags = tags

        self._name = name
        if path is None:
            path = getcwd()
        self._path = path

    def match_tags(self, tags):
        """ True if the supplied tags match our tags.
            Tag matching means that we have all of the tags of the incoming
            tag check for each of the tag types.
        """
        for typ,vals in tags.iteritems():
            if not self.tags[typ] & vals:
                return False
        return True

    @property
    def uid(self):
        return uid(self, TestCase.__name__)
    @property
    def path(self):
        return self._path
    @property
    def name(self):
        return self._name
    @abstractmethod
    def __call__(self, fixtures):
        '''
        Run the test, recieving a dictionary of fixture.name->fixture.

        :code:`fixtures` will contain all of this TestCase objects fixtures in
        addition to all the containing suite's fixtures we have not overriden
        locally.
        '''
        pass

    if __debug__:
        # This is a method that will be created by the test loader in order to
        # manually remove a test.
        __no_collect__ = NotImplemented

class TestFunction(TestCase):
    '''
    A concrete implementation of the abc TestCase. Uses a function as
    a test.
    '''
    def __init__(self, test, name=None, *args, **kwargs):
        if name is None:
            # If not given a name, take the name of the function.
            name = test.__name__
        super(TestFunction, self).__init__(name, *args, **kwargs)
        self._test_function = test

    def __call__(self, fixtures):
        '''
        Override TestCase definition of __call__
        '''
        self._test_function(fixtures)

def testfunction(function=None, name=None, tag=None, tags=None, fixtures=None):
    '''
    A decorator used to wrap a function as a TestFunction.
    '''
    # If tag was given, then the test will be marked with that single tag.
    # elif tags was given, then the test will be marked with all those tags.
    if tag is not None:
        tags = set((tag,))
    elif tags is not None:
        tags = set(tags)

    def testfunctiondecorator(function):
        '''Decorator used to mark a function as a test case.'''
        TestFunction(function, name=name, tags=tags, fixtures=fixtures)
        return function
    if function is not None:
        return testfunctiondecorator(function)
    else:
        return testfunctiondecorator
