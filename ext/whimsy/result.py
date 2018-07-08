import os
import pickle
import xml.sax.saxutils

from config import config
import helper
import state
import log

def _create_uid_index(iterable):
    index = {}
    for item in iterable:
        assert item.uid not in index
        index[item.uid] = item
    return index


class _CommonMetadataMixin:
    @property
    def name(self):
        return self._metadata.name
    @property
    def uid(self):
        return self._metadata.uid
    @property
    def result(self):
        return self._metadata.result
    @result.setter
    def result(self, result):
        self._metadata.result = result


class InternalTestResult(object, _CommonMetadataMixin):
    def __init__(self, obj, suite, directory):
        self._metadata = obj.metadata
        self.suite = suite
    
        self.stderr = os.path.join(
            InternalSavedResults.output_path(self.uid, suite.uid),
            'stderr'
        )
        self.stdout = os.path.join(
            InternalSavedResults.output_path(self.uid, suite.uid),
            'stdout'
        )


class InternalSuiteResult(object, _CommonMetadataMixin):
    def __init__(self, obj, directory):
        self._metadata = obj.metadata
        self.directory = directory
        self._wrap_tests(obj)
    
    def _wrap_tests(self, obj):
        self._tests = [InternalTestResult(test, self, self.directory) 
                       for test in obj]
        self._tests_index = _create_uid_index(self._tests)
    
    def get_test(self, uid):
        return self._tests_index[uid]
    
    def __iter__(self):
        return iter(self._tests)

    def get_test_result(self, uid):
        return self.get_test(uid)
    
    def aggregate_test_results(self):
        results = {}
        for test in self:
            helper.append_dictlist(results, test.result.value, test)
        return results


class InternalLibraryResults(object, _CommonMetadataMixin):
    def __init__(self, obj, directory):
        self.directory = directory
        self._metadata = obj.metadata
        self._wrap_suites(obj)   

    def __iter__(self):
        return iter(self._suites)

    def _wrap_suites(self, obj):
        self._suites = [InternalSuiteResult(suite, self.directory) 
                        for suite in obj]
        self._suites_index = _create_uid_index(self._suites)
    
    def add_suite(self, suite):
        if suite.uid in self._suites:
            raise ValueError('Cannot have duplicate suite UIDs.')
        self._suites[suite.uid] = suite
    
    def get_suite_result(self, suite_uid):
        return self._suites_index[suite_uid]

    def get_test_result(self, test_uid, suite_uid):
        return self.get_suite_result(suite_uid).get_test_result(test_uid)
    
    def aggregate_test_results(self):
        results = {}
        for suite in self._suites:
            for test in suite:
                helper.append_dictlist(results, test.result.value, test)   
        return results

class InternalSavedResults:
    @staticmethod
    def output_path(test_uid, suite_uid, base=None):
        '''
        Return the path which results for a specific test case should be
        stored.
        '''
        if base is None:
            base = config.result_path
        return os.path.join(
                base,
                suite_uid.replace(os.path.sep, '-'), 
                test_uid.replace(os.path.sep, '-'))

    @staticmethod
    def save(results, path, protocol=pickle.HIGHEST_PROTOCOL):
        with open(path, 'w') as f:
            pickle.dump(results, f, protocol)

    @staticmethod
    def load(path):
        with open(path, 'w') as f:
            return pickle.load(f)


class XMLElement(object):
    def write(self, file_):
        self.begin(file_)
        self.end(file_)

    def begin(self, file_):
        file_.write('<')
        file_.write(self.name)
        for attr in self.attributes:
            file_.write(' ')
            attr.write(file_)
        file_.write('>')

        self.body(file_)

    def body(self, file_):
        for elem in self.elements:
            file_.write('\n')
            elem.write(file_)
        file_.write('\n')

    def end(self, file_):
        file_.write('</%s>' % self.name)

class XMLAttribute(object):
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def write(self, file_):
        file_.write('%s=%s' % (self.name, 
                xml.sax.saxutils.quoteattr(self.value)))


class JUnitTestSuites(XMLElement):
    name = 'testsuites'
    result_map = {
        state.Result.Errored: 'errors',
        state.Result.Failed: 'failures',
        state.Result.Passed: 'tests'
    }

    def __init__(self, internal_results):
        results = internal_results.aggregate_test_results()
        
        self.attributes = []
        for result, tests in results.items():
            self.attributes.append(self.result_attribute(result, str(len(tests))))

        self.elements = []
        for suite in internal_results:
            self.elements.append(JUnitTestSuite(suite))

    def result_attribute(self, result, count):
        return XMLAttribute(self.result_map[result], count)

class JUnitTestSuite(JUnitTestSuites):
    name = 'testsuite'
    result_map = {
        state.Result.Errored: 'errors',
        state.Result.Failed: 'failures',
        state.Result.Passed: 'tests',
        state.Result.Skipped: 'skipped'
    }

    def __init__(self, suite_result):
        results = suite_result.aggregate_test_results()
        
        self.attributes = [
            XMLAttribute('name', suite_result.name)
        ]
        for result, tests in results.items():
            self.attributes.append(self.result_attribute(result, str(len(tests))))

        self.elements = []
        for test in suite_result:
            self.elements.append(JUnitTestCase(test))

    def result_attribute(self, result, count):
        return XMLAttribute(self.result_map[result], count)

class JUnitTestCase(XMLElement):
    name = 'testcase'
    def __init__(self, test_result):
        self.attributes = [
            XMLAttribute('name', test_result.name),
            XMLAttribute('classname', test_result.uid), # TODO JUnit expects class of test.. add as test metadata.
            XMLAttribute('status', str(test_result.result)),
        ]

        # TODO JUnit expects a message for the reason a test was 
        # skipped or errored, save this with the test metadata.
        # http://llg.cubic.org/docs/junit/
        self.elements = [
            LargeFileElement('system-err', test_result.stderr),
            LargeFileElement('system-out', test_result.stdout),
        ]

class LargeFileElement(XMLElement):
    def __init__(self, name, filename):
        self.name = name
        self.filename = filename
        self.attributes = []

    def body(self, file_):
        try:
            with open(self.filename, 'r') as f:
                for line in f:
                    file_.write(xml.sax.saxutils.escape(line))
        except IOError:
            # TODO Better error logic, this is sometimes O.K. 
            # if there was no stdout/stderr captured for the test
            #
            # TODO If that was the case, the file should still be made and it should just be empty instead of not existing.
            log.test_log.debug('Error reading from %s' % self.filename)



class JUnitSavedResults:
    @staticmethod
    def save(results, path):
        '''
        Compile the internal results into JUnit format writting it to the given file.
        '''
        results = JUnitTestSuites(results)
        with open(path, 'w') as f:
            results.write(f)
    