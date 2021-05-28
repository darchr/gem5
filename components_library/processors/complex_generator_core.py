from m5.objects import PyTrafficGen, Port

from m5.util.convert import toLatency, toMemoryBandwidth

from .abstract_generator_core import AbstractGeneratorCore

from ..utils.override import overrides


class ComplexGeneratorCore(AbstractGeneratorCore):
    def __init__(self):
        super(ComplexGeneratorCore, self).__init__()
        self.main_generator = PyTrafficGen()
        self._main_traffic = []

    @overrides(AbstractGeneratorCore)
    def connect_dcache(self, port: Port) -> None:
        self.main_generator.port = port

    def add_linear(
        self,
        duration: str,
        rate: str,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ):
        duration = int(toLatency(duration) * 1e12)
        rate = toMemoryBandwidth(rate)
        self._main_traffic.append(
            self._create_linear_traffic(
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
        duration: str,
        rate: str,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ):
        duration = int(toLatency(duration) * 1e12)
        rate = toMemoryBandwidth(rate)
        self._main_traffic.append(
            self._create_random_traffic(
                duration,
                rate,
                block_size,
                min_addr,
                max_addr,
                rd_perc,
                data_limit,
            )
        )

    def _create_linear_traffic(
        self,
        duration: int,
        rate: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
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

    def _create_random_traffic(
        self,
        duration: int,
        rate: int,
        block_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
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

    @overrides(AbstractGeneratorCore)
    def start_traffic(self):
        self.main_generator.start(self._main_traffic.pop(0))
