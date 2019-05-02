#!/usr/bin/env python3
'''
Convert debug info for C interpreter debugger.

Usage: tools/gen_debug_info.py src/game/game_debuginfo.h
'''
import sys

import sundog_info
out = sys.stdout

def gen(out):
    proclist = sundog_info.load_metadata()
    info = []
    for k,v in proclist._map.items():
        if v.name is not None:
            info.append((k, v))

    info.sort()

    out.write('// clang-format off\n')
    for k,v in info:
        if v.name is None:
            continue
        # get name without arguments
        name = v.name
        if '(' in name:
            name = name[0:name.find('(')]
        # output
        out.write('{ { { { "%s" } }, 0x%x }, "%s" },' % (k[0].decode(), k[1], name))
        out.write('\n')
    out.write('    // clang-format on\n')

if __name__ == '__main__':
    with open(sys.argv[1], 'w') as f:
        gen(f)
