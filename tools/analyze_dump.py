#!/usr/bin/python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Analyze dump file, to try prototype techniques that could be useful
for debugger.
'''
import struct

from printable import printable

mem_fake_base = 0x000237ac

def ldw(d, x):
    return (d[x]<<8)|d[x+1]

with open('/tmp/dump', 'rb') as f:
    d = f.read()

def dump_sib(ptr):
    erec = ptr - 0xa
    if ldw(d, erec+4) != ptr:
        erec = 0  # invalid
    print('{:04x} {} {:04x} {:04x} {:04x} {:04x}'.format(ptr,
        printable(d[ptr+0xc:ptr+0xc+8]),
        ldw(d, ptr+0x1c), ldw(d, ptr+0x1e),
        erec, ldw(d, erec+8)))

def residency(sib):
    seg_pool = ldw(d, sib+0)
    seg_base = ldw(d, sib+2)
    if seg_base == 0:
        return '-'
    else:
        addr = seg_base
        if seg_pool:
            addr += ((ldw(d, seg_pool)<<16)|ldw(d, seg_pool+2)) - mem_fake_base
        return 'R %05x' % addr

def dump_erec(erec):
    sib = ldw(d, erec+4)
    print('{:04x} {:04x} {} {} {:04x} {:04x}'.format(
        erec, 
        sib,
        residency(sib),
        printable(d[sib+0xc:sib+0xc+8]),
        ldw(d, erec+0),
        ldw(d, sib+0x1a)
        ))
    # find subsidiary segments
    # these will be in this segment's evec and share the same
    # evec
    evec = ldw(d, erec+2)
    num = ldw(d, evec)
    for sub in range(1, num+1):
        serec = ldw(d, evec+sub*2)
        if serec:
            sevec = ldw(d, serec+2)
            if serec != erec and sevec == evec:
                ssib = ldw(d, serec+4)
                print('  {:04x} {:04x} {} {}'.format(
                    serec, 
                    ssib,
                    residency(ssib),
                    printable(d[ssib+0xc:ssib+0xc+8])
                    ))

def print_backrefs(x):
    print('Backrefs for {:04x}'.format(x))
    ptr = 0
    while True:
        ptr = d.find(struct.pack('>H', x), ptr)
        if ptr == -1 or ptr >= 0x10000:
            break
        values = [ldw(d,y) for y in range(ptr-16, ptr+16, 2)]
        values = [('%04x' % value) for value in values]
        print('  {:04x}: {}'.format(ptr,
            ' '.join(values)))
        ptr += 2

def print_erecs():
    print('erec sib  segname  flg base size')
    erec = ldw(d, 0x14e)
    while erec:
        dump_erec(erec)
        erec = ldw(d, erec+8)

def dump_tib(tib):
    prio = ldw(d, tib + 0x02) & 0xff
    erec = ldw(d, tib + 0x10)
    proc = ldw(d, tib + 0x12) & 0xff
    sib = ldw(d, erec+4)
    loc = '%s:0x%02x' % (printable(d[sib+0xc:sib+0xc+8]),proc)
    print("{:04x} {:02x} {:04x}  {:04x}  {:04x} {}".format(
        tib,
        prio,
        ldw(d, tib + 0x00),
        ldw(d, tib + 0x14),
        ldw(d, tib + 0x0e),
        loc
        ))

'''
print()
print('sib  segname  nsib psib erec nerec')
pool = 0x104a
sib = ldw(d, pool + 0x0c)
sib_prev = None
while sib != sib_prev:
    dump_sib(sib)
    sib_prev = sib
    sib = ldw(d, sib + 0x1c)
print()
'''
print()
known_tasks = [0x006e, 0x01a0, 0x01bc, 0x6260, 0x6348]

print("tib  pr waitq hangp ipc  proc")
for tib in known_tasks:
    dump_tib(tib)
    print_backrefs(tib)
