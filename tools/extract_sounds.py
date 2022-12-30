#!/usr/bin/env python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Extract sundog sounds.
'''
import binascii
import os
import struct
import subprocess
import sys

DOSOUND = os.getenv('DOSOUND', '')
OGGENC = os.getenv('OGGENC', 'oggenc')

URL = os.getenv('URL', '')

with open(sys.argv[1], 'rb') as f:
    f.seek(0x4400)
    data = f.read(1024)

sounds = struct.unpack('>36H', data[0x00:0x48])
snd_bytes = data[0x48:]

print(f'|id  | data|')
print(f'|--- |---- |')

for i in range(0x24):
    if not sounds[i + 1]:
        break
    data = snd_bytes[sounds[i]:sounds[i + 1]]
    data_hex = binascii.hexlify(data).decode() 
    print(f'| [{i}]({URL}/{i}.ogg) | `{binascii.hexlify(data).decode()}` |')

    if DOSOUND:
        outfile = f'{i}.wav'
        oggfile = f'{i}.ogg'
        subprocess.run([DOSOUND, outfile, data_hex], check=True)
        subprocess.run([OGGENC, '-q', '3', '-o', oggfile, outfile])

# snd_off (sound 0) is always concatenated to stop the sound effect afterwards
