# Copyright (c) 2017 Mark D. Hill and David A. Wood
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
#
# Authors: Sean Wilson

'''
Contains the :class:`Loader` which is responsible for discovering and loading
tests.

Loading typically follows the following stages.

1. Recurse down a given directory looking for tests which match a given regex.

The default regex used will match any python file (ending in .py) that has
a name starting or ending in test(s). If there are any additional components
of the name they must be connected with '-' or '_'. Lastly, file names that
begin with '.' will be ignored.

The following names would match:

- `tests.py`
- `test.py`
- `test-this.py`
- `tests-that.py`
- `these-test.py`

These would not match:

- `.test.py`    - 'hidden' files are ignored.
- `test`        - Must end in '.py'
- `test-.py`    - Needs a character after the hypen.
- `testthis.py` - Needs a hypen or underscore to separate 'test' and 'this'


2. With all files discovered execute each file gathering its test items we
   care about collecting. (`TestCase`, `TestSuite` and `Fixture` objects.)

In order to collect these objects the :class:`TestLoader` must find a way to
capture objects as they are instantiated. To do this, before calling
:func:`execfile` on the file about to be loaded, :class:`TestLoader` will
monkey patch the :func:`__new__` method of test item classes. The monkey
patched `__new__` will insert the new instance of the test method as it is
created into an :class:`OrderedSet` saved in the :class:`TestLoader`.

In addition to adding a callback to the :func:`__new__` function of these test
objects the loader adds a :func:`__no_collect__` method. This method can be
used by test writers to prevent a test item from being collected. This
functionallity is exposed to test writers with the :func:`no_collect` function
in this module. Users might create an item that fits almost all their needs
butbut wish to make copy of it and modify a single attribute without the
original version being collected.

>>> from __future__ import print_function
>>> from testlib import *
>>> from copy import copy
>>> hello_func = lambda : print('Hello')
>>> test = TestFunction(hello_func)
>>> # Create a copy, don't collect the original.
>>> test_copy = copy(no_collect(test))
>>> test_copy.name = 'hello-test'

As a final note, :class:`TestCase` instances which are not put into
a :class:`TestSuite` by the test writer will be placed into
a :class:`TestSuite` named after the module.

.. seealso:: :func:`load_file`
'''
import os
import re
import sys
import traceback
from types import MethodType
from functools import partial
import types

import config
from fixture import Fixture
from helper import OrderedSet, absdirpath, OrderedDict
from logger import log
from suite import TestSuite, SuiteList, TestList
from test import TestCase
from uid import path_from_uid

# Will match filenames that either begin or end with 'test' or tests and use
# - or _ to separate additional name components.
default_filepath_regex = \
        re.compile(r'(((.+[-_])?tests?)|(tests?([-_].+)?))\.py$')

def default_filepath_filter(filepath):
    '''The default filter applied to filepaths to marks as test sources.'''
    filepath = os.path.basename(filepath)
    if default_filepath_regex.match(filepath):
        # Make sure doesn't start with .
        return not filepath.startswith('.')
    return False

def path_as_modulename(filepath):
    '''Return the given filepath as a module name.'''
    # Remove the file extention (.py)
    return os.path.splitext(os.path.basename(filepath))[0]

def path_as_testsuite(filepath, *args, **kwargs):
    '''
    Return the given filepath as a testsuite.

    The testsuite will be named after the containing directory of the file.
    '''
    return TestSuite(os.path.split(absdirpath(filepath))[-1],
        *args, **kwargs)

def no_collect(test_item):
    '''
    Prevent the collection of the test item by the :class:`TestLoader`.

    :returns: the item given so this can be easily used in comprehensions.
    '''
    if hasattr(test_item, '__no_collect__'):
        test_item.__no_collect__()
    return test_item

if __debug__:
    def _assert_files_in_same_dir(files):
        if files:
            directory = os.path.dirname(files[0])
            for f in files:
                assert os.path.dirname(f) == directory

class _MethodWrapper(object):
    '''
    Class used to wrap and unwrap a method of a class with an additional
    callback function.
    '''
    _sentinal = object()
    def __init__(self, cls, method_name, callback, clsmethod=False):
        '''
        :param cls: Class to wrap.
        :param method_name: Name of method to wrap

        :param callback: Function to call in addition to the original cls
        method.
        '''
        self._callback = callback
        self._cls = cls
        self._method_name = method_name
        self._old_method = self._sentinal
        if __debug__:
            self._replaced_method = None
        self.clsmethod = clsmethod

    def wrap(self):
        '''
        Wrap the cls method_name with a method that will call both the class'
        original method if there was one and our callback.
        '''
        old_method = getattr(self._cls, self._method_name, self._sentinal)
        sentinal = self._sentinal
        callback = self._callback

        def combined_method(*args, **kwargs):
            if old_method not in (sentinal, NotImplemented):
                return callback(__retval__=old_method(*args, **kwargs),
                                *args, **kwargs)
            else:
                return callback(__retval__=None, *args, **kwargs)

        if self.clsmethod:
            combined_method = classmethod(combined_method)

        replacement = combined_method
        self._old_method = old_method
        setattr(self._cls, self._method_name, replacement)
        if __debug__:
            # NOTE: We do this after we have set since in python2 functions and
            # methods are different types.
            self._replaced_method = getattr(self._cls, self._method_name)

    def unwrap(self):
        '''
        Return the wrapped class method to the state it was in when we
        called :func:`wrap` on it.
        '''
        if self._old_method != self._sentinal:
            assert getattr(self._cls, self._method_name) \
                    == self._replaced_method, \
                    "%s's %s has changed, we can not restore it." \
                    % (self._cls, self._method_name)

            setattr(self._cls, self._method_name, self._old_method)
        else:
            delattr(self._cls, self._method_name)


class TestLoader(object):
    '''
    Base class for discovering tests.

    To simply discover and load all tests using the default filter create an
    instance and `load_root`.

    >>> import os
    >>> tl = TestLoader()
    >>> tl.load_root(os.getcwd())

    .. note:: If tests are not manually placed in a TestSuite, they will
        automatically be placed into one for the module.
    '''
    def __init__(self, filepath_filter=default_filepath_filter, quiet=False):

        self._suites = SuiteList()
        self.filepath_filter = filepath_filter
        self.quiet = quiet

        if __debug__:
            # Used to check if we have ran load_file to make sure we have
            # actually tried to load a file into our suite.
            self._loaded_a_file = False

        # List of all the fixtures we have collected.
        self._fixtures = []

        # Tests and suites are identified by the test loader in a format that
        # enforces uniqueness - both so users and the test system can identify
        # unique tests.
        # Reverse index: testitem->uid
        self._test_index  = OrderedDict()
        self._suite_index = OrderedDict()

        # Reverse index: uid->testitem
        self._test_rindex = OrderedDict()
        self._suite_rindex = OrderedDict()

        # Member variables used to keep track of instances of suites, cases,
        # and fixtures when execfile'ing.
        # They are temporary and will be reset for each file we load.
        self._wrapped_classes = {}
        self._collected_test_items = OrderedSet()
        self._collected_fixtures = OrderedSet()


    @property
    def suites(self):
        assert self._loaded_a_file
        return self._suites

    @property
    def tests(self):
        assert self._loaded_a_file
        return tuple(self._test_rindex)

    @property
    def fixtures(self):
        assert self._loaded_a_file
        return tuple(self._fixtures)

    def get_uid(self, uid):
        '''Return the test item with the given uid.'''
        return self._test_index.get(uid, self._suite_index.get(uid, None))

    def discover_files(self, root):
        '''
        Recurse down from the given root directory returning a list of
        directories which contain a list of files matching
        `self.filepath_filter`.
        '''
        # Will probably want to order this traversal.
        for root, dirnames, filenames in os.walk(root):
            dirnames.sort()
            if filenames:
                filenames.sort()
                filepaths = [os.path.join(root, filename) \
                             for filename in filenames]
                filepaths = filter(self.filepath_filter, filepaths)
                if filepaths:
                    yield filepaths

    def load_root(self, root):
        '''
        Load files from the given root directory which match
        `self.filepath_filter`.
        '''
        if __debug__:
            self._loaded_a_file = True

        for directory in self.discover_files(root):
            if directory:
                if __debug__:
                    _assert_files_in_same_dir(directory)
                for f in directory:
                    self.load_file(f)

    def load_dir(self, directory):
        for dir_ in self.discover_files(directory):
            if __debug__:
                _assert_files_in_same_dir(dir_)
            for f in dir_:
                self.load_file(f)
            break

    @staticmethod
    def load_uid(uid):
        '''
        Attempt to load the given UID.

        :param uid: The uid to attempt to load a test item for.

        :returns: A :class:`whimsy.suite.TestSuite` or
            :class:`whimsy.test.TestCase` if the given UID can be found, else
            None.
        '''
        # Create a dummy TestLoader instance.
        loader = TestLoader(quiet=True)
        # Parse the path back out of the uid.
        path = path_from_uid(uid)
        # Modify the path based on the directory we are actually in.
        loader.load_dir(path)
        return loader.get_uid(uid)

    def load_file(self, path, collection=None):
        '''
        Load the given path for tests collecting all created instances of
        :class:`TestSuite`, :class:`TestCase`, and :code:`Fixture` objects.

        TestSuites are placed into self._suites (a private SuiteList).
        TestCases which are not explicitly stored within a TestSuite are
        placed into a TestSuite which is generated from the filepath name.

        There are couple of things that might be unexpected to the user in the
        loading namespace.

        First, the current working directory will change to the directory path
        of the file being loaded.

        Second, in order to collect tests we wrap __new__ calls of collected
        objects before calling :code:`execfile`.  If a user wishes to prevent
        an instantiated object from being collected (possibly to create a copy
        with a modified attribute) they should use :func:`no_collect` to do
        so. (Internally a :code:`__no_collect__` method is added to objects we
        plan to collect. This method will then remove the object from those
        collected from the file.)

        .. note:: Automatically drop_caches

        .. warning:: There isn't a way to prevent reloading of test modules
            that are imported by other test modules. It's up to users to never
            import a test module from a test module, otherwise those tests
            enumerated during the importer module's load.
        '''
        path = os.path.abspath(path)
        if __debug__:
            self._loaded_a_file = True

        if collection is None:
            collection = self._suites

        # Create a custom dictionary for the loaded module.
        newdict = {
            '__builtins__':__builtins__,
            '__name__': path_as_modulename(path),
            '__file__': path,
            '__directory__': os.path.dirname(path),
        }

        self._wrap_collection(TestSuite, self._collected_test_items, path)
        self._wrap_collection(TestCase, self._collected_test_items, path)
        self._wrap_collection(Fixture, self._collected_fixtures, path)

        # Add the file's containing directory to the system path. So it can do
        # relative imports naturally.
        sys.path.insert(0, os.path.dirname(path))
        cwd = os.getcwd()
        os.chdir(os.path.dirname(path))

        def cleanup():
            # Undo all the wrapping of class methods used to gather test
            # items. (Placed here to compare against the above stuff it
            # undoes.)
            self._unwrap_collection(TestSuite)
            self._unwrap_collection(TestCase)
            self._unwrap_collection(Fixture)
            self._collected_fixtures = OrderedSet()
            self._collected_test_items = OrderedSet()
            del sys.path[0]
            os.chdir(cwd)

        try:
            execfile(path, newdict, newdict)
        except Exception as e:
            if not config.config.quiet:
                log.info('Tried to load tests from %s but failed with an'
                         ' exception.' % path)
            log.debug(traceback.format_exc())
            cleanup()
            return

        # Separate the instances so we can manipulate them more easily.
        # We also keep them together so we maintain ordering.
        test_items = self._collected_test_items
        testcases = OrderedSet()
        testsuites = []

        for item in test_items:
            if isinstance(item, TestCase):
                testcases.add(item)
            elif isinstance(item, TestSuite):
                testsuites.append(item)

        self._index(*self._collected_test_items)
        self._fixtures.extend(self._collected_fixtures)

        if testcases:
            if not config.config.quiet:
                log.display('Discovered %d tests and %d testsuites in %s'
                            '' % (len(testcases), len(testsuites), path))

            # Remove all tests already contained in a TestSuite.
            if testsuites:
                testcases = OrderedSet(testcases)
                for testsuite in testsuites:
                    testcases -= OrderedSet(testsuite.testcases)

            # If there are any remaining tests, create a TestSuite for the
            # module and place those tests into it.
            if testcases:
                module_testsuite = path_as_testsuite(path)
                testsuites.append(module_testsuite)
                for test_item in test_items:
                    if isinstance(test_item, TestCase):
                        module_testsuite.append(test_item)

                # Add our new testsuite into the index as well
                self._index(module_testsuite)


            collection.extend(testsuites)

        elif testsuites:
            if not config.config.quiet:
                log.warn('No tests discovered in %s, but found %d '
                         ' TestSuites' % (path, len(testsuites)))
        else:
            if not config.config.quiet:
                log.info('No tests discovered in %s' % path)

        cleanup()


    def _wrap_collection(self, cls, collector, path):
        '''
        Wrap the given cls' `__new__` method with a wrapper that will keep an
        OrderedSet of the instances. Also attach a `__no_collect__` method
        which can be used to remove the object from our collected objects with
        the exposed :func:`no_collect`.

        :param cls: Class to wrap methods of for collection.

        :param collector: The :code:`set` to add/remove collected instances
            to.

        .. warning:: If any other class monkey patches the `__new__` method as
            well, this will lead to issues. Keep `__debug__` mode enabled to
            enable checks that this never happens.
        '''
        assert cls not in self._wrapped_classes
        def instance_decollector(self, *args, **kwargs):
            collector.remove(self)
            return kwargs['__retval__']
        def instance_new(*args, **kwargs):
            _cls = kwargs['__retval__']
            retval = super(cls, _cls).__new__(type(_cls), *args, **kwargs)
            collector.add(retval)
            return retval

        wrappers = (
                _MethodWrapper(cls, '__no_collect__', instance_decollector),
                _MethodWrapper(cls, '__new__', instance_new, clsmethod=True),
        )
        for wrapper in wrappers:
            wrapper.wrap()
        self._wrapped_classes[cls] = wrappers

    def _unwrap_collection(self, cls):
        '''
        .. warning:: If any other class monkey patches the `__new__`  method
            as well, this will lead to issues. Keep `__debug__` mode enabled to
            enable checks that this never happens.
        '''
        for wrapper in self._wrapped_classes[cls]:
            wrapper.unwrap()
        del self._wrapped_classes[cls]

    def _index(self, *testitems):
        '''Adds collected testitems into our datastructures for querying.'''

        def add_to_index(item, index, rindex):
            if item in index:
                raise DuplicateTestItemError(
                        "Item uid: '%s' already exists" % item.uid)
            rindex[item] = item.uid
            index[item.uid] = item

        for item in testitems:
            if isinstance(item, TestCase):
                add_to_index(item, self._test_index, self._test_rindex)
            elif isinstance(item, TestSuite):
                add_to_index(item, self._suite_index, self._suite_rindex)
            elif __debug__:
                raise AssertionError('Only can enumerate TestCase and'
                                     ' TestSuite objects')

class DuplicateTestItemError(Exception):
    pass
