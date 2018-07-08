import traceback

import fixture
import state
import test as test_mod
import log
import sandbox
from state import Status, Result

def compute_aggregate_result(iterable):
    '''        
    Status of the test suite by default is:
    * Passed if all contained tests passed
    * Errored if any contained tests errored
    * Failed if no tests errored, but one or more failed.
    * Skipped if all contained tests were skipped
    '''
    failed = []
    skipped = []
    for testitem in iterable:
        result = testitem.result

        if result.value == Result.Errored:
            return Result(result.value, result.reason)
        elif result.value == Result.Failed:
            failed.append(result.reason)
        elif result.value == result.Skipped:
            skipped.append(result.reason)
    if failed:
        return Result(Result.Failed, failed)
    elif skipped:
        return Result(Result.Skipped, skipped)
    else:
        return Result(Result.Passed)

class TestParameters(object):
    def __init__(self, test, suite):
        self.test = test
        self.suite = suite
        self.log = log.TestLogWrapper(log.test_log, test, suite)

class RunnerPattern:
    def __init__(self, loaded_testable):
        self.testable = loaded_testable
        self.builder = FixtureBuilder(self.testable.fixtures) # TODO 

    def handle_error(self, trace):
        self.testable.result = Result(Result.Errored, trace)
        self.avoid_children(trace)

    def handle_skip(self, trace):
        self.testable.result = Result(Result.Skipped, trace)
        self.avoid_children(trace)
    
    def avoid_children(self, reason):
        for testable in self.testable:
            testable.result = Result(self.testable.result.value, reason)
            testable.status = Status.Avoided

    def test(self):
        pass

    def run(self):
        avoided = False
        try:
            self.testable.status = Status.Building
            self.builder.setup(self.testable)
        except SkipException:
            self.handle_skip(traceback.format_exc())
            avoided = True
        except BrokenFixtureException:
            self.handle_error(traceback.format_exc())
            avoided = True
        else:
            self.testable.status = Status.Running
            self.test()
        finally:
            self.testable.status = Status.TearingDown
            self.builder.teardown(self.testable)

        if avoided:
            self.testable.status = Status.Avoided
        else:
            self.testable.status = Status.Complete

class TestRunner(RunnerPattern):
    def test(self):
        self.sandbox_test()

    def sandbox_test(self):
        try:
            sandbox.Sandbox(TestParameters(
                    self.testable, 
                    self.testable.parent_suite))
        except sandbox.SubprocessException:
            self.testable.result = Result(Result.Failed, traceback.format_exc())
        else:
            self.testable.result = Result(Result.Passed)

import threading
class SuiteRunner(RunnerPattern):
    def test(self):
        for test in self.testable:
            test.runner(test).run()
        self.testable.result = compute_aggregate_result(
                iter(self.testable))


class LibraryRunner(SuiteRunner):
    pass


class LibraryParallelRunner(RunnerPattern):   
    def _entrypoint(self, suite):
        suite.runner(suite).run()

    def test(self):
        threads = []
        for suite in self.testable:
            thread = threading.Thread(target=self._entrypoint, args=(suite,))
            thread.daemon = True
            thread.start()
            threads.append(thread)
        for thread in threads:
            thread.join()
        self.testable.result = compute_aggregate_result(
                iter(self.testable))


class SkipException(Exception):
    def __init__(self, fixture, testitem):
        self.fixture = fixture
        self.testitem = testitem

        self.msg = 'Fixture "%s" raised SkipException for "%s".' % (
               fixture.name, testitem.name
        ) 
        super(SkipException, self).__init__(self.msg)


class BrokenFixtureException(Exception):
    def __init__(self, fixture, testitem, trace):
        self.fixture = fixture
        self.testitem = testitem
        self.trace = trace

        self.msg = ('%s\n'
                   'Exception raised building "%s" raised SkipException for "%s".' % 
                   (trace, fixture.name, testitem.name)
        ) 
        super(BrokenFixtureException, self).__init__(self.msg)

class FixtureBuilder(object):
    def __init__(self, fixtures):
        self.fixtures = fixtures
        self.built_fixtures = []

    def setup(self, testitem):
        for fixture in self.fixtures:
            # Mark as built before, so if the build fails 
            # we still try to tear it down.
            self.built_fixtures.append(fixture)
            try:
                fixture.setup(testitem)
            except SkipException:
                raise
            except Exception as e:
                exc = traceback.format_exc()
                msg = 'Exception raised while setting up fixture for %s' % testitem.uid
                log.test_log.warn('%s\n%s' % (exc, msg))
                raise BrokenFixtureException(fixture, testitem, traceback.format_exc())
        
    def teardown(self, testitem):
        for fixture in self.built_fixtures:
            try:
                fixture.teardown(testitem)
            except Exception:
                # Log exception but keep cleaning up.
                exc = traceback.format_exc()
                msg = 'Exception raised while tearing down fixture for %s' % testitem.uid
                log.test_log.warn('%s\n%s' % (exc, msg))