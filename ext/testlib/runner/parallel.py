'''
Implements a base :class:`~.WorkerPool` which will execute a serial or parallel
implementation of imap_unordered depending on the number of threads requested
on subclass `__init__`. A single instance of imap_unordered may be active at
one time. Additional executions will require that the previous imap have
finished.
'''
import abc
import multiprocessing
from multiprocessing.managers import SyncManager
import Queue
from itertools import imap

from .. import config_module
from ..logger import log

class WorkerPool(object):
    '''
    Will execute a serial or parallel implementation of imap_unordered
    depending on the number of threads requested on subclass `__init__`.
    A single instance of imap_unordered may be active at one time. Additional
    executions will require that the previous imap have finished.
    '''
    __metaclass__ = abc.ABCMeta

    def __init__(self, threads):
        self.threads = threads
        self.parallel = threads and threads > 1

    @abc.abstractproperty
    def pool(self):
        pass

    def imap_unordered(self, map_function, args):
        '''
        :param function: A module level function to supply jobs to. (Note: Must
            be exposed globaly by a module.
        :param args: An iterable containing arguments provided to members of
            the pool which the function will take.

        Effectively this function performs:

        >>> return (map_function(arg) for arg in args)
        '''
        if self.parallel:
            return self._imap_parallel(map_function, args)
        return self._imap_serial(map_function, args)

    _imap_serial = imap

    def _imap_parallel(self, map_function, args):
        return self.pool.imap_unordered(map_function, args)

class MulticoreWorkerPool(WorkerPool):
    '''
    A worker takes jobs of its queue used to initalize it and sends them to
    the process which it wraps to execute.
    '''
    def __init__(self, threads=None):
        super(MulticoreWorkerPool, self).__init__(threads)

        self._process_pool = None
        if self.parallel:
            self._process_pool = multiprocessing.Pool(threads)

    @property
    def pool(self):
        return getattr(self, '_process_pool', None)

    def _imap_parallel(self, map_function, args):
        jobs = ((map_function, arg) for arg in args)
        try:
            gen = super(MulticoreWorkerPool, self)._imap_parallel(
                    subprocess_exception_wrapper,
                    jobs
            )

            # We need to use polling since termination is broken in python2.
            # (Blocking waits do not internally poll for us.)
            #
            # NOTE: The cpython library will automatically poll faster than
            # the time we give it.
            for res in gen:
                yield res

        except KeyboardInterrupt:
            self._process_pool.terminate()
            self._process_pool.join()
            raise

class ComplexMulticorePool(WorkerPool):
    '''
    Class implements the server container for a multi-client remote and local
    multiprocessing pool. Exposes an :meth:`imap_unordered` method which will
    either execute serially if only initialized with a single thread, or will
    exececute in parallel if more given.

    Additionally, if more than one thread is given will allow remote clients to
    connect and join the pool.
    '''
    def __init__(self, threads=None):
        super(ComplexMulticorePool, self).__init__(threads)

        if self.parallel:
            credentials = config_module.config.credentials
            self.server = WorkServer(*credentials)

            # The work server starts it's own worker, so we only make n-1
            # additional workers.
            self._additional_workers = []
            for thread in range(1, threads):
                new_worker = WorkClient(*credentials, as_client=False)
                new_worker.daemon = True
                # NOTE: When this pool is deleted and the server closes down
                # this process will be killed.
                new_worker.start()
                self._additional_workers.append(new_worker)


    @property
    def pool(self):
        return self.server

    def _imap_parallel(self, function, args):
        self.server.start()
        for i in self.server.imap_unordered(function, args):
            yield i
        self.server.shutdown()

class WorkQueueServer(SyncManager):
    '''
    Implements a server which clients can connect to get work and result queues
    as well as the server's config setup.
    '''
    def __init__(self, hostname, port, passkey):

        self.work_queue = Queue.Queue()
        self.result_queue = Queue.Queue()

        self.register('get_work_queue', lambda:self.work_queue)
        self.register('get_result_queue', lambda:self.result_queue)

        # NOTE: We use a tuple with dictionaries because the SyncManager will
        # not automatically pass 'deepcopy's of objects. So the config manually
        # builds out of dictionaries rather than copying a parent __dict__.
        self.register('get_shared_config',
                lambda:(config_module.config._config,
                        config_module.config._defaults))

        super(WorkQueueServer, self).__init__((hostname, port), passkey)

class WorkQueueClient(SyncManager):
    def __init__(self, hostname, port, passkey):
        self.register('get_work_queue')
        self.register('get_result_queue')
        self.register('get_shared_config')
        super(WorkQueueClient, self).__init__((hostname, port), passkey)

class WorkServer(object):
    '''
    Implements a server object which creates work queues and result queues
    which clients can connect to to assist in work. Additionally in order to
    implement an imap_unordered function that does not block, a separate client
    is spawned with the server.
    '''
    def __init__(self, hostname, port, passkey):
        self.queue_server = WorkQueueServer(hostname, port, passkey)
        self.dest = (hostname, port, passkey)

        # Indicates that a imap function is already in progress.
        self.in_progress = False
        self.started = False

    def start(self):
        '''Start the server and the helping work_client.'''
        # Start the work queue manager.
        self.queue_server.start()

        # Spawn a subprocess to also participate in work in case we are a server
        # with no workers.
        # NOTE: We use a subprocess rather than a thread so we are not required
        # to wait for the sockets to cleanup.
        self.work_client = WorkClient(*self.dest, as_client=False)
        self.work_client.daemon = True
        self.work_client.start()

    def shutdown(self):
        '''
        Shutdown the server, the client will close asynchronosly since it
        will block in its own process waiting for the socket to close.
        '''
        self.queue_server.shutdown()
        # NOTE: It will take a decent amount of time for the work_client to close
        # its sockets so we don't bother joining. Just let the process cleanup
        # on its own.
        # self.p.terminate()
        # self.work_client.join()

    def imap_unordered(self, function, args):
        '''
        Provides functional equivalence of:

        >>> return (function(arg) for arg in args)

        .. note:: This will not block since we also spawn a `work_client` to
            assist this process.
        '''
        work_queue = self.queue_server.get_work_queue()
        result_queue = self.queue_server.get_result_queue()
        length = 0
        for arg in args:
            length += 1
            work_queue.put((function, arg))

        for _ in range(length):
            yield result_queue.get()

class WorkClient(multiprocessing.Process):
    # Signals sent through the work queue.
    def __init__(self, hostname, port, passkey, as_client=True):
        '''
        :param as_client: Set the config.command of the client process to
            'client'.
        '''
        self.dest = (hostname, port, passkey)
        self.queue_client = WorkQueueClient(hostname, port, passkey)
        self.as_client = as_client
        super(WorkClient, self).__init__()


    def run(self):
        '''
        Connects to the server and gathers its work and result queues. Also
        the majority of the config of the server process. (Allowing flags like
        `fail_fast` to be passed to us.)

        .. note:: Exectued on the client process.
        '''
        try:
            self.queue_client.connect()
        except:
            log.bold('WorkClient failed to connect to WorkServer.')
            return
        else:
            log_if_client(log.bold, 'Connected to test server.')

        disconnected_msg = ('WorkClient disconnected from WorkServer before it'
                            ' could start.')
        try:
            self._copy_config()
            work_queue = self.queue_client.get_work_queue()
            result_queue = self.queue_client.get_result_queue()
        except IOError:
            log.bold(disconnected_msg)
        except EOFError:
            log.bold(disconnected_msg)
        else:
            self.imap_task(work_queue, result_queue)
            log_if_client(log.bold, 'Work completed for test server, closing.')


    def _copy_config(self):
        '''
        Copies the config of the server, modifying it sligtly to fit the
        requirements for a work client.
        '''
        newconfig = self.queue_client.get_shared_config()
        config_module.config._init_with_dicts(*newconfig._getvalue())
        # In the copied config, don't spawn additional threads.
        config_module.config._set('test_threads', 1)
        if self.as_client:
            config_module.config._set('command', 'client')

    @staticmethod
    def imap_task(wq, rq):
        try:
            while True:
                (function, arg) = wq.get()
                rq.put(function(arg))
        except EOFError:
            return

def log_if_client(callback, *args, **kwargs):
    '''
    Execute the given callback if the client command was given at program startup.
    '''
    if config_module.config.command == 'client':
        callback(*args, **kwargs)

class SubprocessException(Exception):
    '''
    Exception represents an exception that occured in a child python
    process.
    '''

def subprocess_exception_wrapper(args):
    '''
    Wraps a python child process with a function that will enable tracebacks
    to be printed from child python processes.
    '''
    import traceback
    import sys
    (function, args) = args
    try:
        return function(args)
    except:
        raise SubprocessException("".join(traceback.format_exception(*sys.exc_info())))
