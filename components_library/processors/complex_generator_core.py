from m5.util.convert import toLatency, toMemoryBandwidth

from components_library.processors.cpu_types import CPUTypes
from components_library.processors.py_generator_core import PyGeneratorCore


from .py_generator_core import PyGeneratorCore


class ComplexGeneratorCore(PyGeneratorCore):
    def __init__(
        self,
    ):
        super(ComplexGeneratorCore, self).__init__(CPUTypes.CMXGEN)
        self._main_traffic = []

    def add_linear(
        self,
        duration="1ms",
        rate="100GB/s",
        block_size=64,
        min_addr=0,
        max_addr=32768,
        rd_perc=100,
        data_limit=0,
    ):
        duration = int(toLatency(duration) * 1e12)
        rate = toMemoryBandwidth(rate)
        self._main_traffic.append(
            self.create_linear_traffic(
                duration,
                rate,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )
        )

    def add_random(
        self,
        duration="1ms",
        rate="100GB/s",
        block_size=64,
        min_addr=0,
        max_addr=32768,
        rd_perc=100,
        data_limit=0,
    ):
        duration = int(toLatency(duration) * 1e12)
        rate = toMemoryBandwidth(rate)
        self._main_traffic.append(
            self.create_random_traffic(
                duration,
                rate,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )
        )

    def create_linear_traffic(
        self,
        duration,
        rate,
        block_size,
        min_addr,
        max_addr,
        rd_perc,
        data_limit,
    ):
        period = int(float(block_size * 1e12) / rate)
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

    def create_random_traffic(
        self,
        duration,
        rate,
        block_size,
        min_addr,
        max_addr,
        rd_perc,
        data_limit,
    ):
        period = int(float(block_size * 1e12) / rate)
        min_period = period
        max_period = period
        yield self.main_generator.createRandom(
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

    def start_traffic(self):
        self.main_generator.start(self._main_traffic.pop(0))
