
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
