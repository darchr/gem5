from m5.objects.SimpleMemory import SimpleMemory
from m5.params import *
from m5.proxy import *


class CxlHardwareBuffer(SimpleMemory):
    type = "CxlHardwareBuffer"
    cxx_header = "dev/cxl_hw_buffer.hh"
    cxx_class = "gem5::CxlHardwareBuffer"

    # Inherited from SimpleMemory/AbstractMemory:
    #   range       - address range (replaces pio_addr + pio_size)
    #   latency     - base memory latency
    #   bandwidth   - memory bandwidth
    #   port        - ResponsePort for memory hierarchy connection
    #   in_addr_map - register with PhysicalMemory (default True)
    #   kvm_map     - auto-register as KVM memory slot (default True)

    # The SimpleMemory static `latency` is the on-device SRAM media latency --
    # the floor EVERY access pays (SimpleMemory charges it uniformly). Accesses
    # that actually touch backing DRAM (regular memory + MPSC overflow) pay an
    # extra surcharge of (backing_latency - latency) added by the device; the
    # buffer (SPSC/MPSC) accesses pay only this SRAM floor, never the backing.
    latency = "5ns"

    # CXL link latency, added on TOP of the media latency for EVERY access
    # (modeled by the device as a response-path delay, not in SimpleMemory).
    cxl_latency = Param.Latency("400ns", "CXL link latency per device access")

    # Posted writes: skip the CXL/backing latency on write RESPONSES so the
    # sender's uncacheable store completes fast (it need not wait on the
    # store).
    # Data + routing still happen; only the write's response timing is fast.
    # Reads still pay the full CXL latency.
    posted_writes = Param.Bool(
        True, "Post (fast-ack) uncacheable device writes"
    )

    # this is the latency used to model CXL network time + logic
    transfer_latency = Param.Latency(
        "150ns", "Latency of off-host memory transfer"
    )

    # this is used for setup, we know when to switch modes when we
    # see a file opened for each endpoing
    num_endpoints = Param.Unsigned(1, "Number of MPI endpoints/ranks")

    # helps us figure out offsets of SPSC queues
    segment_size = Param.MemorySize(
        "32MB", "Size of each endpoint's memory segment"
    )

    # size of a cell, currently static
    slot_size = Param.MemorySize("4kB", "Size of an individual message slot")

    # Physical size of the actual MPSC queue, this in real hardware
    # would need actual memory cells
    mpsc_size = Param.MemorySize(
        "1MB", "Size of the MPSC central queue region"
    )

    # Hardware Backing Store Parameters
    backing_size = Param.MemorySize(
        "1GB", "Size of the internal hardware backing store for MPSC overflow"
    )
    backing_chunk_size = Param.MemorySize(
        "32MB", "Size of the macro chunks for scatter-gather overflow tracking"
    )
    # DRAM media latency of the backing store. The device charges the surcharge
    # (backing_latency - latency) on accesses that hit backing/overflow, so the
    # backing tier ends up at cxl_latency + backing_latency total.
    backing_latency = Param.Latency(
        "50ns", "DRAM media latency of the backing/overflow memory"
    )

    # deprecated since adding checkpoint restore support
    rank_offsets = VectorParam.Addr(
        [],
        "List of internal PA offsets for initialized ranks for "
        "checkpoint restore",
    )

    # Host-MPSC mode (docs/cacheable_mpsc_plan.md phase 4): honor handshake-v2
    # registrations (0xC0020000|rank + host ring PA) and deliver those ranks'
    # messages into cacheable host DRAM via coherent functional writes instead
    # of the on-device MPSC region. Ranks that never register keep the legacy
    # device-resident delivery (per-rank mixed mode is allowed).
    host_mpsc_enable = Param.Bool(
        False, "Honor handshake-v2 host-MPSC ring registrations"
    )
    # Optional sanity check: the reserved per-NUMA-node pool ranges the run
    # script carved (a registered PA outside every range only warns). Also
    # index-aligned with `prefetchers` below: ring PA -> range index ->
    # prefetchers[index] selects the SLC to stash into.
    host_mpsc_ranges = VectorParam.AddrRange(
        [], "Reserved host DRAM pool ranges the ring PAs should fall in"
    )

    # Phase 5 stash prefetch (docs/cacheable_mpsc_plan.md §7): when enabled,
    # on each host-ring delivery the device schedules a push at
    # +prefetch_push_latency (models the pushed data crossing the CXL link)
    # that triggers the target node's SLC prefetcher to install the message
    # lines. Performance-only: the functional write is still the data
    # channel, so correctness is unaffected whether the push wins the race
    # with the flag or not.
    prefetch_enable = Param.Bool(
        False, "Push delivered host-ring lines into the node's SLC prefetcher"
    )
    prefetch_push_latency = Param.Latency(
        "150ns", "Delay before the stash push reaches the SLC prefetcher"
    )
    prefetch_bytes = Param.MemorySize(
        "4kB", "Bytes of each delivered slot to stash (from the slot base)"
    )
    # The device finds the SLC stash prefetcher serving a ring PA at push
    # time via a runtime registry (each CxlStashPrefetcher self-registers its
    # pool range). No SimObject param here -- referencing the SLC-owned
    # prefetchers from the device would form a config-hierarchy cycle.
