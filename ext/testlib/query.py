# Copyright (c) 2017 Mark D. Hill and David A. Wood
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
#
# Authors: Sean Wilson

'''
File which implements querying and display logic for metadata about loaded
items.
'''
from config import constants, config
from logger import log
from terminal import separator

def list_fixtures(loader):
    log.display(separator())
    log.display('Listing all Fixtures.')
    log.display(separator())
    for fixture in loader.fixtures:
        log.display(fixture.name)

def list_tests(loader):
    log.display(separator())
    log.display('Listing all TestCases.')
    log.display(separator())
    for test in loader.tests:
        log.display(test.uid)

def list_suites(loader):
    log.display(separator())
    log.display('Listing all TestSuites.')
    log.display(separator())
    for suite in loader.suites:
        log.display(suite.uid)

def list_tags():
    log.display(separator())
    log.display('Listing all Tags.')
    log.display(separator())
    for typ,vals in constants.supported_tags.iteritems():
        log.display("%s: %s" % (typ, ', '.join(vals)))

def list_tests_with_tags(loader, tags):
    log.display(separator())
    log.display('Listing tests based on tags.')
    log.display(separator())
    for uid in [test.uid for test in loader.tests if test.match_tags(tags)]:
        log.display(uid)

def list_suites_with_tags(loader, tags):
    log.display(separator())
    if not config.quiet:
        log.display('Listing suites based on tags.')
        log.display(separator())
    for uid in [suite.uid for suite in loader.suites if suite.match_tags(tags)]:
        log.display(uid)
