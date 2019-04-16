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

import os
import tempfile
import shutil

from testlib.fixture import Fixture, globalfixture
from testlib.config import config, constants
from testlib.helper import log_call, cacheresult, joinpath, absdirpath
import testlib.log as log


class VariableFixture(Fixture):
    def __init__(self, value=None, name=None):
        super(VariableFixture, self).__init__(name=name)
        self.value = value


class TempdirFixture(Fixture):
    def __init__(self):
        self.path = None
        super(TempdirFixture, self).__init__(
                name=constants.tempdir_fixture_name)

    def setup(self, testitem):
        self.path = tempfile.mkdtemp(prefix='gem5out')

    def teardown(self, testitem):
        if self.path is not None:
            shutil.rmtree(self.path)

    def skip_cleanup(self):
        # Set path to none so it's not deleted
        self.path = None


class SConsFixture(Fixture):
    '''
    Fixture will wait until all SCons targets are collected and tests are
    about to be ran, then will invocate a single instance of SCons for all
    targets.

    :param directory: The directory which scons will -C (cd) into before
        executing. If None is provided, will choose the config base_dir.
    '''
    def __init__(self, directory=None, target_class=None, options=[]):
        self.directory = directory if directory else config.base_dir
        self.target_class = target_class if target_class else SConsTarget
        self.threads = config.threads
        self.targets = set()
        self.options = options
        super(SConsFixture, self).__init__()

    def setup(self, testitem):
        if config.skip_build:
            return

        command = [
            'scons', '-C', self.directory,
            '-j', str(self.threads),
            '--ignore-style'
        ]

        if not self.targets:
            log.test_log.warn(
                'No SCons targets specified, this will'
                ' build the default all target.\n'
                'This is likely unintended, and you'
                ' may wish to kill testlib and reconfigure.')
        else:
            log.test_log.message(
                    'Building the following targets.'
                    ' This may take a while.')
            log.test_log.message('%s' % (', '.join(self.targets)))
            log.test_log.message(
                    "You may want to run with only a single ISA"
                    "(--isa=), use --skip-build, or use 'rerun'.")

        command.extend(self.targets)
        if self.options:
            command.extend(self.options)
        log_call(log.test_log, command)


class SConsTarget(Fixture):
    # The singleton scons fixture we'll use for all targets.
    default_scons_invocation = None

    def __init__(self, target, build_dir=None, invocation=None):
        '''
        Represents a target to be built by an 'invocation' of scons.

        :param target: The target known to scons.

        :param build_dir: The 'build' directory path which will be prepended
            to the target name.

        :param invocation: Represents an invocation of scons which we will
            automatically attach this target to. If None provided, uses the
            main 'scons' invocation.
        '''

        if build_dir is None:
            build_dir = config.build_dir
        self.target = os.path.join(build_dir, target)
        super(SConsTarget, self).__init__(name=target)

        if invocation is None:
            if self.default_scons_invocation is None:
                SConsTarget.default_scons_invocation = SConsFixture()
                globalfixture(SConsTarget.default_scons_invocation)

            invocation = self.default_scons_invocation
        self.invocation = invocation

    def schedule_finalized(self, schedule):
        self.invocation.targets.add(self.target)
        return Fixture.schedule_finalized(self, schedule)

class Gem5Fixture(SConsTarget):
    other_invocations = {} # stores scons invocations other than the default

    def __init__(self, isa, variant, protocol=None):
        if protocol:
            # When specifying an non-default protocol, we have to make a
            # separate scons invocation with specific parameters. However, if
            # more than one tests needs the same target, we need to make sure
            # that we don't call scons too many times.
            target_dir = isa.upper()+'-'+protocol
            target = joinpath(target_dir, 'gem5.%s' % variant)
            if target_dir in self.other_invocations.keys():
                invocation = self.other_invocations[target_dir]
            else:
                options = ['PROTOCOL='+protocol, '--default='+isa.upper()]
                invocation = SConsFixture(options=options)
                globalfixture(invocation)
                Gem5Fixture.other_invocations[target_dir] = invocation
        else:
            target = joinpath(isa.upper(), 'gem5.%s' % variant)
            invocation = None # use default
        super(Gem5Fixture, self).__init__(target, invocation=invocation)

        self.name = constants.gem5_binary_fixture_name
        self.path = self.target
        self.isa = isa
        self.variant = variant


class MakeFixture(Fixture):
    def __init__(self, directory, *args, **kwargs):
        name = 'make -C %s' % directory
        super(MakeFixture, self).__init__(build_once=True, lazy_init=False,
                                          name=name,
                                          *args, **kwargs)
        self.required_by = []
        self.directory = directory

    def setup(self, testitem):
        super(MakeFixture, self).setup(testitem)
        targets = set(self.required_by)
        command = ['make', '-C', self.directory]
        command.extend([target.target for target in targets])
        log_call(log.test_log, command)


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
            make_fixture = MakeFixture(absdirpath(target))

        self.make_fixture = make_fixture

        # Add our self to the required targets of the main MakeFixture
        self.make_fixture.required_by.append(self)

    def setup(self, testitem):
        super(MakeTarget, self).setup(testitem)
        self.make_fixture.setup(testitem)
        return self

class TestProgram(MakeTarget):
    def __init__(self, program, isa, os, recompile=False):
        make_dir = joinpath('test-progs', program)
        make_fixture = MakeFixture(make_dir)
        target = joinpath('bin', isa, os, program)
        super(TestProgram, self).__init__(target, make_fixture)
        self.path = joinpath(make_dir, target)
        self.recompile = recompile

    def setup(self, testitem):
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
        """
        path: string
            The path to the directory containing the binary relative to
            $GEM5_BASE/tests
        program: string
            The name of the binary file
        """
        super(DownloadedProgram, self).__init__("download-" + program,
                                                build_once=True, **kwargs)

        self.program_dir = path
        relative_path = joinpath(self.program_dir, program)
        self.url = self.urlbase + relative_path
        self.path = os.path.realpath(
                        joinpath(absdirpath(__file__), '../', relative_path)
                    )

    def _download(self):
        import urllib
        import errno
        log.test_log.debug("Downloading " + self.url + " to " + self.path)
        if not os.path.exists(self.program_dir):
            try:
                os.makedirs(self.program_dir)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise
        urllib.urlretrieve(self.url, self.path)

    def _getremotetime(self):
        import  urllib2, datetime, time
        import _strptime # Needed for python threading bug

        u = urllib2.urlopen(self.url)
        return time.mktime(datetime.datetime.strptime( \
                    u.info().getheaders("Last-Modified")[0],
                    "%a, %d %b %Y %X GMT").timetuple())

    def setup(self, testitem):
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
