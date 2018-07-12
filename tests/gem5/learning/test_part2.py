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
Test file for the scripts from Learning gem5 Part 2
'''

from whimsy import *

gem5_verify_config(
        name='learning_gem5_part2_simple',
        verifiers=((verifier.MatchStdout(
                    joinpath(getcwd(), 'ref', 'simout-simple')),
                   )),
        config=joinpath(config.base_dir, 'configs', 'learning_gem5',
                        'part2', 'run_simple.py'),
        config_args=[],
        valid_isas=((constants.null_tag),)
)

gem5_verify_config(
        name='learning_gem5_part2_hello_goodbye',
        verifiers=((verifier.MatchStdout(
                    joinpath(getcwd(), 'ref', 'simout-hello-goodbye')),
                   )),
        config=joinpath(config.base_dir, 'configs', 'learning_gem5',
                        'part2', 'hello_goodbye.py'),
        config_args=[],
        valid_isas=((constants.null_tag),)
)

valid_isas = (constants.x86_tag, constants.arm_tag)

for isa in valid_isas:

    verifiers = (
        verifier.MatchStdoutNoPerf(joinpath(getcwd(), 'ref', 'simout-hello')),
    )

    gem5_verify_config(
            name='learning_gem5_part2_simple_memobj',
            verifiers=verifiers,
            config=joinpath(config.base_dir, 'configs', 'learning_gem5',
                            'part2', 'simple_memobj.py'),
            config_args=[],
            valid_isas=((isa),),
    )

    verifiers = (
        verifier.MatchStdoutNoPerf(joinpath(getcwd(), 'ref', 'simout-hello')),
    )

    gem5_verify_config(
            name='learning_gem5_part2_simple_cache',
            verifiers=verifiers,
            config=joinpath(config.base_dir, 'configs', 'learning_gem5',
                            'part2', 'simple_cache.py'),
            config_args=[],
            valid_isas=((isa),),
    )
