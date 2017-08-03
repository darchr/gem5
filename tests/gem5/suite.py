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
import copy

from testlib.test import TestFunction
from testlib.suite import TestList, TestSuite
from testlib.helper import log_call, CalledProcessError
from testlib.config import constants, config
from testlib.loader import no_collect
from fixture import TempdirFixture, Gem5Fixture, VariableFixture
import verifier

def gem5_verify_config(name,
                       config,
                       config_args,
                       verifiers,
                       gem5_args=tuple(),
                       fixtures=[],
                       valid_isas=constants.supported_isas,
                       valid_variants=constants.supported_variants,
                       length=constants.supported_lengths[0]):
    '''
    Helper class to generate common gem5 tests using verifiers.

    The generated TestSuite will run gem5 with the provided config and
    config_args. After that it will run any provided verifiers to verify
    details about the gem5 run.

    .. seealso::  For the verifiers see :mod:`whimsy.gem5.verifier`

    :param name: Name of the test.
    :param config: The config to give gem5.
    :param config_args: A list of arguments to pass to the given config.

    :param verifiers: An iterable with Verifier instances which will be placed
        into a suite that will be ran after a gem5 run.

    :param gem5_args: An iterable with arguments to give to gem5. (Arguments
        that would normally go before the config path.)

    :param valid_isas: An iterable with the isas that this test can be ran
        for. If None given, will run for all supported_isas.

    :param valid_variants: An iterable with the variant levels that
        this test can be ran for. (E.g. opt, debug)
    '''
    for verifier in verifiers:
        no_collect(verifier)

    given_fixtures = []
    given_fixtures.extend(fixtures)
    fixtures = given_fixtures
    original_verifiers = verifiers

    testsuites = []
    for opt in valid_variants:
        for isa in valid_isas:

            # Create a tempdir fixture to be shared throughout the test.
            tempdir = TempdirFixture(build_once=True, lazy_init=True)
            gem5_returncode = VariableFixture(
                    name=constants.gem5_returncode_fixture_name)

            # Common name of this generated testcase.
            _name = '{given_name}-{isa}-{opt}'.format(
                    given_name=name,
                    isa=isa,
                    opt=opt)

            # Create the running of gem5 subtest.
            # NOTE: We specifically create this test before our verifiers so
            # this is listed first.
            gem5_subtest = TestFunction(
                    _create_test_run_gem5(config, config_args, gem5_args),
                    name=_name)

            # Create copies of the verifier subtests for this isa and
            # variant.
            verifier_tests = []
            for verifier in original_verifiers:
                verifier = copy.copy(verifier)
                verifier._name = '{name}-{vname}'.format(
                        name=_name,
                        vname=verifier.name)

                verifier_tests.append(verifier)

            # Place the verifier subtests into a collection.
            verifier_collection = TestList(verifier_tests, fail_fast=False)

            # Add the isa and variant to tags list.
            tags = {
                constants.isa_tag_type: set([isa]),
                constants.variant_tag_type: set([opt]),
                constants.length_tag_type: set([length]),
            }

            # Create the gem5 target for the specific architecture and
            # variant.
            fixtures = copy.copy(given_fixtures)
            fixtures.append(Gem5Fixture(isa, opt, tags=tags))
            fixtures.append(tempdir)
            fixtures.append(gem5_returncode)

            # Place our gem5 run and verifiers into a failfast test
            # collection. We failfast because if a gem5 run fails, there's no
            # reason to verify results.
            gem5_test_collection =  TestList(
                    (gem5_subtest, verifier_collection),
                    fail_fast=True)

            # Finally construct the self contained TestSuite out of our
            # tests.
            testsuites.append(TestSuite(
                _name,
                fixtures=fixtures,
                tags=tags,
                tests=gem5_test_collection))
    return testsuites

def _create_test_run_gem5(config, config_args, gem5_args):
    def test_run_gem5(fixtures):
        '''
        Simple \'test\' which runs gem5 and saves the result into a tempdir.

        NOTE: Requires fixtures: tempdir, gem5
        '''

        if gem5_args is None:
            _gem5_args = tuple()
        elif isinstance(gem5_args, str):
            # If just a single str, place it in an iterable
            _gem5_args = (gem5_args,)
        else:
            _gem5_args = gem5_args

        # FIXME/TODO: I don't like the idea of having to modify this test run
        # or always collect results even if not using a verifier. There should
        # be some configuration in here that only gathers certain results for
        # certain verifiers.
        #
        # I.E. Only the returncode verifier will use the gem5_returncode
        # fixture, but we always require it even if that verifier isn't being
        # ran.

        returncode = fixtures[constants.gem5_returncode_fixture_name]
        tempdir = fixtures[constants.tempdir_fixture_name].path
        gem5 = fixtures[constants.gem5_binary_fixture_name].path
        command = [
            gem5,
            '-d',  # Set redirect dir to tempdir.
            tempdir,
            '-re',# TODO: Change to const. Redirect stdout and stderr
        ]
        command.extend(_gem5_args)
        command.append(config)
        # Config_args should set up the program args.
        command.extend(config_args)
        log_call(command)

    return test_run_gem5
