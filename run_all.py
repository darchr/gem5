#!/usr/bin/env python3

import sys
import subprocess

sys.path.append('configs/myconfigs')

from spec import specall

for workload in specall:
  command = ['build/X86/gem5.opt',
             '--outdir=m5out/testing/{}'.format(workload),
             '-re',
             'configs/myconfigs/runtest.py',
             workload
            ]
  subprocess.Popen(command)
