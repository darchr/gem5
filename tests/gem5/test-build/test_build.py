'''
Test file for simply building gem5
'''
import re
import os
from testlib import *

functions = []
for isa in constants.supported_isas:
    for variant in constants.supported_variants:

        tags = {
            constants.isa_tag_type: set([isa]),
            constants.variant_tag_type: set([variant]),
            constants.length_tag_type: set([constants.quick_tag]),
        }

        name = 'build-{isa}-{var}'.format(isa=isa, var=variant)
        fixture = Gem5Fixture(isa, variant, tags=tags)

        function = TestFunction(lambda fixtures: True, name,
                                fixtures=[fixture])

        TestSuite(name, [function], tags=tags, fail_fast=True)
