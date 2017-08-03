'''
Contains two implementations of the classic unix tee command to tee output
from this python process (and all of its subprocesses) into a file and keep
directing output to stdout and stderr.
'''
import contextlib
import os
import subprocess
import sys
import time
from functools import partial
from multiprocessing import Process, Pipe
from Queue import Queue
from threading import Thread

def _unbuffer():
    if sys.stdout is sys.__stdout__:
        sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

def _python_tee(filepath, infd, exit_signal_pipe, append=False):
    '''
    Python only implementation of tee used if unable to locate the system tee
    program. Aims to be cross-platform.
    '''
    flag = 'w'
    if append:
        flag += '+'

    redir = open(filepath, flag)
    inpipe = os.fdopen(infd, 'r', 0)

    queue = Queue(1)

    def finish_up(queue):
        # We have recieved instructions to terminate from our parent
        # process. Every .1 second check if we have finished otherwise
        # continue blocking read.
        if queue.full():
            redir.close()
            return
        queue.put(True)
        time.sleep(.1)
        finish_up(queue)

    def wait_exit(queue):
        # Wait until we recieve the signal to exit.
        exit_signal_pipe.recv()
        finish_up(queue)

    def block_read(queue):
        for string in iter(partial(inpipe.read, 1), b''):
            redir.write(string)
            sys.stdout.write(string)
            if not queue.empty():
                queue.get()

    read_thread = Thread(target=block_read, args=(queue,))
    read_thread.setDaemon(True)
    read_thread.start()
    wait_exit(queue)


def python_tee(filepath, append=False, stdout=True, stderr=True):
    '''
    Python implementation of tee.

    Spawns a multiprocessing python process, a full flegged python process to
    avoid the GIL problem since the subprocess will likely spend a lot of time
    spinning while reading single char bytes.
    '''
    if stdout:
        original_stdout = os.dup(sys.stdout.fileno())
        # Unbuffer normal stdout
        _unbuffer()
    if stderr:
        original_stderr= os.dup(sys.stderr.fileno())

    (r, w) = os.pipe()
    (exit_pipe_read, exit_pipe_write) = Pipe()

    py_tee = Process(
            target=_python_tee,
            args=(filepath, r, exit_pipe_read, append))

    py_tee.start()
    if stdout:
        os.dup2(w, sys.stdout.fileno())
    if stderr:
        os.dup2(w, sys.stderr.fileno())

    # Let the body run
    yield

    # Close the write side of the pipe and wait for python tee to finish
    # reading pipe.
    os.close(w)
    # Tell the subprocess that it's time to close now.
    exit_pipe_write.send(True)
    # Wait for the subprocess to close gracefully and flush it's pipe.
    py_tee.join()
    os.close(r)

    if stdout:
        os.dup2(original_stdout, sys.stdout.fileno())
        os.close(original_stdout)
    if stderr:
        os.dup2(original_stderr, sys.stdout.fileno())
        os.close(original_stderr)

def system_tee(filepath, append=False, stdout=True, stderr=True):
    '''
    An implementation of tee using the system available program tee as
    a subprocess.
    '''

    def _unbuffer():
        if sys.stdout is sys.__stdout__:
            sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', 0)

    # Copy a reference to stdout and stderr
    if stdout:
        original_stdout = os.dup(sys.stdout.fileno())

    if stderr:
        original_stderr = os.dup(sys.stderr.fileno())

    # Unbuffer normal stdout
    _unbuffer()

    # Build the arguments list for tee
    args = ['tee']
    if append:
        args.append('-a')
    args.append(filepath)

    # Start up tee
    tee_process = subprocess.Popen(args, stdin=subprocess.PIPE)
    if stdout:
        os.dup2(tee_process.stdin.fileno(), sys.stdout.fileno())
    if stderr:
        os.dup2(tee_process.stdin.fileno(), sys.stderr.fileno())

    yield

    # Restore original stdout and stderr
    if stdout:
        os.dup2(original_stdout, sys.stdout.fileno())
        # Close backup stdout and stderr
        os.close(original_stdout)
    if stderr:
        os.dup2(original_stderr, sys.stderr.fileno())
        os.close(original_stderr)

    # Close tee's file descriptors, wait for it to finish, and reap it
    tee_process.communicate()

@contextlib.contextmanager
def tee(*args, **kwargs):
    '''
    A context manager for the tee command. Tries to default to the tee program
    for the system for performance reasons, but if it is unavailable, will
    use a pure python implementaion.

    An example of usage:

    >>> with tee('stdout', stdout=True, stderr=False):
    >>>     print ('This is going to both the file and stdout')

    '''
    # Test if there is a system tee program we'll use that if there is.
    if any(os.access(os.path.join(path, 'tee'), os.X_OK) for path in
            os.environ["PATH"].split(os.pathsep)):
        return system_tee(*args, **kwargs)
    else:
        # Otherwise default to the slower python version.
        return python_tee(*args, **kwargs)

if __name__ == '__main__':
    with tee('test.out'):
        print 'ayyy'
        subprocess.call('echo waddup', shell=True)
        print >> sys.stderr, 'uh oh'
