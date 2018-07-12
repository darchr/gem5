import os
import itertools

import config
import fixture as fixture_mod
import handlers
import loader as loader_mod
import log
import query
import result
import runner
import terminal


class RunLogHandler():
    def __init__(self):
        term_handler = handlers.TerminalHandler(
            verbosity=config.config.verbose+log.LogLevel.Info
        )
        summary_handler = handlers.SummaryHandler()
        self.mp_handler = handlers.MultiprocessingHandlerWrapper(summary_handler, term_handler)
        self.mp_handler.async_process()
        log.test_log.log_obj.add_handler(self.mp_handler)
    
    def schedule_finalized(self, test_schedule):
        # Create the result handler object.
        self.result_handler = handlers.ResultHandler(
                test_schedule, config.config.result_path)
        self.mp_handler.add_handler(self.result_handler)
    
    def finish_testing(self):
        self.result_handler.close()


def filter_with_config_tags(loaded_library):
    tags = getattr(config.config, config.StorePositionalTagsAction.position_kword)
    final_tags = []
    regex_fmt = '^%s$'
    cfg = config.config

    def _append_inc_tag_filter(name):
        if hasattr(cfg, name):
            tag_opts = getattr(cfg, name)
            for tag in tag_opts:
                final_tags.append(config.TagRegex(True, regex_fmt % tag))

    def _append_rem_tag_filter(name):
        if hasattr(cfg, name):
            tag_opts = getattr(cfg, name)
            for tag in cfg.constants.supported_tags[name]:
                if tag not in tag_opts:
                    final_tags.append(config.TagRegex(False, regex_fmt % tag))

    # Append additional tags for the isa, length, and variant options.
    # They apply last (they take priority)
    special_tags = (
        cfg.constants.isa_tag_type,
        cfg.constants.length_tag_type,
        cfg.constants.variant_tag_type
    )

    for tagname in special_tags:
        _append_inc_tag_filter(tagname)
    for tagname in special_tags:
        _append_rem_tag_filter(tagname)

    if tags is None:
        tags = tuple()

    filters = list(itertools.chain(tags, final_tags))
    string = 'Filtering suites with tags as follows:'
    filter_string = '\n\t'.join((str(f) for f in filters)) 
    log.test_log.trace(string + filter_string)

    return filter_with_tags(loaded_library, filters)


def filter_with_tags(loaded_library, filters):
    '''
    Filter logic supports two filter types:
    --include-tags <regex>
    --exclude-tags <regex>

    The logic maintains a `set` of test suites. 

    If the regex provided with the `--include-tags` flag matches a tag of a suite, that suite will added to the set.
    If the regex provided with the `--exclude-tags` flag matches a tag of a suite, that suite will removed to the set.
    Suites can be added and removed multiple times.

    First Flag Special Case Logic:
    If include is the first flag, start with an empty set of suites.
    If exclude is the first flag, start with the set of all collected suites.


    Let's trace out the set as we go through the flags to clarify::

        # Say our collection of suites looks like this: set(suite_ARM64, suite_X86, suite_Other).
        # Additionally, we've passed the flags in the following order: --include-tags "ARM64"  --exclude-tags ".*" --include-tags "X86"
        
        # Process --include-tags "ARM64"
        set(suite_ARM64)    # Suite begins empty, but adds the ARM64 suite
        # Process --exclude-tags ".*"
        set()               # Removed all suites which have tags
        # Process --include-tags "X86"
        set(suite_X86)
    '''
    if not filters:
        return

    query_runner = query.QueryRunner(loaded_library)
    tags = query_runner.tags()

    if not filters[0].include:
        suites = set(query_runner.suites())
    else:
        suites = set()

    def exclude(excludes):
        return suites - excludes
    def include(includes):
        return suites | includes
        
    for tag_regex in filters:
        matched_tags = (tag for tag in tags if tag_regex.regex.search(tag))
        for tag in matched_tags:
            matched_suites = set(query_runner.suites_with_tag(tag))
            suites = include(matched_suites) if tag_regex.include else exclude(matched_suites)

    # Set the library's suites to only those which where accepted by our filter
    loaded_library.suites = [suite for suite in loaded_library.suites if suite in suites]

# TODO Add results command for listing previous results.

def load_tests():
    '''
    Create a TestLoader and load tests for the directory given by the config.
    '''
    testloader = loader_mod.Loader()
    log.test_log.message(terminal.separator())
    log.test_log.message('Loading Tests', bold=True)
    testloader.load_root(config.config.directory)
    return testloader

def do_list():
    term_handler = handlers.TerminalHandler(
        verbosity=config.config.verbose+log.LogLevel.Info
    )
    log.test_log.log_obj.add_handler(term_handler)

    test_schedule = load_tests().schedule
    filter_with_config_tags(test_schedule)

    qrunner = query.QueryRunner(test_schedule)

    if config.config.suites:
        qrunner.list_suites()
    elif config.config.tests:
        qrunner.list_tests()
    elif config.config.all_tags:
        qrunner.list_tags()
    else:
        qrunner.list_suites()
        qrunner.list_tests()
        qrunner.list_tags()

def run_schedule(test_schedule, log_handler):
    '''
    Test Phases
    -----------
    * Test Collection
    * Fixture Parameterization
    * Global Fixture Setup
    * Iteratevely run suites:
       * Suite Fixture Setup
       * Iteratively run tests:
          * Test Fixture Setup
          * Run Test
          * Test Fixture Teardown
       * Suite Fixture Teardown
    * Global Fixture Teardown
    '''

    log_handler.schedule_finalized(test_schedule)

    # Iterate through all fixtures notifying them of the test schedule.
    for fixture in test_schedule.all_fixtures():
        fixture.schedule_finalized(test_schedule)

    log.test_log.message(terminal.separator())
    log.test_log.message('Running Tests from {} suites'
            .format(len(test_schedule.suites)), bold=True)
    log.test_log.message("Results will be stored in {}".format(
                config.config.result_path))
    log.test_log.message(terminal.separator())

    # Build global fixtures and exectute scheduled test suites.
    if config.config.test_threads > 1:
        library_runner = runner.LibraryParallelRunner(test_schedule)
        library_runner.set_threads(config.config.test_threads)
    else:
        library_runner = runner.LibraryRunner(test_schedule)
    library_runner.run()

    log_handler.finish_testing()

def do_run():
    # Initialize early parts of the log.
    log_handler = RunLogHandler()
    test_schedule = load_tests().schedule
    # Filter tests based on tags
    filter_with_config_tags(test_schedule)
    # Execute the tests
    run_schedule(test_schedule, log_handler)


def do_rerun():
    # Init early parts of log
    log_handler = RunLogHandler()

    # Load previous results
    results = result.InternalSavedResults.load(
            os.path.join(config.config.result_path,
            config.constants.pickle_filename))
    
    rerun_suites = [suite.uid for suite in results if suite.unsucessful]

    # Use loader to load suites
    loader = loader_mod.Loader()
    test_schedule = loader.load_schedule_for_suites(rerun_suites)

    # Execute the tests
    run_schedule(test_schedule, log_handler)


def main():
    '''
    Main entrypoint for the whimsy test library.
    '''
    config.initialize_config()

    # 'do' the given command.
    globals()['do_'+config.config.command]()
    log.test_log.close()
