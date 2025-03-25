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

import m5
from m5.objects import *
from m5.params import *

# Parse command-line arguments
parser = argparse.ArgumentParser(description="SuperNetwork Simulation")

parser.add_argument(
    "--maximum-packets",
    type=int,
    help="Maximum number of packets to process",
    default=None,
)
parser.add_argument(
    "--file-path", type=str, help="Path to the input file", default=None
)

parser.add_argument(
    "--num-cells", type=int, default=10, help="Number of DataCells to create"
)
parser.add_argument(
    "--dynamic-range",
    type=int,
    default=1000,
    help="Maximum value for random data generation",
)

args = parser.parse_args()

# Ensure that at least one of --maximum-packets or --file-path is provided
if args.maximum_packets is None and args.file_path is None:
    parser.error(
        "At least one of --maximum-packets or --file-path must be provided."
    )

# Create the root SimObject and system
root = Root(full_system=False)
root.system = System()

# Set up the clock domain and voltage domain
root.system.clk_domain = SrcClockDomain()
root.system.clk_domain.clock = "1GHz"
root.system.clk_domain.voltage_domain = VoltageDomain()

# Create several DataCells
num_cells = args.num_cells
data_cells = [DataCell() for _ in range(num_cells)]

layers = [Layer(range_size=args.dynamic_range)]

# Create the SuperNetwork and add the DataCells
super_network = SuperNetwork()
super_network.dataCells = data_cells
super_network.layers = layers
super_network.max_packets = args.maximum_packets
super_network.schedule_path = args.file_path

# Add everything to the system
root.system.super_network = super_network

# Print test configuration
print("SRNoC Test Configuration")
print("==============================")
print(f"Number of DataCells: {num_cells}")
print(f"Dynamic Range: {args.dynamic_range}")

if args.maximum_packets:
    print(f"Maximum Packets: {args.maximum_packets}")
if args.file_path:
    print(f"File Path: {args.file_path}")

print()

m5.instantiate()
exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
