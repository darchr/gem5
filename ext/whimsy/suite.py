
import helper
import runner as runner_mod

class TestSuite(object):
    '''
    An object grouping a collection of tests. It provides tags which enable
    filtering during list and run selection. All tests held in the suite must
    have a unique name.

    ..note::
        The :func:`__new__` method enables collection of test cases, it must be called
        in order for test cases to be collected.
    
    ..note::
        To reduce test definition boilerplate, the :func:`init` method is forwarded
        all `*args` and `**kwargs`. This means derived classes can define init without 
        boilerplate super().__init__(*args, **kwargs).
    '''
    runner = runner_mod.SuiteRunner
    collector = helper.InstanceCollector()

    def __new__(klass, *args, **kwargs):
        obj = super(TestSuite, klass).__new__(klass, *args, **kwargs)
        TestSuite.collector.collect(obj)
        return obj

    def __init__(self, *args, **kwargs):
        self.name = kwargs.pop('name', getattr(self, 'name', self.__class__.__name__))
        self.fixtures = kwargs.pop('fixtures', getattr(self, 'fixtures', []))
        self.tests = kwargs.pop('tests', getattr(self, 'tests', []))
        self.tags = set(kwargs.pop('tags', []))
        self.init(*args, **kwargs)

    def init(self, *args, **kwargs):
        pass

    def __iter__(self):
        return iter(self.tests)