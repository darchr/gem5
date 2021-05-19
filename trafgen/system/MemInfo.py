from m5.objects import *

mem_info = {
    'DDR3' : {
        'gname' : DDR3_1600_8x8,
        'dname' : 'DDR3_8Gb_x8_1600',
        'cache_line_size' : 64,
        'channel_cap' : 2048
    },
    'DDR4' : {
        'gname' : DDR4_2400_8x8,
        'dname' : 'DDR4_4Gb_x8_2400',
        'cache_line_size' : 64,
        'channel_cap' : 1024
    },
    'HBM' : {
        'gname' : HBM_1000_4H_1x128,
        'dname' : 'HBM1_4Gb_x128',
        'cache_line_size' : 64,
        'channel_cap' : 64
    },
    'LPDDR3' : {
        'gname' : LPDDR3_1600_1x32,
        'dname' : 'LPDDR3_8Gb_x32_1600',
        'cache_line_size' : 64,
        'channel_cap' : 256
    }
}