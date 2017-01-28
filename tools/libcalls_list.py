##### Known system library procedure numbers
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# http://www.unige.ch/medecine/nouspikel/ti99/psystem.htm#code%20pool-related

# if tuple: name, ins, outs
LIBCALLS = {
    b'KERNEL  ': {
        0x02: 'Exec_Error', # Called by interpreter on error
        0x04: ('relocseg(erec)', 1, 0),
        0x05: 'ptr_idx(p_0:mem_ptr;idx:integer):integer', # return &p_0[idx]
        0x06: 'ptr_sub(p_0, p_1:mem_ptr):integer', # subtracts pointers, divides by arch byte/word size
        0x07: 'UL(a,b)', # unsigned less than
        0x08: 'UG(a,b)', # unsigned greater than
        0x09: 'UGE(a,b)', # unsigned greater or equal
        0x0a: 'PrintLnConsole(msg:string)',
        0x0b: 'PrintIntConsole(val:integer)',
        0x0c: 'PrintConsole(msg:string)',
        0x0e: ('moveseg(sib,srcpool,srcoffset)', 3, 0),
        0x0f: ('moveleft(source,dest:array; length:integer)', 5, 0),
        0x10: ('moveright(source,dest:array; length:integer)', 5, 0),
        0x11: '_exitproc(a,b,c)', # Called at exit of proc/func/program (Chapter 9, MACAdvantage)
        0x12: ('unitread(unit:integer; buf:array; len,block,ctrl:integer)', 6, 0),
        0x13: ('unitwrite(unit:integer; buf:array; len,block,ctrl:integer)', 6, 0),
        0x14: ('time(hiword,loword)', 2, 0),
        0x15: ('fillchar(dest:bytearray; n_bytes,value:integer)', 4, 0),
        0x16: ('scan(len,exp,source):int', 7, 1),
        0x17: ('iocheck()', 0, 0),
        0x18: ('getpoolbytes(dest,pooldesc,offset,nbytes)', 4, 0),
        0x19: ('putpoolbytes(source,pooldesc,offset,nbytes)', 4, 0),
        0x1a: ('flipsegbytes(erec,offset,nwords)', 3, 0),
        0x1b: ('quiet()', 0, 0),
        0x1c: ('enable()', 0, 0),
        0x1d: ('attach(semaphore,vector)', 2, 0),
        0x1e: ('ioresult()', 1, 1),
        0x1f: ('unitbusy(unit):boolean', 2, 1),
        0x20: ('poweroften', None, None),  # Depends on real size
        0x21: ('unitwait(unit)', 1, 0),
        0x22: ('unitclear(unit)', 1, 0),
        0x23: '_halt',     # Called at "halt" (Chapter 9, MACAdvantage)
        0x24: ('unitstatus(unit,stat_rec,control)', 3, 0),
        0x25: ('idsearch(symrec,buffer)', 2, 0),
        0x26: ('treesearch(root,foundp,target):int', 4, 1),
        0x27: ('readseg(erec)', 2, 1),
        # New/specific
        # Looks like alt calls that allow unrestricted use of the UNIT* calls
        0x28: ('_unitread', 6, 0),   # Alias 0x12
        0x29: ('_unitwrite', 6, 0),  # Alias 0x13
        0x2a: ('_unitbusy', 2, 1),   # Alias 0x1f
        0x2b: ('_unitwait', 1, 0),   # Alias 0x21
        0x2c: ('_unitclear', 1, 0),  # Alias 0x22
        0x2d: ('_unitstatus', 3, 0), # Alias 0x24
        0x2e: ('_readseg', 2, 1),    # Alias 0x27
        0x2f: '_setrestricted',      # overkill?
        0x30: 'Fault_Handler', # Task waits for segmentation and stack faults
        0x36: 'Swap_Endian(addr)',   # Swap word endian at addr
        0x37: 'Halt()',              # Hang infinite loop
        0x38: 'Print?Console1()',    # Prints a character from ? to console
        0x39: 'Print?Console2()',    # Prints a character from ? to console
    },
    b'PASCALIO': {
        2: 'j_get(var f:fib)',
        3: 'f_get(var f:fib)',
        4: 'f_put(var f:fib)',
        5: 'f_eof(var f:fib):boolean',
        6: 'f_eoln(var f:fib):boolean',
        7: 'f_readint(var f:fib; var i:integer)',
        8: 'f_writeint(var f:fib; i,rleng:integer)',
        9: 'f_readchar(var f:fib; var ch:char)',
        10: 'f_readstring(var f:fib; var a:window; aleng:integer)',
        11: 'f_writestring(var f:fib; a:maxstring; rleng:integer)',
        12: 'f_writebytes(var f:fib; var a:window; rleng,aleng:integer)',
        13: 'f_readln(var f:fib)',
        14: 'f_writeln(var f:fib)',
        15: 'FWriteChar', # this conflicts between the NEC APC version and MacAdvantage
        16: 'FPage',      # called at "page" (Chapter 9, MACAdvantage)
        17: 'RdInt2',
        18: 'WrInt2',
        19: 'ReadBytes',
        20: 'WriteBytes',
        21: 'ReadTextChar',
        #15: 'cant_stretch(var f:fib):boolean',
        #16: 'do_blanks(var f:fib; blank_num : integer)',
    },
    b'EXTRAIO ': {
        2: 'fblockio(var f:fib; var a:window; i:integer; nblocks,rblock:integer; doread:boolean):integer',
        3: 'f_writechar(var f:fib; ch:char; rleng:integer)',
    },
    b'EXTRAHEA': {
        2: 's_dispose(var block_p:mem_ptr; n_words:integer)', # called at "dispose" (Chapter 9, MACAdvantage)
        3: 's_var_new(var pointer:mem_ptr; words:integer):integer',
        4: 's_mem_lock(seglist:string)',
        5: 's_mem_avail:integer',
        6: 's_var_avail(seglist:string):integer',
        7: 's_mem_swap(seglist:string)',
        8: 'j_dispose(var block_p:memptr; n_words:integer)',
        9: 'getname(var tseg: alpha; seglist: string; var index: integer; var curseg: sib_p):boolean',
    },
    b'HEAPOPS ': {
        2: 'H_Mark(a)',    # called at "mark" (Chapter 9, MACAdvantage)
        3: 'H_Release(a)', # called at "release" (Chapter 9, MACAdvantage)
        4: 'H_New(dest,words)',
        5: 'H_Dispose(a,b)',
    },
    b'STRINGOP': {
        2: 'concat(a,b,c,d)',
        3: 'insert(a,b,c,d,e)',
        4: 'copy(a,b,c,d,e)',
        5: 'delete(a,b,c)', # called at "delete" (Chapter 9, MACAdvantage)
        6: 'pos(a,b,c,d):integer',
    },
    b'GOTOXY  ': {
        2: 'gotoxy',
    },
    b'FILEOPS ': {
        2: 'open',
        3: 'close',
        4: 'finit',
        5: 'seek',
        6: 'reset',
    },
    b'CONCURRE': {
        2: 'cbexit',
        3: 'SstartP',
        4: 'SstopP',
        6: 'SExitProcess', # called at end of process (Chapter 9, MACAdvantage)
    },
    b'PERMHEAP': {
        3: 'permdispose',
        4: 'permrelease',
        5: 'permnew',
    },
    b'OSUTIL  ': {
        3: 'IntToStr',
        4: 'Int2ToStr',
        5: 'GotIntStr',
        6: 'Upcase',
    },
    b'CUPOPS  ': {
        0x12: 'TamperingDetected',
    },
    b'USERPROG': {
        0x01: 'SystemStart',
        0x04: 'FatalError',
        0x06: 'VolSearch(a,b,c,d): bool', # look up entry on boot volume
        0x0a: 'LoadSystemPascal',
        0x1b: 'ReadSystemMiscInfo',
        0x1c: 'DetermineSystemParameters', #endianness, word-addressing, etc
        0x1f: 'InitIO',
        0x21: 'StartFaultHandler',
        0x28: 'CupsCheck',
    },
    b'GETCMD  ': {
        0x12: 'PrintPadded(a,b)',
        0x14: 'PrintSystemInfo()',
    },
    b'***     ': { # pseudo-segment for initialization/deinitialization
        0x01: ('', 0, 0),
    }
}

