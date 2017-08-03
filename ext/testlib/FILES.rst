Overview of Files
=================

Main Function Modules
---------------------

Files are listed in order of relative importance and are slightly
categorized. See each file's module docstring for more details.

`main.py <main.py>`__
~~~~~~~~~~~~~~~~~~~~~

The main entrypoint for whimsy. Uses parsed command-line arguments to
direct the flow of the program execution.

`loader.py <loader.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~

Contains the ``TestLoader`` class which implements logic for discovering
and parsing test files for their different test items.

`runner.py <runner.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~

Contains the ``Runner`` class. This class is responsible for running all
``TestCase`` and ``TestSuite`` instances, setting up ``Fixture``
objects, and notifying registered ``ResultLogger`` objects of test
results as they are run.

Testing Items
-------------

`suite.py <suite.py>`__
~~~~~~~~~~~~~~~~~~~~~~~

Contains the definition of a ``TestSuite`` class - a completely
self-contained unit of test. Also implents containers for ``TestSuite``
and ``TestCase`` instances (``SuiteList`` and ``TestList`` respectively)
which provide utility methods and supply some metadata about contained
objects.

`test.py <test.py>`__
~~~~~~~~~~~~~~~~~~~~~

Defines the base abstract ``TestCase`` class and the concrete
``TestFunction`` class which is a ``TestCase`` that runs a single
function. Also provides a function decorator ``testfunction`` which
provides a simple interface to create a ``TestFunction`` instance.

`fixture.py <fixture.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Defines the base class for a ``Fixture``, the method for setting up and
cleaning up before and after tests. They also are the preferred method
for transferring variables/data between tests contained in the same
``TestSuite``.

Support Files
-------------

`result.py <result.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~

Contains various classes implementing the ``ResultLogger`` logger
interface which the runner uses to report results. This is the method
used to collect and report test results as they happen or once all
testing is complete.

`config.py <config.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~

Contains a global ``config`` object used to configure options throughout
tests. Stores defaults, constants and parses command-line arguments.

`logger.py <logger.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~

Sets up and creates a ``logging`` object named ``log`` used throughout
the testing framework. The logging object provides a formatting of
various verbosity levels such as ``WARN``, ``BOLD`` and ``ERROR``.

`helper.py <helper.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~

Exposes various functions which might be useful for test writers,
especially ``log_call`` which is used to spawn a subprocess and pipe its
output into any number of file streams but also logs at a low verbosity
level.

`terminal.py <terminal.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Taken from the gem5 source ``gem5/src/m5/util/terminal.py``. For those
unfamiliar it detects terminal colors available and provides them if
they are. This version adds a couple functions to generate a horizontal
``separator`` strings which fit the current console.

`query.py <query.py>`__
~~~~~~~~~~~~~~~~~~~~~~~

Uses a ``TestLoader`` object to return certain information for the
``list`` command.

`tee.py <tee.py>`__
~~~~~~~~~~~~~~~~~~~

Used to tee stdout and stderr of the running testing framework into
files for each ``TestCase``. Provides both a ``system_tee``
implementation as well as a pure ``python_tee`` implementation for
compatibility.

`util.py <util.py>`__
~~~~~~~~~~~~~~~~~~~~~~~~

Provides various utility/common functions used in the testing framework.
