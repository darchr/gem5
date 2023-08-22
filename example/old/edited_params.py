{
    # line number annotations based on riscvmatched_*.py code in stable branch of gem5
    # Current values in lists: index 0 is the original value from the RISCVMatchedBoard code, index 1 is a value larger than the original value,  chosen arbitrarily.
    # Objective: Find values s.t. index 0 is slightly lower, and index 1 is slightly higher than the standard/normal range of values for a particular parameter.
    # in riscvmatched_core.py. Parameters under "core" are all in one block starting at line 143 and ending at line 180
    # The version of the params file immediately after Jason and Kunal's suggestions
    "core": {
        "threadPolicy": ["SingleThreaded", "RoundRobin"],  # starts at line 143
        "fetch1LineSnapWidth": [
            0,
            4,
        ],  # DONE: Jason: not sure if fetch1LineSnapWidth = 0 and fetch LineWidth = 2 in the same simulation will work
        "fetch1LineWidth": [0, 2],  #  line width of 0 might throw an error
        "fetch1FetchLimit": [1, 3],
        "fetch1ToFetch2ForwardDelay": [1, 4],
        "fetch1ToFetch2BackwardDelay": [0, 2],
        "fetch2InputBufferSize": [
            1,
            8,
        ],  # DONE: fetch2inputbuffer could probably be bigger.
        "fetch2ToDecodeForwardDelay": [1, 5],
        "fetch2CycleInput": True,
        "decodeInputBufferSize": [2, 6],
        "decodeToExecuteForwardDelay": [1, 4],
        "decodeInputWidth": [2, 4],
        "decodeCycleInput": True,
        "executeInputWidth": [2, 4],
        "executeCycleInput": True,
        "executeIssueLimit": [
            2,
            5,
        ],  # DONE, HARDCODED: the executeMemoryIssueLImit should
        "executeMemoryIssueLimit": [
            1,
            3,
        ],  # probably be less than or equal to executeIssueLImit
        "executeCommitLimit": [2, 5],
        "executeMemoryCommitLimit": [1, 3],
        "executeInputBufferSize": [4, 8],
        "executeMaxAccessesInMemory": [1, 4],
        "executeLSQMaxStoreBufferStoresPerCycle": [2, 4],
        "executeLSQRequestsQueueSize": [1, 3],
        "executeLSQTransfersQueueSize": [2, 4],
        "executeLSQStoreBufferSize": [3, 6],
        "executeBranchDelay": [2, 5],
        "executeSetTraceTimeOnCommit": True,
        "executeSetTraceTimeOnIssue": False,
        "executeAllowEarlyMemoryIssue": True,
        "enableIdling": False,  # ends at line 180
        "BP": {  # in riscvmatched_core.py. Starts at line 97 with `class U74BP(TournamentBP)`
            # Jason: For branch predictor, you may want to change the actual predictor instead of just the parameters
            "BPChoice": [
                "Local",
                "Tournament",
            ]  # BPs are Local, BiMode, and Tournament
        },
        "FUPool": {  # latencies for functional units. In riscvmatched_core.py
            "U74IntFU": [1, 3],  # line 45
            "U74IntMulFU": [3, 9],  # line 49
            "U74IntDivFU": [6, 18],  # line 53
            "U74MemReadFU": [2, 6],  # line 66
            "U74MemWriteFU": [2, 6],  # line 71
        },
    },
    "cache": {  # in riscvmatched_cache.py
        # Let's assume l1 cache sizes are fixed (32KiB and 8-way)
        "l2_size": ["2MiB", "16MiB"],
        "l2_assoc": [16, 64],  # ends line 83
        # bus width >64 doesn't make any difference
        # L1 latency I would do 1-4
        "l1dcache-response_latency": [1, 4],  # line 113
        # The L2 latency I would do 5-20
        "l2cache-data_latency": [5, 20],  # line 120
        "iptw_caches-size": ["4KiB", "16KiB"],  # line 125
        "dptw_caches-size": ["4KiB", "16KiB"],  # line 130
        # IO cache can be fixed
    },
}
