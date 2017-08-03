'''
Test file for the util m5 exit assembly instruction.
'''
from testlib import *

test_progs = {
    'x86': ('hello64-static', 'hello64-dynamic', 'hello32-static'),
    'arm': ('hello64-static', 'hello32-static'),
}

for isa in test_progs:
    for binary in test_progs[isa]:
        import os
        path = os.path.join('hello', 'bin', isa, 'linux')
        hello_program = DownloadedProgram(path, binary)

        ref_path = joinpath(getcwd(), 'ref')

        verifiers = (
                verifier.MatchStdoutNoPerf(joinpath(ref_path, 'simout')),
        )

        gem5_verify_config(
                name='test'+binary,
                fixtures=(hello_program,),
                verifiers=verifiers,
                config=joinpath(config.base_dir, 'configs', 'example','se.py'),
                config_args=['--cmd', hello_program.path],
                valid_isas=(isa.upper(),),
        )
