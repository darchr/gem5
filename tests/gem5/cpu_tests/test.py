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

'''
Test file containing simple workloads to run on CPU models.
Each test takes ~10 seconds to run.
'''

from testlib import *

valid_isas = ('x86', 'arm')

workloads = ('Bubblesort','FloatMM')

for isa in valid_isas:
    bm_dir = joinpath(getcwd(), 'benchmarks/bin/', isa)
    for workload in workloads:
        ref_path = joinpath(getcwd(), 'ref', workload)
        verifiers = (
                verifier.MatchStdout(ref_path),
        )

        workload_binary = DownloadedProgram(bm_dir, workload)
        workload_path = workload_binary.path

        gem5_verify_config(
                name='cpu_test_atomic_{}'.format(workload),
                verifiers=verifiers,
                config=joinpath(getcwd(), 'run.py'),
                config_args=['--cpu=AtomicSimpleCPU', workload_path],
                valid_isas=(isa.upper(),),
                fixtures=[workload_binary]
        )

        gem5_verify_config(
                name='cpu_test_timing_{}'.format(workload),
                verifiers=verifiers,
                config=joinpath(getcwd(), 'run.py'),
                config_args=['--cpu=TimingSimpleCPU', workload_path],
                valid_isas=(isa.upper(),),
                fixtures=[workload_binary]
        )

        gem5_verify_config(
                name='cpu_test_minor_{}'.format(workload),
                verifiers=verifiers,
                config=joinpath(getcwd(), 'run.py'),
                config_args=['--cpu=MinorCPU', workload_path],
                valid_isas=(isa.upper(),),
                fixtures=[workload_binary]
        )

        gem5_verify_config(
                name='cpu_test_o3_{}'.format(workload),
                verifiers=verifiers,
                config=joinpath(getcwd(), 'run.py'),
                config_args=['--cpu=DerivO3CPU', workload_path],
                valid_isas=(isa.upper(),),
                fixtures=[workload_binary]
        )

