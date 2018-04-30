
specint = [
    #'perlbench',
    'bzip2',
    'gcc',
    'mcf',
    'gobmk',
    'hmmer',
    'sjeng',
    'libquantum',
    'h264ref',
    'omnetpp',
    'astar',
    #'xalancbmk',
]

specfloat = [
    'bwaves',
    'gamess',
    'milc',
    'zeusmp',
    'gromacs',
    'cactusADM',
    'leslie3d',
    'namd',
    #'dealII',
    #'soplex',
    'povray',
    'calculix',
    'GemsFDTD',
    'tonto',
    'lbm',
    #'wrf',
    'sphinx3',
]

specall = specint + specfloat

script_template = """
cd /home/gem5/cpu2006
source shrc
/sbin/gem5 exit
runspec --config=myconfig.cfg --action=run --size=|size| --nobuild \
        --loose --iterations=1 |workload| >simstdout 2>simstderr

/sbin/gem5 writefile simstdout simstdout
/sbin/gem5 writefile simstderr simstderr
"""

filename = 'script'

instructions = {
    'namd':          2275940938540,
    'lbm':           1262529648192,
    'calculix':      6181626640819,
    'h264ref':       3999293582712,
    'gobmk':         1816345974941,
    'astar':         1117056022229,
    'GemsFDTD':      1503515054422,
    'bzip2':         2300049525853,
    'gamess':        1594844781515,
    'sjeng':         2460087123612,
    'tonto':         2868298101287,
    'gromacs':       2044424399277,
    'mcf':           315670374404,
    'zeusmp':        1829356934481,
    'cactusADM':     2682853422935,
    'povray':        1053671684358,
    'libquantum':    1665536392630,
    'milc':          1151485008949,
    'gcc':           1131367048072,
    'leslie3d':      1489181037660,
    'sphinx3':       3161855615367,
    'omnetpp':       593893918278,
    'bwaves':        1961040745572,
    'hmmer':         270929512278,
}

def make_script(workload, size='ref'):
    import random
    import string
    extra = ''.join([random.choice(string.letters) for i in range(5)])
    global filename
    filename += extra

    if workload not in specall:
        print("Invalid workload {}".format(workload))
        print("Valid workloads: {}".format(specall))
        from sys import exit
        exit(1)


    script = script_template.replace(
                '|workload|', workload).replace(
                '|size|', size)

    with open(filename, 'w') as f:
        f.write(script)
    return filename

def rm_script():
    from os import remove
    remove(filename)
