#!/usr/bin/python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import struct
from collections import defaultdict

_dispatch_table = 0xec
with open('files/SYSTEM.INTERP', 'rb') as f:
    data = f.read()

print('By opcode')
print('--------------')

opcodes = []
opcode_by_addr = defaultdict(list)
for x in range(256):
    addr = struct.unpack('>H', data[_dispatch_table+2*x:_dispatch_table+2*x+2])[0]
    opcodes.append(addr)
    opcode_by_addr[addr].append(x)
    print('0x{:02x} 0x{:04x}'.format(x, addr))

addrs = sorted(opcode_by_addr.keys())

print()
print('By address')
print('--------------')

for addr in addrs:
    opstr = list('0x{:02x}'.format(x) for x in opcode_by_addr[addr])
    print('0x{:04x} {}'.format(addr, ' '.join(opstr)))

print()
print('Radare commands')
print('----------------')

for addr in addrs:
    ops = sorted(opcode_by_addr[addr])
    print('f _opcode_{:02x} @ 0x{:04x}'.format(ops[0], addr))

