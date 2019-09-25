# -*- coding: utf-8 -*-
# Copyright (c) 2018 The Regents of the University of California
# All Rights Reserved.
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
#
# Authors: Jason Lowe-Power

import os
import argparse

import m5
from m5.objects import *

class PerfectMemory(SimpleMemory):
    latency = '0ns'
    bandwidth = '0B/s'

if buildEnv['TARGET_ISA'] == 'x86':
  valid_cpu = {'AtomicSimpleCPU': AtomicSimpleCPU,
               'TimingSimpleCPU': TimingSimpleCPU,
               'DerivO3CPU': DerivO3CPU
              }
else:
  valid_cpu = {'AtomicSimpleCPU': AtomicSimpleCPU,
               'TimingSimpleCPU': TimingSimpleCPU,
               'MinorCPU': MinorCPU,
               'DerivO3CPU': DerivO3CPU,
              }

valid_mem = { 'PerfectMemory': PerfectMemory, }

parser = argparse.ArgumentParser()
parser.add_argument('binary', type = str)
parser.add_argument('--cpu', choices = valid_cpu.keys(),
                    default = 'TimingSimpleCPU')
parser.add_argument('--mem', choices = valid_mem.keys(),
                    default = 'PerfectMemory')

args = parser.parse_args()

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

if args.cpu != "AtomicSimpleCPU":
        system.mem_mode = 'timing'

system.mem_ranges = [AddrRange('512MB')]

system.cpu = valid_cpu[args.cpu]()

system.mem_ctrl = valid_mem[args.mem]()
system.mem_ctrl.range = system.mem_ranges[0]
system.cpu.icache_port = system.mem_ctrl.port
system.cpu.dcache_port = system.mem_ctrl.port
system.system_port     = system.mem_ctrl.port

system.cpu.createInterruptController()
if m5.defines.buildEnv['TARGET_ISA'] == "x86":
    system.interrupt_xbar = SystemXBar()
    system.cpu.interrupts[0].pio = system.interrupt_xbar.master
    system.cpu.interrupts[0].int_master = system.interrupt_xbar.slave
    system.cpu.interrupts[0].int_slave = system.interrupt_xbar.master

process = Process()
process.cmd = [args.binary]
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system = False, system = system)
m5.instantiate()

exit_event = m5.simulate()

if exit_event.getCause() != 'exiting with last active thread context':
    exit(1)
