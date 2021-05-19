def createLinearTraffic(tgen, tgen_options):
    yield tgen.createLinear(tgen_options.duration,
                            tgen_options.min_addr,
                            tgen_options.max_addr,
                            tgen_options.block_size,
                            tgen_options.min_period,
                            tgen_options.max_period,
                            tgen_options.rd_perc, 0)
    yield tgen.createExit(0)

def createRandomTraffic(tgen, tgen_options):
    yield tgen.createRandom(tgen_options.duration,
                            tgen_options.min_addr,
                            tgen_options.max_addr,
                            tgen_options.block_size,
                            tgen_options.min_period,
                            tgen_options.max_period,
                            tgen_options.rd_perc, 0)
    yield tgen.createExit(0)

