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

# Time in ticks to keep generating after end of 'dosound' loop.
TRAILING_TICKS = 15

if not DOSOUND:
    # Default to built-in dosound, if built.
    if os.path.isfile("dosound/dosound"):
        DOSOUND = "dosound/dosound"

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

# warp sounds from shiplib_1A
# for warp1, one-to-last byte is changed to 2,4,8 based on "warp speed"
warp1_speed2 = bytes([7,56,8,16,9,16,10,16,0,100,1,5,2,110,3,5,4,103,5,5,12,5,128,0,129,13,2,1])
warp1_speed4 = bytes([7,56,8,16,9,16,10,16,0,100,1,5,2,110,3,5,4,103,5,5,12,5,128,0,129,13,4,1])
warp1_speed8 = bytes([7,56,8,16,9,16,10,16,0,100,1,5,2,110,3,5,4,103,5,5,12,5,128,0,129,13,8,1])
warp2 = bytes([12,255,7,7,8,16,9,16,10,16,128,0,13,9,129,6,1,90,8,12,9,12,10,11,255,0])
warp3 = bytes([12,4,6,0,7,8,0,100,1,0,2,103,3,0,4,95,5,0,8,16,9,16,10,16,13,15,255,0])

sound_mapping = []
for i in range(0x24):
    if not sounds[i + 1]:
        break
    data = snd_bytes[sounds[i]:sounds[i + 1]]
    name = sounds_names[i]
    sound_mapping.append((i, name, data))

sound_mapping.append((28, "warp1_speed2", warp1_speed2))
sound_mapping.append((29, "warp1_speed4", warp1_speed4))
sound_mapping.append((30, "warp1_speed8", warp1_speed8))
sound_mapping.append((31, "warp2", warp2))
sound_mapping.append((32, "warp3", warp3))

print(f'|id  | name | data|')
print(f'|--- |------| ---- |')

for (i, name, data) in sound_mapping:
    data_hex = binascii.hexlify(data).decode()

    print(f'| [{i}](sounds/{name}.ogg) | {name} | `{data_hex}` |')

    if DOSOUND:
        outfile = f'{name}.wav'
        oggfile = f'{name}.ogg'
        subprocess.run([DOSOUND, outfile, data_hex, str(TRAILING_TICKS)], check=True)
        subprocess.run([OGGENC, '-q', '3', '-o', oggfile, outfile])

# snd_off (sound 0) is always concatenated to stop the sound effect afterwards
with open('../src/game/sundog_soundfx.h', 'w') as f:
    f.write('// clang-format off\n')
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
    f.write('// clang-format on\n')
