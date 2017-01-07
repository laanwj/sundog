Bootstrapping the interpreter
================================

Bootstrap program receives a structure on the stack
with initial values and memory and boot disk information:
```
a3  interpreter base
```
```
0x00  2b        0x1c46 Boot unit id = 0x0004
0x02  4b     d0 0x1c24 0x000209ac
0x06  4b     d1 0x1c28 Bootstrap BIOS vector = 0x00018876 
0x0a  4b     d2 0x1c2c Memory base (a6) 0x000237ac 
0x0e  4b     d3 0x1c30 Initial stack pointer (=end of base memory) 
                       membase+SP_Upr for initial task
                       = 0x00033598 = a6+0xfdec
0x12  4b     d4 0x1c34 Number of tracks per disk | Number of sectors per track = 0x00500009 
0x16  4b     d5 0x1c38 Number of bytes per sector | ? =  0x02000001 
0x1a  4b     d6 0x1c3c Starting sector? | Another floppy related offset = 0x00010000
0x1e  4b     d7 0x1c40 ? | ? = 0xffecfdec
0x22  2b        0x1c4a Size of extended pool in kB? = 0x00cd
0x24  0x800     Global directory (padded with 0xe5 0xe6)
                This is loaded from block 11 (relative to boot sector) or block 2 (relative to p-system logical base)
                It is always 4 blocks large.
```

Information necessary to set up initial interpreter state
and memory:
```c
struct psys_bootstrap_info {
    psys_word boot_unit_id;             /* Boot disk unit number */
    psys_word isp;                      /* Initial stack pointer */
    psys_byte global_directory[0x800];  /* Global file directory */
    psys_fulladdr fake_mem_base;        /* Fake 32-bit memory base address */
    psys_word ext_mem_size;             /* Size of extended memory in KiB */
};
```


Steps
--------

- Determine initial SP a7 (sp\_upr)
- Copy global directory 0x800 words
- Set memory base a6
- Initialize syscom (zero 0x60 bytes, fill in initial values)
  - Global directory pointer at 0x0008
- Determine endian of global directory from (+3).b != 0 -> good endian
- If wrong endian, swap a few words (this code seems broken, luckily we're in the right endian)
- Look for SYSTEM.PASCAL, get base block and size
  - If not found, fatal error
- Load first block (segment dictionary) of SYSTEM.PASCAL into memory
  (push on stack)
- Determine endian of segment dictionary (+0x1fe).w=0x0001 -> good endian
- If wrong endian, swap a few words (not taken)
- Load last segment of segment dictionary (=USERPROG) into memory. A segment dictionary
  can describe up to 16 distinct segments. Segment 15 of the initial segment dictionary
  is special and is the boot segment.
  (push on stack)
- Determine endian of USERPROG (+0xe).w=0x0001 -> good endian
- Byteswap USERPROG if necessary
  - swap two words at offset 0x0 (procedure dict offset, relocation offset)
  - swap four words at offset 0xe
  - swap number of procedures at end of procedure dictionary
  - for each procedure swap word at procaddr(numlocals) and the word before it(endaddr)
- Set up structure at membase+0x60 (extended memory pool?)
- Set up initial TIB at membase+0x60+0x0e
- KERNEL globals are at membase+0x60+0x0e(extdescsize)+0x1c(tibsize)
  (which is also the first global MSCW)
- Fill in values in inititial TIB
- sp-=0x34 for initial EVEC, EREC, SIB
- Fill in initial EVEC
- Fill in initial EREC
- Fill in initial SIB
- sp-=0xa for initial local MSCW
- Fill in initial local MSCW
- Set up other interpreter registers (READYQ etc)
- CURPROC=1
- Determine address of USERPROG:0x01
- Set IPC to that
- Jump to interpreter

Initial memory layout
------------------------

```
0x0000  SYSCOM
0x0060  Extended memory pool info
0x006e  Initial TIB
0x008a  Initial global MSCW / KERNEL globals
isp-0xa3e-s  0xa    Initial local MSCW
isp-0xa34-s  0x34   Initial EVEC, EREC, SIB
isp-0xa00-s  s      USERPROG (where s=0xef7*2 is USERPROG code size from segment dictionary)
0xf3ec=isp-0xa00  0x200    First segment dictionary of SYSTEM.PASCAL
0xf5ec=isp-0x800  0x800    Global directory
0xfdec=isp  Start of 512-byte sector buffer, initial SP
0xffec  Start of ? table (contains 0x0000 0x0001 .. 0x0008 0x0000), apparently filled in by boot sector
                          Logical to physical sector mapping?
```

Initial stack layout
----------------------

So the initial stack layout will be:

0x0              SP\_Upr

