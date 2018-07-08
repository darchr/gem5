import multiprocessing
import pdb
import os
import sys
import threading
import traceback

import log

pdb._Pdb = pdb.Pdb
class ForkedPdb(pdb._Pdb):
    '''
    A Pdb subclass that may be used from a forked multiprocessing child
    '''
    io_manager = None
    def interaction(self, *args, **kwargs):
        _stdin = sys.stdin
        self.io_manager.restore_pipes()
        try:
            sys.stdin = open('/dev/stdin')
            pdb._Pdb.interaction(self, *args, **kwargs)
        finally:
            sys.stdin = _stdin
            self.io_manager.replace_pipes()


class IoManager(object):
    def __init__(self, test, suite):
        self.test = test
        self.suite = suite
        self.log = log.test_log
        self._init_pipes()
    
    def _init_pipes(self):
        self.stdout_rp, self.stdout_wp = os.pipe()
        self.stderr_rp, self.stderr_wp = os.pipe()

    def close_parent_pipes(self):
        os.close(self.stdout_wp)
        os.close(self.stderr_wp)

    def setup(self):
        self.replace_pipes()
        self.fixup_pdb()
    
    def fixup_pdb(self):
        ForkedPdb.io_manager = self
        pdb.Pdb = ForkedPdb

    def replace_pipes(self):
        self.old_stderr = os.dup(sys.stderr.fileno())
        self.old_stdout = os.dup(sys.stdout.fileno())

        os.dup2(self.stderr_wp, sys.stderr.fileno())
        sys.stderr = os.fdopen(self.stderr_wp, 'w', 0)
        os.dup2(self.stdout_wp, sys.stdout.fileno())
        sys.stdout = os.fdopen(self.stdout_wp, 'w', 0)
    
    def restore_pipes(self):
        self.stderr_wp = os.dup(sys.stderr.fileno())
        self.stdout_wp = os.dup(sys.stdout.fileno())

        os.dup2(self.old_stderr, sys.stderr.fileno())
        sys.stderr = os.fdopen(self.old_stderr, 'w', 0)
        os.dup2(self.old_stdout, sys.stdout.fileno())
        sys.stdout = os.fdopen(self.old_stdout, 'w', 0)

    def start_loggers(self):
        self.log_ouput()

    def log_ouput(self):
        def _log_output(pipe, log_callback):
            with os.fdopen(pipe, 'r') as pipe:
                # Read iteractively, don't allow input to fill the pipe.
                for line in iter(pipe.readline, ''):
                    log_callback(line)

        # Don't keep a backpointer to self in the thread.            
        log = self.log
        test = self.test
        suite = self.suite

        self.stdout_thread = threading.Thread(
                target=_log_output,
                args=(self.stdout_rp, 
                      lambda buf: log.test_stdout(test, suite, buf))
        )
        self.stderr_thread = threading.Thread(
                target=_log_output,
                args=(self.stderr_rp, 
                      lambda buf: log.test_stderr(test, suite, buf))
        )

        # Daemon + Join to not lock up main thread if something breaks 
        # but provide consistent execution if nothing goes wrong.
        self.stdout_thread.daemon = True
        self.stderr_thread.daemon = True
        self.stdout_thread.start()
        self.stderr_thread.start()
    
    def join_loggers(self):
        self.stdout_thread.join()
        self.stderr_thread.join()


class SubprocessException(Exception):
    def __init__(self, exception, trace):
        super(SubprocessException, self).__init__(trace)

class ExceptionProcess(multiprocessing.Process):
    class Status():
        def __init__(self, exitcode, exception_tuple):
            self.exitcode = exitcode
            if exception_tuple is not None:
                self.trace = exception_tuple[1]
                self.exception = exception_tuple[0]
            else:
                self.exception = None
                self.trace = None

    def __init__(self, *args, **kwargs):
        multiprocessing.Process.__init__(self, *args, **kwargs)
        self._pconn, self._cconn = multiprocessing.Pipe()
        self._exception = None

    def run(self):
        try:
            super(ExceptionProcess, self).run()
            self._cconn.send(None)
        except Exception as e:
            tb = traceback.format_exc()
            self._cconn.send((e, tb))
            raise

    @property
    def status(self):
        if self._pconn.poll():
            self._exception = self._pconn.recv()
        
        return self.Status(self.exitcode, self._exception)


class Sandbox(object):
    def __init__(self, test_parameters):

        self.params = test_parameters
        self.io_manager = IoManager(self.params.test, self.params.suite)

        self.p = ExceptionProcess(target=self.entrypoint)
        self.p.daemon = True # Daemon + Join to not lock up main thread if something breaks
        self.io_manager.start_loggers()
        self.p.start()
        self.io_manager.close_parent_pipes()
        self.p.join()
        self.io_manager.join_loggers()

        status = self.p.status
        if status.exitcode:
            raise SubprocessException(status.exception, status.trace)

    def entrypoint(self):
        self.io_manager.setup()
        self.params.test.test(self.params)