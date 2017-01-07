# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

##### Instruction information

# inst flags
LOAD   = 1     # Load
STORE  = 2     # Store
ADDR   = 4     # Get address
LOCAL  = 8     # Local
GLOBAL = 16    # Global
INTRMD = 32    # Intermediate
SEGM   = 64    # Segment/constant pool
MEM    = 128   # Memory
CFLOW  = 256   # Intraprocedure
CALL   = 512   # Interprocedure
ISEG   = 1024  # Intersegment
ALU    = 2048  # Arithmetic
UNDEF  = 4096  # Undefined instruction
FLOAT  = 8192  # Floating point
SETOP  = 16384 # Sets

# arg flags
VAR=0  # "big" encoding
BYTE=1
WORD=2
ARGTMASK=15  # argument type mask
SIGNED=16    # argument is signed (only makes sense for 1/2, not VAR)
RELFLAG=32   # argument is relative to end of instruction
REL=RELFLAG | SIGNED # combi for jumps

#     opname   args       flags          in    out
OPCODES = [
    # 0x00
    ('SLDC0',  (),        LOAD,          0,    1),
    ('SLDC1',  (),        LOAD,          0,    1),
    ('SLDC2',  (),        LOAD,          0,    1),
    ('SLDC3',  (),        LOAD,          0,    1),
    ('SLDC4',  (),        LOAD,          0,    1),
    ('SLDC5',  (),        LOAD,          0,    1),
    ('SLDC6',  (),        LOAD,          0,    1),
    ('SLDC7',  (),        LOAD,          0,    1),
    ('SLDC8',  (),        LOAD,          0,    1),
    ('SLDC9',  (),        LOAD,          0,    1),
    ('SLDC10', (),        LOAD,          0,    1),
    ('SLDC11', (),        LOAD,          0,    1),
    ('SLDC12', (),        LOAD,          0,    1),
    ('SLDC13', (),        LOAD,          0,    1),
    ('SLDC14', (),        LOAD,          0,    1),
    ('SLDC15', (),        LOAD,          0,    1),
    # 0x10
    ('SLDC16', (),        LOAD,          0,    1),
    ('SLDC17', (),        LOAD,          0,    1),
    ('SLDC18', (),        LOAD,          0,    1),
    ('SLDC19', (),        LOAD,          0,    1),
    ('SLDC20', (),        LOAD,          0,    1),
    ('SLDC21', (),        LOAD,          0,    1),
    ('SLDC22', (),        LOAD,          0,    1),
    ('SLDC23', (),        LOAD,          0,    1),
    ('SLDC24', (),        LOAD,          0,    1),
    ('SLDC25', (),        LOAD,          0,    1),
    ('SLDC26', (),        LOAD,          0,    1),
    ('SLDC27', (),        LOAD,          0,    1),
    ('SLDC28', (),        LOAD,          0,    1),
    ('SLDC29', (),        LOAD,          0,    1),
    ('SLDC30', (),        LOAD,          0,    1),
    ('SLDC31', (),        LOAD,          0,    1),
    # 0x20
    ('SLDL1',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL2',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL3',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL4',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL5',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL6',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL7',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL8',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL9',  (),        LOAD|LOCAL,    0,    1),
    ('SLDL10', (),        LOAD|LOCAL,    0,    1),
    ('SLDL11', (),        LOAD|LOCAL,    0,    1),
    ('SLDL12', (),        LOAD|LOCAL,    0,    1),
    ('SLDL13', (),        LOAD|LOCAL,    0,    1),
    ('SLDL14', (),        LOAD|LOCAL,    0,    1),
    ('SLDL15', (),        LOAD|LOCAL,    0,    1),
    ('SLDL16', (),        LOAD|LOCAL,    0,    1),
    # 0x30
    ('SLDO1',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO2',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO3',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO4',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO5',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO6',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO7',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO8',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO9',  (),        LOAD|GLOBAL,   0,    1),
    ('SLDO10', (),        LOAD|GLOBAL,   0,    1),
    ('SLDO11', (),        LOAD|GLOBAL,   0,    1),
    ('SLDO12', (),        LOAD|GLOBAL,   0,    1),
    ('SLDO13', (),        LOAD|GLOBAL,   0,    1),
    ('SLDO14', (),        LOAD|GLOBAL,   0,    1),
    ('SLDO15', (),        LOAD|GLOBAL,   0,    1),
    ('SLDO16', (),        LOAD|GLOBAL,   0,    1),
    # 0x40 (undefined)
    ('UND40',  (),        UNDEF,         None, None),
    ('UND41',  (),        UNDEF,         None, None),
    ('UND42',  (),        UNDEF,         None, None),
    ('UND43',  (),        UNDEF,         None, None),
    ('UND44',  (),        UNDEF,         None, None),
    ('UND45',  (),        UNDEF,         None, None),
    ('UND46',  (),        UNDEF,         None, None),
    ('UND47',  (),        UNDEF,         None, None),
    ('UND48',  (),        UNDEF,         None, None),
    ('UND49',  (),        UNDEF,         None, None),
    ('UND4A',  (),        UNDEF,         None, None),
    ('UND4B',  (),        UNDEF,         None, None),
    ('UND4C',  (),        UNDEF,         None, None),
    ('UND4D',  (),        UNDEF,         None, None),
    ('UND4E',  (),        UNDEF,         None, None),
    ('UND4F',  (),        UNDEF,         None, None),
    # Nonex5None (undefined)
    ('UND50',  (),        UNDEF,         None, None),
    ('UND51',  (),        UNDEF,         None, None),
    ('UND52',  (),        UNDEF,         None, None),
    ('UND53',  (),        UNDEF,         None, None),
    ('UND54',  (),        UNDEF,         None, None),
    ('UND55',  (),        UNDEF,         None, None),
    ('UND56',  (),        UNDEF,         None, None),
    ('UND57',  (),        UNDEF,         None, None),
    ('UND58',  (),        UNDEF,         None, None),
    ('UND59',  (),        UNDEF,         None, None),
    ('UND5A',  (),        UNDEF,         None, None),
    ('UND5B',  (),        UNDEF,         None, None),
    ('UND5C',  (),        UNDEF,         None, None),
    ('UND5D',  (),        UNDEF,         None, None),
    ('UND5E',  (),        UNDEF,         None, None),
    ('UND5F',  (),        UNDEF,         None, None),
    # 0x60    
    ('SLLA1',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA2',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA3',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA4',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA5',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA6',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA7',  (),        ADDR|LOCAL,    0,    1),
    ('SLLA8',  (),        ADDR|LOCAL,    0,    1),
    ('SSTL1',  (),        STORE|LOCAL,   1,    0),
    ('SSTL2',  (),        STORE|LOCAL,   1,    0),
    ('SSTL3',  (),        STORE|LOCAL,   1,    0),
    ('SSTL4',  (),        STORE|LOCAL,   1,    0),
    ('SSTL5',  (),        STORE|LOCAL,   1,    0),
    ('SSTL6',  (),        STORE|LOCAL,   1,    0),
    ('SSTL7',  (),        STORE|LOCAL,   1,    0),
    ('SSTL8',  (),        STORE|LOCAL,   1,    0),
    # 0x70
    ('SCXG1',  (1,),      CALL|ISEG,     None, None),
    ('SCXG2',  (1,),      CALL|ISEG,     None, None),
    ('SCXG3',  (1,),      CALL|ISEG,     None, None),
    ('SCXG4',  (1,),      CALL|ISEG,     None, None),
    ('SCXG5',  (1,),      CALL|ISEG,     None, None),
    ('SCXG6',  (1,),      CALL|ISEG,     None, None),
    ('SCXG7',  (1,),      CALL|ISEG,     None, None),
    ('SCXG8',  (1,),      CALL|ISEG,     None, None),
    ('SIND0',  (),        LOAD|MEM,      1,    1),
    ('SIND1',  (),        LOAD|MEM,      1,    1),
    ('SIND2',  (),        LOAD|MEM,      1,    1),
    ('SIND3',  (),        LOAD|MEM,      1,    1),
    ('SIND4',  (),        LOAD|MEM,      1,    1),
    ('SIND5',  (),        LOAD|MEM,      1,    1),
    ('SIND6',  (),        LOAD|MEM,      1,    1),
    ('SIND7',  (),        LOAD|MEM,      1,    1),
    # 0x80
    ('LDCB',   (1,),      LOAD,          0,    1),
    ('LDCI',   (2,),      LOAD,          0,    1),
    ('LCO',    (VAR,),    ADDR|SEGM,     0,    1),
    ('LDC',    (1,VAR,1), LOAD|SEGM,     0,    None),
    ('LLA',    (VAR,),    ADDR|LOCAL,    0,    1),
    ('LDO',    (VAR,),    LOAD|GLOBAL,   0,    1),
    ('LAO',    (VAR,),    ADDR|GLOBAL,   0,    1),
    ('LDL',    (VAR,),    LOAD|LOCAL,    0,    1),
    ('LDA',    (1,VAR),   ADDR|INTRMD,   0,    1),
    ('LOD',    (1,VAR),   LOAD|INTRMD,   0,    1),
    ('UJP',    (1|REL,),  CFLOW,         0,    0),
    ('UJPL',   (2|REL,),  CFLOW,         0,    0),
    ('MPI',    (),        ALU,           2,    1),
    ('DVI',    (),        ALU,           2,    1),
    ('STM',    (1,),      STORE|MEM,     None, 0),
    ('MODI',   (),        ALU,           2,    1),
    # 0x90
    ('CLP',    (1,),      CALL|LOCAL,    None, None),
    ('CGP',    (1,),      CALL|GLOBAL,   None, None),
    ('CIP',    (1,1),     CALL|INTRMD,   None, None),
    ('CXL',    (1,1),     CALL|LOCAL|ISEG,  None, None),
    ('CXG',    (1,1),     CALL|GLOBAL|ISEG, None, None),
    ('CXI',    (1,1,1),   CALL|INTRMD|ISEG, None, None),
    ('RPU',    (VAR,),    CALL|ISEG,     None, None), # not really call but ok
    # In contrast to what the p-systems internal reference manual says, cfp has no argument
    ('CFP',    (),        CALL|GLOBAL|ISEG, None, None),
    ('LDCN',   (),        LOAD,          0,    1),
    ('LSL',    (1,),      ADDR|INTRMD,   0,    1),
    ('LDE',    (1,VAR),   LOAD|GLOBAL|ISEG, 0, 1),
    ('LAE',    (1,VAR),   ADDR|GLOBAL|ISEG, 0, 1),
    ('NOP',    (),        0,             0,    0),
    ('LPR',    (),        LOAD,          1,    1),
    ('BPT',    (),        CFLOW,         0,    0),
    ('BNOT',   (),        ALU,           1,    1),
    # 0xA0
    ('LOR',    (),        ALU,           2,    1),
    ('LAND',   (),        ALU,           2,    1),
    ('ADI',    (),        ALU,           2,    1),
    ('SBI',    (),        ALU,           2,    1),
    ('STL',    (VAR,),    STORE|LOCAL,   1,    0),
    ('SRO',    (VAR,),    STORE|GLOBAL,  1,    0),
    ('STR',    (1,VAR),   STORE|INTRMD,  1,    0),
    ('LDB',    (),        LOAD|MEM,      2,    1),
    ('NATIVE', (),        CFLOW,         None, None), # Jump to native
    ('NAT-INFO',(VAR|REL,), CFLOW,         0,    0),
    ('UNDAA',  (),        UNDEF,         None, None),
    ('CAP',    (VAR,),    MEM,           2,    0),
    ('CSP',    (1,),      MEM,           2,    0),
    ('SLOD1',  (VAR,),    LOAD|INTRMD,   0,    1),
    ('SLOD2',  (VAR,),    LOAD|INTRMD,   0,    1),
    ('UNDAF',  (),        UNDEF,         None, None),
    # 0xB0
    ('EQUI',   (),        ALU,           2,    1),
    ('NEQI',   (),        ALU,           2,    1),
    ('LEQI',   (),        ALU,           2,    1),
    ('GEQI',   (),        ALU,           2,    1),
    ('LEUSW',  (),        ALU,           2,    1),
    ('GEUSW',  (),        ALU,           2,    1),
    ('EQPWR',  (),        ALU,           None, 1),
    ('LEPWR',  (),        ALU,           None, 1),
    ('GEPWR',  (),        ALU,           None, 1),
    ('EQBYTE', (1,1,VAR), MEM,           2,    1),
    ('LEBYTE', (1,1,VAR), MEM,           2,    1),
    ('GEBYTE', (1,1,VAR), MEM,           2,    1),
    ('SRS',    (),        SETOP,         2,    None),
    ('SWAP',   (),        0,             2,    2),
    ('UNDBE',  (),        UNDEF,         None, None),
    ('UNDBF',  (),        UNDEF,         None, None),
    # 0xC0
    ('UNDC0',  (),        UNDEF,         None, None),
    ('UNDC1',  (),        UNDEF,         None, None),
    ('UNDC2',  (),        UNDEF,         None, None),
    ('UNDC3',  (),        UNDEF,         None, None),
    ('STO',    (),        STORE|MEM,     2,    0),
    ('MOV',    (1,VAR),   LOAD|STORE|MEM|SEGM, 2, 0),
    ('DUP2',   (),        FLOAT,         None, None),
    ('ADJ',    (1,),      SETOP,         None, None),
    ('STB',    (),        STORE|MEM,     3,    0),
    ('LDP',    (),        LOAD|MEM,      3,    1),
    ('STP',    (),        STORE|MEM,     4,    0),
    ('CHK',    (),        0,             3,    1),
    ('FLT',    (),        FLOAT,         None, None),
    ('EQREAL', (),        FLOAT,         None, None),
    ('LEREAL', (),        FLOAT,         None, None),
    ('GEREAL', (),        FLOAT,         None, None),
    # 0xD0
    ('LDM',    (1,),      LOAD|MEM,      1,    None),
    ('SPR',    (),        STORE,         2,    0),
    ('EFJ',    (1|REL,),  CFLOW,         2,    0),
    ('NFJ',    (1|REL,),  CFLOW,         2,    0),
    ('FJP',    (1|REL,),  CFLOW,         1,    0),
    ('FJPL',   (2|REL,),  CFLOW,         1,    0),
    ('XJP',    (VAR,),    CFLOW,         1,    0),
    ('IXA',    (VAR,),    0,             2,    1),
    ('IXP',    (1,1),     0,             2,    3),
    ('STE',    (1,VAR),   STORE|GLOBAL|ISEG, 1, 0 ),
    ('INN',    (),        SETOP,         None, 1),
    ('UNI',    (),        SETOP,         None, None),
    ('INT',    (),        SETOP,         None, None),
    ('DIF',    (),        SETOP,         None, None),
    ('SIGNAL', (),        0,             1,    0),
    ('WAIT',   (),        0,             1,    0),
    # 0xE0
    ('ABI',    (),        ALU,           1,    1),
    ('NGI',    (),        ALU,           1,    1),
    ('DUP1',   (),        0,             1,    2),
    ('ABR',    (),        FLOAT,         None, None),
    ('NGR',    (),        FLOAT,         None, None),
    ('LNOT',   (),        ALU,           1,    1),
    ('IND',    (VAR,),    LOAD|MEM,      1,    1),
    ('INC',    (VAR,),    ALU,           1,    1),
    ('EQSTR',  (1,1),     MEM,           2,    1),
    ('LESTR',  (1,1),     MEM,           2,    1),
    ('GESTR',  (1,1),     MEM,           2,    1),
    ('ASTR',   (1,1),     MEM,           2,    0),
    ('CSTR',   (),        MEM,           2,    2),
    ('INCI',   (),        ALU,           1,    1),
    ('DECI',   (),        ALU,           1,    1),
    ('SCIP1',  (1,),      CALL|INTRMD,   None, None),
    # 0xF0
    ('SCIP2',  (1,),      CALL|INTRMD,   None, None),
    ('TJP',    (1|REL,),  CFLOW,         1,    0),
    ('LDCRL',  (),        FLOAT,         None, None),
    ('LDRL',   (),        FLOAT,         None, None),
    ('STRL',   (),        FLOAT,         None, None),
    ('UNDF5',  (),        UNDEF,         None, None), # (undefined)
    ('UNDF6',  (),        UNDEF,         None, None),
    ('UNDF7',  (),        UNDEF,         None, None),
    ('UNDF8',  (),        UNDEF,         None, None),
    ('UNDF9',  (),        UNDEF,         None, None),
    ('UNDFA',  (),        UNDEF,         None, None),
    ('UNDFB',  (),        UNDEF,         None, None),
    ('UNDFC',  (),        UNDEF,         None, None),
    ('UNDFD',  (),        UNDEF,         None, None),
    ('UNDFE',  (),        UNDEF,         None, None),
    ('UNDFF',  (),        UNDEF,         None, None),
]

# opcode constants
SLDC0    = 0x00
SLDC1    = 0x01
SLDC2    = 0x02
SLDC3    = 0x03
SLDC4    = 0x04
SLDC5    = 0x05
SLDC6    = 0x06
SLDC7    = 0x07
SLDC8    = 0x08
SLDC9    = 0x09
SLDC10   = 0x0a
SLDC11   = 0x0b
SLDC12   = 0x0c
SLDC13   = 0x0d
SLDC14   = 0x0e
SLDC15   = 0x0f
SLDC16   = 0x10
SLDC17   = 0x11
SLDC18   = 0x12
SLDC19   = 0x13
SLDC20   = 0x14
SLDC21   = 0x15
SLDC22   = 0x16
SLDC23   = 0x17
SLDC24   = 0x18
SLDC25   = 0x19
SLDC26   = 0x1a
SLDC27   = 0x1b
SLDC28   = 0x1c
SLDC29   = 0x1d
SLDC30   = 0x1e
SLDC31   = 0x1f
SLDL1    = 0x20
SLDL2    = 0x21
SLDL3    = 0x22
SLDL4    = 0x23
SLDL5    = 0x24
SLDL6    = 0x25
SLDL7    = 0x26
SLDL8    = 0x27
SLDL9    = 0x28
SLDL10   = 0x29
SLDL11   = 0x2a
SLDL12   = 0x2b
SLDL13   = 0x2c
SLDL14   = 0x2d
SLDL15   = 0x2e
SLDL16   = 0x2f
SLDO1    = 0x30
SLDO2    = 0x31
SLDO3    = 0x32
SLDO4    = 0x33
SLDO5    = 0x34
SLDO6    = 0x35
SLDO7    = 0x36
SLDO8    = 0x37
SLDO9    = 0x38
SLDO10   = 0x39
SLDO11   = 0x3a
SLDO12   = 0x3b
SLDO13   = 0x3c
SLDO14   = 0x3d
SLDO15   = 0x3e
SLDO16   = 0x3f
UND40    = 0x40
UND41    = 0x41
UND42    = 0x42
UND43    = 0x43
UND44    = 0x44
UND45    = 0x45
UND46    = 0x46
UND47    = 0x47
UND48    = 0x48
UND49    = 0x49
UND4A    = 0x4a
UND4B    = 0x4b
UND4C    = 0x4c
UND4D    = 0x4d
UND4E    = 0x4e
UND4F    = 0x4f
UND50    = 0x50
UND51    = 0x51
UND52    = 0x52
UND53    = 0x53
UND54    = 0x54
UND55    = 0x55
UND56    = 0x56
UND57    = 0x57
UND58    = 0x58
UND59    = 0x59
UND5A    = 0x5a
UND5B    = 0x5b
UND5C    = 0x5c
UND5D    = 0x5d
UND5E    = 0x5e
UND5F    = 0x5f
SLLA1    = 0x60
SLLA2    = 0x61
SLLA3    = 0x62
SLLA4    = 0x63
SLLA5    = 0x64
SLLA6    = 0x65
SLLA7    = 0x66
SLLA8    = 0x67
SSTL1    = 0x68
SSTL2    = 0x69
SSTL3    = 0x6a
SSTL4    = 0x6b
SSTL5    = 0x6c
SSTL6    = 0x6d
SSTL7    = 0x6e
SSTL8    = 0x6f
SCXG1    = 0x70
SCXG2    = 0x71
SCXG3    = 0x72
SCXG4    = 0x73
SCXG5    = 0x74
SCXG6    = 0x75
SCXG7    = 0x76
SCXG8    = 0x77
SIND0    = 0x78
SIND1    = 0x79
SIND2    = 0x7a
SIND3    = 0x7b
SIND4    = 0x7c
SIND5    = 0x7d
SIND6    = 0x7e
SIND7    = 0x7f
LDCB     = 0x80
LDCI     = 0x81
LCO      = 0x82
LDC      = 0x83
LLA      = 0x84
LDO      = 0x85
LAO      = 0x86
LDL      = 0x87
LDA      = 0x88
LOD      = 0x89
UJP      = 0x8a
UJPL     = 0x8b
MPI      = 0x8c
DVI      = 0x8d
STM      = 0x8e
MODI     = 0x8f
CLP      = 0x90
CGP      = 0x91
CIP      = 0x92
CXL      = 0x93
CXG      = 0x94
CXI      = 0x95
RPU      = 0x96
CFP      = 0x97
LDCN     = 0x98
LSL      = 0x99
LDE      = 0x9a
LAE      = 0x9b
NOP      = 0x9c
LPR      = 0x9d
BPT      = 0x9e
BNOT     = 0x9f
LOR      = 0xa0
LAND     = 0xa1
ADI      = 0xa2
SBI      = 0xa3
STL      = 0xa4
SRO      = 0xa5
STR      = 0xa6
LDB      = 0xa7
NATIVE   = 0xa8
NAT_INFO = 0xa9
UNDAA    = 0xaa
CAP      = 0xab
CSP      = 0xac
SLOD1    = 0xad
SLOD2    = 0xae
UNDAF    = 0xaf
EQUI     = 0xb0
NEQI     = 0xb1
LEQI     = 0xb2
GEQI     = 0xb3
LEUSW    = 0xb4
GEUSW    = 0xb5
EQPWR    = 0xb6
LEPWR    = 0xb7
GEPWR    = 0xb8
EQBYTE   = 0xb9
LEBYTE   = 0xba
GEBYTE   = 0xbb
SRS      = 0xbc
SWAP     = 0xbd
UNDBE    = 0xbe
UNDBF    = 0xbf
UNDC0    = 0xc0
UNDC1    = 0xc1
UNDC2    = 0xc2
UNDC3    = 0xc3
STO      = 0xc4
MOV      = 0xc5
DUP2     = 0xc6
ADJ      = 0xc7
STB      = 0xc8
LDP      = 0xc9
STP      = 0xca
CHK      = 0xcb
FLT      = 0xcc
EQREAL   = 0xcd
LEREAL   = 0xce
GEREAL   = 0xcf
LDM      = 0xd0
SPR      = 0xd1
EFJ      = 0xd2
NFJ      = 0xd3
FJP      = 0xd4
FJPL     = 0xd5
XJP      = 0xd6
IXA      = 0xd7
IXP      = 0xd8
STE      = 0xd9
INN      = 0xda
UNI      = 0xdb
INT      = 0xdc
DIF      = 0xdd
SIGNAL   = 0xde
WAIT     = 0xdf
ABI      = 0xe0
NGI      = 0xe1
DUP1     = 0xe2
ABR      = 0xe3
NGR      = 0xe4
LNOT     = 0xe5
IND      = 0xe6
INC      = 0xe7
EQSTR    = 0xe8
LESTR    = 0xe9
GESTR    = 0xea
ASTR     = 0xeb
CSTR     = 0xec
INCI     = 0xed
DECI     = 0xee
SCIP1    = 0xef
SCIP2    = 0xf0
TJP      = 0xf1
LDCRL    = 0xf2
LDRL     = 0xf3
STRL     = 0xf4
UNDF5    = 0xf5
UNDF6    = 0xf6
UNDF7    = 0xf7
UNDF8    = 0xf8
UNDF9    = 0xf9
UNDFA    = 0xfa
UNDFB    = 0xfb
UNDFC    = 0xfc
UNDFD    = 0xfd
UNDFE    = 0xfe
UNDFF    = 0xff
