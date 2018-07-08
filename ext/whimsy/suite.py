
import config
import helper
import runner as runner_mod
import state
import test as test_mod
import uid

class TestSuite(object):
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