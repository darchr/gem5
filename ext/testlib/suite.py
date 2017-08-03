from os import getcwd

import util
from config import constants
from uid import uid


class TestSuite(object):
    '''
    An object containing a collection of tests, represents a completely
    self-contained testing unit. That is, if it should be able to be ran
    without relying on any other suites running.

    .. note:: In order for TestSuite objects to be enumerated by the test
        system this class' :code:`__new__` method must be called. The loader class
        will monkey patch this method in order to enumerate suites.
    '''
    def __init__(self, name, tests=tuple(),
                 tags = {typ: set() for typ in constants.supported_tags},
                 fixtures=None, fail_fast=True):
        '''
        :param name: Name of the TestSuite

        :param tags: Iterable of tags to mark this suite and all containted
            tests with.

        :param tests: An interable of test cases. If is a TestList the
            heirarchy will be maintained.

        :param fail_fast: If True indicates the first test to fail in the test
            suite will cause the execution of the test suite to halt.
        '''
        self.testlist = TestList(tests)

        # Make sure all of the tests have the same tags as the suite
        for test in self.testlist:
            test.tags = tags

        self.fail_fast = fail_fast

        self._name = name

        if fixtures is None:
            fixtures = {}
        elif not isinstance(fixtures, dict):
            fixtures = {fixture.name: fixture for fixture in fixtures}
        self.fixtures = fixtures

        self.tags = tags

        self._path = getcwd()

    match_tags = util.match_tags

    @property
    def name(self):
        return self._name
    @property
    def path(self):
        return self._path
    @property
    def uid(self):
        return uid(self, TestSuite.__name__)

    @property
    def testcases(self):
        '''
        Return a tuple containing all the test cases this TestSuite holds.
        '''
        return tuple((testcase for testcase in self))

    def iter_testlists(self):
        '''
        Iterate through tests yielding a tuple of the containing TestList and
        the TestCase.

        :returns: iterator of :code:`(testlist, testcase)` objects.
        '''
        return self.testlist.iter_keep_containers()

    def __iter__(self):
        '''Iterates over all the contained test cases in this suite.'''
        return iter(self.testlist)

    def __len__(self):
        '''Returns the number of test cases in this suite.'''
        return len(self.testlist)

    def append(self, item):
        '''Adds the given TestCase or TestList to this TestSuite'''
        self.testlist.append(item)
    def extend(self, items):
        '''Adds the given TestCases or TestLists to this TestSuite'''
        self.testlist.extend(items)

    if __debug__:
        # This is a method that will be created by the test loader in order to
        # manually remove a suite. See the TestLoader load_file description for
        # more details.
        __no_collect__ = NotImplemented


class SuiteList(object):
    '''
    Container class for test suites which provides some utility functions.
    '''
    def __init__(self, suites=[]):
        self.suites = []
        self.suites.extend(suites)

    def __iter__(self):
        return iter(self.suites)
    def __len__(self):
        return len(self.suites)
    def append(self, item):
        self.suites.append(item)
    def extend(self, items):
        self.suites.extend(items)

    def iter_fixtures(self):
        '''
        Return an iterable of all fixtures of all TestSuites and their
        contained TestCases in this collection.

        .. note:: Will return duplicates if fixtures are duplicated across
            test cases.
        '''
        for suite in self:
            for fixture in suite.fixtures.values():
                yield fixture

            for testcase in suite.testcases:
                for fixture in testcase.fixtures.values():
                    yield fixture

    def __add__(self, other):
        return SuiteList(super(list, self).__add__(self, other))

class TestList(object):
    '''
    Container class for :class:`whimsy.test.TestCase` objects which provides
    some utility functions for iteration as well as the fail_fast variable.
    A TestList can be heirarchical, in which case iteration yields tests in
    in-order traversal.
    '''
    def __init__(self, items=[], fail_fast=False):
        '''
        :param fail_fast: If any TestCase fails in this TestList, all remaing
        tests in this collection should be skipped.
        '''
        self.fail_fast = fail_fast
        self.items = []
        if isinstance(items, TestList):
            self.append(items)
        else:
            self.extend(items)

    def _iter(self, keep_containers):

        # We can't use the util iter_recursively function to do this since we
        # have defined a unusual __iter__
        for item in self.items:
            if isinstance(item, TestList):
                # Recurse into that list.
                for item_of_item in item._iter(keep_containers):
                    yield item_of_item
            else:
                if keep_containers:
                    yield (self, item)
                else:
                    yield item

    def iter_keep_containers(self):
        '''
        Iterate through tests yielding a tuple of the containing TestList and
        the TestCase.

        :returns: iterator of :code:`(testlist, testcase)` objects.
        '''
        return self._iter(True)

    def iter_tests(self):
        '''Iterate in-order over all contained tests.'''
        return self._iter(False)
    def __iter__(self):
        '''Iterate in-order over all contained tests.'''
        return self.iter_tests()
    def __len__(self):
        '''Return the number of tests contained in this tree.'''
        return len(self.items)
    def append(self, item):
        '''Add the item to the end of this TestList.'''
        self.items.append(item)
    def extend(self, items):
        '''Add the items to the end of this TestList.'''
        self.items.extend(items)
