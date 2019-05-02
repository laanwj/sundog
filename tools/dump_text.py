#!/usr/bin/env python3
'''
Attempt to decompress all dialog text.
'''
import sys
import binascii

PASSTHROUGH=[0x20,0x27,0x2c,0x7e,0x2e,0x21]

def decompress1(x):
    '''
    Kind 1: allow upper/lowercase, no alternatives
    '''
    out = []
    for ch in x:
        space = False
        if ch & 128:
            ch &= 127
            space = True
        hval = b'[%02x]' % ch

        if ch in PASSTHROUGH:
            out.append(ch)
        elif ch == 0x7c or ch == 0x5c:
            out.extend(b'I')
        elif ch & 32:
            if ch == 0x60:
                out.extend(b'|')
            elif ch >= 0x61 and ch <= 0x7a:
                out.append(ch)
            else:
                out.extend(hval)
        else: # Alternative?
            if ch >= 0x41 and ch <= 0x5a:
                out.append(ch)
            else:
                out.extend(hval)
        if space:
            out.append(0x20)
    return bytearray(out)


# Characters allowed within alternative
# 0x0000,0x0000,0x0000,0x0000,0xfffe,0xffff,0x0001 -> 0x41..0x60

def decompress2(x):
    '''
    Kind 2: all-lowercase, with alternatives
    '''
    out = []
    for ch in x:
        space = False
        if ch & 128:
            ch &= 127
            space = True
        hval = b'[%02x]' % ch

        if ch in PASSTHROUGH:
            out.append(ch)
        elif ch == 0x7c or ch == 0x5c:
            out.extend(b'I')
        elif ch & 32:
            if ch == 0x60:
                out.extend(b'|')
            elif ch >= 0x61 and ch <= 0x7a:
                out.append(ch)
            else:
                out.extend(hval)
        else: # Alternative?
            # 0x0000,0x0000,0x0000,0x0000,0xfffe,0x07ff
            # 0x41..0x4f 0x50..0x5a -> A..Z
            if ch >= 0x41 and ch <= 0x5a:
                out.append(ch | 32)
                out.extend(b'|')
            else:
                out.extend(hval)
        if space:
            out.append(0x20)
    return bytearray(out)

with open(sys.argv[1], 'rb') as f:
    base = 0x06c00
    f.seek(0x06c00)
    data = f.read(0x08c00 - 0x06c00)

#out = bytearray(out)
#for x in range(0, len(out), 32):
#    print(out[x:x+32])
'''
ptr = 0x8a7c - 0x06c00
while ptr < len(data):
    l = data[ptr]
    print(decompress1(data[ptr+1:ptr+1+l]).decode('utf8'))
    ptr += l + 1
'''
#print(decompress2(data[0x6c40-base:0x8c00-base]))

print('## Stock phrases')
# Determine start and end offsets from initial table
lastofs = 0
sbase = 0
offsets = []
for x in range(65):
    ofs = data[x]
    if ofs < lastofs:
        sbase += 0x100
    offsets.append(sbase + ofs)
    lastofs = ofs

# Print all words/phrases
for x in range(64):
    print('%02x %04x %s' % (x,
        base+offsets[x],
        decompress2(data[offsets[x]:offsets[x+1]]).decode()))

# Other sectors have ???
print()
for sbase in range(0x400,0x2000,0x200):
    print('## Sector 0x%05x (dialog block 0x%02x)' % (base+sbase, (sbase-0x400)//512 * 0x10 + 0x10))
    '''
    # "code"
    for x in range(16):
        ofs1 = data[sbase+x]
        ofs2 = data[sbase+x+1]
        print(binascii.b2a_hex(data[sbase+ofs1:sbase+ofs2]))
    '''
    # "data"
    ptr = sbase + data[sbase+0x11]*2
    num_stock = data[sbase+0x10]
    idx = 0
    while ptr < (sbase + 0x200):
        l = data[ptr]
        if idx < num_stock:
            print('%04x [=%02x] %s' % (
                base + ptr,
                0x34 + idx,
                decompress2(data[ptr+1:ptr+1+l]).decode()))
        else:
            print('%04x %s' % (
                base + ptr,
                decompress1(data[ptr+1:ptr+1+l]).decode()))
        ptr += 1 + l
        ptr += ptr & 1
        idx += 1
    print()

