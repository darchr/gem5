# two_tgens_two_channels_chi.py
#
# Build: a Ruby CHI system with 2 traffic generators, private L1I/L1D + L2,
# shared L3 (HNF), and 2 main-memory channels (SNFs).
#
# Requires:
#   - gem5 built with Ruby + CHI protocol (e.g., PROTOCOL=CHI)
#   - configs/ruby/chi/CHI.py and CHI_config.py (your two files)
#
# Notes:
#   - We attach each TrafficGen to the *data* Ruby sequencer so requests flow
#     through L1D -> L2 -> L3(HNF) -> memory.
#   - Set your traffic patterns via the TrafficGen 'config_file' arguments
#     (JSON state machine); placeholders are provided below.

import argparse
import math

import m5
from m5.objects import (
    AddrRange,
    DDR4_2400_16x4,
    GarnetNetwork,
    MemCtrl,
    Root,
    RubySystem,
    SimpleNetwork,
    SrcClockDomain,
    System,
    TrafficGen,
    VoltageDomain,
)
from m5.util import addToPath

# ---------------------------------------------------------------------------
# If you placed your two files under configs/ruby/chi/ (recommended),
# this import will work as-is:
addToPath("configs")
from configs.ruby.chi import CHI as chi_protocol  # your CHI.py

# CHI_config.py is loaded *by* CHI.py via --chi-config; no direct import needed.
# ---------------------------------------------------------------------------


def _build_mem_ctrls(system, mem_size_str, num_channels, cacheline_bytes):
    """
    Create N interleaved MemCtrls and return (mem_ctrls, channel_ranges).
    """
    total = AddrRange(mem_size_str)
    block_bits = int(math.log(cacheline_bytes, 2))  # e.g., 64B -> 6
    intlv_bits = int(math.log(num_channels, 2)) if num_channels > 1 else 0
    high_bit = block_bits + intlv_bits - 1 if intlv_bits > 0 else block_bits

    mem_ctrls = []
    chan_ranges = []
    for ch in range(num_channels):
        if intlv_bits == 0:
            r = total
        else:
            r = AddrRange(
                start=total.start,
                size=total.size(),
                intlvHighBit=high_bit,
                intlvBits=intlv_bits,
                intlvMatch=ch,
                intlvLowBit=block_bits,
            )
        dram = DDR4_2400_16x4(range=r)
        mem = MemCtrl(dram=dram)
        mem_ctrls.append(mem)
        chan_ranges.append(r)

    return mem_ctrls, chan_ranges


def _make_options(args):
    """
    Build a minimal 'options' namespace with the parameters CHI.py/CHI_config.py expect.
    """

    class NS:  # simple argparse-like namespace
        pass

    o = NS()
    # CPU/TG & network shape
    o.num_cpus = 2
    o.num_dirs = 2  # == memory channels/SNFs
    o.num_l3caches = args.num_l3  # HNF count
    o.network = args.network  # 'garnet' or 'simple'
    o.topology = args.topology  # 'CustomMesh', 'Crossbar', or 'Pt2Pt'

    # cache line size (Ruby expects this to match System.cache_line_size)
    o.cacheline_size = args.cacheline_size

    # CHI config file (your CHI_config.py). CHI will import this file.
    o.chi_config = args.chi_config

    # DVM disabled by default for simple TGs
    o.enable_dvm = False

    # L1/L2/L3 sizes & associativity (used by CHI_config)
    o.l1i_size = args.l1i_size
    o.l1i_assoc = args.l1i_assoc
    o.l1d_size = args.l1d_size
    o.l1d_assoc = args.l1d_assoc
    o.l2_size = args.l2_size
    o.l2_assoc = args.l2_assoc
    o.l3_size = args.l3_size
    o.l3_assoc = args.l3_assoc

    return o


def build_system(args):
    # Clock/voltage
    system = System()
    system.clk_domain = SrcClockDomain()
    system.clk_domain.clock = args.sys_clock
    system.clk_domain.voltage_domain = VoltageDomain()

    # Memory range and cacheline size (must match options.cacheline_size)
    system.cache_line_size = args.cacheline_size
    system.mem_ranges = [AddrRange(args.mem_size)]

    # Create the traffic generators (2)
    # Point these at your own JSON state-machine files for real traffic:
    # Convert to use PyTrafficGen instead
    system.tgens = [
        PyTrafficGen(),
        PyTrafficGen(),
    ]

    # system.tgens = [
    #     TrafficGen(config_file=args.tgen0),
    #     TrafficGen(config_file=args.tgen1),
    # ]

    # Create Ruby system + network
    system.ruby = RubySystem()
    if args.network == "garnet":
        system.ruby.network = GarnetNetwork(ruby_system=system.ruby)
    elif args.network == "simple":
        system.ruby.network = SimpleNetwork(ruby_system=system.ruby)
    else:
        raise ValueError(f"Unknown network: {args.network}")

    # Create 2 memory channels (controllers) with cacheline-sized interleaving
    system.mem_ctrls, chan_ranges = _build_mem_ctrls(
        system,
        args.mem_size,
        num_channels=2,
        cacheline_bytes=args.cacheline_size,
    )

    # Options for CHI protocol creation
    options = _make_options(args)

    # Build the Ruby/CHI memory system:
    # create_system(options, full_system, system, dma_ports, bootmem, ruby_system, cpus)
    cpu_seqs, mem_cntrls, topology = chi_protocol.create_system(
        options=options,
        full_system=False,
        system=system,
        dma_ports=[],  # we're using cached path, not DMA
        bootmem=None,  # no separate boot ROM
        ruby_system=system.ruby,
        cpus=system.tgens,  # pass the two traffic gens as "cpus"
    )

    # Wire each TG to its Ruby data sequencer (through L1D/L2/L3 path)
    # CPUSequencerWrapper exposes .in_ports for non-BaseCPU objects.
    for tg, seqwrap in zip(system.tgens, cpu_seqs):
        tg.port = seqwrap.in_ports

    # Bind each CHI memory controller to a MemCtrl and range
    for snf_cntrl, mem in zip(mem_cntrls, system.mem_ctrls):
        # CHI_SNF_Base expects:
        #   - memory_out_port: connect to MemCtrl.port
        #   - addr_ranges:     the range that SNF should serve
        snf_cntrl.memory_out_port = mem.port
        # For DDR4_2400_16x4, the range lives at mem.dram.range
        snf_cntrl.addr_ranges = mem.dram.range

    # Install the topology on the network
    # In current configs, the Topology object returned by create_topology()
    # exposes a makeTopology(...) method. Try the common signatures:
    if hasattr(topology, "makeTopology"):
        # Modern signature usually needs the network instance and options.
        try:
            topology.makeTopology(system.ruby.network, options)
        except TypeError:
            # Older signature: (network)
            topology.makeTopology(system.ruby.network)
    elif hasattr(topology, "makeLinks"):
        # Very old configs
        topology.makeLinks(system.ruby.network)
    else:
        raise RuntimeError(
            "Unknown Topology object; cannot wire the Ruby network."
        )

    # Expose convenience list
    system.system_port = (
        system.ruby._io_port if hasattr(system.ruby, "_io_port") else None
    )
    return system


def main():
    p = argparse.ArgumentParser(
        description="2 TGs + 2 mem channels + 3-level caches (CHI/Ruby)"
    )
    p.add_argument("--sys-clock", default="1GHz", dest="sys_clock")
    p.add_argument("--mem-size", default="2GB")
    p.add_argument("--cacheline-size", type=int, default=64)

    # Cache sizes/assocs (private L1I/L1D/L2; shared L3 via HNF)
    p.add_argument("--l1i-size", default="32kB")
    p.add_argument("--l1i-assoc", type=int, default=4)
    p.add_argument("--l1d-size", default="64kB")
    p.add_argument("--l1d-assoc", type=int, default=8)
    p.add_argument("--l2-size", default="256kB")
    p.add_argument("--l2-assoc", type=int, default=8)
    p.add_argument("--l3-size", default="2MB")
    p.add_argument("--l3-assoc", type=int, default=16)
    p.add_argument(
        "--num-l3", type=int, default=2, help="Number of HNFs / L3 slices"
    )

    # Ruby network + topology
    p.add_argument("--network", choices=["garnet", "simple"], default="garnet")
    p.add_argument(
        "--topology",
        choices=["CustomMesh", "Crossbar", "Pt2Pt"],
        default="CustomMesh",
    )

    # Your CHI_config.py path (used by CHI.py)
    p.add_argument("--chi-config", default="configs/ruby/chi/CHI_config.py")

    # TrafficGen configs (JSON state machine files)
    # p.add_argument("--tgen0", default="configs/traffic/random_readwrite.cfg")
    # p.add_argument("--tgen1", default="configs/traffic/random_readwrite.cfg")

    args = p.parse_args()

    system = build_system(args)
    root = Root(full_system=False, system=system)
    m5.instantiate()

    print("System built. You can now start simulate() as desired, e.g.:")
    print("  m5.simulate(m5.MaxTick)")
    # We don't auto-run here to keep this file purely a builder; bring your own loop.


if __name__ == "__main__":
    main()
