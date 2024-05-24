
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
        + "stream.hw.m5 67108864;",
        "numastat; m5 --addr=0x10010000 exit;",
    ],
}

stream_remote_memory_address_ranges = [
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

###################################################################################

npb_benchmarks = ["bt", "cg", "ep", "ft", "is", "lu", "mg", "ua", "sp"]

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