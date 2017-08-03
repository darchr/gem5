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
