'''
Module which contains streaming result loggers. The main goal of these loggers
is to support large amounts of results and not create large standing pools of
strings.
'''
import pickle
from xml.sax.saxutils import escape as xml_escape
from string import maketrans

import terminal
from config import config
from helper import joinpath
from test import TestCase
from suite import TestSuite
from logger import log
from util import Timer, Enum

class InvalidResultException(Exception):
    pass

Outcome = Enum(
    [
    'PASS',   # The test passed successfully.
    'XFAIL',  # The test ran and failed as expected.
    'SKIP',   # The test was skipped.
    'ERROR',  # There was an error during the setup of the test.
    'FAIL',   # The test failed to pass.
    ],
)
Outcome.__doc__ = \
    '''
    An :class:`Enum` which contains the different possible test outcomes.

    PASS    - The test passed successfully.
    XFAIL   - The test ran and failed as expected.
    SKIP    - The test was skipped.
    ERROR   - There was an error during the setup of the test.
    FAIL    - The test failed to pass.
    '''

# Add all result enums to this module's namespace.
for result in Outcome.enums:
    globals()[str(result)] = result

# Hard fail cases which mean a test contained in a failfast container should
# fail remaining test items.
Outcome.failfast = {ERROR, FAIL}

def test_results_output_path(test_case):
    '''
    Return the path which results for a specific test case should be
    stored.
    '''
    return joinpath(config.result_path, test_case.uid.replace('/','-'))


class ResultLogger(object):
    '''
    Interface which allows writing of streaming results to a file stream.
    '''

    not_supplied = object()
    '''A sentinel value indicating that the kwarg wasn't supplied.'''

    bad_item = ('Result formatter can only handle test cases'
                ' and test suites')

    def delegate_instance(self, function, instance, *args, **kwargs):
        '''
        Helper function to delegate a method for the given function based on
        the type of the given instance.

        Effectively is shorthand for:

        >>> if isinstance(instance, TestSuite):
        >>>     self._call_this_testsuite(*args, **kwargs)
        >>> elif isinstance(instance, TestCase):
        >>>     self._call_this_testsuite(*args, **kwargs)
        >>> elif __debug__:
        >>>     raise AssertionError
        '''

        for class_ in (TestSuite, TestCase):
            if isinstance(instance, class_):
                instance_name = class_.__name__.lower()
                break
        else:
            if __debug__:
                raise AssertionError(self.bad_item)

        mem_func = getattr(self, '_'.join(('', function.__name__,
                                           instance_name)),
                           lambda *args, **kwargs : None)
        return mem_func(instance, *args, **kwargs)


    def begin_testing(self):
        '''
        Signal the beginning of writing to the file stream. Indicates that
        results are about to be logged.

        This is garunteed to be called before any results are added.
        '''

    def begin(self, item):
        '''
        Signal the beginning of the given item.
        :param item: The test item which is about to begin running
        '''

    def set_outcome(self, item, outcome, **kwargs):
        '''
        Set the outcome of the given item.

        :param item: The test item which we are setting the outcome of
        :param outcome: The outcome the test item will be set to


        TestCase Only kwargs:

        :param reason: Reason for the test case outcome.
        :param fstdout_name: Name of the file stdout is available at.
        :param fstderr_name: Name of the file stdout is available at.

        :param ff_skipped: Indicates that the test was skipped due to
            a fail_fast condition.
        '''

    def end(self, item):
        '''
        Signal the end of the current item.

        :param item: The test item which is finished running
        '''

    def end_testing(self):
        '''
        Indicates that results are done being collected.

        This is guaranteed to be called after all results are added.
        '''

class ConsoleLogger(ResultLogger):
    '''
    A logger implementing the streaming ResultLogger interface. This logger is
    used to stream testing result output to a user terminal.
    '''
    color = terminal.get_termcap()
    reset = color.Normal
    colormap = {
            FAIL: color.Red,
            ERROR: color.Red,
            PASS: color.Green,
            XFAIL: color.Cyan,
            SKIP: color.Cyan,
            }
    sep_fmtkey = 'separator'
    sep_fmtstr = '{%s}' % sep_fmtkey


    def __init__(self):
        self.outcome_count = {outcome: 0 for outcome in Outcome.enums}
        self.timer = Timer()
        self._started = False

    def begin_testing(self):
        self.timer.start()
        self._started = True

    def begin(self, item):
        self.delegate_instance(self.begin, item)
    def _begin_testsuite(self, test_suite):
        log.info('Starting TestSuite: %s' % test_suite.name)
    def _begin_testcase(self, test_case):
        log.info('Starting TestCase: %s' % test_case.name)

    def set_outcome(self, item, outcome, **kwargs):
        self.delegate_instance(self.set_outcome, item, outcome, **kwargs)
    def _set_outcome_testcase(self, item, outcome, reason=None, **kwargs):
        self.outcome_count[outcome] += 1
        self._display_outcome(item.name, outcome, reason)

    def end_testing(self):
        if self._started:
            self.timer.stop()
            log.display(self._display_summary())
            self._started = False

    def _display_outcome(self, test_case_name, outcome, reason=None):
        log.bold(self.colormap[outcome]
                 + test_case_name
                 + self.reset)

        if reason is not None:
            log.info('Reason:')
            log.info(reason)
            log.info(terminal.separator('-'))

    def _display_summary(self):
        most_severe_outcome = None
        outcome_fmt = ' {count} {outcome}'
        strings = []

        # Iterate over enums so they are in order of severity
        for outcome in Outcome.enums:
            count  = self.outcome_count[outcome]
            if count:
                strings.append(outcome_fmt.format(count=count,
                                                  outcome=outcome.name))
                most_severe_outcome = outcome
        string = ','.join(strings)
        if most_severe_outcome is None:
            string = ' No testing done'
            most_severe_outcome = Outcome.PASS
        string += ' in {time:.2} seconds '.format(time=self.timer.runtime())

        return terminal.insert_separator(
                string,
                color=self.colormap[most_severe_outcome] + self.color.Bold)

    def insert_results(self, internal_results):
        '''
        Insert the given results from an :class:`whimsy.result.InternalLogger`
        into the console logger. (Display them.)
        '''
        for result in internal_results.testcases:
            self._display_outcome(result.name, result.outcome, result.reason)
            self.outcome_count[result.outcome] += 1

class TestResult(object):
    def __init__(self, item, outcome, runtime=0):
        self.name = item.name
        self.uid = item.uid
        self.outcome = outcome
        self.runtime = runtime

class TestCaseResult(TestResult):
    def __init__(self, fstdout_name=None, fstderr_name=None, reason=None,
            ff_skipped=None, **kwargs):
        super(TestCaseResult, self).__init__(**kwargs)
        self.fstdout_name = fstdout_name
        self.fstderr_name = fstderr_name
        self.reason = reason

class TestSuiteResult(TestResult):
    def __init__(self, test_case_results, **kwargs):

        super(TestSuiteResult, self).__init__(**kwargs)
        self.test_case_results = test_case_results


class InternalLogger(ResultLogger):
    '''
    An internal logger which writes streaming pickle items on completion of
    TestSuite items.

    This logger also offers some metadata methods to and can load back out
    previous results.

    .. seealso:: :func:`load` :func:`suites`
    '''
    def __init__(self, filestream):
        self.timer = Timer()
        self.filestream = filestream

        # Dictionaries mapping uid->result
        self.test_case_results = {}
        self.test_suite_results = {}
        # We keep a list to maintain ordering
        self.results = []

    def _write(self, obj):
        pickle.dump(obj, self.filestream)

    def begin_testing(self):
        self.timer.start()

    def set_outcome(self, item, **kwargs):
        result = self.delegate_instance(self.set_outcome, item, **kwargs)
        self._write(result)
        self.results.append(result)

    def _set_outcome_testcase(self, item, **kwargs):
        result = TestCaseResult(item=item, **kwargs)
        self.test_case_results[result.uid] = result
        return result

    def _set_outcome_testsuite(self, item, **kwargs):
        suite_results = []
        for tc in item:
            if tc.uid in self.test_case_results:
                suite_results.append(self.test_case_results[tc.uid])

        result = TestSuiteResult(suite_results, item=item, **kwargs)
        self.test_suite_results[result.uid] = result
        return result

    def end_testing(self):
        self.timer.stop()

    @staticmethod
    def load(filestream):
        '''Load results out of a dumped file replacing our own results.'''
        loaded_results = []
        try:
            while True:
                item = pickle.load(filestream)
                loaded_results.append(item)
        except EOFError:
            pass

        new_logger = InternalLogger(filestream)
        new_logger.results = loaded_results
        return new_logger

    @property
    def suites(self):
        '''
        Return an iterator over all the test suite results loaded or collected.
        '''
        for result in self.results:
            if isinstance(result, TestSuiteResult):
                yield result

    @property
    def testcases(self):
        for result in self.results:
            if isinstance(result, TestCaseResult):
                yield result

    def insert_results(self, internal_results):
        self.results.extend(internal_results.results)

class JUnitLogger(InternalLogger):
    '''
    Logger which uses the internal logger to collect streaming results to the
    internal_fstream, and on completion of testing writes the results out to
    a junit_fstream.

    :param junit_fstream: File stream to write junit formatted results to.

    :param internal_fstream: File stream to write internal formatted results
        to.

    .. seealso:: :class:`~InternalLogger`
    '''
    # We use an internal logger to stream the output into a format we can
    # retrieve at the end and then format it into JUnit.
    def __init__(self, junit_fstream, internal_fstream):
        super(JUnitLogger, self).__init__(internal_fstream)
        self._junit_fstream = junit_fstream

    def end_testing(self):
        '''
        Signal the end of writing to the file stream. We will write all our
        results to our junit_fstream.
        '''
        super(JUnitLogger, self).end_testing()
        JUnitFormatter(self).dump(self._junit_fstream)


class JUnitFormatter(object):
    '''
    Formats TestResults into the JUnit XML format.
    '''
    # NOTE: We manually build the tags so we can place possibly large
    # system-out system-err logs without storing them in memory.

    xml_header = '<?xml version="1.0" encoding="UTF-8"?>\n'
    passing_results = {PASS, XFAIL}
    # Testcase stuff
    testcase_opening = ('<testcase name="{name}" classname="{classname}"'
                        ' status="{status}" time="{time}">\n')
    # Indicates test skipped
    skipped_tag = '<skipped/>'
    error_tag = '<error message="{message}"></error>\n'
    fail_tag = '<failure message="{message}"></error>\n'
    system_out_opening = '<system-out>'
    system_err_opening = '<system-err>'

    # Testsuite stuff
    testsuite_opening = ('<testsuite name="{name}" tests="{numtests}"'
                         ' errors="{errors}" failures="{failures}"'
                         ' skipped="{skipped}" id={suitenum}'
                         ' time="{time}">\n'
                         )
    # Testsuites stuff
    testsuites_opening = ('<testsuites errors="{errors}" failures="{failures}"'
                          ' tests="{tests}"' # total number of sucessful tests.
                          ' time="{time}">\n')
    # Generic closing tag for any opening tag.
    generic_closing = '</{tag}>\n'


    def __init__(self, internal_results, translate_names=True):
        self.results = internal_results.results
        self.runtime = internal_results.timer.runtime()

        if translate_names:
            self.name_table = maketrans('/.', '.-')
        else:
            self.name_table = maketrans('', '')

    def dump_testcase(self, fstream, testcase):

        tag = ''
        if testcase.outcome in self.passing_results:
            outcome = PASS
            status = 'passed'
        elif testcase.outcome == SKIP:
            outcome = SKIP
            status = 'skipped'
            tag = self.skipped_tag
        elif testcase.outcome == FAIL:
            outcome = SKIP
            status = 'failed'
            tag = self.fail_tag.format(message=testcase.reason)
        elif testcase.outcome == ERROR:
            outcome = SKIP
            status = 'errored'
            tag = self.error_tag.format(message=testcase.reason)
        elif __debug__:
            raise AssertionError('Unknown test state')

        fstream.write(self.testcase_opening.format(
                name=testcase.name,
                classname=testcase.name,
                time=testcase.runtime,
                status=status))

        fstream.write(tag)

        # Write out systemout and systemerr from their containing files.
        if testcase.fstdout_name is not None:
            fstream.write(self.system_out_opening)
            with open(testcase.fstdout_name, 'r') as testout_stdout:
                for line in testout_stdout:
                    fstream.write(xml_escape(line))
            fstream.write(self.generic_closing.format(tag='system-out'))

        if testcase.fstderr_name is not None:
            fstream.write(self.system_err_opening)
            with open(testcase.fstderr_name, 'r') as testout_stderr:
                for line in testout_stderr:
                    fstream.write(xml_escape(line))
            fstream.write(self.generic_closing.format(tag='system-err'))

        fstream.write(self.generic_closing.format(tag='testcase'))

    def dump_testsuite(self, fstream, suite, idx):
        # Tally results first.
        outcome_tally = dict.fromkeys((PASS, SKIP, FAIL, ERROR), 0)
        for testcase in suite.test_case_results:
            if testcase.outcome in self.passing_results:
                outcome_tally[PASS] += 1
            else:
                outcome_tally[testcase.outcome] += 1

        fstream.write(
                self.testsuite_opening.format(
                    name=suite.name,
                    numtests=outcome_tally[PASS],
                    errors=outcome_tally[ERROR],
                    failures=outcome_tally[FAIL],
                    skipped=outcome_tally[SKIP],
                    suitenum=idx,
                    time=suite.runtime))

        for testcase in suite.test_case_results:
            self.dump_testcase(fstream, testcase)

        fstream.write(self.generic_closing.format(tag='testsuite'))

    def dump(self, dumpfile):
        idx = 0

        # First tally results.
        outcome_tally = dict.fromkeys((PASS, SKIP, FAIL, ERROR), 0)
        for item in self.results:
            if isinstance(item, TestCaseResult):
                if item.outcome in self.passing_results:
                    outcome_tally[PASS] += 1
                else:
                    outcome_tally[item.outcome] += 1

        dumpfile.write(self.testsuites_opening.format(
            tests=outcome_tally[PASS],
            errors=outcome_tally[ERROR],
            failures=outcome_tally[FAIL],
            time=self.runtime))

        for item in self.results:
            # NOTE: We assume that all tests are contained within a testsuite,
            # although as far as junit is concerned this isn't neccesary.
            if isinstance(item, TestSuiteResult):
                self.dump_testsuite(dumpfile, item, idx)
                idx += 1

        dumpfile.write(self.generic_closing.format(tag='testsuites'))
