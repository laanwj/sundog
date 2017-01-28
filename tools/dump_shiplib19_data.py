#!/usr/bin/python3

blk = ' █'

with open('files/SYSTEM.STARTUP', 'rb') as f:
    data = f.read()

def byte_to_bin(x):
    return bytes(int(bool(x&(1<<i))) for i in range(7,-1,-1))

img1 = data[0x00005446:0x00005932]
img2 = data[0x00005932:0x00005e1e]

img = img2
ptr = 0
for y in range(63):
    row = b''
    for x in range(20):
        row += byte_to_bin(img[y*20+x])
        ptr += 1
    print(''.join(blk[i] for i in row[80:]))

