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
Exposes the :class:`~Fixture` class.
'''
from helper import cacheresult

class Fixture(object):
    '''
    Base Class for a test Fixture.

    Fixtures are items which possibly require setup and/or tearing down after
    a TestCase or a TestSuite has run.

    Fixtures are the prefered method of carrying incremental results or
    variables between TestCases in TestSuites. (Rather than using globals.)
    The test system assumes that TestSuites are self contained so
    paralellization is possible in the future. Using global variables will
    likely cause things to break unexpectedly.

    .. note:: no_collect function is provided by the :mod:`~whimsy.loader` to
        remove a fixture that should not be collected.

    .. note:: In order for Fixtures to be enumerated by the test system this
        class' :code:`__new__` method must be called. The loader class will
        monkey patch (modify at runtime) this method in order to enmerate
        tests.
    '''
    def __init__(self, name, build_once=False, lazy_init=True):
        '''
        :param name: Name of the fixture.

        :param lazy_init: If True, wait until test cases that use this fixture
            are ran to setup this fixture. Otherwise init the fixture before
            the first test case is ran.

        :param build_once: This fixture will only be built once. This is
            particularly useful for gem5 targets or build systems.

        :var requires: List of fixtures which require this Fixture. Before
            they can be built.

        :var required_by: List of fixtures this Fixture requires.
        :var built: Indicates that this fixture has been built.
        '''
        self.requires = []
        self.required_by = []
        self.name = name
        self._built = False
        self.lazy_init = lazy_init

        if build_once:
            self.setup = cacheresult(self.setup)

    @property
    def built(self):
        return self._built

    def require(self, other_fixture):
        '''
        Require that the other_fixture be built before us.
        This is particularly useful for build targets.

        .. seealso:: :class:`~whimsy.gem5.fixture.SConsTarget`
        '''
        self.requires.append(other_fixture)
        other_fixture.required_by.append(self)

    def setup(self):
        '''Call setup of fixtures we require.'''
        self._built = True
        for fixture in self.requires:
            fixture.setup()

    def teardown(self):
        '''Empty method, meant to be overriden if fixture requires teardown.'''
        pass

    if __debug__:
        # This is a method that will be created by the test loader in order to
        # manually remove a fixture.
        __no_collect__ = NotImplemented
