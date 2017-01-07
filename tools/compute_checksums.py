#!/usr/bin/python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Verify SunDog: Frozen Legacy 3.0 Atari ST disk image.

Only raw .st images are supported right now, not .stx.

Usage:
    compute_checksums.py <image.st> <0|1>
'''
import sys
import struct

with open(sys.argv[1], 'rb') as f:
    data = f.read()

sectors_per_track = 9
tracks = 80
base_track = 3
sector_size = 512
skipset = 0x182

track_size = sector_size * sectors_per_track

if int(sys.argv[2]): # use hardcoded data
    valid='''
    f2 52 71 8b 5e 15 ad 94 2c d2 b2 84 8b ef 9d ee
    65 c0 ed b0 24 0d 71 f0 2a 0b 30 3f bc b9 06 d8
    e4 d4 67 ec 2d 63 9b 27 f5 48 4d 45 15 6c 54 78
    25 ae 9d 17 21 37 57 44 8b de bc b3 f8 42 c6 aa
    45 3c a2 f1 da f4 1b 81 3b e5 0c 8e c6 31 da b6
    e2 4c c1 9f 30 7e 9b 21 52 99 69 8a 31 2c 3c 65
    47 a3 32 62 0a a8 07 4d c0 92 01 eb 03 d5 65 2a
    bd aa a6 6d 96 a4 06 eb 2c f3 8d 8e de 88 aa cf
    7c 34 dc c1 a2 4a 94 20 d9 45 74 eb 9a eb 08 9a
    4d 5f 25 ff 2d f5 ba 0b 28 8a f2 69 56 4b 08 d9
    '''
    valid = bytes(int(x.strip(),16) for x in valid.split())
    valid = struct.unpack('>80H', valid)
else:
    valid = list(struct.unpack('>80H', data[0xd760:0xd760+2*80]))
    for x in range(80):
        valid[x] = (valid[x] - x*0x169)&0xffff

for track in range(tracks):
    base = ((base_track + track)%tracks) * sectors_per_track * sector_size
    expect = valid[track]
    ptr = 0
    x = 0
    while ptr < track_size:
        (a,b) = struct.unpack('>HH', data[base+ptr:base+ptr+4])
        x -= a
        x += b
        x &= 0xffff
        ptr += 4
    if (1<<track)&skipset:
        s = 'SKIP'
    elif x == expect:
        s = 'MATCH'
    else:
        s = 'FAIL'
    print('%3i %08x-%08x 0x%04x 0x%04x %s' % (track, base, base + sectors_per_track * sector_size, x, expect, s))
