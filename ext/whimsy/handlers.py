from __future__ import print_function

import multiprocessing
import os
import pickle
import Queue
import sys
import threading
import time
import traceback

from config import config
import helper
import log
import terminal
import test
import result
import state
import uid

class Timer():
    def __init__(self):
        self.restart()

    def restart(self):
        self._start = self.timestamp()
        self._stop = None

    def stop(self):
        self._stop = self.timestamp()
        return self._stop - self._start

    def runtime(self):
        return self._stop - self._start

    def active_time(self):
        return self.timestamp() - self._start 

    @staticmethod
    def timestamp():
        return time.time()


class _TestStreamManager(object):
    def __init__(self):
        self._writers = {}
    
    def open_writer(self, test_result):
        if test_result in self._writers:
            raise ValueError('Cannot have multiple writters on a single test.')
        self._writers[test_result] = _TestStreams(test_result.stdout, test_result.stderr)
    
    def get_writer(self, test_result):
        if test_result not in self._writers:
            self.open_writer(test_result)
        return self._writers[test_result]
    
    def close_writer(self, test_result):
        if test_result in self._writers:
            writer = self._writers.pop(test_result)
            writer.close()

    def close(self):
        for writer in self._writers.values():
            writer.close()
        self._writers.clear()

class _TestStreams(object):
    def __init__(self, stdout, stderr):
        helper.mkdir_p(os.path.dirname(stdout))
        helper.mkdir_p(os.path.dirname(stderr))
        self.stdout = open(stdout, 'w')
        self.stderr = open(stderr, 'w')

    def close(self):
        self.stdout.close()
        self.stderr.close()

class ResultHandler(log.Handler):
    def __init__(self, schedule, directory):
        self.directory = directory
        self.internal_results = result.InternalLibraryResults(schedule, directory)
        self.test_stream_manager = _TestStreamManager()

        self.mapping = {
            log.LibraryStatus.type_id: self.handle_library_status,

            log.SuiteResult.type_id: self.handle_suite_result,
            log.TestResult.type_id: self.handle_test_result,

            log.TestStderr.type_id: self.handle_stderr,
            log.TestStdout.type_id: self.handle_stdout,
        }

    def handle(self, record):
        self.mapping.get(record.type_id, lambda _:None)(record)

    def handle_library_status(self, record):
        if record['status'] in (state.Status.Complete, state.Status.Avoided):
            self.test_stream_manager.close()

    def handle_suite_result(self, record):
        suite_result = self.internal_results.get_suite_result(
                    record['metadata'].uid)
        suite_result.result = record['result']

    def handle_test_result(self, record):
        test_result = self._get_test_result(record)       
        test_result.result = record['result']

    def handle_stderr(self, record):
        self.test_stream_manager.get_writer(
            self._get_test_result(record)
        ).stderr.write(record['buffer'])

    def handle_stdout(self, record):
        self.test_stream_manager.get_writer(
            self._get_test_result(record)
        ).stdout.write(record['buffer'])

    def _get_test_result(self, test_record):
        return self.internal_results.get_test_result(
                    test_record['metadata'].uid, 
                    test_record['metadata'].suite_uid)

    def _save(self):
        #FIXME Hardcoded path name
        result.InternalSavedResults.save(
            self.internal_results, 
            os.path.join(self.directory, 'results.pickle'))
        result.JUnitSavedResults.save(
            self.internal_results, 
            os.path.join(self.directory, 'results.xml'))

    def close(self):
        self._save()


#TODO Change from a handler to an internal post processor so it can be used to reprint results
class SummaryHandler(log.Handler):
    color = terminal.get_termcap()
    reset = color.Normal
    colormap = {
            state.Result.Errored: color.Red,
            state.Result.Failed: color.Red,
            state.Result.Passed: color.Green,
            state.Result.Skipped: color.Cyan,
    }
    sep_fmtkey = 'separator'
    sep_fmtstr = '{%s}' % sep_fmtkey

    def __init__(self):
        self.mapping = {
            log.TestResult.type_id: self.handle_testresult,
            log.LibraryStatus.type_id: self.handle_library_status,
        }
        self._timer = Timer()
        self.results = []
    
    def handle_library_status(self, record):
        if record['status'] == state.Status.Building:
            self._timer.restart()

    def handle_testresult(self, record):
        result = record['result'].value
        if result in (state.Result.Skipped, state.Result.Failed, state.Result.Passed, state.Result.Errored):
            self.results.append(result)

    def handle(self, record):
        self.mapping.get(record.type_id, lambda _:None)(record)

    def close(self):
        print(self._display_summary())

    def _display_summary(self):
        most_severe_outcome = None
        outcome_fmt = ' {count} {outcome}'
        strings = []

        outcome_count = [0] * len(state.Result.enums)
        for result in self.results:
            outcome_count[result] += 1

        # Iterate over enums so they are in order of severity
        for outcome in state.Result.enums:
            outcome = getattr(state.Result, outcome)
            count  = outcome_count[outcome]
            if count:
                strings.append(outcome_fmt.format(count=count,
                                                  outcome=state.Result.enums[outcome]))
                most_severe_outcome = outcome
        string = ','.join(strings)
        if most_severe_outcome is None:
            string = ' No testing done'
            most_severe_outcome = state.Result.Passed
        else:
            string = ' Results:' + string + ' in {:.2} seconds '.format(
                    self._timer.active_time())
        string += ' '
        return terminal.insert_separator(
                string,
                color=self.colormap[most_severe_outcome] + self.color.Bold)

class TerminalHandler(log.Handler):
    color = terminal.get_termcap()
    verbosity_mapping = {
        log.LogLevel.Warn: color.Yellow,
        log.LogLevel.Error: color.Red,
    }
    default = color.Normal

    def __init__(self, verbosity=log.LogLevel.Info):
        self.stream = verbosity >= log.LogLevel.Trace
        self.verbosity = verbosity
        self.mapping = {            
            log.TestResult.type_id: self.handle_testresult,
            log.SuiteStatus.type_id: self.handle_suitestatus,
            log.TestStatus.type_id: self.handle_teststatus,
            log.TestStderr.type_id: self.handle_stderr,
            log.TestStdout.type_id: self.handle_stdout,
            log.TestMessage.type_id: self.handle_testmessage,
            log.LibraryMessage.type_id: self.handle_librarymessage,
        }

    def _display_outcome(self, name, outcome, reason=None):
        print(self.color.Bold
                 + SummaryHandler.colormap[outcome]
                 + name
                 + ' '
                 + state.Result.enums[outcome]
                 + SummaryHandler.reset)

        if reason is not None:
            log.test_log.info('')
            log.test_log.info('Reason:')
            log.test_log.info(reason)
            log.test_log.info(terminal.separator('-'))
    
    def handle_teststatus(self, record):
        if record['status'] == state.Status.Running:
            log.test_log.debug('Starting Test Case: %s' % record['metadata'].name)

    def handle_testresult(self, record):
        self._display_outcome(
            'Test: %s'  % record['metadata'].name, 
            record['result'].value)

    def handle_suitestatus(self, record):
        if record['status'] == state.Status.Running:
              log.test_log.debug('Starting Test Suite: %s ' % record['metadata'].name)
    
    def handle_stderr(self, record):
        if self.stream: 
            print(record.data['buffer'], file=sys.stderr, end='')
        
    def handle_stdout(self, record):
        if self.stream: 
            print(record.data['buffer'], file=sys.stdout, end='')
    
    def handle_testmessage(self, record):
        if self.stream: 
            print(self._colorize(record['message'], record['level']))

    def handle_librarymessage(self, record):
        print(self._colorize(record['message'], record['level'], record['bold']))

    def _colorize(self, message, level, bold=False):
        return '%s%s%s%s' % (
                self.color.Bold if bold else '',
                self.verbosity_mapping.get(level, ''),
                message, 
                self.default)

    def handle(self, record):
        if record.data.get('level', self.verbosity) > self.verbosity:
            return
        self.mapping.get(record.type_id, lambda _:None)(record)
    
    def set_verbosity(self, verbosity):
        self.verbosity = verbosity


class PrintHandler(log.Handler):
    def __init__(self):
        pass
    
    def handle(self, record):
        print(str(record).rstrip())
    
    def close(self):
        pass

class MultiprocessingHandlerWrapper(log.Handler):
    '''
    A class which wraps other handlers and to enable 
    logging across multiprocessing processes
    '''
    def __init__(self, *subhandlers):
        # Create thread to spin handing recipt of messages
        # Create queue to push onto
        self.queue = multiprocessing.Queue()
        self.queue.cancel_join_thread()
        self._shutdown = threading.Event()

        # subhandlers should be accessed with the _handler_lock
        self._handler_lock = threading.Lock()
        self._subhandlers = subhandlers
    
    def add_handler(self, handler):
        self._handler_lock.acquire()
        self._subhandlers = (handler, ) + self._subhandlers
        self._handler_lock.release()
    
    def _with_handlers(self, callback):
        exception = None
        self._handler_lock.acquire()
        for handler in self._subhandlers:
            # Prevent deadlock when using this handler by delaying
            # exception raise until we get a chance to unlock.
            try:
                callback(handler)
            except Exception as e:
                exception = e
                break
        self._handler_lock.release()
        
        if exception is not None:
            raise exception

    def async_process(self):
        self.thread = threading.Thread(target=self.process)
        self.thread.daemon = True
        self.thread.start()

    def process(self):
        while not self._shutdown.is_set():
            try:
                item = self.queue.get(timeout=0.1)
                self._handle(item)
            except (KeyboardInterrupt, SystemExit):
                raise
            except EOFError:
                return
            except Queue.Empty:
                continue
    
    def _drain(self):
        while True:
            try:
                item = self.queue.get(block=False)
                self._handle(item)
            except (KeyboardInterrupt, SystemExit):
                raise
            except EOFError:
                return
            except Queue.Empty:
                return
    
    def _handle(self, record):
        self._with_handlers(lambda handler: handler.handle(record))

    def handle(self, record):
        self.queue.put(record)
    
    def close(self):
        self._shutdown.set()
        if hasattr(self, 'thread'):
            self.thread.join()
        _wrap(self._drain)
        self._with_handlers(lambda handler: _wrap(handler.close))

        # NOTE Python2 has an known bug which causes IOErrors to be raised
        # if this shutdown doesn't go cleanly on both ends.
        # This sleep adds some time for the sender threads on this process to 
        # finish pickling the object and complete shutdown after the queue is closed.
        time.sleep(.1)
        self.queue.close()
        time.sleep(.1)

def _wrap(callback, *args, **kwargs):
    try:
        callback(*args, **kwargs)
    except:
        traceback.print_exc()