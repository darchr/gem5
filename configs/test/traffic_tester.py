import os
import sys
import time
import tempfile

import m5
import m5.ticks
from m5.objects import *
from m5.ticks import fromSeconds
from m5.util.convert import toLatency

sys.path.append('configs/common/') # For the next line...
import SimpleOpts

from caches import *
from dram_cache import DRAMCache

from main_systems import getSystem

def getBandwidth(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        bandwidth = 0
        stats = statsfile.read()
        statname = 'bw_total::total'
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        startchar = stats.find(statname, startchar)
        num = 0
        while startchar >= 0:
            num += 1;
            bwstr = stats[startchar+len(statname):stats.find('#', startchar)]
            bandwidth += int(bwstr)
            startchar = stats.find(statname, startchar + 1)
    return bandwidth

def getGenBW(dumps = 0, opts = None):
    import os
    outdir = m5.options.outdir
    if opts:
        cpus = opts.readers + opts.writers + opts.mixers
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        bandwidth = 0
        stats = statsfile.read()
        if opts and cpus > 10:
            statname = 'system.cpu%02d.numPackets'
        else:
            statname = 'system.cpu%d.numPackets'
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        cpunum = 0
        startchar = stats.find(statname % cpunum, startchar)
        while startchar >= 0:
            bwstr = stats[startchar+len(statname):stats.find('#', startchar)]
            bandwidth += int(bwstr)
            cpunum += 1
            startchar = stats.find(statname % cpunum, startchar + 1)

    return bandwidth

def getHitRate(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        misses = 0
        hits = 0
        stats = statsfile.read()
        statname1 = "missMapReadMiss"
        statname2 = "realReadMiss"
        statname3 = "realReadHit"
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        cpunum = 0
        startchar = stats.find(statname1, startchar)
        while startchar >= 0:
            bwstr = stats[startchar+len(statname1):stats.find('#', startchar)]
            misses += int(bwstr)
            startchar = stats.find(statname2, startchar)
            bwstr = stats[startchar+len(statname2):stats.find('#', startchar)]
            misses += int(bwstr)
            startchar = stats.find(statname3, startchar)
            bwstr = stats[startchar+len(statname3):stats.find('#', startchar)]
            hits += int(bwstr)
            startchar = stats.find(statname1, startchar)

    if hits + misses > 0:
        return float(hits)/(hits + misses)
    else:
        return 0

def getReplaceRate(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        accesses = 0
        replaces = 0
        stats = statsfile.read()
        statname1 = "storageReadForRepl"
        statname2 = "storageCleanWrite"
        statname3 = "storageDirtyWrite"
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        cpunum = 0
        startchar = stats.find(statname1, startchar)
        while startchar >= 0:
            bwstr = stats[startchar+len(statname1):stats.find('#', startchar)]
            replaces += int(bwstr)
            startchar = stats.find(statname2, startchar)
            bwstr = stats[startchar+len(statname2):stats.find('#', startchar)]
            accesses += int(bwstr)
            startchar = stats.find(statname3, startchar)
            bwstr = stats[startchar+len(statname3):stats.find('#', startchar)]
            accesses += int(bwstr)
            startchar = stats.find(statname1, startchar)

    if accesses > 0:
        return float(replaces)/(accesses)
    else:
        return 0

def getColdMissRate(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        accesses = 0
        cold = 0
        stats = statsfile.read()
        statname1 = "storage_ctrl.hits"
        statname2 = "storage_ctrl.misses"
        statname3 = "storage_ctrl.coldMisses"
        statname4 = "storage_ctrl.coldReplaces"
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        cpunum = 0
        startchar = stats.find(statname1, startchar)
        while startchar >= 0:
            bwstr = stats[startchar+len(statname1):stats.find('#', startchar)]
            accesses += int(bwstr)
            startchar = stats.find(statname2, startchar)
            bwstr = stats[startchar+len(statname2):stats.find('#', startchar)]
            accesses += int(bwstr)
            startchar = stats.find(statname3, startchar)
            bwstr = stats[startchar+len(statname3):stats.find('#', startchar)]
            cold += int(bwstr)
            startchar = stats.find(statname4, startchar)
            bwstr = stats[startchar+len(statname4):stats.find('#', startchar)]
            cold += int(bwstr)
            startchar = stats.find(statname1, startchar)

    if accesses+cold > 0:
        return float(cold)/(accesses+cold)
    else:
        return 0



def getRealReplaceRate(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        accesses = 0
        replaces = 0
        stats = statsfile.read()
        statname1 = "realReplaceMiss"
        statname2 = "storageReadForRepl"
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        cpunum = 0
        startchar = stats.find(statname1, startchar)
        while startchar >= 0:
            bwstr = stats[startchar+len(statname1):stats.find('#', startchar)]
            replaces += int(bwstr)
            startchar = stats.find(statname2, startchar)
            bwstr = stats[startchar+len(statname2):stats.find('#', startchar)]
            accesses += int(bwstr)
            startchar = stats.find(statname1, startchar)

    if accesses+replaces > 0:
        return float(replaces)/(accesses)
    else:
        return 0

def getL3MissSize(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        bandwidth = 0
        stats = statsfile.read()
        statname = 'l3cache.banks%02d.overall_misses::total'
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        cpunum = 0
        startchar = stats.find(statname % cpunum, startchar)
        while startchar >= 0:
            bwstr = stats[startchar+len(statname):stats.find('#', startchar)]
            bandwidth += int(bwstr)
            cpunum += 1
            startchar = stats.find(statname % cpunum, startchar + 1)

    return bandwidth * float(64) / 2**30

def getMembusBW(dumps = 0):
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        bandwidth = 0
        stats = statsfile.read()
        statname = 'membus.pkt_size::total'
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        startchar = stats.find(statname, startchar)
        num = 0
        while startchar >= 0:
            num += 1;
            bwstr = stats[startchar+len(statname):stats.find('#', startchar)]
            bandwidth += int(bwstr)
            startchar = stats.find(statname, startchar + 1)
    return bandwidth

def getBottleneck(dumps = 0):
    import os
    import re
    outdir = m5.options.outdir
    pattern = ".*utilization +(?:9[0-9].[0-9]|100)"
    with open(os.path.join(outdir, 'stats.txt')) as statsfile:
        stats = statsfile.read()
        startchar = 0
        for i in range(dumps+1):
            startchar = stats.find('Begin Simulation Statistics',
                                    startchar + 1)
        stats = stats[startchar:]
        results = re.findall(pattern, stats)
        return [result.split(' ')[0] for result in results]

def deleteStats():
    import os
    outdir = m5.options.outdir
    with open(os.path.join(outdir, 'stats.txt'), "w"):
        pass

if __name__ == "__m5_main__":

    (opts, args) = SimpleOpts.parse_args()

    assert len(args) == 1

    # create the system we are going to simulate
    system = getSystem(args[0], opts)

    root = Root(full_system = False, system = system)
    root.system.mem_mode = 'timing'

    m5.instantiate()

    # warmup_time = '10ms'
    # print "Warming up for", warmup_time
    # exit_event = m5.simulate(fromSeconds(toLatency(warmup_time)))
    #
    # if exit_event.getCause().startswith("user"):
    #     print "exit for", exit_event.getCause()
    #     exit(1)
    #
    # m5.stats.reset()

    # sim_time = '20ms'
    # print "simulating for", sim_time
    # exit_event = m5.simulate(fromSeconds(toLatency(sim_time)))
    #
    # m5.stats.dump()
    # print "Bandwidth at MC:", getBandwidth()/float(2**30), "GB/s"
    # genbw = getGenBW() / toLatency(sim_time) * 64.0 / 2**30
    # print "Bandwidth at generators:", genbw, "GB/s"
    # print "Bottlenecks:", ", ".join(getBottleneck())
    # print "Hit rate:", getHitRate()
    # print "L3 miss bandwidth:", getL3MissSize()/ toLatency(sim_time)
    # print "Membus bandwidth:", getMembusBW() / 2**30 / toLatency(sim_time)
    #
    # print "mc_bw, gen_bw, hit_rate, l3_bw, membus_bw"
    # print getBandwidth()/float(2**30),
    # print genbw,
    # print getHitRate(),
    # print getL3MissSize()/ toLatency(sim_time),
    # print getMembusBW() / 2**30 / toLatency(sim_time)

    warmup_time = "50ms"
    sim_time = "10ms"

    print "simulating for", warmup_time
    exit_event = m5.simulate(fromSeconds(toLatency(warmup_time)))
    m5.stats.reset()

    print "simulating for", sim_time
    exit_event = m5.simulate(fromSeconds(toLatency(sim_time)))

    i = 0
    m5.stats.dump()
    print "Bandwidth at MC:", getBandwidth(i)/float(2**30), "GB/s"
    genbw = getGenBW(i, opts) / toLatency(sim_time) * 64.0 / 2**30
    print "Bandwidth at generators:", genbw, "GB/s"
    print "Bottlenecks:", ", ".join(getBottleneck(i))
    print "Read Hit rate:", getHitRate(i)
    print "Replace rate:", getReplaceRate(i)
    print "Real replace rate:", getRealReplaceRate(i)
    print "L3 miss bandwidth:", getL3MissSize(i)/ toLatency(sim_time)
    print "Cold miss rate:", getColdMissRate(i)
    print "Membus bandwidth:",
    print getMembusBW(i) / 2**30 / toLatency(sim_time)

    # if "dramcache" in args[0]:
    #     iters = 100
    # else:
    #     iters = 5
    # sim_time = '1ms'
    # for i in range(iters):
    #     print "simulating for", sim_time
    #     exit_event = m5.simulate(fromSeconds(toLatency(sim_time)))
    #
    #     if exit_event.getCause().startswith("user"):
    #         break
    #
    #     m5.stats.dump()
    #     m5.stats.reset()
    #     print "Bandwidth at MC:", getBandwidth(i)/float(2**30), "GB/s"
    #     genbw = getGenBW(i, opts) / toLatency(sim_time) * 64.0 / 2**30
    #     print "Bandwidth at generators:", genbw, "GB/s"
    #     print "Bottlenecks:", ", ".join(getBottleneck(i))
    #     print "Read Hit rate:", getHitRate(i)
    #     print "Replace rate:", getReplaceRate(i)
    #     print "Real replace rate:", getRealReplaceRate(i)
    #     print "L3 miss bandwidth:", getL3MissSize(i)/ toLatency(sim_time)
    #     print "Cold miss rate:", getColdMissRate(i)
    #     print "Membus bandwidth:",
    #     print getMembusBW(i) / 2**30 / toLatency(sim_time)
    #
    #     if "dramcache" in args[0] and getColdMissRate(i) < 0.02:
    #         break
