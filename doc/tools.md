Tools
===========

This project comes with tooling for analyzing p-system files. To use this, you
first need to extract the individual files from the image.

Extracting files
-------------------

To extract the files `SYSTEM.INTERP`, `SYSTEM.PASCAL`, `SYSTEM.STARTUP` and
`SYSTEM.MISCINFO` use the tool `ucsdpsys_disk` tool from
[ucsd-psystem-fs](http://ucsd-psystem-fs.sourceforge.net/):

```bash
mkdir files
ucsdpsys_disk -f sundog.st -g files
```

### Files

- `SYSTEM.INTERP` contains 68000 code for the interpreter (PME). This is not used
  by this project.
- `SYSTEM.PASCAL` contains the Pascal library and p-system OS. This is the first
  file that is located and loaded.
- `SYSTEM.STARTUP` contains the game itself.
- `SYSTEM.MISCINFO` contains miscelleneous configuration information. This is loaded
  by the p-system OS. Some of the parameters (those to do with memory) are overridden
  in our loader.

Only `SYSTEM.PASCAL` add `SYSTEM.STARTUP` contain p-code, and can be disassembled using
the following tool.

Disassembling
---------------

The main tool for disassembling and analyzing p-code files is `tools/list_pcode.py`.

Basic usage is:

```bash
tools/list_pcode.py files/SYSTEM.STARTUP | less -R
```

To dump a specific segment and procedure number use:

```bash
tools/list_pcode.py -s USERPROG:0x02 files/SYSTEM.PASCAL
```

A full list of options is available with:

```bash
tools/list_pcode.py --help
```

Example output for `tools/list_pcode.py -c 0 -s SUNDOG:0x01 files/SYSTEM.STARTUP`:
```
       ; start of procedure SUNDOG:0x01 Main()
       ;   num params: 0
02c4:  .dw 0x0340
02c6:  .dw 0x0000
 1 02c8:  82 80 29    lco 0x29 ; "GEMBIND,MAINLIB" @ 0x0394
 2 02cb:  08          sldc8 
 2 02cc:  9d          lpr 
 0 02cd:  94 0a 04    cxg 0xa,0x4 ; EXTRAHEA:0x04 s_mem_lock(seglist:string)
 1 02d0:  00          sldc0 
 0 02d1:  d9 04 83 ae ste 0x4,0x3ae ; MAINLIB_G3ae
 1 02d5:  00          sldc0 
 0 02d6:  d9 04 8a 76 ste 0x4,0xa76 ; MAINLIB_Ga76
 1 02da:  9b 04 8a 74 lae 0x4,0xa74 ; MAINLIB_Ga74
 2 02de:  e2          dup1 
 3 02df:  01          sldc1 
 1 02e0:  c4          sto 
 1 02e1:  e7 01       inc 0x1
 2 02e3:  98          ldcn 
 0 02e4:  c4          sto 
 1 02e5:  9b 04 01    lae 0x4,0x1 ; MAINLIB_G1
 2 02e8:  e2          dup1 
 3 02e9:  00          sldc0 
 1 02ea:  c4          sto 
 1 02eb:  e7 01       inc 0x1
 2 02ed:  98          ldcn 
 0 02ee:  c4          sto 
 1 02ef:  9b 04 01    lae 0x4,0x1 ; MAINLIB_G1
 2 02f2:  00          sldc0 
 0 02f3:  70 1d       scxg1 0x1d ; KERNEL:0x1d attach(semaphore,vector)
 0 02f5:  8a 04       ujp 02fb

   02f7:  86 01       lao 0x1 ; SUNDOG_G1
   02f9:  91 02       cgp 0x2 ; SUNDOG:0x02 BackgroundTask(a)

 1 02fb:  9b 04 8a 77 lae 0x4,0xa77 ; MAINLIB_Ga77
 2 02ff:  81 d0 07    ldci 0x7d0
 3 0302:  80 a0       ldcb 0xa0
 4 0304:  86 01       lao 0x1 ; SUNDOG_G1
 0 0306:  94 09 03    cxg 0x9,0x3 ; CONCURRE:0x03 SstartP
 0 0309:  8a 03       ujp 030e
   030b:  9c          nop 
   030c:  8a e9       ujp 02f7

 1 030e:  01          sldc1 
 0 030f:  76 02       scxg7 0x2 ; DONESOFA:0x02 DoStartup(flag)

 1 0311:  01          sldc1 
 2 0312:  80 3c       ldcb 0x3c
 0 0314:  73 30       scxg4 0x30 ; MAINLIB:0x30

 1 0316:  9b 04 32    lae 0x4,0x32 ; MAINLIB_G32
 2 0319:  04          sldc4 
 3 031a:  08          sldc8 
 1 031b:  c9          ldp 
 2 031c:  02          sldc2 
 1 031d:  b2          leqi 
 0 031e:  d4 04       fjp 0324
 0 0320:  91 03       cgp 0x3 ; SUNDOG:0x03
 0 0322:  8a 02       ujp 0326

 0 0324:  91 04       cgp 0x4 ; SUNDOG:0x04 MainDispatch()

 1 0326:  9b 04 32    lae 0x4,0x32 ; MAINLIB_G32
 2 0329:  04          sldc4 
 3 032a:  08          sldc8 
 1 032b:  c9          ldp 
 2 032c:  0b          sldc11 
 1 032d:  b3          geqi 
 0 032e:  d4 e6       fjp 0316
 1 0330:  9b 04 8a 78 lae 0x4,0xa78 ; MAINLIB_Ga78
 2 0334:  00          sldc0 
 3 0335:  80 30       ldcb 0x30
 4 0337:  00          sldc0 
 0 0338:  70 15       scxg1 0x15 ; KERNEL:0x15 fillchar(dest:bytearray; n_bytes,value:integer)
 1 033a:  00          sldc0 
 0 033b:  76 02       scxg7 0x2 ; DONESOFA:0x02 DoStartup(flag)
 1 033d:  00          sldc0 
 0 033e:  d4 d1       fjp 0311
   0340:  96 00       rpu 0x0
; ----------------------------------
```

The first two `.dw` numbers are the "end pointer" for the procedure (the VM will go here in case of an exit),
and the number of locals to be allocated for this function. After that the disassembled code follows.

Overall format:

```
[depth]  [addr]  [bytes]   [instruction]   ; [comment]
```

- The number in the first column is the stack depth after the instruction. This
  can be used to quickly correlate certain code with arguments of functions or
  other instructions. If specified it is always correct, however some
  instruction such as the set instructions push or pop variable numbers of
  words that can not be statically determined in that case the depth may be
  unknown (left empty).

- Then the address, relative to the current segment, follows.

- Then the instruction bytes follow.

- Then the disassembled instruction follows.

- Optionally a comment wil follow.
  - For procedure calls this will specify the
  fully qualified identifier of the procedure and a human-readable name (if known).
  - For locals and globals it will also fabricate a name, for easier searching.
  - When loading the address of a constant it will display the contents of that constant,
  if recognizable as a string.

Other tools
-------------

There are a few miscellaneous single-purpose tools, most of them related to the
game, for example `extract_gfx.py` can be used to extract the font and mouse
cursors.
0x0200-0x05ae [d]0x0542 [2]0x05ac [4]0x0200 SUNDOG   BE 00000000 0201 0000 8002 0x0002 0x000b
