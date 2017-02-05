The game does not use any kind of file system for the game data, not even a
custom one, it just addresses disk blocks at hardcoded logical addresses. This is
peculiar from a modern day point of view, but perhaps not for 1984.

`ofs` is relative to start of the disk image, `blk` is the logical block from the
viewpoint of the p-system, `sz` is the size in blocks of the described area.

### Main disk

```
ofs     blk   sz   desc
0x03600 -0x09 9    P-system boot track, "physical addressing" base block
0x03e00 -0x05 4    Track 3, sector 4 - used in copy-protection. First, the block at 0x4400 is read,
                   to direct the drive there. Then, blocks at 0x3e00, 0x4000 and 0x4200
                   are read, subsequently. Block at 0x3e00 is verified against
                   a checksum.
0x03f00-0x4400     Loaded at a6+0x89cc
  MAINLIBc+0x1344   MAINLIB:0x4b
  DONESOFAc+0x37    DONESOFA:0x04
  XSTARTUPc+0x7be   XSTARTUP:0x0f
  XSTARTUPc+0x2e11  XSTARTUP:0x01
  DONESOFAc+0x69    DONESOFA:0x02
  SUNDOGc+0x311     SUNDOG:0x01 main
  KERNELc+0xea4     KERNEL:0x33
  KERNELc+0xf01     KERNEL:0x01
  USERPROGc+0x1ab2  end of USERPROG:0x02
  KERNELc+0x1ab8
  KERNELc+0x1ab8    USERPROG:0x01  (loop, root activation record at a6+0x008a)
0x04400 -0x01 1    Sound table (0x48 bytes offsets then actual DoSound data)
0x04800 0x00       P-system "logical addressing" base block
        0x06  1    XSTARTUP:0x0f LoadData
0x05600 0x07  1    Check for writeability at startup in SUNDOG.PRG
0x05800 0x08  3    Systems,planets,cities (XSTARTUP:0x0a LoadData)
0x05e00 0x0b  1    Inv items
0x06600 0x0f  2    Graphics: First 1018 bytes of font (XSTARTUP:0x0f)
0x06a00 0x11  2    Cargo
0x06c00 0x12  2    Compressed(?) text XDOINTER:0x24 start XSTARTUP:0x0f LoadData
0x07000 0x14       Compressed(?) text XDOINTER:0x1f
0x08c00 0x22       Looks like end of compressed text
0x0bc00 0x3a  1    Sundog disk identifier
0x0d200 0x45  1    XSTARTUP:0x0b (first four bytes are ??), also XSTARTUP:0x09
0x0d400 0x46  1    XSTARTUP:0x0f LoadData
0x0d600 0x47  1    ??? + checksums (at offset 0x160) XSTARTUP:0x0f LoadData
0x0d800 0x48  260  MAINLIB:0x4b  Image data
0x2e000 0x14c      SYSTEM.PASCAL
```

### Images

2b width 2h height then compressed image data:

0x0d800  320x200  0x26b4 Title image
0x0feb8  224x51   0x323  Sundog logo
0x101e0  320x200  ?      ?
0x11552  496x16   0x645  Items
0x11b9c  16x133   ?      ?
0x126ca  320x200  0x1057 Ship interior
0x13726  320x200  ?      ?
0x1b166  48x40    0x1f0  Background
0x1b35a  160x32   0x25d  Special buildings
0x1cd18  416x32   0x698  Buildings
0x1d3b6  416x32   0x870  Buildings

### Library disks

Library disks can be used to store extra save games. These are also accessed by
means of raw disk access.

```
0x05600 0x07  2    XSTARTUP:0x1a?
0x05a00 0x07  1    XSTARTUP:0x1a?
```
