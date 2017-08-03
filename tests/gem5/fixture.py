import os
import tempfile

import testlib.util as util
from testlib.fixture import Fixture
from testlib.config import config, constants
from testlib.helper import log_call, cacheresult, joinpath, absdirpath
from testlib.logger import log


class VariableFixture(Fixture):
    def __init__(self, value=None, name=None):
        super(VariableFixture, self).__init__(name)
        self.value = value
        self.teardown = self.setup = lambda : None


class TempdirFixture(Fixture):
    default_name = 'tempdir'
    def __init__(self, name=None, **kwargs):
        name = self.default_name if name is None else name
        super(TempdirFixture, self).__init__(name, **kwargs)
        self.path = None
        self.cleanup = True

    def setup(self):
        self.path = tempfile.mkdtemp(prefix='gem5out')

    def teardown(self):
        if self.cleanup:
            import shutil
            shutil.rmtree(self.path)

    def skip_cleanup(self):
        self.cleanup = False

class SConsFixture(Fixture):
    '''
    Fixture will wait until all SCons targets are collected and tests are
    about to be ran, then will invocate a single instance of SCons for all
    targets.

    :param directory: The directory which scons will -C (cd) into before
        executing. If None is provided, will choose the config base_dir.
    '''
    def __init__(self, name='SCons Fixture', directory=None, *args, **kwargs):
        super(SConsFixture, self).__init__(name, *args, lazy_init=True)
        self.directory = directory if directory else config.base_dir
        self.targets = []

    @cacheresult
    def setup(self):
        super(SConsFixture, self).setup()
        targets = set(self.required_by)
        command = ['scons', '-C', self.directory, '-j', str(config.threads)]

        def target_is_required(target):
            if target.tags is None:
                return True
            return util.match_tags(target, config.get_tags())

        to_build = [target.target for target in targets
                    if target_is_required(target)]

        log.display("Building the following targets. This may take a while.")
        log.display(', '.join([str(build) for build in set(to_build)]))

        command.extend(to_build)
        log_call(command)

    def teardown(self):
        pass

class SConsTarget(Fixture):
    # The singleton scons fixture we'll use for all targets.
    default_scons_invocation = None

    def __init__(self, target, build_dir=None, invocation=None,
                 tags=None, *args, **kwargs):
        '''
        Represents a target to be built by an 'invocation' of scons.

        :param target: The target known to scons.

        :param build_dir: The 'build' directory path which will be prepended
            to the target name.

        :param invocation: Represents an invocation of scons which we will
            automatically attach this target to. If None provided, uses the
            main 'scons' invocation.

        :param tags: If tags are specified, the fixture will only be built by
            scons if those specific tags are set in the config.
        '''

        if build_dir is None:
            build_dir = config.build_dir

        self.tags = tags

        self.target = os.path.join(build_dir, target)
        super(SConsTarget, self).__init__(self.target, *args, **kwargs)

        if invocation is None:
            if self.default_scons_invocation is None:
                SConsTarget.default_scons_invocation = \
                    SConsFixture(lazy_init=True)

            invocation = self.default_scons_invocation

        # Add our self to the required targets of the SConsFixture
        self.require(invocation)
        self.invocation = invocation

    def setup(self):
        super(SConsTarget, self).setup()
        self.invocation.setup()
        return self

class Gem5Fixture(SConsTarget):

    def __init__(self, isa, variant, autotag=True, tags=None):
        '''
        :param tags: If tags is None, this fixture will be automatically
            intialize the underlying Gem5Fixture with its isa and variant. If
            tags is False, no tags will be set. If tags is anything else, the
            Gem5Fixture will be handed those tags.
        '''
        target = joinpath(isa.upper(), 'gem5.%s' % variant)

        if tags is None:
            tags = {
                constants.isa_tag_type: set([isa]),
                constants.variant_tag_type: set([variant]),
            }
        elif tags is False: # Explicit check for false here.
            tags = None
        super(Gem5Fixture, self).__init__(target, tags=tags)

        self.name = constants.gem5_binary_fixture_name
        self.path = self.target
        self.isa = isa
        self.variant = variant

    def setup(self):
        if config.skip_build:
            log.debug('Skipping build of %s' % self.target)
        else:
            super(Gem5Fixture, self).setup()


class MakeFixture(Fixture):
    def __init__(self, directory, *args, **kwargs):
        name = 'make -C %s' % directory
        super(MakeFixture, self).__init__(build_once=True, lazy_init=False,
                                          name=name,
                                          *args, **kwargs)
        self.targets = []
        self.directory = directory

    def setup(self):
        super(MakeFixture, self).setup()
        targets = set(self.required_by)
        command = ['make', '-C', self.directory]
        command.extend([target.target for target in targets])
        log_call(command)


class MakeTarget(Fixture):
    def __init__(self, target, make_fixture=None, *args, **kwargs):
        '''
        :param make_fixture: The make invocation we will be attached to.
        Since we don't have a single global instance of make in gem5 like we do
        scons we need to know what invocation to attach to. If none given,
        creates its own.
        '''
        super(MakeTarget, self).__init__(name=target, *args, **kwargs)
        self.target = self.name

        if make_fixture is None:
            make_fixture = MakeFixture(
                    absdirpath(target),
                    lazy_init=True,
                    build_once=False)

        self.make_fixture = make_fixture

        # Add our self to the required targets of the main MakeFixture
        self.require(self.make_fixture)

    def setup(self):
        super(MakeTarget, self).setup()
        self.make_fixture.setup()
        return self

class TestProgram(MakeTarget):
    def __init__(self, program, isa, os, recompile=False):
        make_dir = joinpath('test-progs', program)
        make_fixture = MakeFixture(make_dir)
        target = joinpath('bin', isa, os, program)
        super(TestProgram, self).__init__(target, make_fixture)
        self.path = joinpath(make_dir, target)
        self.recompile = recompile

    def setup(self):
        # Check if the program exists if it does then only compile if
        # recompile was given.
        if self.recompile:
            super(MakeTarget, self).setup()
        elif not os.path.exists(self.path):
            super(MakeTarget, self).setup()

class DownloadedProgram(Fixture):
    """ Like TestProgram, but checks the version in the gem5 binary repository
        and downloads an updated version if it is needed.
    """
    urlbase = "http://gem5.org/dist/current/"

    def __init__(self, path, program, **kwargs):
        super(DownloadedProgram, self).__init__("download-" + program,
                                                build_once=True, **kwargs)

        self.program_dir = joinpath('test-progs', path)
        self.path = joinpath(self.program_dir, program)

        self.url = self.urlbase + self.path

    def _download(self):
        import urllib
        log.debug("Downlading " + self.url + " to " + self.path)
        if not os.path.exists(self.program_dir):
            os.makedirs(self.program_dir)
        urllib.urlretrieve(self.url, self.path)

    def _getremotetime(self):
        import urllib2, datetime, time
        u = urllib2.urlopen(self.url)
        return time.mktime(datetime.datetime.strptime( \
                    u.info().getheaders("Last-Modified")[0],
                    "%a, %d %b %Y %X GMT").timetuple())

    def setup(self):
        import urllib2
        # Check to see if there is a file downloaded
        if not os.path.exists(self.path):
            self._download()
        else:
            try:
                t = self._getremotetime()
            except urllib2.URLError:
                # Problem checking the server, use the old files.
                log.debug("Could not contact server. Binaries may be old.")
                return
            # If the server version is more recent, download it
            if t > os.path.getmtime(self.path):
                self._download()
