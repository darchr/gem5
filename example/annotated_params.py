{
    # line number annotations based on riscvmatched_*.py code in stable branch of gem5
    # Current values in lists: index 0 is the original value from the RISCVMatchedBoard code, index 1 is a value larger than the original value,  chosen arbitrarily.
    # Objective: Find values s.t. index 0 is slightly lower, and index 1 is slightly higher than the standard/normal range of values for a particular parameter.
    # in riscvmatched_core.py. Parameters under "core" are all in one block starting at line 143 and ending at line 180
    "core": {
        "threadPolicy": ["SingleThreaded", "RoundRobin"],  # starts at line 143
        "fetch1LineSnapWidth": [0, 4],
        "fetch1LineWidth": [0, 2],
        "fetch1FetchLimit": [1, 3],
        "fetch1ToFetch2ForwardDelay": [1, 4],
        "fetch1ToFetch2BackwardDelay": [0, 2],
        "fetch2InputBufferSize": [1, 3],
        "fetch2ToDecodeForwardDelay": [1, 5],
        "fetch2CycleInput": True,
        "decodeInputBufferSize": [2, 6],
        "decodeToExecuteForwardDelay": [1, 4],
        "decodeInputWidth": [2, 4],
        "decodeCycleInput": True,
        "executeInputWidth": [2, 4],
        "executeCycleInput": True,
        "executeIssueLimit": [2, 5],
        "executeMemoryIssueLimit": [1, 3],
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
            "BTBEntries": [32, 128],  # starts at line 98
            "RASSize": [12, 24],
            "localHistoryTableSize": [4096, 8192],
            "localPredictorSize": [16384, 32768],
            "globalPredictorSize": [16384, 32768],
            "choicePredictorSize": [16384, 32768],
            "localCtrBits": [4, 8],
            "globalCtrBits": [4, 8],
            "choiceCtrBits": [4, 8],
            "indirectSets": [16, 32],  # ends at line 108
        },
        "FUPool": {  # latencies for functional units. In riscvmatched_core.py
            "U74IntFU": [1, 3],  # line 45
            "U74IntMulFU": [3, 9],  # line 49
            "U74IntDivFU": [6, 18],  # line 53
            "U74MemReadFU": [2, 6],  # line 66
            "U74MemWriteFU": [2, 6],  # line 71
        },
    },
    "board": {  # in riscvmatched_board.py
        "memory": ["16GB", "128GB"],  # line 85, under `def U74Memory()`
        "clk_freq": ["1.2GHz", "4GHz"],  # line 134
        "riscvRTC-frequency": ["100MHz", "500MHz"],  # line 154
        "disk-VirtIO-pio_size": [4096, 9182],  # line 167
        "rng-VirtIO-pio_size": [4096, 9182],  # line 175
        "bridge-delay": ["10ns", "100ns"],  # line 216
        "pci_state-addr_cells": [3, 6],  # starts line 425
        "pci_state-size_cells": [2, 4],
        "pci_state-cpu_cells": [1, 2],
        "pci_state-interrupt_cells": [1, 2],  # ends line 425
    },
    "cache": {  # in riscvmatched_cache.py
        "l1i_size": ["32KiB", "512KiB"],  # starts line 78
        "l1i_assoc": [4, 16],
        "l1d_size": ["32KiB", "512KiB"],
        "l1d_assoc": [8, 32],
        "l2_size": ["2MiB", "16MiB"],
        "l2_assoc": [16, 64],  # ends line 83
        "self.membus-SystemXBar-width": [64, 128],  # line 86
        "l1dcache-response_latency": [10, 30],  # line 113
        "l2cache-data_latency": [20, 60],  # line 120
        "iptw_caches-size": ["4KiB", "16KiB"],  # line 125
        "dptw_caches-size": ["4KiB", "16KiB"],  # line 130
        "iocache-assoc": [8, 16],  # starts line 164
        "iocache-tag_latency": [50, 100],
        "iocache-data_latency": [50, 100],
        "iocache-response_latency": [50, 100],
        "iocache-mshrs": [20, 40],
        "iocache-size": ["1kB", "8kB"],
        "iocache-tgts_per_mshr": [12, 24],  # ends line 170
    },
    "processor": {  # in riscvmatched_processor.py
        "U74Processor-num_cores": [4, 8]  # line 52
    },
}
