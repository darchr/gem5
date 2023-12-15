# set up a gem5 file for a new object called GraphInit
#
# Path: numa_gem5/gem5/src/mem/GraphInit.py

import m5

from m5.params import *
from m5.proxy import * 
from m5.objects.ClockedObject import ClockedObject


class GraphInit(ClockedObject):
    type = 'GraphInit'
    cxx_header = "mem/graph_init.hh"
    cxx_class = 'gem5::GraphInit'

    mirrors_map_mem = RequestPort("Port to a memory storing mirrors map file.")

    # vertex_image_file = Param.String("Path to the vertex image file.")
    graph_file = Param.String("Path to the vertex image file.")
    
    EL_addr = Param.Addr("Address of the edge list.")
    VL_addr = Param.Addr("Address of the vertex list.")

    # mirrors_mem = Param.SimpleMemory("Memory to store the vertex mirrors.")

    # cxx_exports = [
    #                 PyBindMethod("setAsyncMode"),
    #                 PyBindMethod("setBSPMode"),
    #                 PyBindMethod("setPGMode"),
    #                 PyBindMethod("createPopCountDirectory"),
    #                 PyBindMethod("createBFSWorkload"),
    #                 PyBindMethod("createBFSVisitedWorkload"),
    #                 PyBindMethod("createSSSPWorkload"),
    #                 PyBindMethod("createCCWorkload"),
    #                 PyBindMethod("createAsyncPRWorkload"),
    #                 PyBindMethod("createPRWorkload"),
    #                 PyBindMethod("createBCWorkload"),
    #                 PyBindMethod("workCount"),
    #                 PyBindMethod("getPRError"),
    #                 PyBindMethod("printAnswerToHostSimout")
    #             ]

