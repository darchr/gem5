# Copyright (c) 2023-25 The Regents of the University of California
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

"""
This python script is a unified script that allows executing multiple gem5
nodes to run and take checkpoints and then restore them in SST.
"""

import os
import sys
import json
import time
import argparse
import traceback
import subprocess

from typing import List

# All utility functions are defined here.

# ----------------------- start of all utility functions ---------------------#
def fatal(reason: str) -> None:
    """
    A simple function to print an error message and exit the program.

    @params
    :reason: a string to print before exiting.
    """
    print("fatal: " + reason)
    exit(-1)

def warn(reason: str) -> None:
    """
    A simple function to print a warning message and continue the program.

    @params
    :reason: a string to print as a warning.
    """
    print("warn: " + reason)

def info(reason: str) -> None:
    """
    A simple function to print an information relavant to the simulation.

    @params
    :reason: a string to print prefixed with \"info\"
    """
    print("info: " + reason)

def check_path(path: str): # -> List[str, bool]:
    """
    A function to return the absolute path for a given relative path provided.
    This function does not check if the path exists. It returns a dictionary
    with the relevant information.

    @params
    :path: an absolute or relative path provided.

    @returns
    :dict: {path: <absolute/path>, exists: <boolean/whether/the/path/exists>}
    """
    full_path = ""
    if path[0] == '/':
        full_path = path
    else:
        full_path = os.path.join(os.getcwd(), path)
    return {"path": full_path, "exist": os.path.exists(full_path)}

def check_binaries(gem5_binary: str, sst_binary: str) -> None:
    """This function checks the presence of the gem5 and SST binaries in the
    system. It exits if either is not found.
    
    @params
    :gem5_binary: the compiled gem5 source may have an ISA specified."""
    if gem5_binary[0] != "/":
        # append current directory information in the check
        if os.path.exists(os.path.join(os.getcwd(), gem5_binary)) == False:
            fatal("gem5 path " + gem5_binary + " does not exist!")
    else:
        # The user provided a full path starting with /.
        if os.path.exists(gem5_binary) == False:
            fatal("gem5 path " + gem5_binary + " does not exist!")
    if sst_binary[0] != "/":
        # append current directory information in the check
        if os.path.exists(os.path.join(os.getcwd(), sst_binary)) == False:
            fatal("SST path " + sst_binary + " does not exist!")
    else:
        # The user provided a full path starting with /.
        if os.path.exists(sst_binary) == False:
            fatal("SST path " + sst_binary + " does not exist!")

# We need a utility function to convert MiB and GiB regions
def util_size_to_int(size: str) -> int:
    """
    A function to convert size as a string to integer.
    Limitations: only works for MiB and GiB sizes.
                Assumes everything to be in the powers of 2.
                
    @params
    :size: a string with size
    
    @returns
    :int: converted size in integer.
    """
    if "GiB" in size or "MiB" in size:
        quantity = 0x0
        number = 0
        if "GiB" in size:
            # This size is in GiBs
            quantity = 0x40000000
            number = int(size.split("G")[0])
        else:
            quantity = 0x100000
            number = int(size.split("M")[0])
        return quantity * number
    else:
        fatal("Could not convert size. It's either in KiB or 1024 multiple " +
              "in terms of iB is not specified.")

# ------------------------ end of all utility functions ----------------------#

# make sure that the user is running this script using python and NOT gem5 or
# SST.
if "python" not in sys.executable:
    fatal("Run this script using python!\nA top-level interface is required" +\
          " to connect two different softwares without creating a library" + \
          "for both. This may be done in the future.")

# Write a parser function for parsing the joblist. Hosts can be variable.
def create_joblist(count: int, ff_core: str, roi_core: str, isa: str,
                    local_memory_size: str, remote_memory_size: str,
                    shared_memory_enable: bool,
                    experiment_name: str,
                    shared_memory_addr: List = None) -> bool:
    """
    This function will create a on-the-fly joblist for configuring all the
    different hosts and the devices. This function should only be called when
    an explicit joblist is not provided.
    
    @params
    :count: The number of gem5 nodes to simulate
    :ff_core: Specify the fast-forwarding core to take a checkpoint
    :roi_core: Specify the ROI core to restore a checkpoint
    :isa: Specify which ISA to use. Must be uniform
    :local_memory_size: Specify a size to the local memory to simulate
    :remote_memory_size: Similarly specify one for the remote meomry
    :shared_memory_enable: Specify if you want to simulate shared memory
    :shared_memory_addr: Specify a start and end address as a list"""

    # Okay, this function is not as easy as it seems!
    jobs = {}
    # First parse the remote memory start and ends.
    start = []
    end = []
    if shared_memory_enable == True:
        # Great! There is only one address range and we do not support
        # multiple shared memory ranges across disfferent sets of nodes rn.
        for nodes in range(count):
            start.append(shared_memory_addr[0])
            end.append(shared_memory_addr[1])
    else:
        # Oh no!
        local_memory_size_in_int = util_size_to_int(remote_memory_size)
        remote_memory_size_in_int = util_size_to_int(remote_memory_size)
        if isa == "x86":
            # This is problematic. We hard code the start of the remote memory
            # to 4 GiB.
            start_in_x86 = 0x100000000
            for node in range(count):
                start.append(start_in_x86 + node * remote_memory_size_in_int)
                end.append(start_in_x86 + (node + 1) * \
                            remote_memory_size_in_int)
        else:
            # First there is an I/O hole from 0 - 2GiB.
            blank_memory_size_in_int = 0x80000000
            # Now, the next region is allocated to the local memory.
            # print(type(blank_memory_size_in_int), type(node))
            for node in range(count):
                start.append(blank_memory_size_in_int + (node + 1) * \
                            local_memory_size_in_int + (node) * \
                            (remote_memory_size_in_int))
                end.append(blank_memory_size_in_int + (node + 1) * \
                            local_memory_size_in_int + (node) * \
                            (remote_memory_size_in_int) + \
                            remote_memory_size_in_int)

    for node in range(count):

        jobs[node] = {
            "metadata": {
                "comment": "This is a simple uniform job creator!",
                "experiment": experiment_name,
            },
            "cpu": {
                "ff-core": ff_core,
                "roi-core": roi_core,
                "isa": isa,
                "count": ""
            },
            "cache": {
                "type": "",
                "l1i-size": "",
                "l1d-size": "",
                "l2-size": "",
                "l3-size": "",
                "l3-assoc": ""
            },
            "local-memory": {
                "type": "",
                "size": local_memory_size
            },
            "remote-memory": {
                "size": "",
                "shared": str(shared_memory_enable),
                "start": hex(start[node]),
                "end": hex(end[node])
            },
            "workitem": {
                "cmd": [],
                "disk": "",
                "kernel": "",
                "bootloader": ""
            }
        }
    return jobs

def parse_csvlist(jobfile: str):
    """A simple csv parser. A json parser might be better IMO."""

    warn("A CSV format is dangerous if not formatted properly! This feature" +
        " is disabled for now and there are no plans to fix this in the " +
        "near future! Use a JSON file instead to define the systems and " +
        "workloads.")

    raise NotImplementedError

    f = open(check_path(jobfile)["path"], "r")
    for idx, lines in enumerate(f.read().split("\n")):
        params = lines.split(",")
        # The node id must be the same as the first paramter. This makes
        # sure that the count is right!
        assert(str(idx) == params[0])

        # Format the jsons.
        jobs[str(idx)] = {}
        jobs[str(idx)]["cpu"] = {"ff-core": "", "roi-core": "", "isa": "",
                                  "count": ""}
        jobs[str(idx)]["cache"] = {"type": "", "l1i-size": "",
                                "l1d-size": "", "l1i-assoc": "",
                                "l1d-assoc": "", "l2-size": "",
                                "l2-assoc": "", "l3-size": "",
                                "l3-assoc": ""}
        jobs[str(idx)]["local-memory"] = {"type": "", "size": ""}
        jobs[str(idx)]["remote-memory"] = {"size": "", "start": "",
                                        "end": ""}
        jobs[str(idx)]["workitem"] = {"cmd" : [], "disk": "", 
                                            "kernel": "", "bootloader": ""}
        # parse each of the words now!
        jobs[str(idx)]["cpu"]["type"] = params[1]
        jobs[str(idx)]["cpu"]["isa"] = params[2]
        jobs[str(idx)]["cpu"]["count"] = params[3]

        jobs[str(idx)]["cache"]["type"] = params[4]
        jobs[str(idx)]["cache"]["l1i-size"] = params[5]
        jobs[str(idx)]["cache"]["l1d-size"] = params[6]
        jobs[str(idx)]["cache"]["l2-size"] = params[7]
        jobs[str(idx)]["cache"]["l3-size"] = params[8]
        jobs[str(idx)]["cache"]["l3-assoc"] = params[9]

        jobs[str(idx)]["local-memory"]["type"] = params[10]
        jobs[str(idx)]["local-memory"]["size"] = params[11]
        
        jobs[str(idx)]["remote-memory"]["size"] = ""
        jobs[str(idx)]["remote-memory"]["shared"] = params[12]
        jobs[str(idx)]["remote-memory"]["start"] = params[13]
        jobs[str(idx)]["remote-memory"]["end"] = params[14]

        # the command list will be split into multiple commands. The script
        # will NOT add m5 exits automatically!
        for cmds in params[15].split(";"):
            jobs[str(idx)]["workitem"]["cmd"].append(cmds)

        jobs[str(idx)]["workitem"]["disk"] = params[16]
        jobs[str(idx)]["workitem"]["kernel"] = params[17]
        jobs[str(idx)]["workitem"]["bootloader"] = params[18]
        
        # Check if the resources exists
        if check_path(jobs[str(idx)]["workitem"]["disk"])["exist"] \
                == False:
            fatal("Disk image " +
                    check_path(jobs[str(idx)]
                                ["workitem"]["disk"])["path"] +
                    " for node " + str(idx) + " does not exist!")
        if check_path(jobs[str(idx)]["workitem"]["kernel"])["exist"] \
                == False:
            fatal("Kernel " +
                    check_path(jobs[str(idx)]
                                ["workitem"]["kernel"])["path"] +
                    " for node " + str(idx) + " does not exist!")
        # Bootloader is optional as this does not exist for X86
    f.close()
    return jobs

def parse_json(jobfile: str):
    f = open(jobfile, "r")
    jobs = json.loads(f.read())
    f.close()
    # print(jobs.keys())
    for idx, nodes in enumerate(jobs):
        # Check if the resources exists
        if check_path(jobs[nodes]["workitem"]["disk"])["exist"] \
                == False:
            fatal("Disk image " +
                    check_path(jobs[nodes]
                                ["workitem"]["disk"])["path"] +
                    " for node " + str(idx) + " does not exist!")
        if check_path(jobs[nodes]["workitem"]["kernel"])["exist"] \
                == False:
            fatal("Kernel " +
                    check_path(jobs[nodes]
                                ["workitem"]["kernel"])["path"] +
                    " for node " + str(idx) + " does not exist!")
        # Bootloader is optional as this does not exist for X86
    return jobs

parser = argparse.ArgumentParser()

# this is the complicated part where we might have multiple instances of gem5
# either running the same or different application in the guest system. Each of
# the instances may have a separate or the same physical address range. Users
# can use a joblist.csv file to specify jobs for each of the gem5 node in case
# they want to execute different jobs.

parser.add_argument(
    "--count",
    "-c",
    type=int,
    required=True,
    help="Specify the number of hosts to simulate. The simulation will begin" +
        " in gem5 until a checkpoint is taken. The checkpoint will then be " +
        "restored in SST."
)

parser.add_argument(
    "--experiment",
    "-e",
    type=str,
    required=False,
    default="",
    help="Specify a name for the experiment. If not provided, then the name " +
            "is automatically assigned as the current timestamp."
)

parser.add_argument(
    "--clock",
    "-f",
    type=str,
    required=False,
    default="4GHz",
    help="Specify the clock frequency of the ysstem to simulate. Must be the" +
            "for all the hosts."
)

parser.add_argument(
    "--fast-forward-core",
    type=str,
    required=False,
    default="kvm",
    help="Specify the fast forwarding core. By default, it is kvm.",
    choices=["atomic", "kvm"]
)

# This script should not be used for debugging! Limiting the available options
# to the user for simulating ff and roi cores.
parser.add_argument(
    "--roi-core",
    type=str,
    required=False,
    default="o3",
    help="Specify the fast forwarding core. By default, it is kvm.",
    choices=["timing", "o3"]
)
parser.add_argument(
    "--isa",
    type=str,
    required=False,
    default="arm",
    help="Specify the ISA to use. This will be uniform across all the " +
        "different hosts. To simulate multi-ISA configuration, see how to " +
        "use a job list by specifying --joblist or --joblist-help",
    choices=["arm", "riscv", "x86"]
)
# Cores will be hardcoded to 8 when simulating identical hosts.

# The user cannot change the cache hierarchy when simulating identical hosts
# from the parent script. This is done to simplify the front-end.

# Local memory is fixed in size so there aren't a lot of things to do.
parser.add_argument(
    "--local-memory-size",
    "-l",
    type=str,
    required=False,
    default="2GiB",
    help=""
)

# The remote memory can either be allocated using size or an address range.
# All the different cases are handled in the main function. Only one can be
# used to initialize the memory. Address ranges will be automatically created
# when using this option.
parser.add_argument(
    "--remote-memory-size",
    "-r",
    type=str,
    required=False,
    default="0GiB",
    help="Specify a remote memory size. This will create a uniform " +
            "same-sized allocation on the remote memory node. CANNOT be used" +
            " with --variable-remote-memory."
)

# In case there is a shared memory range, it has to be specified as an argument
# This can also be done using a joblist. The difference between this and a
# joblist is that this creates identical hosts mapped to the same region.
parser.add_argument(
    "--shared-memory-addr",
    "-s",
    type=str,
    required=False,
    default=None,
    help="Specify a remote memory address range. MUST be used only if there " +
            "a SHARED memory range."
)
# The user can set the variable memory range True, however, a joblist MUST be
# specified in that case.
parser.add_argument(
    "--variable-remote-memory",
    "-x",
    choices=["True", "False"],
    required=False,
    default="False",
    type=str,
    help="Use variable sized remote memory allocation. Similar to memory " +
            "pooling. REQUIRES a --joblist to be specified and IGNORES " +
            "--remote-memory-size."
)

# For simplicity, the user can define a full joblist instead of creating
# homogeneous machines. Each host is fully customizable (includes most of the
# parameters present in the standard library).
parser.add_argument(
    "--joblist",
    "-j",
    type=str,
    required=False,
    default=None,
    help="To run different applications on different hosts, the user needs " +
        "to specify a joblist in csv format: <host_instance>," +
        "<main command_to_execute>,<address_range_start>," +
        "<address_range_end>\\n An example of the same is:\"0,/home/ubuntu/" +
        "arm-bench/npb-hooks/NPB3.4.2/NPB3.4-OMP/bin/bt.A.x,0x100000000," +
        "0x180000000\\n1,/home/ubuntu/arm-bench/npb-hooks/NPB3.4.2/NPB3" + 
        ".4-OMP/bin/cg.D.x,0x180000000,0x1c0000000\""
)

# Similar to providing --outdir in gem5, all the outputs of the experiment for
# both gem5 and SST will be stored in this directory.
parser.add_argument(
    "--exp-name",
    "-m",
    type=str,
    required=True,
    help="The user needs to provide a name for the experiment. All the stats" +
        " and checkpoints will be stored in this directory."
)

# Path to gem5
parser.add_argument(
    "--gem5",
    "-g",
    type=str,
    required=False,
    default="ALL",
    help="Optionally specify which gem5 binary to use. Useful when using " +
            "a different ISA or RUBY protocol. Expects gem5 to have .opt " +
            "compiled. Otherwise provide full path."
)

# Path to SST.
parser.add_argument(
    "--sst",
    "-t",
    type=str,
    required=False,
    default="ext/sst/bin/sst",
    help="Optionally specify which SST binary to use. By default, the " +
        "script assumes that SST is compiled in ext/sst directory."
)

# An option to skip gem5 simulation and let SST start from an already stored
# checkpoint.
# TODO: Another option is needed to specify if gem5 only needs to take check-
# points and skip SST altogether.
parser.add_argument(
    "--checkpoints",
    "-k",
    type=str,
    required=False,
    choices=["True", "False"],
    default="True",
    help="Optionally specify if SST needs to resume from a checkpoint or has" +
        " to do the full simulation."
)

# Finally add an option to simulate the disk image with or without systemd
# This needs to be a global feature.
parser.add_argument(
    "--systemd",
    type=str,
    required=False,
    choices=["True", "False"],
    default="True",
    help="An option to simulate the system with or without systemd"
)

args = parser.parse_args()
# prepare the gem5 and SST paths
gem5_binary = ""
sst_binary = ""
if args.gem5[0] != "/":
    gem5_binary = "build/" + args.gem5 + "/gem5.opt"
else:
    gem5_binary = args.gem5

sst_binary = args.sst

# Put all the fatal cases before proceeding any further. First check for all
# the binaries.
check_binaries(gem5_binary, sst_binary)

# convert True/False strings into boolean.
variable_memory =  {"True": True, "False": False}[args.variable_remote_memory]
checkpoints =  {"True": True, "False": False}[args.checkpoints]
# This needs to be a string to string conversion.
systemd = {"True": "true", "False": "false"}[args.systemd]

# Put the absolute path to the output directory
experiment_path = os.path.join(os.getcwd(), args.exp_name)

# create the path if necessary
if os.path.exists(experiment_path) == False:
    os.mkdir(experiment_path)

# Store the entire joblist into one dictionary to start all the processes. In
# the case where the user did not supply a joblist, we'll create one on-the-fly
jobs = {}
joblist = False
need_addr_ranges = False

# See if there are conflicting options.
if variable_memory is True:
    # A joblist must be specified.
    if args.joblist is not None:
        fatal("A joblist was not specified to simulate variable remote memory")
    joblist = True
    # If uniform remote memory size is specified
    if args.remote_memory_size != "0GiB":
        warn("Ignoring remote memory size provided. Instead using --joblist")
    # check if shared range is enabled. If so then warn the user that it'll
    # get ignored and the joblist will override that.
    if args.shared_memory_addr is not None:
        warn("Conflicting option --shared-memory-addr. --joblist will " +
             "override this memory range.")
    
    # All options checked!

    # Now parse the joblist and create a map in the memory as a JSON file.

else:
    # This should be simple. First check if a joblist is specified. This will
    # override if a uniform memory is supplied.
    if args.joblist is not None:
        joblist = True
    # Now see if the user wants to simulate with a uniform memory range.
    if args.remote_memory_size != "0GiB":
        if joblist == True:
            warn("Conflicting options: --joblist and --remote-memory-size. " +
                 " the --joblist will override the --remote-memory-size.")
        else:
            need_addr_ranges = True
    # Next up is shared memory.
    if args.shared_memory_addr is not None:
        if joblist == True:
            warn("Conflicting options: --joblist and --shared-memory-addr. " +
                 " the --joblist will override the --shared-memory-addr.")
    
    # All options checked!

    # Now parse the joblist and create a map in the memory as a JSON file.

if joblist == True:
    # Make sure to notify the user all the parameters that will be overridden
    # since they specified a joblist
    warn("Since a --joblist is provided, the following command line paramters"
         "will be ignored: --fast-forward-core, --roi-core," +
         " --local-memory-size, --remote-memory-size, --shared-memory-addr")
    
    # the user says that they have provided a joblist. check it out first.
    if check_path(args.joblist)["exist"] == False:
        fatal(args.joblist + " does not exist!")
    else:
        # parse the joblist into a dict/JSON and make all the processes ready
        # to be launched.

        # We should be able to switch between csv and JSON freely. The user
        # needs to specify the file type.
        # create a JSON from the csv. The problem with csv is that it is very
        # hard-defined and enforces all of checks.
        if ".csv" in args.joblist:
            jobs = parse_csvlist(args.joblist)
        elif ".json" in args.joblist:
            jobs = parse_json(args.joblist)
        else:
            fatal("Please specify a file type for the joblist!")
else:
    # We need to create our own joblist. We'll mostly use default parameters.
    ff_core = args.fast_forward_core
    roi_core = args.roi_core
    isa = args.isa

    # core_count is hardcoded and is directly used in the gem5-side script
    # cache hierarchy is hardcoded and is directly used in the gem5-side script

    # Local memory will be hardcoded to use DDR4 
    local_memory = args.local_memory_size
    remote_memory = args.remote_memory_size

    shared_memory = False
    if args.shared_memory_addr is not None:
        shared_memory = True

    if remote_memory == "0GiB":
        # see if there is a shared memory specified.
        if shared_memory == False:
            fatal("Please specify a --remote-memory-size to simulate!")
    
    # Work items and the commands are hardcoded to the gem5-side script. This
    # includes cmds, disk, kernel and optionally the bootloader.

    jobs = create_joblist(args.count, ff_core, roi_core, isa, local_memory,
                        remote_memory, shared_memory, args.experiment,
                        args.shared_memory_addr)


# Jobs should be ready at this point!!

# make sure that the size of the jobs is same as --count
# Make sure that the number of simulation matches the number of jobs specified.
assert(args.count == len(jobs))

# -------------------------- All jobs ready ----------------------------------#

# Now simply launch a new gem5 thread for each of the instances. Make sure that
# the output directory is stored in an array for resuming
output_directories = []

# If the user does not want to use gem5 to fast forward, then start the
# simualtion directly in SST!
if checkpoints == True:

    # These many gem5 processes needs to be created!
    gem5_processes = []
    for job in jobs:
        # depending upon shared memory, the config script will change (the
        # board changes behind the scene). Will be read directly
        # Next up is the ISA. Are we running this process from the gem5
        # directory?
        p_config = os.getcwd()
        if os.path.exists(
                os.path.join(p_config, "disaggregated_memory")) == False:
            # Force the user to restart the process!
            fatal("Please run this script from the top gem5 directory.")

        # Make sure to handle exception for malformed JSONs
        try:
            if jobs[job]["cpu"]["isa"].lower() == "arm":
                p_config = os.path.join(os.getcwd(),
                                "disaggregated_memory/configs/arm_unified.py")
            elif jobs[job]["cpu"]["isa"].lower() == "riscv":
                p_config = os.path.join(os.getcwd(),
                            "disaggregated_memory/configs/riscv_unified.py")
            elif jobs[job]["cpu"]["isa"].lower() == "x86":
                p_config = os.path.join(os.getcwd(),
                                "disaggregated_memory/configs/x86_unified.py")
            else:
                fatal("Unsupported ISA!")
        except KeyError:
            fatal("Malformed JSON! \"isa\" for the \"cpu\" is undefined!")
        except:
            traceback.print_exc()
            fatal("Unhandled exception while looking for ISA.")
        
        # The user needs to supply an experiment name to make sure that there
        # is a unique name in the outdir. If not, then put the name as
        # experiment as the timestamp.
        p_experiment = experiment_path
        if jobs[job]["metadata"]["experiment"] == "":
            # The user needs to know this.
            timestamp = str(time.time())
            info("The experiment results will be stored in " + timestamp)
            # Temporarily store the path in the json in memory. This will not
            # require us to supply the path of the experiments to SST.
            jobs[job]["metadata"]["experiment"] = timestamp
        
        # this will never have KeyError
        p_experiment = os.path.join(p_experiment,
                                    jobs[job]["metadata"]["experiment"])
        
        output_directories.append(p_experiment)

        # Ready to launch gem5?
        # Still need a exception handler for missing KeyErrors
        try:
            gem5_processes.append(subprocess.Popen([gem5_binary,
                        "-re",
                        "--outdir=" + p_experiment + "_" + str(job),
                        p_config,
                        "--instance=" + str(job),
                        "--ff-core-type=" + jobs[job]["cpu"]["ff-core"],
                        "--roi-core-type=" + jobs[job]["cpu"]["roi-core"],
                        "--core-count=" + jobs[job]["cpu"]["count"],
                        # The clock is fixed to the same frequency.
                        "--core-frequency=" + args.clock,
                        "--cache-type=" + jobs[job]["cache"]["type"],
                        "--l1i-size=" + jobs[job]["cache"]["l1i-size"],
                        "--l1d-size=" + jobs[job]["cache"]["l1d-size"],
                        "--l2-size=" + jobs[job]["cache"]["l2-size"],
                        "--l3-size=" + jobs[job]["cache"]["l3-size"],
                        "--l3-assoc=" + jobs[job]["cache"]["l3-assoc"],
                        "--local-memory-type=" + jobs[job]["local-memory"]
                                            ["type"],
                        "--local-memory-size=" + jobs[job]["local-memory"]
                                            ["size"],
                        "--remote-memory-shared=" + jobs[job]["remote-memory"]
                                                            ["shared"].lower(),
                        "--remote-memory-start=" + str(jobs[job]
                                                ["remote-memory"]["start"]),
                        "--remote-memory-end=" + str(jobs[job]["remote-memory"]
                                                ["end"]),
                        # When running gem5 processes, we have to specify
                        # --is-composable as False as we don't use SST here.
                        "--is-composable=false",
                        "--cmd=" + "__fmt__".join(
                                                jobs[job]["workitem"]["cmd"]),
                        "--disk-path=" + jobs[job]["workitem"]["disk"],
                        "--kernel-path=" + jobs[job]["workitem"]["kernel"],
                        "--bootloader-path=" + jobs[job]["workitem"]
                                                        ["bootloader"],
                        "--systemd=" + systemd
                        ]))
        except KeyError:
            traceback.print_exc()
            fatal("Malformed JSON!")
        except:
            traceback.print_exc()
            fatal("Unhandled exception while launching gem5 processes!")

    # wait for all the processes! The user can be impatient (like me) or see an
    # error and kill the simulation.
    try:
        while True:
            for process in gem5_processes:
                process.wait()
            break
            print("after gem5 wait")
            continue
    except KeyboardInterrupt:
        # Make sure that the process that started is killed as well. The down-
        # side to this is that innocent gem5 processes will also get killed!
        # FIXME
        os.system("killall -9 gem5.opt")
        fatal("Simulation ended by user. All gem5.opt processes are killed!")
else:
    # Turns out that checkpoints are already taken and simulation can skip gem5
    info("No checkpoints to take! Will start SST directly.")

# -------------------------- All gem5s done ----------------------------------#

# Inform the user that gem5 has ended!
info("All gem5 processes completed! Will start SST now!")

# SST has one more process than the number of gem5 nodes for the memory.
sst_processes = args.count + 1

# Processes are already created when saving the checkpoint. The only
# information SST needs is the experiment name, memory sizes and instance ids.
# Workitems will load the checkpoint?
sst_workitems = []
sst_remote_memory_sizes = []

for job in jobs:
    sst_workitems.append(jobs[job]["metadata"]["experiment"])
    sst_remote_memory_sizes.append(
        int(jobs[job]["remote-memory"]["end"], 16) -
        int(jobs[job]["remote-memory"]["start"], 16)
    )
    if jobs[job]["cpu"]["isa"].lower() == "arm" or \
            jobs[job]["cpu"]["isa"].lower() == "riscv":
        sst_blank_memory_range = 0x80000000
    elif jobs[job]["cpu"]["isa"].lower() == "x86":
        sst_blank_memory_range = 0x100000000
    else:
        # Although gem5 supports other ISAs like SPARC, ALPHA etc, we do not
        # support FS simulation of the same in the gem5 + SST infrastructure.
        fatal("unknown ISA detected while resuming the work in SST!")
    
# The total memory size will be passed as an integer
sst_total_memory_size = sum(sst_remote_memory_sizes)

# How to start MPI processes from this script?
# Since most of the parameters will be sent from SST, we need to pass the json
# directly to SST. Create a temporary json with gem5 processes.
jobs_json = os.path.join(os.getcwd(), os.path.join(args.exp_name,
                                                 "job_config.json"))
# Get another path for storing the mpi output for the experiment. This is
# useful to note down the maximum memory consumed during the simulation and
# also the host's total execution time.
mpi_stats_path = os.path.join(os.getcwd(), os.path.join(args.exp_name,
                                                        "sst-mpistats.txt"))
# The temporary job file is created in the experiment directory. This can be
# useful when debugging the connection between the two simulators.
with open(jobs_json, "w") as outfile: 
    json.dump(jobs, outfile)

# The os needs to move paths as gem5component exists in the SST directory
os.chdir("ext/sst")
# check if gem5 process config is set
sst_config = os.path.join(os.getcwd(), "sst/unified_sst.py") 

# Finally, if we are doing a simulation event up to a certain time, then SST
# must end the specified time. The JSON config will have this value.
until_when = "0s"
# This must be in the metadata of the simulation. We need to simulate all the
# systems until that point to have an accurate simulation stats!
try:
    if jobs[0]["metadata"]["max-time"] != "":
        # There is a value! Must be in unit time (i.e. s)
        until_when = jobs[0]["metadata"]["max-time"]
        if until_when[-1] != "s":
            # there is a time but not in seconds! fatally kill the program
            fatal("Runtime must be in unit seconds (ns, us, ms, s)!")
except KeyError:
    # There is no max-time. It's okay, we can live with it.
    until_when = "0s"

# I think we're ready to start the simulation
sst_command = "mpirun " + \
                  "-np " + str(sst_processes) + \
                  " -- bin/sst -v --add-lib-path=./ "
# Make sure there is no stop at
if until_when != "0s":
    sst_command += "--stop-at=" + until_when

sst_command += " " + sst_config + " -- " + \
                  "--output-directory=" + experiment_path + " " + \
                  "--jobs-path=" + jobs_json + " " + \
                  "--clock=" + args.clock + " " + \
                  "--systemd=" + systemd + " " + \
                  " | tee " + mpi_stats_path

sst_mpi_process = subprocess.Popen([sst_command], shell=True)

try:
    while True:
        sst_mpi_process.wait()
        break
        continue
except KeyboardInterrupt:
    # Make sure that the process that started is killed as well.
    os.system("killall -9 sstsim.x")
    fatal("Simulation ended by user. All sstsim.x processes are killed!")

# --------------------------- All SST done -----------------------------------#

