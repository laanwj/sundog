#!/usr/bin/env python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''Print sprite patterns from GEMBIND:0x29'''
# 0x00002802 / GEMBIND segment + 0xa02
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
colors=[
0x00010100000001010001010000000101,
0x00010001000101000001000100010100,
0x00010000010100010001000001010001,
0x00010000000000000100010101010101,
]

def byte_to_bin(x):
    return bytes(int(bool(x&(1<<i))) for i in range(7,-1,-1))

blk = ' █'
for i,p in enumerate(patterns):
    print('== %d ==' % i)
    bit = 56
    while bit > 0:
        line = (p >> bit) & 0xff
        print(''.join(blk[i] for i in byte_to_bin(line)))
        bit -= 8
    print()

print()
print('Colors')
bit = 15*8
pixels = []
while bit >= 0:
    flags = [(p >> bit) & 0xff for p in colors]
    pixel = flags[0] | (flags[1]<<1) | (flags[2]<<2) | (flags[3]<<3)
    pixels.append(pixel)
    bit -= 8
print(pixels)
