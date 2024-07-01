
stream_run_commands = {
    "local" : [
        'echo "starting STREAM locally!";',
        "numastat;",
        "numactl --membind=0 -- "
        + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
        + "stream.hw.m5 67108864;",
        "numastat; m5 --addr=0x10010000 exit;",
    ],

    "interleave" : [
        'echo "starting interleaved STREAM!";',
        "numastat;",
        "numactl --interleave=0,1 -- "
        + "/home/ubuntu/simple-vectorizable-benchmarks/stream/"
        + "stream.hw.m5 67108864;",
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

stream_remote_memory_address_ranges = [
    (10, 12),
    (12, 14),
    (14, 16),
    (16, 18),
    (18, 20),
    (20, 22),
    (22, 24),
    (24, 26),
    (26, 28),
    (28, 30),
    (30, 32),
    (32, 34),
    (34, 36),
    (36, 38),
    (38, 40),
    (40, 42),
    (42, 44),
    (44, 46),
    (46, 48),
    (48, 50),
    (50, 52),
    (52, 54),
    (54, 56),
    (56, 58),
    (58, 60),
    (60, 62),
    (62, 64),
    (64, 66),
    (66, 68),
    (68, 70),
    (70, 72),
    (72, 74)
]


###################################################################################

npb_benchmarks = ["bt", "cg", "ep", "ft", "is", "lu", "mg", "sp", "ua"]

npb_benchmarks_index = {
    "bt": 1,
    "cg": 2,
    "ep": 3,
    "ft": 4,
    "is": 5,
    "lu": 6,
    "mg": 7,
    "sp": 8,
    "ua": 9,
}

npb_D_remote_mem_size = {
    "bt": (10,14),
    "cg": (14,23),
    "ep": (23,24),
    "ft": (24,101),
    "is": (101,127),
    "lu": (127,128),
    "mg": (128,157),
    "sp": (157,161),
    "ua": (161,162),
}

npb_classes = ["S", "A", "B", "C", "D"]

npb_mem_size = {
    "bt.C.x": 1,
    "cg.C.x": 1,
    "ep.C.x": 1,
    "ft.C.x": 5,
    "is.C.x": 1,
    "lu.C.x": 1,
    "mg.C.x": 4,
    "sp.C.x": 1,
    "ua.C.x": 1,
    "bt.D.x": 11,
    "cg.D.x": 17,
    "ep.D.x": 1,
    "ft.D.x": 85,
    "is.D.x": 34,
    "lu.D.x": 9,
    "mg.D.x": 27,
    "sp.D.x": 12,
    "ua.D.x": 8,
}