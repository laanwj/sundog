/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_opcodes.h"

// clang-format off
/* auto-generated by gen_interpreter.py */
struct psys_opcode_desc psys_opcode_descriptions[256] = {
/* 0x00 */ {"sldc0"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x01 */ {"sldc1"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x02 */ {"sldc2"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x03 */ {"sldc3"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x04 */ {"sldc4"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x05 */ {"sldc5"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x06 */ {"sldc6"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x07 */ {"sldc7"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x08 */ {"sldc8"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x09 */ {"sldc9"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x0a */ {"sldc10"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x0b */ {"sldc11"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x0c */ {"sldc12"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x0d */ {"sldc13"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x0e */ {"sldc14"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x0f */ {"sldc15"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x10 */ {"sldc16"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x11 */ {"sldc17"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x12 */ {"sldc18"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x13 */ {"sldc19"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x14 */ {"sldc20"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x15 */ {"sldc21"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x16 */ {"sldc22"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x17 */ {"sldc23"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x18 */ {"sldc24"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x19 */ {"sldc25"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x1a */ {"sldc26"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x1b */ {"sldc27"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x1c */ {"sldc28"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x1d */ {"sldc29"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x1e */ {"sldc30"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x1f */ {"sldc31"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x20 */ {"sldl1"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x21 */ {"sldl2"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x22 */ {"sldl3"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x23 */ {"sldl4"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x24 */ {"sldl5"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x25 */ {"sldl6"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x26 */ {"sldl7"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x27 */ {"sldl8"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x28 */ {"sldl9"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x29 */ {"sldl10"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x2a */ {"sldl11"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x2b */ {"sldl12"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x2c */ {"sldl13"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x2d */ {"sldl14"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x2e */ {"sldl15"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x2f */ {"sldl16"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x30 */ {"sldo1"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x31 */ {"sldo2"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x32 */ {"sldo3"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x33 */ {"sldo4"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x34 */ {"sldo5"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x35 */ {"sldo6"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x36 */ {"sldo7"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x37 */ {"sldo8"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x38 */ {"sldo9"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x39 */ {"sldo10"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x3a */ {"sldo11"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x3b */ {"sldo12"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x3c */ {"sldo13"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x3d */ {"sldo14"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x3e */ {"sldo15"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x3f */ {"sldo16"  ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x40 */ {"und40"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x41 */ {"und41"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x42 */ {"und42"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x43 */ {"und43"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x44 */ {"und44"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x45 */ {"und45"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x46 */ {"und46"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x47 */ {"und47"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x48 */ {"und48"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x49 */ {"und49"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x4a */ {"und4a"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x4b */ {"und4b"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x4c */ {"und4c"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x4d */ {"und4d"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x4e */ {"und4e"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x4f */ {"und4f"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x50 */ {"und50"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x51 */ {"und51"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x52 */ {"und52"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x53 */ {"und53"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x54 */ {"und54"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x55 */ {"und55"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x56 */ {"und56"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x57 */ {"und57"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x58 */ {"und58"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x59 */ {"und59"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x5a */ {"und5a"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x5b */ {"und5b"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x5c */ {"und5c"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x5d */ {"und5d"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x5e */ {"und5e"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x5f */ {"und5f"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x60 */ {"slla1"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x61 */ {"slla2"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x62 */ {"slla3"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x63 */ {"slla4"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x64 */ {"slla5"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x65 */ {"slla6"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x66 */ {"slla7"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x67 */ {"slla8"   ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x68 */ {"sstl1"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x69 */ {"sstl2"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x6a */ {"sstl3"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x6b */ {"sstl4"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x6c */ {"sstl5"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x6d */ {"sstl6"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x6e */ {"sstl7"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x6f */ {"sstl8"   ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0x70 */ {"scxg1"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x71 */ {"scxg2"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x72 */ {"scxg3"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x73 */ {"scxg4"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x74 */ {"scxg5"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x75 */ {"scxg6"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x76 */ {"scxg7"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x77 */ {"scxg8"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x78 */ {"sind0"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x79 */ {"sind1"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x7a */ {"sind2"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x7b */ {"sind3"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x7c */ {"sind4"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x7d */ {"sind5"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x7e */ {"sind6"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x7f */ {"sind7"   ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x80 */ {"ldcb"    ,  1, {0x01, 0x00, 0x00},  0,  1},
/* 0x81 */ {"ldci"    ,  1, {0x02, 0x00, 0x00},  0,  1},
/* 0x82 */ {"lco"     ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0x83 */ {"ldc"     ,  3, {0x01, 0x00, 0x01},  0, -1},
/* 0x84 */ {"lla"     ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0x85 */ {"ldo"     ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0x86 */ {"lao"     ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0x87 */ {"ldl"     ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0x88 */ {"lda"     ,  2, {0x01, 0x00, 0x00},  0,  1},
/* 0x89 */ {"lod"     ,  2, {0x01, 0x00, 0x00},  0,  1},
/* 0x8a */ {"ujp"     ,  1, {0x31, 0x00, 0x00},  0,  0},
/* 0x8b */ {"ujpl"    ,  1, {0x32, 0x00, 0x00},  0,  0},
/* 0x8c */ {"mpi"     ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0x8d */ {"dvi"     ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0x8e */ {"stm"     ,  1, {0x01, 0x00, 0x00}, -1,  0},
/* 0x8f */ {"modi"    ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0x90 */ {"clp"     ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x91 */ {"cgp"     ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0x92 */ {"cip"     ,  2, {0x01, 0x01, 0x00}, -1, -1},
/* 0x93 */ {"cxl"     ,  2, {0x01, 0x01, 0x00}, -1, -1},
/* 0x94 */ {"cxg"     ,  2, {0x01, 0x01, 0x00}, -1, -1},
/* 0x95 */ {"cxi"     ,  3, {0x01, 0x01, 0x01}, -1, -1},
/* 0x96 */ {"rpu"     ,  1, {0x00, 0x00, 0x00}, -1, -1},
/* 0x97 */ {"cfp"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0x98 */ {"ldcn"    ,  0, {0x00, 0x00, 0x00},  0,  1},
/* 0x99 */ {"lsl"     ,  1, {0x01, 0x00, 0x00},  0,  1},
/* 0x9a */ {"lde"     ,  2, {0x01, 0x00, 0x00},  0,  1},
/* 0x9b */ {"lae"     ,  2, {0x01, 0x00, 0x00},  0,  1},
/* 0x9c */ {"nop"     ,  0, {0x00, 0x00, 0x00},  0,  0},
/* 0x9d */ {"lpr"     ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0x9e */ {"bpt"     ,  0, {0x00, 0x00, 0x00},  0,  0},
/* 0x9f */ {"bnot"    ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0xa0 */ {"lor"     ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xa1 */ {"land"    ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xa2 */ {"adi"     ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xa3 */ {"sbi"     ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xa4 */ {"stl"     ,  1, {0x00, 0x00, 0x00},  1,  0},
/* 0xa5 */ {"sro"     ,  1, {0x00, 0x00, 0x00},  1,  0},
/* 0xa6 */ {"str"     ,  2, {0x01, 0x00, 0x00},  1,  0},
/* 0xa7 */ {"ldb"     ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xa8 */ {"native"  ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xa9 */ {"nat-info",  1, {0x30, 0x00, 0x00},  0,  0},
/* 0xaa */ {"undaa"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xab */ {"cap"     ,  1, {0x00, 0x00, 0x00},  2,  0},
/* 0xac */ {"csp"     ,  1, {0x01, 0x00, 0x00},  2,  0},
/* 0xad */ {"slod1"   ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0xae */ {"slod2"   ,  1, {0x00, 0x00, 0x00},  0,  1},
/* 0xaf */ {"undaf"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xb0 */ {"equi"    ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xb1 */ {"neqi"    ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xb2 */ {"leqi"    ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xb3 */ {"geqi"    ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xb4 */ {"leusw"   ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xb5 */ {"geusw"   ,  0, {0x00, 0x00, 0x00},  2,  1},
/* 0xb6 */ {"eqpwr"   ,  0, {0x00, 0x00, 0x00}, -1,  1},
/* 0xb7 */ {"lepwr"   ,  0, {0x00, 0x00, 0x00}, -1,  1},
/* 0xb8 */ {"gepwr"   ,  0, {0x00, 0x00, 0x00}, -1,  1},
/* 0xb9 */ {"eqbyte"  ,  3, {0x01, 0x01, 0x00},  2,  1},
/* 0xba */ {"lebyte"  ,  3, {0x01, 0x01, 0x00},  2,  1},
/* 0xbb */ {"gebyte"  ,  3, {0x01, 0x01, 0x00},  2,  1},
/* 0xbc */ {"srs"     ,  0, {0x00, 0x00, 0x00},  2, -1},
/* 0xbd */ {"swap"    ,  0, {0x00, 0x00, 0x00},  2,  2},
/* 0xbe */ {"undbe"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xbf */ {"undbf"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xc0 */ {"undc0"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xc1 */ {"undc1"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xc2 */ {"undc2"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xc3 */ {"undc3"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xc4 */ {"sto"     ,  0, {0x00, 0x00, 0x00},  2,  0},
/* 0xc5 */ {"mov"     ,  2, {0x01, 0x00, 0x00},  2,  0},
/* 0xc6 */ {"dup2"    ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xc7 */ {"adj"     ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0xc8 */ {"stb"     ,  0, {0x00, 0x00, 0x00},  3,  0},
/* 0xc9 */ {"ldp"     ,  0, {0x00, 0x00, 0x00},  3,  1},
/* 0xca */ {"stp"     ,  0, {0x00, 0x00, 0x00},  4,  0},
/* 0xcb */ {"chk"     ,  0, {0x00, 0x00, 0x00},  3,  1},
/* 0xcc */ {"flt"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xcd */ {"eqreal"  ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xce */ {"lereal"  ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xcf */ {"gereal"  ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xd0 */ {"ldm"     ,  1, {0x01, 0x00, 0x00},  1, -1},
/* 0xd1 */ {"spr"     ,  0, {0x00, 0x00, 0x00},  2,  0},
/* 0xd2 */ {"efj"     ,  1, {0x31, 0x00, 0x00},  2,  0},
/* 0xd3 */ {"nfj"     ,  1, {0x31, 0x00, 0x00},  2,  0},
/* 0xd4 */ {"fjp"     ,  1, {0x31, 0x00, 0x00},  1,  0},
/* 0xd5 */ {"fjpl"    ,  1, {0x32, 0x00, 0x00},  1,  0},
/* 0xd6 */ {"xjp"     ,  1, {0x00, 0x00, 0x00},  1,  0},
/* 0xd7 */ {"ixa"     ,  1, {0x00, 0x00, 0x00},  2,  1},
/* 0xd8 */ {"ixp"     ,  2, {0x01, 0x01, 0x00},  2,  3},
/* 0xd9 */ {"ste"     ,  2, {0x01, 0x00, 0x00},  1,  0},
/* 0xda */ {"inn"     ,  0, {0x00, 0x00, 0x00}, -1,  1},
/* 0xdb */ {"uni"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xdc */ {"int"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xdd */ {"dif"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xde */ {"signal"  ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0xdf */ {"wait"    ,  0, {0x00, 0x00, 0x00},  1,  0},
/* 0xe0 */ {"abi"     ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0xe1 */ {"ngi"     ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0xe2 */ {"dup1"    ,  0, {0x00, 0x00, 0x00},  1,  2},
/* 0xe3 */ {"abr"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xe4 */ {"ngr"     ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xe5 */ {"lnot"    ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0xe6 */ {"ind"     ,  1, {0x00, 0x00, 0x00},  1,  1},
/* 0xe7 */ {"inc"     ,  1, {0x00, 0x00, 0x00},  1,  1},
/* 0xe8 */ {"eqstr"   ,  2, {0x01, 0x01, 0x00},  2,  1},
/* 0xe9 */ {"lestr"   ,  2, {0x01, 0x01, 0x00},  2,  1},
/* 0xea */ {"gestr"   ,  2, {0x01, 0x01, 0x00},  2,  1},
/* 0xeb */ {"astr"    ,  2, {0x01, 0x01, 0x00},  2,  0},
/* 0xec */ {"cstr"    ,  0, {0x00, 0x00, 0x00},  2,  2},
/* 0xed */ {"inci"    ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0xee */ {"deci"    ,  0, {0x00, 0x00, 0x00},  1,  1},
/* 0xef */ {"scip1"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0xf0 */ {"scip2"   ,  1, {0x01, 0x00, 0x00}, -1, -1},
/* 0xf1 */ {"tjp"     ,  1, {0x31, 0x00, 0x00},  1,  0},
/* 0xf2 */ {"ldcrl"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf3 */ {"ldrl"    ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf4 */ {"strl"    ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf5 */ {"undf5"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf6 */ {"undf6"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf7 */ {"undf7"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf8 */ {"undf8"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xf9 */ {"undf9"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xfa */ {"undfa"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xfb */ {"undfb"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xfc */ {"undfc"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xfd */ {"undfd"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xfe */ {"undfe"   ,  0, {0x00, 0x00, 0x00}, -1, -1},
/* 0xff */ {"undff"   ,  0, {0x00, 0x00, 0x00}, -1, -1}
};
// clang-format on
