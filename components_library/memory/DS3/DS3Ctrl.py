import configparser
import os

import m5
from m5.objects import DRAMsim3

def config_ds3(mem_type, num_chnls):
    config = configparser.ConfigParser()
    thispath = os.path.dirname(os.path.realpath(__file__))
    input_file = os.path.join(thispath, 'mem_configs/' + mem_type + '.ini')
    output_file = '/tmp/' + mem_type + '_chnls' + str(num_chnls) + '.ini'
    new_config = open(output_file, 'w')
    config.read(input_file)
    config.set('system', 'channels', str(num_chnls))
    config.write(new_config)
    new_config.close()
    return output_file, m5.options.outdir


class DS3MemCtrl(DRAMsim3):
    def __init__(self, mem_name, num_chnls):
        super(DS3MemCtrl, self).__init__()
        ini_path, outdir = config_ds3(mem_name, num_chnls)
        self.configFile = ini_path
        self.filePath = outdir