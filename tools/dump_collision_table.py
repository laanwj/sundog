#!/usr/bin/python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''Visualize collision patterns from GEMBIND:0x28'''
import struct
with open('files/SYSTEM.STARTUP', 'rb') as f:
    d = f.read()
# x,y offsets are relative to hotspot 4,4 of sprite
offsets_table = 0x24e0 
counts_table = 0x24f4
ops_table = 0x24fe
blk = ' █'

patterns=[
0x0038383838383800,
0x00081c3e7c381000,
0x00007e7e7e000000,
0x0010387c3e1c0800,
0x0000001818000000,
0x00003c3c3c000000,
0x00063e1e3a50a000,
0x063e5c3c54a84000,
0x0a1e1c3c58b04000,
0x063e0c3cd4142000,
]

def byte_to_bin(x):
    return bytes(int(bool(x&(1<<i))) for i in range(7,-1,-1))
def bin_to_byte(x):
    return sum(x[7-i] << i for i in range(8))

cpatterns = []
for i in range(10):
    print('== %d ==' % i)
    offset = struct.unpack('>H', d[offsets_table + 2*i:offsets_table + 2*i + 2])[0]
    count = d[counts_table+i]
    ptr = ops_table + offset
    grid = [bytearray(7) for _ in range(7)]
    yofs = 0
    for j in range(count+1):
        if j == 0:
            yofs = d[ptr]
            xofs = 0
        else:
            if d[ptr]:
                yofs += 1
                xofs = 0
        ptr += 1
        xofs += d[ptr]
        ptr += 1
        grid[yofs][xofs] = 1
        print('  x+%d y+%d' % (xofs, yofs))

    #for line in grid:
    #    print(''.join(blk[i] for i in line))

    # compare with original sprite
    sprite = []
    p = patterns[i]
    bit = 56
    while bit > 0:
        line = (p >> bit) & 0xff
        sprite.append(byte_to_bin(line))
        bit -= 8

    # difference
    for y in range(7):
        l = []
        for x in range(7):
            if grid[y][x] and not sprite[y][x]:
                ch = '+'
            elif not grid[y][x] and sprite[y][x]:
                ch = '-'
            else:
                ch = blk[grid[y][x]]
            l.append(ch)
        print(''.join(l))

    # build pattern
    pvals = [bin_to_byte(grid[y]+bytearray([0])) for y in range(7)]
    cpatterns.append(pvals)

# output patterns for C
for pvals in cpatterns:
    pvals = ', '.join(('0x%02x' % x) for x in pvals)
    print('{ %s }, ' % pvals)
