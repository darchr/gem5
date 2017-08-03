'''
Test file for the util m5 exit assembly instruction.
'''
import re
import os
from testlib import *

m5_exit_regex = re.compile(
r'Exiting @ tick \d* because m5_exit instruction encountered'
)

test_program = DownloadedProgram('m5-exit/bin/x86/linux/', 'm5_exit')

a = verifier.MatchRegex(m5_exit_regex)
gem5_verify_config(
    name='m5_exit_test',
    verifiers=[a],
    fixtures=(test_program,),
    config=os.path.join(config.base_dir, 'configs', 'example','se.py'),
    config_args=['--cmd', test_program.path],
    valid_isas=('X86',)
)
