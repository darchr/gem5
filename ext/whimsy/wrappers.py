import itertools

import log
import uid
from state import Status, Result

class TestCaseMetadata():
    def __init__(self, name, uid, path, result, status, suite_uid):
        self.name = name
        self.uid = uid
        self.path = path
        self.status = status
        self.result = result
        self.suite_uid = suite_uid


class TestSuiteMetadata():
    def __init__(self, name, uid, tags, path, status, result):
        self.name = name
        self.uid = uid
        self.tags = tags
        self.path = path
        self.status = status
        self.result = result


class LibraryMetadata():
    def __init__(self, name, result, status):
        self.name = name
        self.result = result
        self.status = status


class LoadedTestable(object):
    def __init__(self, obj):
        self.obj = obj
        self.metadata = self._generate_metadata()

    @property
    def status(self):
        return self.metadata.status

    @status.setter
    def status(self, status):
        self.log_status(status)
        self.metadata.status = status

    @property
    def result(self):
        return self.metadata.result

    @result.setter
    def result(self, result):
        self.log_result(result)
        self.metadata.result = result

    @property
    def uid(self):
        return self.metadata.uid

    @property
    def name(self):
        return self.metadata.name
        
    @property
    def fixtures(self):
        return self.obj.fixtures

    @property
    def runner(self):
        return self.obj.runner

    # TODO Change log to provide status_update, result_update for all types.
    def log_status(self, status):
        log.test_log.status_update(self, status)

    def log_result(self, result):
        log.test_log.result_update(self, result)

    def __iter__(self):
        return iter(())


class LoadedTest(LoadedTestable):
    def __init__(self, test_obj, loaded_suite, path):
        self.parent_suite = loaded_suite
        self._path = path
        LoadedTestable.__init__(self, test_obj)

    def test(self, *args, **kwargs):
        self.obj.test(*args, **kwargs)
    
    def _generate_metadata(self):
        return TestCaseMetadata( **{
            'name':self.obj.name,
            'path': self._path,
            'uid': uid.uid(self.obj, self._path),
            'status': Status.Unscheduled,
            'result': Result(Result.NotRun),
            'suite_uid': self.parent_suite.metadata.uid
        })
        # Fill out metadata with name, uid, path
    

class LoadedSuite(LoadedTestable):
    def __init__(self, suite_obj, path):
        self._path = path
        LoadedTestable.__init__(self, suite_obj)
        self.tests = self._wrap_children(suite_obj)

    def _wrap_children(self, suite_obj):
        return [LoadedTest(test, self, self.metadata.path) for test in suite_obj]
    
    def _generate_metadata(self):
        return TestSuiteMetadata( **{
            'name': self.obj.name,
            'tags':self.obj.tags,
            'path': self._path, # TODO Loader supply info?
            'uid': uid.uid(self.obj, self._path), # TODO Requires path
            'status': Status.Unscheduled,
            'result': Result(Result.NotRun)
        })
    
    def __iter__(self):
        return iter(self.tests)
    
    @property
    def tags(self):
        return self.metadata.tags


class LoadedLibrary(LoadedTestable):
    def __init__(self, suites, global_fixtures):
        LoadedTestable.__init__(self, suites)
        self.global_fixtures = global_fixtures

    def _generate_metadata(self):
        return LibraryMetadata( **{
            'name': 'Test Library',
            'status': Status.Unscheduled,
            'result': Result(Result.NotRun)
        })
    
    def __iter__(self):
        return iter(self.obj)

    def all_fixtures(self):
        return itertools.chain(
            self.global_fixtures,
            *(self.suite_fixtures(suite) for suite in self.obj)
        )

    def suite_fixtures(self, suite):
        return itertools.chain(*(test.fixtures for test in suite))
    
    @property
    def fixtures(self):
        return self.global_fixtures

    @property
    def suites(self):
        return self.obj
    
    @suites.setter
    def suites(self, suites):
        self.obj = suites