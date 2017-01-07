Analysis: Parameters
=======================

How to determine the number of parameters for a
p-system function?

In the usual case this is easy: just look for the highest
local accessed, subtract the number of locals specified
in the function header from that.

This is complicated by the fact that PASCAL has nested
procedures, and child procedures may access parents'
locals through "intermediate" instructions. This analysis
could be helped by constructing a procedure hierarchy
and scanning all the children for intermediate loads.

Another trick is to compute it from the number
of allocated locals versus the number of stack frame
words thrown away by RTU.

How well does this work? Does this also work for
functions, procedures that return a value? My guess is
that the number will be off by the amount of words
returned in that case. So scanning functions instructions
for local accesses is still necessary.

Functions can easily be detected: they always write to the
return argument at the end of the function (not necessarily at the very end,
it may also happen before a jump to the end). The other arguments
are never written to? So the return value is the only local
outside the specified range that is ever written to AND it will
be the TOS after RTU. Can functions return multiple words?
According to p-systems reference 3-48 they can return one or two words,
or four if reals are that size. This might mean that only real
return values are larger than one words which we don't worry about here.
(but what about string/bytearray addresses?)

Locals
----------------

```
0x20-0x2f  SLDL  Short load local word
0x68-0x6f  SSTL  Short store local word
0x84       LLA   Load local address
0x87       LDL   Load local
0xa4       STL   Store local word
```

Globals
----------------

```
0x30-0x3f  SLDO  Short load global word
0x85       LDO   Get global from varlen encoded address
0x86       LAO   Load global address
0xa5       SRO   Store global word
```

Intermediate
------------------

```
0xad       SLOD1  Load memory from next intermediate stack frame
0xae       SLOD2  Load memory from next+1 intermediate stack frame
0xa6       STR    Store intermediate word
0x88       LDA    Load intermediate address
0x89       LOD    Load intermediate word
```

Analysis: Procedure hierarchy
=================================

The PASCAL procedure hierarchy can be reconstructed.

- Start out with no connections
- Look for global calls:

```
0x91       CGP    Call global
0x94       CXG    Intersegment call global
```

Encountering these from procedure A to B places
B at top level.

- Look for intermediate calls:

```
0x92       CIP    Call intermediate
0x95       CXI    Call intermediate intersegment
```

CXI should only happen within one segment group
between subsidiary segments. Inconsistencies should
be detected.

Encountering one from procedure A to procedure B places
B above A in the hierarchy, by the specified number
of lex levels (so it is either a parent or uncle).

- Look for local calls:

```
0x90       CLP    Call local
0x93       CXL    Intersegment call local
```

Encountering these from procedure A to procedure B
places B as a (direct) child of A.

- Inconsistencies, for example loops or conflicts in this hierarchy
  should be detected.

This will produce a "labeled call tree" which is useful for other
purposes as well. The tree could be per segment group or global.

CFP (effectively: function pointers) would complicate this analysis.
Fortunately it is only used in twice place in `SYSTEM.PASCAL`, in
`KERNEL` ExecError function and in `ASSOCIAT`.

Other observations:

- Procedures without any intermediate accesses or calls may as well be
  top-level.




