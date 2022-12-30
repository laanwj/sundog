
SunDog gamestate.

Base addresses are in words, sizes are in bytes.


MAINLIB
----------------

```
0x001  ?  Semaphore for task coordination
0x003  2  Event flag (for coordination?)
0x004  2  Something with graphics/font rendering
0x005  2  Current screen line
0x006  2  Current screen column
0x007  2  ?
0x013  2  Related to score
0x01f  512  Saved game state (256 words)
0x01f  ?  String: character name
0x027  4  ?
0x029     ?
0x02a  2  Location: City (6,9) Planet (5,4) System (4,0)
0x02b  2  Location ? (6,0)
0x02c  2  Location ? (5,0)
0x032  ?  Packed: ? state (ends game)
          (4,8)  current player environment (ground, building, menu, repair, ...)
0x033  2  Money
0x034  2  Money
0x035  8  byte array
  [0]  Attribute ST
  [1]  Attribute IQ
  [2]  Attribute DX
  [3]  Attribute CH
  [4]  Attribute LK
  [5]  Max Attribute ST
  [6]  Max Attribute IQ
  [7]  Max Attribute DX
  [8]  Max Attribute CH
  [9]  Max Attribute LK
  [10] ?
  [11] ?
  [12] ?
  [13] ?
  [14] ?
0x03d  ?  byte array
0x03f  4  Items on player
0x041  ?  byte array
0x042  2  Score
0x043  2  Score
0x044  2  packed values
0x04a     (4,0) something with warp
          (5,4)
          (6,9)
          Looks like another location. Usually the same as 0x2a: ship location?
0x05b  20 
  [N*5+0]  "Warp drives" row N col 0 (control node 0x1f)
  [N*5+1]  "Warp drives" row N col 1 (flux modulator 0x1c)
  [N*5+2]  "Warp drives" row N col 2 (photon bridge 0x1a)
  [N*5+3]  "Warp drives" row N col 3 (s/t distorter 0x1e)
  [N*5+4]  Row status 0..4 (normal item gives 1, shunts 0)
0x067  4?
  [0]      "Warp drives" status 4*num working rows, 0x10=max
0x069  20
  [N*5+0]  "Sub-c Engines" row N col 0 (control node 0x1f)
  [N*5+1]  "Sub-c Engines" row N col 1 (flux modulator 0x1c)
  [N*5+2]  "Sub-c Engines" row N col 2 (s/t distorter 0x1e)
  [N*5+3]  "Sub-c Engines" row N col 3 (cryofuse 0x18)
  [N*5+4]  Row status 0..4
0x075  4?
  [0]      "Sub-c Engines" status 4*num working rows, 0x10=max
0x077  20
  [N*5+0]  "Ship's Guns" row N col 0 (control node 0x1f)
  [N*5+1]  "Ship's Guns" row N col 1 (cryofuse 0x18)
  [N*5+2]  "Ship's Guns" row N col 2 (concentrator 0x13)
  [N*5+3]  "Ship's Guns" row N col 3 (plasma tube 0x1d)
  [N*5+4]  Row status 0..4
0x083  4?
  [0]      "Ship's Guns" status 4*num working rows, 0x10=max
0x085  20
  [N*5+0]  "Tactical Disply" row N col 0 (control node 0x1f)
  [N*5+1]  "Tactical Disply" row N col 1 (scanner 0x1b)
  [N*5+2]  "Tactical Disply" row N col 2 (decloaker 0x14 or j-junc module 0x19)
  [N*5+3]  "Tactical Disply" row N col 3 (photon bridge 0x1a)
  [N*5+4]  Row status 0..4
0x091  4?
  [0]      "Tactical Disply" status 4*num working rows, 0x10=max
0x093  20
  [N*5+0]  "Force Shields" row N col 0 (control node 0x1f)
  [N*5+1]  "Force Shields" row N col 1 (cryofuse 0x18)
  [N*5+2]  "Force Shields" row N col 2 (flux modulator 0x1c)
  [N*5+3]  "Force Shields" row N col 3 (flux modulator 0x1c)
  [N*5+4]  Row status 0..4
0x09f  4?
  [0]      "Force Shields" status 4*num working rows, 0x10=max
0x0a1  20
  [N*5+0]  "Pilotage" row N col 0 (control node 0x1f)
  [N*5+1]  "Pilotage" row N col 1 (j-junc module 0x19)
  [N*5+2]  "Pilotage" row N col 2 (scanner 0x1b)
  [N*5+3]  "Pilotage" row N col 3 (j-junc module 0x19)
  [N*5+4]  Row status 0..4
0x0ad  4?
  [0]      "Pilotage" status 4*num working rows, 0x10=max
0x0af  20 Ship's locker
0x0bd  20 Ship's stores
0x101  2  Pseudo-random state word 1
0x102  2  Pseudo-random state word 2
0x10b  20 byte array
  [1]  XSTARTUP:0x2d Phase of game 0..9
  [2]  XSTARTUP:0x2d
  [3]  XSTARTUP:0x2a
  [4]  XSTARTUP:0x2a
  [5]  XMOVEONG:0x14
  [6]  XDOTRADI:0x07
  [8]  XDOFIGHT:0x17
  [9]  XDOFIGHT:0x17
  [10] XDOCOMBA:0x08
  [11] XDOFIGHT:0x17
  [12] XDOINTER:0x20
  [16] XDOFIGHT:0x01
  [17] XDOFIGHT:0x01
  [19] XMOVEONG:0x19
  [20] XMOVEONG:0x19
  [21] XMOVEONG:0x19
  [22] XMOVEONG:0x19
  ...
  [39] 
0x11f  2  Array of systems (0x14 per element)
0x197  2  Array of planets (0x8 per element)
0x227  2  Array of cities (0x6 per element)
0x3b2  4  Copied from unitstatus(0x80) = start of gfx stash area? used in MAINLIB:0x41
0x3b4  2  Disk I/O count?
0x435  10 track_checked WeirdScan state: set (packed array)
0x43a  2  Flag: skip disk i/o status check
0x43d  4  crc/checksum something, first four bytes from block 0x45
0x43f  176  ?
0x4ef  80 Table of track checksums for weirddisksscan
0x53f  36\*2  Table of sound effects, these are offsets into next structure
0x563        Base address for sound effects
0x6e5  2     Index of trailing data for all sounds
0x6e6        temporary buffer for sound
0x737  32*2 Mapping table from mouse position x/y to ship movement

GEMBIND
---------------------

```
0x1    2  Mouse cursor shown/hidden
0x2    2  Sprite flag used in GEMBIND:0x2c [never referenced] - used to avoid doing anything while VDI/AES calls in progress
0x3    2  Clipping rectangle
0x4    2  Clipping rectangle
0x5    2  Clipping rectangle
0x6    2  Clipping rectangle
0x7    2  Write mode
0x8    2  Text color
0x9    2  Line color
0xa    2  Fill color
0xb    2  Pointer to structure for VDI calls
0xc    2  Pointer to VDI communication area: ptsout
0xd    2  Pointer to VDI communication area: ptsin
0xe    2  Pointer to VDI communication area: intout
0xf    2  Pointer to VDI communication area: intin
0x10   2  Pointer to VDI communication area: contrl
0x11   2  Pointer to structure for VDI calls (copy for task)
0x12   2  Alt 0x0c (pointer to 32-bit addr?)
0x13   2  Alt 0x0d (pointer to 32-bit addr?)
0x14   2  Alt 0x0e (pointer to 32-bit addr?)
0x15   2  Alt 0x0f (pointer to 32-bit addr?)
0x16   2  Alt 0x10 (pointer to 32-bit addr?)
0x17   30 Filled with zeros in InitGFX
0x26      Used for ? (absolute address is referenced)
0x36   32 Filled with zeros in initGFX
0x46   4  Absolute address [0x10] (contrl)
0x48   4  Absolute address of GEMBIND+0x17
0x4a   4  Absolute address [0x0f] (intin)
0x4c   4  Absolute address [0x0e] (intout)
0x4e   4  Absolute address of GEMBIND+0x36
0x50   4  Absolute address of GEMBIND+0x26
0x52   4  Screen MFDB: Absolute address of GEMBIND+0x68
0x54   4  Font MFDB: Absolute address of GEMBIND+0x72
0x56   4  Icons MFDB: Absolute address of GEMBIND+0x7c
0x58   4  Absolute address of GEMBIND+0x86
0x50   4  Absolute address of GEMBIND+0x90?
0x5c   4  Absolute address of GEMBIND+0x9a?
0x5e   4  Absolute address of GEMBIND+0xa4?
0x60   4  Absolute address of GEMBIND+0xae?
0x62   4  Absolute address of GEMBIND+0xb8?
0x66   4  Zeros, set in XSTARTUP:0x03
0x68   20 Screen MFDB: Zeros, set in XSTARTUP:0x03
0x72   4  Font MFDB: Absolute address of GEMBIND+0x239
0x74   2  Font MFDB: fd_w  944
0x75   2  Font MFDB: fd_h  8
0x76   2  Font MFDB: fd_wdwidth 59
0x77   2  Font MFDB: fd_stand 1
0x78   2  Font MFDB: fd_nplanes 1
0x79   2  Font MFDB: fd_r1
0x7a   2  Font MFDB: fd_r2
0x7b   2  Font MFDB: fd_r3
0x7c   4  Absolute address of GEMBIND+0x536
0x7e   2  16*31  496
0x7f   2  16
0x80   2  31
0x81   2  1
0x82   2  4
0x86   20 Referenced
0x90   ?  Referenced
0x9a   ?  Referenced
0xa4   20 Referenced
0xae   ?  Referenced
0xb8   ?  Referenced
0xec   16*?  Sprite table: communication with gembind_Set2CParamB handler
0x22e  4  Receives jump table address from SUNDOG.PRG
0x230  16 Used in collision detection - 1 for each (hardware) color, if passable
0x238  2  WeirdDiskScan result / game state (see `weirddiskscan_debug.md`)
0x239  0x3b0+0x4a     Font: Graphics data from disk  @ 0x6600
0x411  0x4a  Mouse cursors
0x436  512 Item names (16 bytes per name, 32 names, why is this in GEMBIND?)
0x536  Icons

SHIPLIB
------------

```
0x6  2   Address of 256(random)+256(line offsets) byte table, used in SHIPLIB:0x16
         Allocates as 0x300 sized buffer from heap
0xc  2   Only ever set to 30 - used in SHIPLIB:0x18
0xd  2   Mouse Y (ship movement) - Used in SHIPLIB:0x18
0xe  2   Mouse X (ship movement) - Used in SHIPLIB:0x18
0x10 2   Start offset in "random table", used in SHIPLIB:0x16
0x11 2   Starting x of viewscreen, used in SHIPLIB:0x16
0x12 2   Number of lines of viewscreen, used in SHIPLIB:0x16
0x13 32  16 x coordinates followed by 16 y coordinates. Used in SHIPLIB:0x18
0x23 32  16 x deltas followed by 16 y deltas. Used in SHIPLIB:0x18
0x23 2   Used in SHIPLIB:0x18
```
SHIPLIB notes
---------------

Viewscreen is at screen position (96,6) to (256,117)
- or (0x60,0x6) to (0x100,0x75)
- or width,height (160,111)
- or width,height (0xa0,0x6f)

Items/Icons
---------------

```
0x00 0x00  (nothing)
0x01 0x00  (freeze/quit)
0x02 0x00  (trash)
0x03 0x00  (next frame)
0x04 0x00  (time)
0x05 0x00  (money)
0x06 0x00  (location)
0x07 0x00  (dead part)
0x08 0x02  burger
0x09 0x01  beer
0x0a 0x32  rapidheal
0x0b 0x64  shield
0x0c 0xbe  stinger
0x0d 0xe1  scattergun
0x0e 0x00  (reserved)
0x0f 0x13  peptab
0x10 0x29  charmer
0x11 0x3f  dexboost
0x12 0x49  nutrapack
0x13 0xb4  concentrator
0x14 0xff  decloaker
0x15 0x0a  cloaker
0x16 0x64  ground scanner
0x17 0x0c  shunt
0x18 0x12  cryofuse
0x19 0x1f  j-junc module
0x1a 0x34  photon bridge
0x1b 0x55  scanner
0x1c 0x73  flux modulator
0x1d 0x8e  plasma tube
0x1e 0xbf  s/t distorter
0x1f 0xff  control node
```

UI loops
------------

Places where the game will busy-loop waiting for user input.

Initial screen
```
  GEMBIND :0x1f:0326 mp=0xe954 base=0x348e erec=0x1906
  XSTARTUP:0x46:26fe mp=0xe964 base=0x0000 erec=0x1b2a
  XSTARTUP:0x01:2e13 mp=0xe982 base=0x0000 erec=0x1b2a
```

Walking around on ship:
```
  GEMBIND :0x1f:0325 mp=0xfd7e base=0x348e erec=0x1906
  XMOVEONS:0x01:0626 mp=0xfd8c base=0x1c8e erec=0x19a2
  SHIPLIB :0x08:0061 mp=0xfda4 base=0x1c8e erec=0x187c
  SUNDOG  :0x03:01ca mp=0xfdae base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0322 mp=0xfdbe base=0x4f02 erec=0x17da
```

Ground combat:
```
  GEMBIND :0x1f:0325 mp=0xfcb2 base=0x348e erec=0x1906
  XDOCOMBA:0x10:0875 mp=0xfcc0 base=0x0000 erec=0x1c60
  XDOCOMBA:0x01:0af6 mp=0xfcd4 base=0x0000 erec=0x1c60
  GRNDLIB :0x02:001f mp=0xfd96 base=0x0000 erec=0x1820
```

Modal interface:
```
  GEMBIND :0x1f:0325 mp=0xf5d6 base=0x348e erec=0x1906
  XDOINTER:0x01:1793 mp=0xf5e4 base=0x0000 erec=0x1afc
  DONESOFA:0x03:0076 mp=0xfcba base=0x0000 erec=0x184e
```

Pilotage:
```
  GEMBIND :0x1f:0326 mp=0xfd46 base=0x348e erec=0x1906
  XPILOTAG:0x04:00af mp=0xfd54 base=0x1c8e erec=0x1a88
  XPILOTAG:0x06:0289 mp=0xfd6e base=0x1c8e erec=0x1a88
  XPILOTAG:0x01:04f2 mp=0xfd96 base=0x1c8e erec=0x1a88
  SHIPLIB :0x03:0026 mp=0xfda4 base=0x1c8e erec=0x187c
  SUNDOG  :0x03:01c6 mp=0xfdae base=0x4f02 erec=0x17da
  SUNDOG  :0x01:0322 mp=0xfdbe base=0x4f02 erec=0x17da
  KERNEL  :0x33:0ea4 mp=0xfdc8 base=0x008a erec=0x055e
  KERNEL  :0x01:0f01 mp=0xfdd8 base=0x008a erec=0x055e
          :0x02:1ab2 mp=0xfde2 base=0x08be erec=0xd5d0
  KERNEL  :0x01:1ab8 mp=0x008a base=0x008a erec=0x055e
```

What they all share: busy loop call to vq_mouse VDI to get mouse updates.

Locals
--------------

### XDOFIGHT

- `XDOFIGHT_01_L3`  Tractor beam enabled.

### XDOINTER

- `XDOINTER_01_L64`  Current interaction block is read here
- `XDOINTER_01_L164`  Block 0x12 and 0x13 of compressed interaction text are read here
- `XDOINTER_01_L1e` current pointer into XDOINTER_01_L64

Savegame state
---------------

512 bytes of game state from MAINLIB are saved for savegames. These contain
the main player stats, inventory, ship status, location and so on.

State dump with fully repaired and refueled ship:

```
01fac: 06 43 68 65 61 74 74 65 61 6e 00 00 00 00 00 00 .Cheattean......
01fbc: 00 00 00 00 14 db 02 11 0c 0b 07 06 00 98 00 72 ...............r
01fcc: 00 00 00 00 00 00 22 3b 03 e7 03 e7 1e 1e 1e 1f ......";........
01fdc: 62 1e 1e 1e 1f 62 05 05 05 05 01 00 f7 ef ff f3 b....b..........
01fec: 0c 0b 00 0d 00 00 00 02 14 db 02 11 0c 0b 07 06 ................
01ffc: 00 11 00 14 14 db 02 11 0c 0b 07 06 00 11 00 14 ................
0200c: 00 00 00 00 00 00 00 00 00 80 00 00 00 00 12 00 ................
0201c: 00 00 00 00 00 00 00 01 1f 1c 1a 1e 04 1f 1c 1a ................
0202c: 1e 04 1f 1c 1a 1e 04 1f 1c 1a 1e 04 1f 1c 1a 1e ................
0203c: 10 1f 00 00 1f 1c 1e 18 04 1f 1c 1e 18 04 1f 1c ................
0204c: 1e 18 04 1f 1c 1e 18 04 1f 1c 1e 18 10 1f 00 00 ................
0205c: 1f 18 1a 1d 04 1f 18 1a 1d 04 1f 18 1a 1d 04 1f ................
0206c: 18 1a 1d 04 1f 18 1a 1d 10 1f 13 02 1f 1b 19 1a ................
0207c: 04 1f 1b 19 1a 04 1f 1b 19 1a 04 1f 1b 19 1a 04 ................
0208c: 1f 1b 19 1a 10 1f 14 02 1f 18 1c 1c 04 1f 18 1c ................
0209c: 1c 04 1f 18 1c 1c 04 1f 18 1c 1c 04 1f 18 1c 1c ................
020ac: 10 1f 15 01 1f 19 1b 19 04 1f 19 1b 19 04 1f 19 ................
020bc: 1b 19 04 1f 19 1b 19 04 1f 19 1b 19 10 1f 16 02 ................
020cc: 0c 0d 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b ................
020dc: 1c 1d 1e 00 00 00 00 00 10 00 08 00 08 09 0a 00 ................
020ec: 00 00 0e 0f 10 11 12 13 14 15 16 17 18 19 1a 1b ................
020fc: 00 00 00 00 00 00 00 00 00 00 00 03 00 00 00 00 ................
0210c: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0211c: 00 00 00 00 00 00 00 00 00 00 00 01 00 00 00 00 ................
0212c: 00 00 00 00 00 00 00 00 00 00 00 00 00 ff 14 db ................
0213c: 02 11 0c 0b 07 06 00 11 00 14 01 e1 00 00 01 e1 ................
0214c: 00 00 04 66 75 65 6c 69 6e 67 00 00 00 00 00 00 ...fueling......
0215c: 00 00 04 66 75 65 6c 69 6e 67 00 00 00 00 00 00 ...fueling......
0216c: 00 00 00 02 54 ed 0a a8 00 00 00 00 00 00 00 00 ....T...........
0217c: 00 00 00 00 00 00 00 00 00 00 00 10 00 09 00 00 ................
0218c: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
0219c: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ................
```
