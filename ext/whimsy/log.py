from __future__ import print_function
import os 
import sys
import time
import terminal
import Queue
import threading
import multiprocessing

import helper
import wrappers

# next bit filched from 1.5.2's inspect.py
def currentframe():
    """Return the frame object for the caller's stack frame."""
    try:
        raise Exception
    except:
        return sys.exc_info()[2].tb_frame.f_back

if hasattr(sys, '_getframe'): currentframe = lambda: sys._getframe(3)
# done filching
#
# _srcfile is used when walking the stack to check when we've got the first
# caller stack frame.
#
_srcfile = os.path.normcase(currentframe.__code__.co_filename)

def find_caller():
    '''        
    Find the stack frame of the caller so that we can note the source
    file name, line number and function name.
    
    .. note:  
        From the cpython 2.7 source
        Copyright (C) 2001-2014 Vinay Sajip. All Rights Reserved.
    '''
    f = currentframe()
    #On some versions of IronPython, currentframe() returns None if
    #IronPython isn't run with -X:Frames.
    if f is not None:
        f = f.f_back
    rv = "(unknown file)", 0, "(unknown function)"
    while hasattr(f, "f_code"):
        co = f.f_code
        filename = os.path.normcase(co.co_filename)
        if filename == _srcfile:
            f = f.f_back
            continue
        
        rv = (co.co_filename, f.f_lineno, co.co_name)
        break
    return rv

class LogLevel():
    Fatal = 0
    Error = 1
    Warn  = 2
    Info  = 3
    Debug = 4
    Trace = 5

# Record Type - 
# Uses static rather than typeinfo so idenifiers can be used across processes/networks.
class RecordTypeCounterMetaclass(type):
    counter = 0
    def __init__(cls, name, bases, dct):
        cls.type_id = RecordTypeCounterMetaclass.counter
        RecordTypeCounterMetaclass.counter += 1

class Record(object):
    __metaclass__ = RecordTypeCounterMetaclass

    def __init__(self, **data):
        self.data = data

    def __getitem__(self, item):
        if item not in self.data:
            raise KeyError('%s not in record %s' % (item, self.__class__.__name__))
        return self.data[item]

    def __str__(self):
        return str(self.data)

class StatusRecord(Record):
    def __init__(self, obj, status):
        Record.__init__(self, metadata=obj.metadata, status=status)
class ResultRecord(Record):
    def __init__(self, obj, result):
        Record.__init__(self, metadata=obj.metadata, result=result)
#TODO Refactor this shit... Not ideal. Should just specify attributes.
class TestStatus(StatusRecord):
    pass
class SuiteStatus(StatusRecord):
    pass
class LibraryStatus(StatusRecord):
    pass
class TestResult(ResultRecord):
    pass
class SuiteResult(ResultRecord):
    pass
class LibraryResult(ResultRecord):
    pass
# Test Output Types
class TestStderr(Record):
    pass
class TestStdout(Record):
    pass
# Message (Raw String) Types
class TestMessage(Record):
    pass
class LibraryMessage(Record):
    pass


class Log(object):
    def __init__(self):
        self.handlers = []
        self._opened = False # TODO Guards to methods
        self._closed = False # TODO Guards to methods

    def finish_init(self):
        self._opened = True

    def close(self):
        self._closed = True
        for handler in self.handlers:
            handler.close()

    def log(self, record):
        if not self._opened:
            self.finish_init()

        map(lambda handler:handler.prehandle(), self.handlers)
        for handler in self.handlers:
            handler.handle(record)
            handler.posthandle()

    def add_handler(self, handler):
        self.handlers.append(handler)
    
    def close_handler(self, handler):
        handler.close()
        self.handlers.remove(handler)

class Handler(object):
    def __init__(self):
        pass
    
    def handle(self, record):
        pass
    
    def close(self):
        pass

    def prehandle(self):
        pass
    
    def posthandle(self):
        pass

class LogWrapper(object):
    _result_typemap = {
        wrappers.LoadedLibrary.__name__: LibraryResult,
        wrappers.LoadedSuite.__name__: SuiteResult,
        wrappers.LoadedTest.__name__: TestResult,
    }
    _status_typemap = {
        wrappers.LoadedLibrary.__name__: LibraryStatus,
        wrappers.LoadedSuite.__name__: SuiteStatus,
        wrappers.LoadedTest.__name__: TestStatus,
    }
    def __init__(self, log):
        self.log_obj = log
    
    def log(self, *args, **kwargs):
        self.log_obj.log(*args, **kwargs)

    # Library Logging Methods
    # TODO Replace these methods in a test/create a wrapper?
    # That way they still can log like this it's just hidden that they capture the current test.
    def message(self, message, level=LogLevel.Info, bold=False):
        self.log_obj.log(LibraryMessage(message=message, level=level, bold=bold))
    
    def error(self, message):
        self.message(message, LogLevel.Error)

    def warn(self, message):
        self.message(message, LogLevel.Warn)

    def info(self, message):
        self.message(message, LogLevel.Info)

    def debug(self, message):
        self.message(message, LogLevel.Debug)

    def trace(self, message):
        self.message(message, LogLevel.Trace)

    # Ongoing Test Logging Methods
    def status_update(self, obj, status):
        self.log_obj.log(self._status_typemap[obj.__class__.__name__](obj, status))

    def result_update(self, obj, result):
        self.log_obj.log(self._result_typemap[obj.__class__.__name__](obj, result))

    def test_message(self, test, message, level):
        self.log_obj.log(TestMessage(message=message, level=level, 
                test_uid=test.uid, suite_uid=test.parent_suite.uid))

    # NOTE If performance starts to drag on logging stdout/err 
    # replace metadata with just test and suite uid tags.
    def test_stdout(self, test, suite, buf):
        self.log_obj.log(TestStdout(buffer=buf, metadata=test.metadata))

    def test_stderr(self, test, suite, buf):
        self.log_obj.log(TestStderr(buffer=buf, metadata=test.metadata))

    def close(self):
        self.log_obj.close()

class TestLogWrapper(object):
    def __init__(self, log, test, suite):
        self.log_obj = log
        self.test = test

    def test_message(self, message, level):
        self.log_obj.test_message(test=self.test,
                message=message, level=level)

    def error(self, message):
        self.test_message(message, LogLevel.Error)

    def warn(self, message):
        self.test_message(message, LogLevel.Warn)

    def info(self, message):
        self.test_message(message, LogLevel.Info)

    def debug(self, message):
        self.test_message(message, LogLevel.Debug)

    def trace(self, message):
        self.test_message(message, LogLevel.Trace)

test_log = LogWrapper(Log())