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

sounds_names = [
  'off',       # 0
  'tractor',   # 1
  'nme_laser', # 2
  'jackpot',   # 3
  'menu_click',# 4
  'hit_shield',# 5
  'hit_hull',  # 6
  'part_dies', # 7
  'laser',     # 8
  'cannon',    # 9
  'fuel_here', # 10
  'stinger',   # 11
  'ship_arivs',# 12
  'pod',       # 13
  'hit_pirate',# 14
  'disk_dead', # 15
  'atck_siren',# 16 
  'distress',  # 17
  'jettison',  # 18
  'rpair_bay', # 19
  'warp_fail', # 20
  'tele_up',   # 21
  'tele_down', # 22
  'explosion', # 23
  'phase_done',# 24
  'hail',      # 25
  'pir_laser', # 26
  'disk_warn', # 27
]

print(f'|id  | name | data|')
print(f'|--- |------| ---- |')

sound_mapping = []

for i in range(0x24):
    if not sounds[i + 1]:
        break
    data = snd_bytes[sounds[i]:sounds[i + 1]]
    data_hex = binascii.hexlify(data).decode()
    name = sounds_names[i]

    print(f'| [{i}](sounds/{name}.ogg) | {name} | `{data_hex}` |')

    sound_mapping.append((i, name, data))
    if DOSOUND:
        outfile = f'{name}.wav'
        oggfile = f'{name}.ogg'
        subprocess.run([DOSOUND, outfile, data_hex], check=True)
        subprocess.run([OGGENC, '-q', '3', '-o', oggfile, outfile])

# snd_off (sound 0) is always concatenated to stop the sound effect afterwards
with open('../src/game/sundog_soundfx.h', 'w') as f:
    for (i, name, data) in sound_mapping:
        f.write('static const uint8_t sound_%s[] = {%s};\n' % (name, ','.join('0x%02x' % x for x in data)))
    f.write('''
static struct {
    const char *name;
    size_t len;
    const uint8_t *data;
} sundog_sound_fx[%d] = {
''' % (len(sound_mapping)))
    for (i, name, data) in sound_mapping:
        f.write('/* %2i */ {"%s", %d, sound_%s},\n' % (i, name, len(data), name))
    f.write('''};
    static const size_t sundog_sound_fx_count = %d;
    ''' % (len(sound_mapping)))
