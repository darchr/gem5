#!/usr/bin/env python2
'''
The main source for whimsy. Ties together the default test runners and
loaders.

Discovers and runs all tests from a given root directory.
'''
from __future__ import print_function

import sys
import os

base_dir = os.path.dirname(os.path.abspath(__name__))
ext_path = os.path.join(base_dir, os.pardir, 'ext')

sys.path.insert(0, base_dir)
sys.path.insert(0, ext_path)

import whimsy.main as whimsy
import whimsy.config as config
import whimsy.helper as helper

print("Running the new gem5 testing script.")
print("For more information see TESTING.md.")
print("To see details as the testing scripts are running, use the option "
      "-v, -vv, or -vvv")

config.basedir = helper.absdirpath(__file__)
whimsy()
