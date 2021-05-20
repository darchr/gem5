from components_library.processors.cpu_types import CPUTypes
from components_library.processors.py_generator_core import PyGeneratorCore


from .py_generator_core import PyGeneratorCore


class LinearGeneratorCore(PyGeneratorCore):
    def __init__(self, rate: int):
        super(LinearGeneratorCore, self).__init__(CPUTypes.LINGEN)
        self.set_traffic(rate)

    def create_traffic(self, rate):
        duration = 100000000
        block_size = 64
        min_addr = 0
        max_addr = 16384
        rd_perc = 100
        data_limit = 0
        period = int(float(block_size / rate) * float(1e12 / 1073741824))
        min_period = period
        max_period = period
        yield self.main_generator.createLinear(
            duration,
            min_addr,
            max_addr,
            block_size,
            min_period,
            max_period,
            rd_perc,
            data_limit,
        )
        yield self.main_generator.createExit(0)

    def set_traffic(self, rate):
        self._main_traffic = self.create_traffic(rate)
