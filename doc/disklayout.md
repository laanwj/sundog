The game does not use any kind of file system for the game data, not even a
custom one, it just addresses disk blocks at hardcoded logical addresses. This is
peculiar from a modern day point of view, but perhaps not for 1984.

### Main disk

```
ofs     blk   sz   desc
0x03600            P-system boot track, "physical addressing" base block
0x03e00 0x2cb?     Looks like some extra data
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
0x04800 0x00       P-system "logical addressing" base block
        0x06  1    XSTARTUP:0x0f LoadData
0x05600 0x07  1    Check for writeability at startup in SUNDOG.PRG
0x05800 0x08  3    Systems,planets,cities (XSTARTUP:0x0a LoadData)
0x05e00 0x0b  1    Inv items
0x06600 0x0f  2    Graphics: First 1018 bytes of font (XSTARTUP:0x0f)
0x06a00 0x11  2    Cargo
0x06c00 0x12  3    Compressed? text XSTARTUP:0x0f LoadData
0x0bc00 0x3a  1    Sundog disk identifier
0x0d200 0x45  1    XSTARTUP:0x0b (first four bytes are ??), also XSTARTUP:0x09
0x0d400 0x46  1    XSTARTUP:0x0f LoadData
0x0d600 0x47  1    ??? + checksums (at offset 0x160) XSTARTUP:0x0f LoadData
0x0d800 0x48  ?    MAINLIB:0x4b
```

### Library disk

```
0x05600 0x07  2    XSTARTUP:0x1a?
0x05a00 0x07  1    XSTARTUP:0x1a?
```
