Glossary
------------

- words: are 2 bytes (like ST)
- longs: are 4 bytes (like ST)
- activation record or MSCW: structure at the base of local/global variables
  describing static and dynamic call hierarchy.
- SIB: Segment Information Block
- TIB: Task Information Block
- MSCW: Mark Stack Control Word
- E\_Rec: Environment record
- E\_Vec: Environment vector
- RSP: Run-Time Support Package. This is a component related to the interpreter
  that provides standardized, natively implemented utility calls for I/O and
  moving memory to the p-system.

Which p-system version?
------------------------

According to memory dump: [IV.2.1 R3.4]
This is a version of the p-system developed commercially by Softech.
One of the latest (last?) releases according to Wikipedia.

A version IV of the p-system for the Nec APC (8086) could be found [here](https://winworldpc.com/product/ucsd-p-system/iv).

Although there is no working emulation for this, the disk image has some hints
in regards to opcodes. A full overview (with descriptions) of opcodes can be
found in [1].

VM
-------

- Stack-based machine (similar to JVM)
- Stack grows down
- Locals are on the stack frame, can be fetched/indexed separately as a frame pointer
  is stored.
- Globals are per segment (segment library info specifies size to allocate)
- Intermediate load/store are used for nested procedure handling
  "Lexical levels in Pascal correspond to the static nesting levels of procedures and functions"
- CXP (or one of the 0x7x variants) is used to call between segments
- CXP1 is special and calls the OS
  - OS calls are dispatched from a table. From disassembling the code it seems
    OS call 0x11 is used even though it's not defined. Is the slot in the table filled in later
    somehow? KERNEL? I think so: 
    - "segnum in parent" for the kernel is 0x01. It is the only segment with this property.
    - there are many holes in KERNEL's procedure list. Some of the OS call numbers snugly
      fit in here (but not all!). Looks like they line up except for the restricted calls.
- CXP2 is also special: import 0x02 is always the (top level) segment itself? likely depends on segnum in header
  This is correct, off-by-one "In fact, only one segment has a fixed number: KERNEL, which is segment number 0. For all others, the compiler assigns each segment an arbitrary number, from 2 to 255 (number 1 is reserved for the current segment)"

- Subsegments share the imports table of the segment they're in

- What are the different variable areas? I'm lost.
  - memory (addressed from fixed memory base pointer)
  - globals (segment-local, reachable from memory base pointer)
  - locals (function-local, on stack, reachable from memory base pointer)
  - segment code (addressed from 0 in segment)
  - segment data (addressed from pointer at offset 14 in segment)
  - anything else? What is addressed by STVM/LDVM? segment descriptor something (internal VM state?)

  Segment code/data is not reachable from the memory base pointer, and should be considered read-only. It can be
  copied/compared using some strings ops.

- Strings are in ASCII format, prefixed by a length byte.
  This means the maximum string length is 255 bytes (or 256 with the length byte included).
  They are usually copied from the constant pool with LCO followed by an ASTR 0x1,<maxlen> instruction.

- Arguments directly follow the local variables on the stack. If a function has no locals, SLDL0 will get the
  first argument.
  Return values follow the arguments. These are passed in as dummy (zero) arguments, and overwritten, and not
  popped of the stack on return.

- Segments can access globals from another segment.

- Globals for a subsidiary segment are the same as those for the parent segment.

- A simple (non-preemptive, synchronous) form of concurrency is supported.
  SunDog has a background process: SUNDOG:0x02.
  This blocks on p-system event 0 then, depending on game state, dispatches to a function.
  (what is event 0? a timer? VBLANK interrupt?)

- The SYSCOM area is at the start of memory accessible by the VM.
  The first word of this is the IORESULT.
  The exact layout is specified nowhere, however field names appear both in the reference guide as in ti99-psystem.htm

- On initial load, the globals of a segment are zeroed. No initial values are set.

- Like in C, locals are not initialized.

- Boolean operations (fjp etc) test only bit 0 of the operand.

- `SYSTEM.MISCINFO` contains some initial state loaded to SYSCOM.

Calling convention
-------------------

The typical PASCAL calling convention is used: arguments are pushed starting from the last,
ending at the first. Most values are one word in size. There are the following exceptions:

- string: represented by two words, an offset and an EREC pointer. If the EREC pointer is NIL,
  the offset is relative to memory, otherwise relative to that segment.
- bytearray: same as string
- packed element address: addr, lowbit, highbit

Functions (that return a value) work by having the caller push, before the arguments, dummy
arguments that will be overwritten by the function. These will then not be removed on return
(excluded from RPU value).

Opcodes
-------------

No direct match with opcodes in `lib/pcode.h` from ucsd-xc, though I expect them to be similar.
Overall structure of the interpreter is the same as the version II one.

Encoding:
- 1: byte
- 2: word (LE)
- V: 1/2 byte varlen. Called "big" in other documentation.

```
0x00-0x1f  0      SLDC   Short load constant
0x20-0x2f  0      SLDL   Short load local word
0x30-0x3f  0      SLDO   Short load global word
0x40-0x5f  0      -      Generate error 0xb: Instruction not implemented
0x60-0x67  0      SLLA   Short load local address
0x68-0x6f  0      SSTL   Short store local word
0x70-0x77  1      SCXGx  Extended call 94 w/ arg 1..8 
0x78       0      SIND0  Load indirect word [a6 + top]
0x79-0x7f  0      SINDx  Load indirect word [a6 + (opcode-0x78)*2 + top]
0x80       1      LDCB   Push immediate byte as word
0x81       2      LDCI   Push immediate word
0x82       V      LCO    Computes a pointer relative to segment, from constant pool number
0x83       1,V,1  LDC    Copy words from segment data to stack
0x84       V      LLA    Load local address
0x85       V      LDO    Get global from varlen encoded address
0x86       V      LAO    Load global address
0x87       V      LDL    Get local from varlen encoded address
0x88       1,V    LDA    Load intermediate address
0x89       1,V    LOD    Load intermediate word
0x8a       1      UJP    Jump +/- by argument signed byte
0x8b       2      UJPL   Jump +/- by argument LE word
0x8c       0      MPI    Signed multiply integers
0x8d       0      DVI    Signed divide integers
0x8e       1      STM    Store multiple
0x8f       0      MODI   Signed modulus integers
0x90       1      CLP    Call local
0x91       1      CGP    Call global procedure
0x92       1,1    CIP    Call intermediate
0x93       1,1    CXL    Intersegment call local
0x94       1,1    CXG    Intersegment call global
0x95       1,1,1  CXI    Intersegment call intermediate
0x96       V      RPU    Return from function? (always at end of functions) Argument is number of words on stack to throw away.
0x97       1      CPF    Call Formal Procedure (dynamic call)
0x98       0      LDCN   Push word NIL (0 on 68000)
0x99       1      LSL    Load static link
0x9a       1,V    LDE    Load global word from another segment
0x9b       1,V    LAE    Get global address from another segment
0x9c       0      NOP    NOP
0x9d       0      LPR    Load word from VM state
0x9e       0      BPT    Generate error 0x10
0x9f       0      BNOT   Boolean (logical) NOT
0xa0       0      LOR    Bitwise OR
0xa1       0      LAND   Bitwise AND
0xa2       0      ADI    Add integers
0xa3       0      SBI    Subtract integers
0xa4       V      STL    Store local word
0xa5       V      SRO    Store global word
0xa6       1,V    STR    Store intermediate word
0xa7       0      LDB    Load memory byte from (base address, offset)
0xa8       4+     NATIVE Jump to 68000 code
0xa9       V      NAT-INFO Jump: add argument varlen word to PC
0xaa              -      Not implemented (error 0xb)
0xab       V      CAP    Memory copy operation
0xac       1      CSP    Memory copy operation
0xad       V      SLOD1  Load memory from next intermediate stack frame
0xae       V      SLOD2  Load memory from next+1 intermediate stack frame
0xaf       -      -      Not implemented (error 0xb)
0xb0       0      EQUI   1 if eq otherwise 0
0xb1       0      NEQI   1 if ne otherwise 0
0xb2       0      LEQI   1 if bge otherwise 0 (signed)
0xb3       0      GEQI   1 if ble otherwise 0 (signed)
0xb4       0      LEUSW  0 if bcs otherwise 1 (unsigned)
0xb5       0      GEUSW  1 if bls otherwise 0 (unsigned)
0xb6       0      EQPWR  Equal Set
0xb7       0      LEPWR  Lower than or equal set
0xb8       0      GEPWR  Greater than or equal set
0xb9       1,1,V  EQBYTE Compare strings ==
0xba       1,1,V  LEBYTE Compare strings >=
0xbb       1,1,V  GEBYTE Compare strings <=
0xbc       0      SRS    ?
0xbd       0      SWAP   Swap top and top-1 words
0xbe       ?      TRUNC  Not implemented (error 0xc)
0xbf       ?      ROUND  Not implemented (error 0xc)
0xc0       ?      ADR    Not implemented (error 0xc)
0xc1       ?      SBR    Not implemented (error 0xc)
0xc2       ?      MPR    Not implemented (error 0xc)
0xc3       ?      DVR    Not implemented (error 0xc)
0xc4       0      STO    Store word top+0 into memory address top+1, pop 2
0xc5       1,V    MOV    Memory copy words from memory/segment to memory
0xc6       ?      DUP2   Not implemented (error 0xc)
0xc7       1      ADJ
0xc8       0      STB    Store byte at (base address, offset)
0xc9       0      LDP    Load packed
0xca       0      STP    Store packed
0xcb       0      CHK    Check bounds
0xcc       ?      FLT    Not implemented (error 0xc)
0xcd       ?      EQREAL Not implemented (error 0xc)
0xce       ?      LEREAL Not implemented (error 0xc)
0xcf       ?      GEREAL Not implemented (error 0xc)
0xd0       1      LDM    Memory copy operation (temporarily uses a0)
0xd1       0      SPR    Store word to VM state
0xd2       1      EFJ    Jump if top+0 != top+1, pop 2
0xd3       1      NFJ    Jump if top+0 == top+1, pop 2
0xd4       1      FJP    Jump if ==0
0xd5       2      FJPL   Jump +/- by argument LE word if top == 0, pop 2
0xd6       V      XJP    Jump table: argument is offset in segment data, first word is min value, second is max value, following words are relative offsets
0xd7       V      IXA    Multiply top element by immediate varlen, double it, add to second-to-top. Array indexing?
0xd8       1,1    IXP    Index packed array
0xd9       1,V    STE    Store global word in another segment
0xda       0      INN    Set membership
0xdb       0      UNI    Set union
0xdc       0      INT    Set intersection
0xdd       0      DIF    Set difference
0xde       0      SIGNAL
0xdf       0      WAIT
0xe0       0      ABI    Absolute value of integer
0xe1       0      NGI    Negate integer
0xe2       0      DUP1   Duplicate top element
0xe3       0      ABR    Not implemented (error 0xc)
0xe4       0      NGR    Not implemented (error 0xc)
0xe5       0      LNOT   Bitwise NOT
0xe6       V      IND    Load word from memory [Varlen + Top]
0xe7       V      INC    Add varlen imm to top
0xe8       1,1    EQSTR  Equal string
0xe9       1,1    LESTR  Less than or equal string
0xea       1,1    GESTR  Greater than or equal string
0xeb       1,1    ASTR   Memory copy bytes from memory/segment to memory
0xec       0      CSTR   Check string index
0xed       0      INCI   Increase integer
0xee       0      DECI   Decrease integer
0xef       0      SCIP1  CIP 1
0xf0       0      SCIP2  CIP 2
0xf1       1      TJP    Jump if !=0
0xf2       ?      LDCRL  Not implemented (error 0xc)
0xf3       ?      LDRL   Not implemented (error 0xc)
0xf4       ?      STRL   Not implemented (error 0xc)
0xf5       ?      CNTRL  Not implemented (error 0xb)
0xf6       ?      EXPRL  Not implemented (error 0xb)
0xf5-0xff  0      -      Generate error 0xb: Instruction not implemented
```

- Floating point operations are not implemented: These raise error 0xc
- `psystem.htm` has a list of descriptions of many of these instructions, by name
- More extensive reference can be found in the "softech microsystems p-systems reference.pdf"

Variable-length words
----------------------

Variable-length words are encoded as 1 or 2 bytes:

- 1 byte `0xXX` < `0x80`. Value is `0xXX`.
- 2 byte `0xXX` `0xYY`. Value is `((X&0x7f)<<8)|Y`

Usually this result is multiplied with 2, at least when addresses are involved,
but this is not inherent to the encoding but to the Atari ST being a byte-addressed machine.

Interpreter state
------------------

(registers)
- a0 (MP/MSCW locals base)
- a1 (BASE globals base)
- a2 (CURSEG segment base)
- a3 (interp base) at 0x000209AC   SYSTEM.INTERP+0
- a4 (IPC interp pc) at 0x0003FCD7
- a5 (interp loop) at 0xde(a3)
- a6 (memory base)
- a7 (SP stack)

```
0x1b66(a3) 2  Real size (0 if no reals installed)
0x1b6c(a3) 4  Stored a0 MP
0x1b70(a3) 4  Stored a1 BASE
0x1b74(a3) 4  Stored a2 CURSEG
0x1b7c(a3) 4  Stored a4 IPC instruction pointer
0x1b80(a3) 4  Stored a5 _func486
0x1b84(a3) 4  Stored a6 (memory base)
0x1b88(a3) 2  READYQ: Set/used during initialization, copied to 1b8c
0x1b8a(a3) 2  EVEC: references to linked segments EREC
0x1b8c(a3) 2  CURTASK
0x1b8e(a3) 2  EREC: Pointer to current segment E_Rec
0x1b90(a3) 2  CURPROC: Current procedure number
0x1b92(a3) 1  flags 
              &1 set on error
              &2 something wih WAIT/SIGNAL
              &4 determines sys call jump table
              &8 initialization
              &32  cleared in function 0x0000048c
0x1b94(a3) 2  Saved EREC? copied to EREC in 0xebc New erec for call.
0x1b96(a3) 4  Store old CURSEG in _hand_return_a
0x1b9a(a3) 128   64 words: event to semaphore table
0x1c1a(a3) 4  ? cleared in 0x1866
0x1c1e(a3) 4  ? cleared in 0x1866
0x1c22(a3) 1  flags? cleared in 0x1866 set 0x188a
0x1c24(a3) 4  Initial d0
0x1c28(a3) 4  Initial d1 = Bootstrap vector
0x1c2c(a3) 4  Initial d2 = memory base (a6)
0x1c30(a3) 4  Initial d3 = membase+SP_Upr for initial task
0x1c34(a3) 4  Initial d4 = Number of tracks per disk | Number of sectors per track
0x1c38(a3) 4  Initial d5 = Number of bytes per sector | ?
0x1c3c(a3) 4  Initial d6
0x1c40(a3) 4  Initial d7
0x1c44(a3) 2  Disk base block for SYSTEM.PASCAL
0x1c46(a3) 2  Initial state (first init word on stack), physical unit of boot disk
0x1c48(a3) 2  Initial SIB
0x1c4a(a3) 2  Initial state (last init word on stack) = Code pool memory in KiB
0x1c4c(a3) 2  Initial MSCW and globals pointer for USERPROG
0x1c4e(a3) 4  Set from 0x1c30 in initialization (base of code pool?)
0x1c52(a3)    End of interpreter state
```

Looks like there is some (read-only?) data after this state
E.g. from `0x0000268e` looks like some graphics or another jump table

```
0x1c42(a3) 2  disk buffer
0x1c46(a3) 2  used during init
0x2196(a3) 4  interrupt handler (fetched by _bios_UNK80), this will point to a3+0x10e2
0x2188(a3) 2  current disk sector
0x218a(a3) 2  current disk track
```

Interpreter/p-system data structures
--------------------------------------

Pages in p-systems internal reference [1]:

- SIB Segment descriptor (3-9 / 79)
- E\_Vec "Environment Vector" (3-13 / 83)
- E\_Rec "Environment Record" (3-13 / 83)
- TIB Task\_Info (see 3-19 / 89)
- Pooldes (see 5-14 / 392)
- Heapinfo (see 5-10 / 388)
- MemLink (see 5-9 / 387)
- Fault\_Message (see 5-18 / 396)
- Fault\_Sem (see 5-18 / 396)
- FIB (see 5-22 / 400)
- Activation record/MSCW/MP (see 3-47)
- Task\_Info (see 5-20)
- SYSCOM 14-26 (see 3-30 / 100)
- Devices and device (unit) numbers (see 4-10 / 310)
- Low-level I/O errors (see 4-14 / 314)
- Execution error codes (see 3-33 / 103)
- Instruction parameter types (see 3-42 / 112)
- Instruction dynamic operand types (3-44 / 114)
- Executable code segment format (2-5 / 35)

SYSCOM
--------------

SYSCOM layout. This layout differs from any of the (sparse) documented ones I could.
Offsets are in bytes
```
0x00      2  IORESULT     Last I/O completion status
0x04      2  ?            Physical unit of bootloader Pushed to stack in 0x00001866 (initailized to 0x1c46(a3))
0x08      2  ?            Global directory pointer
0x0e      ?  Real_Sem     semaphore to start the faulthandler
0x12      2  Fault_TIS    TIS of faulting task
0x14      2  Fault_EREC   E_REC of segment to read
0x16      2  Fault_Words  number of words needed on stack
0x18      2  Fault_Type   80H=segment fault, 81H=stack fault, 82H=heap fault, 83H=pool fault
0x1a      2  ?            Used in _unitrw_internal (initialized to 0x80)
0x20      2               Have code pool
0x22      2               Code pool size in kb
0x2a      2  Time_Stamp   Current "time stamp" counter
0x2c      2  ?            Used in _unitrw_internal
0x2e      2  ? | ?        Used in _unitrw_internal
0x32      2  ?            Initialized to 0xb
0x34      2  ?            Initialized to 0x60 - points to code pool information block
0x36      2  ?            initialized to 0x2
0x38      2  ?            initialized to 0x1b66(a3) - real size (0 if no reals installed)
0x3a      2  ? | NOBREAK ("MISCINFO")
0x52      2  FLUSH |  EOF
0x54      2  STOP/START |  BREAK
0x5c      2  ALPHALOCK | CHARMASK  (initialized to 0x7f)
```
From FRONTLST.TXT
(this is not entirely correct for us)

```
0x00  iorslt : iorsltwd;   { RESULT OF LAST IO CALL }
0x02  rsrvd1 : integer;
0x04  sysunit : unitnum;   { PHYSICAL UNIT OF BOOTLOAD }
0x06  rsrvd2 : integer;
0x08  gdirp : dirp;        { GLOBAL DIR POINTER,SEE VOLSEARCH }
      fault_sem : RECORD
0x0a         real_sem, 
0x0e         message_sem : SEMAPHORE;
             message : fault_message = RECORD 
0x12           fault_tib : tib_p;
0x14           fault_e_rec : e_rec_p;
0x16           fault_words : INTEGER;
0x18           fault_type : seg_fault..pool_fault;
             END {of fault_message};
           END {of fault_sem};
        { starting unit number for subsidiary volumes}
0x1a  subsidstart : unitnum;  
0x1c  rsrvd3 : integer;  
0x1e  spool_avail : boolean;
      poolinfo : record
0x20               pooloutside : boolean;
0x22               poolsize    : integer;
0x24               poolbase    : fulladdress;
0x28               resolution  : integer;
                 end;
0x2a  timestamp : integer;
0x2c  unitable : ^utable;
      unitdivision : packed record
0x2e               serialmax : byte;  {number of user serial units}
                   subsidmax : byte;  {max number of subsid vols}
                 end;
      expaninfo: packed record
0x30               insertchar,deletchar:char;
0x32               expan1,
0x34               expan2:integer;
                 end;
0x36  pmachver : (pre_iv_i, iv_i, post_iv_1);
0x38  realsize : integer;
0x3a  MISCINFO: PACKED RECORD
                  NOBREAK,STUPID,SLOWTERM,
                  HASXYCRT,HASLCCRT,HAS8510A,HASCLOCK: BOOLEAN;
                  USERKIND:(NORMAL, AQUIZ, BOOKER, PQUIZ)
                END;
0x3c  CRTTYPE: INTEGER;
      CRTCTRL: PACKED RECORD
0x3e             RLF,NDFS,
0x40             ERASEEOL,ERASEEOS,
0x42             HOME,ESCAPE: CHAR;
0x44             BACKSPACE: CHAR; FILLCOUNT: 0..255;
0x46             CLEARSCREEN, CLEARLINE: CHAR;
0x48             PREFIXED: PACKED ARRAY [0..8] OF BOOLEAN
               END;
      CRTINFO: PACKED RECORD
0x4a             WIDTH,
0x4c             HEIGHT: INTEGER;
0x4e             RIGHT,LEFT,
0x50             DOWN,UP: CHAR;
0x52             BADCH,CHARDEL,
0x54             STOP,BREAK,          BREAK (LE)0x54/0x55(BE) according to reference
0x56             FLUSH,EOF: CHAR;     FLUSH (LE)0x53/0x52(BE) according to reference?
0x58             ALTMODE,LINEDEL: CHAR;
0x5a             alphalok,char_mask,  0x5d/0x5c according to reference
0x5c             ETX,PREFIX: CHAR;
0x5e             PREFIXED: PACKED ARRAY [0..15] OF BOOLEAN;
               END
    END { SYSCOM };
(size is 0x60)
0x60      ?               Some memory description structure, not sure
0x6e      ?               Initial TIB
0x7c      2  ?            TIB.IPC at initialization
```
Looks like a,b,c,d,e,f,...:CHAR will end up in memory as
```
...
word e | f
word c | d
word a | b
```

Semaphore
--------------

Layout of a semaphore in memory:
```
0x00      2               Count
0x02      2               First tib in queue
```

Task descriptor
-----------------------
Called a "TIB" in the documentation.
TIB (see 3-19)

```
curTIB = a6+(a3+0x1b8c).w
```

```
(spr/lpr arguments are in words, these offsets in bytes!)
0x00 2 Wait_Q
0x02 1 Prior
0x03 1 Flags
0x04 2 SP_Low
0x06 2 SP_Upr
0x08 2 SP
0x0a 2 MP
0x0c 2 Reserved
0x0e 2 IPC
0x10 2 ENV (EREC)
0x12 1 TI_BIOResult (stored IORESULT)
0x13 1 Proc_Num
0x14 2 Hang_Ptr
0x16 2 M_Depend
0x18 2 Main_Task
0x1a 2 Start_MSCW
(size 0x1c)
```

Segment descriptor
-------------------
There's multiple segment descriptors: at top-level the E\_Rec, which points to E\_Vec and SIB.

`EREC = a6+(a3+0x1b8e).w`

Every loaded segment has a segment descriptor. For example

    GEMBIND  a6+0x1906  0x250b2
    MAINLIB  a6+0x18d8  0x25084

```
E_Rec
+0x00 2    Pointer to globals (relative to memory)
+0x02 2    Pointer to EVEC =(r3+0x1b8a).w
+0x04 2    Pointer to SIB
+0x06 2    Link count (only principal)
+0x08 2    Next_rec (only principal)
(size 0x0a, the SIB usually starts straight after here)
```

```
SIB
+0x00 2    Seg_Pool
+0x02 2    Seg_Base
+0x04 2    Seg_Refs
+0x06 2    Time_Stamp
+0x08 2    Link_Count
+0x0a 2    Residency
+0x0c 8    Seg_Name
+0x14 2    Seg_Leng
+0x16 2    Seg_Addr
+0x18 2    Vol_Info
+0x1a 2    Data_Size
+0x1c 2    Next_Sib
+0x1e 2    Prev_Sib
+0x20 2    Scratch
+0x22 2    M_Type
(size 0x24, the E_Rec for next segment usually starts after here)
```

```
E_Vect/EVEC
+0x00 2    Vec_Length
+0x02 2    Array[1..Vec_Length]   E_Rec pointers
```

Locals/globals descriptor
---------------------------
Activation record/MSCW/MP:

MSCW for globals and locals has the same form:

```
0x00 2 MSSTAT  pointer to the activation record of the lexical parent, 0 if none
0x02 2 MSDYN   point to the activation record of the caller
0x04 2 MSIPC   segment relative byte pointer to point of call in the caller
0x06 2 MSENV   E_Rec pointer of the caller
0x08 2 MSPROC  procedure number of caller
... (locals/globals follow)
```

TODO: figure out what MSDYN etc is for non-kernel segments globals. Probably just 0, or points to self.

Pool descriptor
------------------

```
[1]
0x00 4  PoolBase
0x04 2  PoolSize
0x06 2  MinOffset
0x08 2  MaxOffset
0x0a 2  Resolution
0x0c 2  PoolHead
0x0e 2  Perm_SIB
0x10 2  Extended
(0x12 bytes)
```

```
[2]
Extended memory info
Structure at 0x60, pointed to from SYSCOM+0x34

+0x00 2  0x6
+0x02 2  ?
+0x04 2  ?
+0x06 2  ?
+0x08 4  base of code pool (absolute address) = 0x00033598
+0x0c 2  size of code pool in 512 byte blocks = 0x01a0
(0xe bytes)
```

Executable format
--------------------
Header:

```
0x00 2b  Total size (/2)
0x02 2b  Always 8
0x04 8b  Name of segment
0x0c 2b  Always 1
0x0e 2b  Code size (/2) - after this, data follows
0x10 2b  Always 4?
0x12 4b  ?
0x16 2b  First function header (size)
```

```
h=ffff    native code
   Where is relocation data
```

Runtime errors
----------------

List of error conditions:

```
INVNDX  01  Invalid index
NOPROC  02  Non-existent segment
INTOVR  05  Integer overflow
DIVZER  06  Divide by zero
UBREAK  08  User break
UIOERR  0A  I/O error
NOTIMP  0B  Instruction not implemented
FPIERR  0C  Floating point error
S2LONG  0D  String too long
BPT     10  Breakpoint
```

Descriptions from: http://www.unige.ch/medecine/nouspikel/ti99/psystem.htm#LDC

Disk
---------
KERNEL has the most inner function that handles disk reads
and probably other interfaces to the OS.
This is always done through SUNDOG.PRG's "BIOS" interface.

```
KERNEL+0x0930 (:0x02) 000340E4  First address after reading from floppy: opcodes "0x70 0x27"
KERNEL+0x09d1 (:0x02) 00034185  After return
KERNEL+0x09d7  0003418B  After return
KERNEL+0x08a2  00034056  After return
KERNEL+0x0a48  000341FC  After return
KERNEL+0x0a9b  0003424F  After return
SUNDOG+0x00ca  0003E7E6  After return
```

Loading string constants
---------------------------

This loads the "env" register (EREC?). What is this used for?

```
0417:  08          sldc8
0418:  9d          lpr
```

This is passed to some system unit calls (HEAP, STRINGIO).
"GenConstStrParam" in COMPIV.PAS does this
{ erec ptr if byte array }
Apparently the calling convention for "string" dictates this: a string consists of two words,
either <erec> <segofs> or <NIL> <memofs>

Bootstrap interface
---------------------

The interface from the "BIOS interface" in the interpreter upstream to the
operating system consists of a vector list. In SunDog this interface is
provided by a few compact routines in SUNDOG.PRG.

```
0x00   Init (SYSCTRL)
0x04   HALT (SYSREAD/WRITE)
0x08   Nop. CONSOLECTRL
0x0c   Nop. CONSOLESTAT
0x10   Nop. CONSOLEREAD
0x14   Nop. CONSOLEWRITE
0x18   Nop. Start disk?
0x1c   Disk: Set track to d0
0x20   Disk: Set sector to d0
0x24   Disk: Set buffer to a2
0x28   Disk: Floppy read
0x2c   Disk: Floppy write
0x30   Nop. DISKWRITE/DISKREAD/DISKCTRL
0x34   Nop. DISKCTRL2
0x38   Nop. End disk?
0x3c   Nop. PRINTERCTRL
0x40   Nop. PRINTERSTAT
0x44   Nop. PRINTERREAD
0x48   Nop. PRINTERWRITE
0x4c   Nop. REMOTECTRL
0x50   Nop. REMOTESTAT
0x54   Nop. REMOTEREAD
0x58   Nop. REMOTEWRITE
0x5c   Nop. USERCTRL
0x60   Nop. USERSTAT
0x64   Nop. USERREAD
0x68   Nop. USERWRITE
0x6c   SYSSTAT
0x70   QUIET
0x74   ENABLE
```

Mysteries
--------------

- What is the CUP device? It is mentioned, and is apparently something that should
  (not) be tampered with.
  "Concurrent Use Protection".
  Some p-system leftover that has nothing to do with SUNDOG, I think.

- Why are these (I guess?) ship names in the bootsector?

    VW-69
    Z1 Vector
    Annihilator
    Voton Mk IV
    Voton Mk X
    Phantom UL
    UNKNOWN

These are not part of the boot loader - it seems that that stops at 0x800.
This is simply auxiliary data of the game itself, making use of the fact that sectors
wrap around at 80?

- SunDog: Module names

    WINDOWLI  WINDOWLIB
    XDOUSERM  XDOUSERMENU?
    XPILOTAG  XPILOTAGE?
    XDOREPAI  XDOREPAIR?
    XSHOWMOV  XSHOWMOVE?
    XMOVEONS  XMOVEONSHIP?
    DONESOFA  DONESOFAR?
    XDOINTER  XDOINTERFACE?
    XDOCOMBA  XDOCOMBAT?
    XMOVEONG  XMOVEONGROUND?
    XDOUNITE  XDOUNITELLER?
    XDOTRADI  XDOTRADING?
    XMOVEINB  XMOVEINBUILDING?

- What does XSTARTUP:0x0b do?
  Reads 4 bytes from block 0x45 to MAINLIB+0x43d - used in startup
  Checks status, loops if fail
  0x4800 + 0x45\*0x200 = 0xd200 these bytes are "98 42 18 46"
  No idea what these are used for, but the two words 0x43d 0x43e are used all over the program.

- What does MAINLIB:0x46 do?
  Loads disk block 0x3a and 0x06 (all 512 bytes)

  It looks for 0x30 0x39 0x15 0x38
  This happens to be found here on the disk:

    0000bc20  00 00 00 00 00 00 00 00  30 39 15 38 00 00 00 00  |........09.8....|
  
  However, in my computation 0xbc00 would be block 0x43. A difference of 9 blocks.
  Apparently, p-system's base logical block address is thus 0x4800 - a track after the boot
  track.

    0x4800 + 0x3a*0x200 = 0xbc00  sundog disk identifier
    0x4800 + 0x06*0x200 = 0x5400  library disk identifier

- Block at 0x5600 (track 3+1, sector 7) is accessed in SUNDOG.PRG to check disk for writeability.
  This incidentally is the first block of the 'hidden file' that seems to contain
  SunDog resource data. This would be "block 16" counting 512-byte blocks from the start of the p-code disk area.
  "The system views a disk as a zero-based linear array of 512-byte logical blocks"
  "Unit 4 is disk 0"

- Vertical movement in cities is broken with TOS 2.06. Apparently that breaks the vertical scroll function.
  The game is only compatible with TOS v1.02

- What is the red screen of death? Looks like it happens on failed integrity check,
  or is it a copy protection?

- <https://atariage.com/forums/topic/106987-red-screen-on-sundog-emulation-in-steem/>
- <https://www.reddit.com/r/atari/comments/4xnirp/red_screen_emulating_sundog_frozen_legacy/>

- What sets the array in `MAINLIB:0x4ef`?
  I can only find the loop that changes the values (deobfuscates)? but not where they are originally
  set, or load from disk.

- Where does `03 12 00 03 37 ac` at offset 0x22 in SYSTEM.MISCINFO come from?
  this is written to disk in SUNDOG.PRG (!) every time the program runs.
  These are memory parameters that could be different every execution. What a messy design!

References
-------------

- [1] "p-System Software Reference Library: Internal Architecture"

