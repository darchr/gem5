# -*- coding: utf-8 -*-
# Copyright (c) 2016 Jason Lowe-Power
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
# Authors: Jason Lowe-Power, Keshav Mathur, Brian Coutinho

""" This script is a part of the Statistical sampling methodology for
multi-threaded workload simulations in gem5. The method involves taking
multiple samples in the ROI of the benchmark and running multiple observations
of the same sample by adding random perturbations.

This file provides functions and methods to parse a set of
    experimental results of gem5.

Assumed directory hierarchy:
basedir -
        |
        |----benchmark suite1
                |
                |---application1
                        |
                        |--config1
                            |
                            |--sample1
                                |-----run1
                                |-----run2
                                |-----runN
                             --sample2
                             --sampleN
                        |--config2
                |----appN
        |----suite2
The functions can be used to parse a different hierarchy as well. The smallest
granularity at which stats are collected is config/ which means all samples and
all runs are iterated through.
The parsed statistics are added to a pandas dataframe.

"""

import os,sys
from collections import defaultdict
import sys

import numpy as np
import pandas

def readStatFile(basedir, config, suite, app, sample, run):
    """ Function to read a particular gem5 stat.txt file
    Assumption: basedir/sample/run/stats.txt is the file path

    @param config: Architectural configuration name being evaluated
    @param basedir: base directory
    @param suite: Suite of benchmark applications
    @param app: The benchmark intended to be parsed
    @param sample: directory name of a sample point in the ROI
    @param run: outdir of one particular run of given sample point
    """
    filename = os.path.join(basedir,suite,app, config, \
                            sample, run, 'stats.txt')
    with open(filename) as f:
        text = f.read()
    return text


def getStat(text, statname, typ=int):
    """ Function to find desired scalar statistic in stat.txt file,
    returns typecasted value

    @param text: contents of the stat.txt
    @param statname: name of statistic ( type string)
    @param typ: data type of statistic
    """
    start = text.find(statname)
    if start < 0:
        #raise Exception
        print "Warning. Missing", statname
        return typ(0)
    start += len(statname) + 1
    # the end is either the start of the desc, or a newline
    end = min(text.find('#', start) - 1, text.find('\n', start) - 1)
    nums = filter(bool, text[start:end].split(' '))
    stat = typ(nums[0])
    return stat


def getVectorStat(text, statname, num, typ=int):
    """ Function to find desired vector statistic in stat.txt file,
    returns value of the stat

    @param text: contents of the stat.txt
    @param statname: name of statistic ( type string)
    @param num: length of vector
    @param typ: data type of statistic
    """
    stat = []
    for i in range(num):
        stat.append(getStat(text, statname % (i), typ))
    return stat


def getSamples(config, suite, app):
    """ Function to return number of samples in the outdirectory
    returns a list of sample point directories
    OLDVER: params(config,suite,app)
    """
    basedir = "."
    return os.listdir(os.path.join(basedir, config, suite, app))

def getValue(text, statname, num=0, typ=int, name=''):
    """ Function to populate the statistic value by calling appropriate
    scalar/vector getStat function

    returns value and/or name touple
    @param text: contents of the stat.txt
    @param statname: name of the statistic
    @param num: Length of the statistic (for vectors)
    @param typ: Datatype of the statistic
    @param name: Name string, if an alternate stat name is desired
    """
    # check if vector statistic
    if statname.find('%') > 0:
        assert(num > 0)

        # store array of returned vector
        statvalue = np.array(getVectorStat(text, statname, num, typ))

    else:
        # Get scalar statistic value
        statvalue = getStat(text, statname, typ)
    # Return aliased name or split the last part of provided stat name
    if name:
        return name, statvalue
    else:
        return statname.split('.')[-1].split('::')[0], statvalue




def getStats(stat_dict,rows,basepath,config,suite,app,sample,run):
    """ Function to populate all statistics for a given sample and run
    Appends a dictionary ( named row) into a list of dicts ( named rows)
    The 'rows' object is used to create pandas dataframe.

    @param rows: existing list of dicts
    @param stat_dict: global user modified dictionary of desired statistics
                      with a format as mentioned below

      Key (string): name of the statistic in stat.txt file
      Value (3 tuple):  1 - size of statistic, 0 for scalar,
                            positive integer vector length for vector
                        2 - data type of statistic
                        3 - alternate name if desired
      To use wildcards in the stat name use '%d' (for vector stats)

    @param basepath: base directory
    @param config: Simulated system configuration
    @param suite: Name of benchmark suite
    @param app: Benchmark application
    """
    # read the stat file
    text = readStatFile(basepath,config,suite,app,sample,run)

    # get the list of statistics to be parsed
    stat_list = stat_dict.keys()

    # Append all statistics in a list,each member is (name,value) touple
    stats = []
    for stat in stat_list:
        (size, stype, sname) = stat_dict[stat]
        stats.append( getValue(text, stat, size, typ=stype, name=sname) )

    # Create a row for the pandas dataframe
    row = { 'config': config,
            'suite' : suite,
            'app'   : app,
            'Sample': sample, # use Sample not sample, later confuses pandas
            'run'   : run
            }

    for stat,value in stats:
        row[stat] = value
    rows.append(row)

#--------------------------------------------------------------------------
# Follwing functions (which are named discover*.. ) can be tweaked and used
# in varying order to enable discovery and hence parsing of the diretctory
# hierarchy.

def discoverSuites(path):
    """ Function to find suites
    @param path: provide the path to working directory
    """
    suites = [suite for suite in os.listdir(path) \
               if os.path.isdir(path + '/' + suite)]
    return path,suites

def discoverApps(path,suite):
    """ Function to find apps
    @param path: path to working directory
    @param suite: benchmark suite to be parsed
    """
    s_path,suites = discoverSuites(path)
    if not suite in suites:
        print "%s directory not found in current directory. Aborting..."\
                 % (suite)
        exit(1)
    else:
        appspath = os.path.join(path,suite)
        apps = [a for a in os.listdir(appspath) \
                 if os.path.isdir(appspath + '/' + a)]
        return appspath,apps

def discoverConfigs(path,suite,app):
    """ Function to find configs
    @param path: path to working directory
    @param suite: benchmark suite to be parsed
    @param config: architectural configuration
    """
    s_path,suites = discoverSuites(path)
    a_path,apps   = discoverApps(path,suite)
    if not app in apps:
        print "%s directory not found at path:\n%s. Aborting..."\
            % (app,a_path)
        exit(1)
    else:
        configpath = os.path.join(a_path,app)
        configs = [c for c in os.listdir(configpath) \
                    if os.path.isdir(configpath + '/' + c)]
        return configpath,configs

def discoverSamples(path,suite,app,config):
    """ Function to find samples and runs
    @param path: path to working directory
    @param suite: benchmark suite to be parsed
    @param config: architectural configuration
    """
    c_path,configs = discoverConfigs(path,suite,app)
    if not config in configs:
        print "%s directory not found at path:\n %s. Aborting..."\
                % (config,c_path)
        exit(1)
    else:
        #appspath   = os.path.join(path,suite)
        #configpath = os.path.join(appspath,app)
        samplepath = os.path.join(c_path,config)
        samples = [smpl for smpl in os.listdir(samplepath)\
                    if os.path.isdir(samplepath + '/' + smpl)]
        return samplepath,samples


def applyToAllSamples(func,bpath,config,suite,app):
    """ Function to parse a unique suite/app/config
    Calls discover samples which in turn can check if supplied arguements
    are valid.
    @param func: Function, ideally getStats(line: ) function is passed here.
    @param bpath: base path to look up directories
    @param config, suite, app : these are dir names for locating
                  the sample dirs - basepath/suite/app/config/*
    """
    # get base path
    basepath = bpath

    # get the list of samples for this config and the path to all these samples
    samplepath,samples = discoverSamples(basepath,suite,app,config)

    skipcnt = 0
    # Iterate over all samples found for this config
    for sample in samples:

        # Get path to each runs for this sample
        runpath = os.path.join(samplepath,sample)
        # Get list of runs
        runs = os.listdir(runpath)

        # Iterate over each run
        for run in runs:

            # Get path to stat.txt file for this config/sample/run
            statpath = os.path.join(runpath,run,'stats.txt')

            # Check for empty stat.txt file
            if not os.path.isfile(statpath) or os.path.getsize(statpath)==0:
                continue
            if os.path.getsize(statpath) == 0:
                print "Skipping (empty statfile) : %s" % statpath
                skipcnt = skipcnt +1
                continue

            # Apply the getStats function to populate the stat value
            func(config,suite,app,sample,run)
        print "\n Skipped a total of %d directories" % skipcnt


def load_stats(stat_dict,basepath,suite, app, config, rows,pickle_en=True):
    # picklefile =  "%s_%s.pkl" %(rundir,app)
    from os import path
    #i f(path.isfile(picklefile)):
    #    print "loading from: ", picklefile
    #    nativedata = pandas.read_pickle(picklefile)
    #else:
    applyToAllSamples(lambda config, suite, app, sample,\
       run: getStats(stat_dict,rows, basepath,config, suite, app, sample, run),
                      basepath, config,suite, app)
    nativedata = pandas.DataFrame(rows)

    # TBD : add the ability to cache the parsed data
    #getDerivedStats(nativedata)
    #nativedata.to_pickle('data_pickles/' + "%s_%s.pkl" %(rundir,app))
    return nativedata

#   -------------
#   Code for tests
if __name__ == '__main__':
    stat_dict = {   #'sim_ticks' : (0,int,''),
                'system.clk_domain.clock' : (0,int,''),
                'system.timingCpu%d.numCycles' : (8,int,'')
            }

    rows = []
    path = os.path.dirname(os.path.realpath(__file__))
    s_path,my_suites = discoverSuites(path)
    a_path,my_apps = discoverApps(path,'parsec')
    c_path,configs =  discoverConfigs(path,'parsec','streamcluster')
    mydf = load_stats(stat_dict,'parsec','streamcluster','ddr4',rows)
    #print mydf.Sample()
