from m5.objects import *

mem_info = {
    'DDR3' : {
        'gname' : DDR3_1600_8x8,
        'cache_line_size' : 64,
        'channel_cap' : 8192
    },
    'DDR4' : {
        'gname' : DDR4_2400_8x8,
        'cache_line_size' : 64,
        'channel_cap' : 1024
    },
    'HBM' : {
        'gname' : HBM_1000_4H_1x128,
        'cache_line_size' : 64,
        'channel_cap' : 64
    },
    'LPDDR3' : {
        'gname' : LPDDR3_1600_1x32,
        'cache_line_size' : 64,
        'channel_cap' : 256
    },
    'NVM' : {
        'gname' : NVM_2400_1x64,
        'cache_line_size' : 64,
        'channel_cap' : 8192
    }
}