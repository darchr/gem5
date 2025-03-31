import argparse

def run_synth_saga_test():
    import sys, os
    sys.path.append(
        os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir))
    )
    from m5.debug import flags
    from m5.stats import dump, reset
    from m5.util import inform

    from gem5.components.memory import SingleChannelDDR4_2400, DualChannelDDR4_2400
    from gem5.components.processors.linear_generator import LinearGenerator
    from gem5.components.processors.random_generator import RandomGenerator
    from gem5.components.processors.strided_generator import StridedGenerator
    from gem5.simulate.exit_event import ExitEvent
    from gem5.simulate.simulator import Simulator

    from gem5.components.boards.test_board import TestBoard
    from cachehierarchies.saga.cache_hierarchy import SagaCacheHierarchy

    memory = DualChannelDDR4_2400(size="32GiB")
    generator = RandomGenerator(
        num_cores=24,
        duration="1ms",
        rate="4GB/s",
        block_size=64,
        min_addr=0x0,
        max_addr= 1024*1024*1024,
        rd_perc=100,
    )

    board = TestBoard(
        clk_freq="4GHz",
        generator=generator,
        cache_hierarchy=SagaCacheHierarchy(),
        memory=memory,
    )

    simulator = Simulator(
        board=board,
        full_system=False,
        # on_exit_event={ExitEvent.EXIT: handle_exit(generator)},
    )

    print("Starting simulation.")
    simulator.run()


def get_inputs():
    parser = argparse.ArgumentParser()

    args = parser.parse_args()

    return []


if __name__ == "__m5_main__":
    run_synth_saga_test(*get_inputs())
