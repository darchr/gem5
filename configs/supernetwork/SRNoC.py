# Copyright (c) 2025 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import random
from enum import Enum as PyEnum

import m5
from m5.objects import *
from m5.params import *


class NetworkDelays(PyEnum):
    CROSSPOINT_DELAY = 4.1  # picoseconds
    MERGER_DELAY = 8.84
    SPLITTER_DELAY = 2.06
    CIRCUIT_VARIABILITY = 1.2
    VARIABILITY_COUNTING_NETWORK = 4.38
    CROSSPOINT_SETUP_TIME = 8.0


class ComponentPower(PyEnum):
    # Static power consumption in microwatts
    SPLITTER_STATIC = 5.98
    MERGER_STATIC = 5.0
    CROSSPOINT_STATIC = 7.9
    COUNTING_NETWORK_STATIC = 66.82
    TFF_STATIC = 10.8

    # Active power consumption in nanowatts
    SPLITTER_ACTIVE = 83.2
    MERGER_ACTIVE = 69.6
    CROSSPOINT_ACTIVE = 60.7
    COUNTING_NETWORK_ACTIVE = 163.0
    TFF_ACTIVE = 105.6


class ComponentJJ(PyEnum):
    # Number of Josephson Junctions (JJs)
    SPLITTER = 3
    MERGER = 5
    CROSSPOINT = 13
    COUNTING_NETWORK = 60
    TFF = 10


def calculate_power_and_area(radix):
    # Scale counting network power based on network radix
    counting_network_ratio = ((radix / 2) + 1) / 4.0
    counting_network_active_power = (
        ComponentPower.COUNTING_NETWORK_ACTIVE.value * counting_network_ratio
    )
    counting_network_static_power = (
        ComponentPower.COUNTING_NETWORK_STATIC.value * counting_network_ratio
    )
    counting_network_jj = int(
        ComponentJJ.COUNTING_NETWORK.value * counting_network_ratio
    )

    # Calculate number of components based on network radix
    r = radix // 2
    num_counting_networks = r
    num_crosspoints = r * r
    num_splitters = r * (r * (r - 1) + 1)
    num_mergers = r * (r * (r - 1) + 1)

    # Calculate active power consumption
    active_power = (
        num_counting_networks * counting_network_active_power
        + num_crosspoints * ComponentPower.CROSSPOINT_ACTIVE.value
        + num_splitters * ComponentPower.SPLITTER_ACTIVE.value
        + num_mergers * ComponentPower.MERGER_ACTIVE.value
    )

    # Calculate static power consumption
    static_power = (
        num_counting_networks * counting_network_static_power
        + num_crosspoints * ComponentPower.CROSSPOINT_STATIC.value
        + num_splitters * ComponentPower.SPLITTER_STATIC.value
        + num_mergers * ComponentPower.MERGER_STATIC.value
    )

    # Convert power units
    active_power *= 1e-9  # nanowatts to watts
    static_power *= 1e-6  # microwatts to watts
    total_power = active_power + static_power

    # Calculate total Josephson Junctions
    total_jj = (
        num_counting_networks * counting_network_jj
        + num_crosspoints * ComponentJJ.CROSSPOINT.value
        + num_splitters * ComponentJJ.SPLITTER.value
        + num_mergers * ComponentJJ.MERGER.value
    )

    # Log and store power and area statistics
    print(f"Active power: {active_power:.6f} W")
    print(f"Static power: {static_power:.6f} W")
    print(f"Total power: {total_power:.6f} W")
    print(f"Total JJ: {total_jj}")

    return {
        "active_power": active_power,
        "static_power": static_power,
        "total_power": total_power,
        "total_jj": total_jj,
    }


# Create a parent parser for the common (global) arguments.
parent_parser = argparse.ArgumentParser(add_help=False)
parent_parser.add_argument(
    "--maximum-packets",
    type=int,
    help="Maximum number of packets to process",
    default=None,
)
parent_parser.add_argument(
    "--file-path", type=str, help="Path to the input file", default=None
)
parent_parser.add_argument(
    "--num-cells", type=int, default=10, help="Number of DataCells to create"
)
parent_parser.add_argument(
    "--dynamic-range",
    type=int,
    nargs="+",
    default=[1000],
    help="Maximum value(s) for random data generation. Multiple values create multiple layers.",
)

# Main parser that includes a sub-command for traffic mode.
parser = argparse.ArgumentParser(
    description="SuperNetwork Simulation", parents=[parent_parser]
)
subparsers = parser.add_subparsers(
    dest="traffic_mode", required=True, help="Traffic mode sub-commands"
)

# Sub-command for random traffic mode.
random_parser = subparsers.add_parser(
    "random", help="Random traffic mode", parents=[parent_parser]
)

# Sub-command for hotspot traffic mode with additional required parameters.
hotspot_parser = subparsers.add_parser(
    "hotspot", help="Hotspot traffic mode", parents=[parent_parser]
)
hotspot_parser.add_argument(
    "--hotspot-addr",
    type=int,
    required=True,
    help="Hotspot address for hotspot traffic mode",
)
hotspot_parser.add_argument(
    "--hotspot-fraction",
    type=float,
    required=True,
    help="Hotspot fraction for hotspot traffic mode",
)

args = parser.parse_args()

# Check that at least one of --maximum-packets or --file-path is provided.
if args.maximum_packets is None and args.file_path is None:
    parser.error(
        "At least one of --maximum-packets or --file-path must be provided."
    )

# Create the root SimObject and system.
root = Root(full_system=False)
root.system = System()

# Set up the clock and voltage domains.
root.system.clk_domain = SrcClockDomain()
root.system.clk_domain.clock = "1.4GHz"
root.system.clk_domain.voltage_domain = VoltageDomain()

# Create the DataCells.
num_cells = args.num_cells
data_cells = [DataCell() for _ in range(num_cells)]

layers = [
    Layer(
        dynamic_range=dr,
        data_cells=data_cells,
        max_packets=args.maximum_packets,
        schedule_path=args.file_path,
        crosspoint_delay=NetworkDelays.CROSSPOINT_DELAY.value,
        merger_delay=NetworkDelays.MERGER_DELAY.value,
        splitter_delay=NetworkDelays.SPLITTER_DELAY.value,
        circuit_variability=NetworkDelays.CIRCUIT_VARIABILITY.value,
        variability_counting_network=NetworkDelays.VARIABILITY_COUNTING_NETWORK.value,
        crosspoint_setup_time=NetworkDelays.CROSSPOINT_SETUP_TIME.value,
    )
    for dr in args.dynamic_range
]

# Create the SuperNetwork and add the layers.
super_network = SuperNetwork()
super_network.layers = layers
root.system.super_network = super_network

# Print test configuration.
print("SRNoC Test Configuration")
print("==============================")
print(f"Number of DataCells: {num_cells}")
print(f"Dynamic Range: {args.dynamic_range}")
print()
print("Power and Area Statistics")
print("==============================")
power_and_area = calculate_power_and_area(radix=(num_cells * 2))
print()

if args.maximum_packets:
    print(f"Maximum Packets: {args.maximum_packets}")
if args.file_path:
    print(f"File Path: {args.file_path}")
print()

m5.instantiate()
# Set the traffic mode based on the chosen sub-command.
if args.traffic_mode == "hotspot":
    for layer in layers:
        layer.setHotspotTrafficMode(args.hotspot_addr, args.hotspot_fraction)
else:  # Random mode selected.
    for layer in layers:
        layer.setRandomTrafficMode()
exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
