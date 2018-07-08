import terminal
import log

# TODO Refactor print logic out of this so the objects
# created are separate from print logic.
class QueryRunner(object):
    def __init__(self, test_schedule):
        self.schedule = test_schedule
    
    def tags(self):
        tags = set()
        for suite in self.schedule:
            tags = tags | set(suite.tags)
        return tags
    
    def suites(self):
        return [suite for suite in self.schedule]

    def suites_with_tag(self, tag):
        return filter(lambda suite: tag in suite.tags, self.suites())

    def list_tests(self):
        log.test_log.message(terminal.separator())
        log.test_log.message('Listing all Test Cases.', bold=True)
        log.test_log.message(terminal.separator())
        for suite in self.schedule:
            for test in suite:
                log.test_log.message(test.uid)

    def list_suites(self):
        log.test_log.message(terminal.separator())
        log.test_log.message('Listing all Test Suites.', bold=True)
        log.test_log.message(terminal.separator())
        for suite in self.suites():
            log.test_log.message(suite.uid)

    def list_tags(self):
        #TODO In Gem5 override this with tag types (isa,variant,length)

        log.test_log.message(terminal.separator())
        log.test_log.message('Listing all Test Tags.', bold=True)
        log.test_log.message(terminal.separator())

        for tag in self.tags():
            log.test_log.message(tag)