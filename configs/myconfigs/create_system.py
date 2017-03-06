
import m5

from mysys import getSystem
from sampling import checkSystem

import sys
sys.path.append('../workloads/') # For the next line...
import utils

def createSystem(args):
    """ Takes in arguments from the command line formatted as follows and
        parses the inputs into a valid system object. Also returns the number
        of expected ROI begins.
        Format: suite.app samples runs configuration
    """
    workload_info = args[0].split('.')

    if len(args) == 4:
        config = args[3]
    else:
        config = 'baseline'

    if len(workload_info) == 2:
        return createSystemOne(args, config)
    else:
        return createSystemMulti(args, config)

def createSystemOne(args, config):
    """ Takes in arguments from the command line formatted as follows and
        parses the inputs into a valid system object. Also returns the number
        of expected ROI begins.
        Format: suite.app samples runs configuration
    """

    suite, app = args[0].split('.')

    if suite == 'parsec':
        secondDisk = '/p/multifacet/users/powerjg/dramcache/' \
                     'disk-images/parsec-inputs.img'
    elif suite == 'npb':
        secondDisk = '/p/multifacet/users/powerjg/dramcache/' \
                     'disk-images/npb.img'
    #elif suite == 'tpch':
        #secondDisk = '/mnt/gluster/jgpower/disk-images/tpch.img'
    else:
        fatal("No disk for suite %s." % (suite))

    appInfo = utils.getWorkload(suite, app)

    if 'size' in appInfo.keys():
        reqMem = appInfo['size']
    else:
        reqMem = '8GB'

    if 'instructions' in appInfo.keys():
        roiInstructions = appInfo['instructions']
    else:
        fatal("No instructions for %s %s" % (suite, app))

    # create the system we are going to simulate
    system = getSystem(config, secondDisk, reqMem)

    # Check if the system is compatible with sampling library
    checkSystem(system)

    # For workitems to work correctly
    # This will cause the simulator to exit simulation when the first work
    # item is reached and when the first work item is finished.
    system.exit_on_work_items = True

    # Read in the script file passed in via an option.
    # This file gets read and executed by the simulated system after boot.
    # Note: The disk image needs to be configured to do this.
    system.readfile = '/local.chinook/research/dramcache/workloads/' \
                      'runscripts/' + app + '.rcS'

    return system, 1, roiInstructions

def createSystemMulti(args, config):
    """ Takes in arguments from the command line formatted as follows and
        parses the inputs into a valid system object. This is used for
        multi-programmed workloads
        Format: suite.app1.app2.app3... samples runs configuration
    """

    suite = args[0].split('.')[0]
    apps = args[0].split('.')[1:]

    if suite == 'parsec':
        secondDisk = '/p/multifacet/users/powerjg/dramcache/' \
                     'disk-images/parsec-inputs.img'
    elif suite == 'npb':
        secondDisk = '/p/multifacet/users/powerjg/dramcache/' \
                     'disk-images/npb.img'
    else:
        fatal("No disk for suite %s." % (suite))

    reqMem = 0
    roiInstructions = 0
    for app in apps:
        appInfo = utils.getWorkload(suite, app)
        if 'size' in appInfo.keys():
            req = appInfo['size']
        else:
            req = '8GB'

        reqMem += m5.params.MemorySize(str(req)).value

        if 'instructions' in appInfo.keys():
            instructions = appInfo['instructions']
        else:
            fatal("No instructions for %s %s" % (suite, app))

        roiInstructions += instructions

    # create the system we are going to simulate
    system = getSystem(config, secondDisk, reqMem)

    # Check if the system is compatible with sampling library
    checkSystem(system)

    # For workitems to work correctly
    # This will cause the simulator to exit simulation when the first work
    # item is reached and when the first work item is finished.
    system.exit_on_work_items = True

    # Read in the script file passed in via an option.
    # This file gets read and executed by the simulated system after boot.
    # Note: The disk image needs to be configured to do this.
    system.readfile = '/local.chinook/research/dramcache/workloads/' \
                      'runscripts/' + '.'.join(apps) + '.rcS'

    return system, len(apps), roiInstructions
