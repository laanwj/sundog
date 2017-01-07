Make sure WeirdDiskScan always triggers:

w $0003FA80 0x91 0x4a 0x96 0x00


b pc=0x20a8a && a4="0x3eacc+0x1158"

Get state:

    > m "0x26c3a+8+0x238*2"

    Set state:

    w "0x26c3a+8+0x238*2" 0x04 0xd2

Break at MAINLIB:0x37 - seems to trigger any time a menu opens

b pc=0x20a8a && a4="0x3eacc+0x0122"


Single-step:

b pc=0x20a8a

Break in MAINLIB:0x54 after slod 0xd test:

b pc=0x20a8a && a4="0x3eacc+0x0fb8"

```
??? state changed to 0x4766
Traceback [tib=006e sp=e976]
  XSTARTUP:0x03:0162 mp=0xe976 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e07 mp=0xe982 base=0x0000 erec=0x1b2a
  DONESOFA:0x02:0069 mp=0xfdb2 base=0x0000 erec=0x184e
  SUNDOG  :0x01:0311 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x008a erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x000d
Traceback [tib=006e sp=d366]
  MAINLIB :0x4a:122d mp=0xd366 base=0x1f66 erec=0x18d8
  XSTARTUP:0x0f:0847 mp=0xe576 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e11 mp=0xe982 base=0x0000 erec=0x1b2a
  DONESOFA:0x02:0069 mp=0xfdb2 base=0x0000 erec=0x184e
  SUNDOG  :0x01:0311 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x4eab erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x8ad0
Traceback [tib=006e sp=d366]
  MAINLIB :0x4a:1282 mp=0xd366 base=0x1f66 erec=0x18d8
  XSTARTUP:0x0f:0847 mp=0xe576 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e11 mp=0xe982 base=0x0000 erec=0x1b2a
  DONESOFA:0x02:0069 mp=0xfdb2 base=0x0000 erec=0x184e
  SUNDOG  :0x01:0311 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x4eab erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0xf71a
Traceback [tib=6348 sp=72da]
  MAINLIB :0x34:0b5b mp=0x72da base=0x1f66 erec=0x18d8
  SUNDOG  :0x02:00e2 mp=0x72e4 base=0x4f02 erec=0x17da
  SUNDOG  :0x03:02fb mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x4eab erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x0012
Traceback [tib=006e sp=fdae]
  SUNDOG  :0x03:018d mp=0xfdae base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0322 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0xfe93
Traceback [tib=006e sp=fd62]
  XMOVEONS:0x06:0234 mp=0xfd62 base=0x1c8e erec=0x19a2
  XMOVEONS:0x09:03fb mp=0xfd78 base=0x1c8e erec=0x19a2
  XMOVEONS:0x01:05d3 mp=0xfd8c base=0x1c8e erec=0x19a2
  SHIPLIB :0x08:0061 mp=0xfda4 base=0x1c8e erec=0x187c
  SUNDOG  :0x03:01ca mp=0xfdae base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0322 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0x129d
Traceback [tib=006e sp=fcce]
  MAINLIB :0x35:0c40 mp=0xfcce base=0x1f66 erec=0x18d8
  XMOVEONG:0x14:0ac6 mp=0xfcfa base=0x0000 erec=0x1c32
  XMOVEONG:0x01:282f mp=0xfd12 base=0x0000 erec=0x1c32
  GRNDLIB :0x03:002f mp=0xfd90 base=0x0000 erec=0x1820
  SUNDOG  :0x04:0267 mp=0xfda6 base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0326 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
```
??? state changed to 0xf71a
Traceback [tib=6348 sp=72da]
  MAINLIB :0x34:0b5b mp=0x72da base=0x1f66 erec=0x18d8
  SUNDOG  :0x02:00e2 mp=0x72e4 base=0x4f02 erec=0x17da
  SUNDOG  :0x03:02fb mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
screen v_pline vr=2 col=15 width=1 count=12
```
```
??? state changed to 0x04d2
Traceback [tib=6348 sp=72da]
  MAINLIB :0x34:0b52 mp=0x72da base=0x1f66 erec=0x18d8   PossiblySetEvilState
  SUNDOG  :0x02:00e2 mp=0x72e4 base=0x4f02 erec=0x17da
  SUNDOG  :0x03:02fb mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```
### Related variables

- GEMBIND\_238 (duh, main state)
- MAINLIB\_3b1 checksum computed in XMOVEONS:0x06, compared to 0x13a in MAINLIB:0x34, set to 0x13a
- MAINLIB\_392 set to 0x120 in MAINLIB:0x34, compared to 0x120
- MAINLIB\_3ab checksum wrong counter if 0xcc37
- MAINLIB\_3ac checksum wrong counter if 0x129d
- MAINLIB\_3ad error counter increased in MAINLIB:0x4a WeirdDiskCheck
- MAINLIB\_a73 compared to 0xab7 in MAINLIB:0x34
- SHIPLIB\_6  pointer to sector used in XMOVEONS:0x06
- MAINLIB\_435 packed array of tracks visited

### Related procedures

- MAINLIB:0x34 PossiblySetEvilState: possibly sets MAINLIB\_238 to 0x04d2
  - check depends on whether state is 0x129d, 0xcc37
  - checks MAINLIB\_3b1, MAINLIB\_392, MAINLIB\_a73
  - keeps counters MAINLIB\_3ab, MAINLIB\_3ac
  - if MAINLIB\_3ad + MAINLIB\_3ac + MAINLIB\_3ab >= 3 set state to 0x4d2 (dead), else 0xf71a
- XMOVEONS:0x06 
  - computes checksum of 512-byte sector to MAINLIB\_3b1
  - sets GEMBIND\_238 to 0xfe93
  - sets MAINLIB\_a73 to ?[0x23] * ?[0x66]
  - SHIPLIB\_6 is used as address of the sector
- MAINLIB:0x35
  - sets MAINLIB\_238 to 0x04d2 (evil state) if it was 0x000d
  - sets it to 0x129d if it was 0xfe93
- MAINLIB:0x4a WeirdDiskCheck
  - checks a random track against list of checksums (see `tools/compute_checksums.py`)
  - for track 0 do some extra processing
- XDOINTER:0x1f  Reads a sector, MAINLIB\_3ab
- XDOUSERM:0x0f  GEMBIND\_238, MAINLIB\_3ab
- XSTARTUP:0x06  Initializes MAINLIB\_43a, MAINLIB\_43c, MAINLIB\_3ad, MAINLIB\_3ac, MAINLIB\_3ab
  MAINLIB\_435, MAINLIB\_3b4, MAINLIB\_3b5, MAINLIB\_3b2, GEMBIND\_22e,
  GEMBIND\_230
  - Calls unitstatus(0x80). Looks like the mother function for all integrity-check-related behavior.
  - First dword (overwritable data?) goes to MAINLIB\_3b2
  - Second dword (jump table address) goes to GEMBIND\_22e, where it looks like never used
  - Third word goes to MAINLIB\_3b0 + 1, added 'A' = drive letter

### Weird, may be related or not

- SHIPLIB:0x17 Generate 256 random bytes at address SHIPLIB+0x06

States
--------

States of GEMBIND\_238. These are used to communicate the current state of the integrity
check to different parts of the game code:

     0x000d   Set if MAINLIB:0x4a WeirdDiskCheck error, changed to 0x4d2 in MAINLIB:0x35
     0x0012   Do the checksum compute in XMOVEONS:0x06
     0x04d2   Red screen of death
     0x129d   Tested for in MAINLIB:0x34, set in MAINLIB:0x35
     0x4766   Never tesetd for, set in XSTARTUP:0x03 InitGFX
     0x8ad0   Never tested for, set in MAINLIB:0x4a WeirdDiskCheck when increasing 0x3ad error counter
     0xcc37   Tested for in MAINLIB:0x34, set in XDOINTER:0x1f
     0xf71a   Never tested for, set in MAINLIB:0x34
     0xfe93   Checksum compute in XMOVEONS:0x06 done, MAINLIB_a73 and MAINLIB_3b1 are filled in, changed to 0x129d in MAINLIB:0x35

