'''
Provides a common logging system. With ability to add additional logging
levels.
'''
# TODO: Should add a debug flag system.

import logging as _logging
import sys

import terminal

# Logging level to be used to always display information to the user
def add_logging_level(name, val):
    name = name.lower()
    _logging.addLevelName(val, name.upper())

    def log_at_level(self, message, *args, **kwargs):
        if self.isEnabledFor(val):
            self._log(val, message, args, **kwargs)
    # Add the logging helper function as lowercase
    setattr(_logging.Logger, name.lower(), log_at_level)

    # Add the value to the naming module as CAPITALIZED
    globals()[name.upper()] = val

# Add all default logging levels to this module
for level in ('ERROR', 'DEBUG', 'FATAL', 'INFO', 'WARN'):
    globals()[level] = getattr(_logging, level)

# The minimum level which will always be displayed no matter verbosity.
always_display_level = WARN

# Logging level used to display bold information for users.
add_logging_level('bold', 1000)
# Logging level to always output (Use for UI stuff.)
add_logging_level('display', 999)

# Make sure these items will always be displayed.
assert DISPLAY > always_display_level
assert BOLD > always_display_level

# Logging level will be incredibly verbose, use to trace through testing.
add_logging_level('trace', 1)

def set_logging_verbosity(verbosity):
    '''
    Set the logging level based on the number of verbosity flags given.
    '''
    log.setLevel(max(always_display_level - verbosity * 10, TRACE))

class ConsoleLogFormatter(object):
    '''
    Formats output to be sent to an interactive terminal. Colors may be added
    based on logging level.
    '''
    # For now, just change color based on the logging level.
    color = terminal.get_termcap()
    reset = color.Normal
    level_colormap = {
        BOLD: color.White + color.Bold,
        FATAL: color.Red,
        WARN: color.Yellow
    }

    def __init__(self):
        pass

    def format(self, record):
        color_str = self.level_colormap.get(record.levelno, self.color.Normal)
        return color_str + record.msg + self.reset


# The common logger used everywhere in the framework.
log = _logging.getLogger('Main Console Logger')

# Save stdout and stderr incase they are modified somewhere else. (Tee for
# example.)
saved_stderr = sys.stderr
saved_stdout = sys.stdout

stdout_logger = _logging.StreamHandler(saved_stdout)
stdout_logger.formatter = ConsoleLogFormatter()
log.addHandler(stdout_logger)
