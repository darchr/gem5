from m5.util.convert import toLatency, toMemoryBandwidth

from components_library.processors.cpu_types import CPUTypes
from components_library.processors.py_generator_core import PyGeneratorCore


from .py_generator_core import PyGeneratorCore


class RandomGeneratorCore(PyGeneratorCore):
    def __init__(
        self,
        duration="1ms",
        rate="100GB/s",
        block_size=64,
        min_addr=0,
        max_addr=32768,
        rd_perc=100,
        data_limit=0,
    ):
        super(RandomGeneratorCore, self).__init__(CPUTypes.RNDGEN)
        self._duration = int(toLatency(duration) * 1e12)
        self._rate = toMemoryBandwidth(rate)
        self._block_size = block_size
        self._min_addr = min_addr
        self._max_addr = max_addr
        self._rd_perc = rd_perc
        self._data_limit = data_limit
        self.set_traffic()

    def create_traffic(self):
        period = int(float(self._block_size * 1e12) / self._rate)
        self._min_period = period
        self._max_period = period
        yield self.main_generator.createRandom(
            self._duration,
            self._min_addr,
            self._max_addr,
            self._block_size,
            self._min_period,
            self._max_period,
            self._rd_perc,
            self._data_limit,
        )
        yield self.main_generator.createExit(0)

    def set_traffic(self):
        self._main_traffic = self.create_traffic()
