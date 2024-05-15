
run_commands = {
    "local" : [
        'echo "starting STREAM locally!";',
        "numastat;",
        "numactl --membind=0 -- "
        + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
        + "stream.hw.m5 3145728;",
        "numastat; m5 --addr=0x10010000 exit;",
    ],

    "interleave" : [
        'echo "starting interleaved STREAM!";',
        "numastat;",
        "numactl --interleave=0,1 -- "
        + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
        + "stream.hw.m5 3145728;",
        "numastat; m5 --addr=0x10010000 exit;",
    ],

    "remote" : [
        'echo "starting STREAM remotely!";',
        "numastat;",
        "numactl --membind=1 -- "
        + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
        + "stream.hw.m5 3145728;",
        "numastat; m5 --addr=0x10010000 exit;",
    ],
}

remote_memory_address_ranges = [
    (10, 11),
    (11, 12),
    (12, 13),
    (13, 14),
    (14, 15),
    (15, 16),
    (16, 17),
    (17, 18),
    (18, 19),
    (19, 20),
    (20, 21),
    (21, 22),
    (22, 23),
    (23, 24),
    (24, 25),
    (25, 26),
    (26, 27),
    (27, 28),
    (28, 29),
    (29, 30),
    (30, 31),
    (31, 32),
    (32, 33),
    (33, 34),
    (34, 35),
    (35, 36),
    (36, 37),
    (37, 38),
    (38, 39),
    (39, 40),
    (40, 41),
    (41, 42)
]