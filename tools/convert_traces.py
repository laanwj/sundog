#!/usr/bin/python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import sys
import collections
import struct

A3 = 0x00209ac # interpreter base address
PC_INTERP = A3+0xde
PC_EVENT = A3+0x10e2
PC_SYSSTAT = A3+0x2186

def read_mem_lines(i, n):
    # 00030C90: d4 f0 d5 c0 d5 c0 1a ba d5 d0 00 01 00 00 00 00   ................
    firstaddr = None
    lastaddr = None
    data = bytearray()
    for _ in range(n):
        line = next(i)
        addr = int(line[0:8],16)
        if firstaddr is None:
            firstaddr = addr
        if lastaddr is not None:
            assert(addr == lastaddr + 16)
        h = line[10:10+16*3-1].split(' ')
        data.extend(int(x,16) for x in h)

        # store last address for continuity checking
        lastaddr = addr

    return (firstaddr, data)

HatariTraceRecord = collections.namedtuple('HatariTraceRecord', ('regs','stack','state','syscom'))
HatariEventRecord = collections.namedtuple('HatariEventRecord', ('regs'))
HatariSysstatRecord = collections.namedtuple('HatariSysstatRecord', ('statrec'))

def parse_hatari_trace(f):
    i = iter(f)
    while True:
        # <register lines>
        #   D0 0000D4F0   D1 00000000   D2 000003C0   D3 00030002  * 5
        regs = {}
        for _ in range(5):
            tokens = next(i).split()
            for x in range(len(tokens)//2):
                regs[tokens[x*2]] = int(tokens[x*2+1],16)

        # T=00 S=0 M=0 X=0 N=1 Z=0 V=0 C=0 IMASK=3 STP=0
        next(i)
        # 00020A8A 7000                     MOVE.L #$00000000,D0
        pcline = next(i)
        pc = int(pcline[0:8], 16)
        # Next PC: 00020a8c
        next(i)
        if pc == PC_INTERP:
            # <mem lines> for stack
            stack = read_mem_lines(i, 8)
            # <mem lines> for VM state @ a3+0x1b88
            state = read_mem_lines(i, 8)
            # <mem lines> for VM state @ a3+0x1b88
            syscom = read_mem_lines(i, 8)
            # 00030C90: d4 f0 d5 c0 d5 c0 1a ba d5 d0 00 01 00 00 00 00   ................
            yield HatariTraceRecord(regs,stack,state,syscom)
        elif pc == PC_EVENT:
            yield HatariEventRecord(regs)
        elif pc == PC_SYSSTAT:
            sysstat = read_mem_lines(i, 4)
            yield HatariSysstatRecord(struct.unpack('>30H', sysstat[1][0:60]))
        else:
            raise ValueError('Unhandled PC 0x%08x' % pc)

PTraceRegs = collections.namedtuple('PTraceRegs', ('curseg', 'ipc', 'sp', 'mp', 'base', 'readyq', 'evec', 'curtask', 'erec', 'curproc', 'event'))

class PTraceRecord:
    def __init__(self, regs, stack, syscom):
        self.regs = regs
        self.stack = stack
        self.syscom = syscom
        self.state = None

def hatari_to_psystem_trace(ht):
    event = 0
    outrecs = 0
    nextrec = None  # buffer for one record
    state = (0,)    # current time for now
    for rec in ht:
        if isinstance(rec, HatariTraceRecord):
            if nextrec is not None: # emit buffered record, with accumulated state
                nextrec.state = state
                yield nextrec
                nextrec = None
            if rec.regs['A5'] != PC_INTERP:
                print('Delayed event at %d' % outrecs)
                event = 0x8000|event

            ipc  = rec.regs['A4'] - rec.regs['A6']
            sp   = rec.regs['A7'] - rec.regs['A6']
            mp   = rec.regs['A0'] - rec.regs['A6']
            base = rec.regs['A1'] - rec.regs['A6']
            curseg = rec.regs['A2'] - rec.regs['A6']
            assert(rec.stack[0] == rec.regs['A7'] and len(rec.stack[1])==128)
            assert(rec.syscom[0] == rec.regs['A6'] and len(rec.syscom[1])==128)
            #0x1b88(a3) 2  READYQ: Set/used during initialization, copied to 1b8c
            #0x1b8a(a3) 2  EVEC: references to linked segments EREC
            #0x1b8c(a3) 2  CURTASK
            #0x1b8e(a3) 2  EREC: Pointer to current segment E_Rec
            #0x1b90(a3) 2  CURPROC: Current procedure number
            (readyq, evec, curtask, erec, curproc) = struct.unpack('>HHHHH', rec.state[1][0:10])

            nextrec = PTraceRecord(
                    regs=PTraceRegs(curseg,ipc,sp,mp,base,readyq,evec,curtask,erec,curproc,event),
                    stack=rec.stack[1],
                    syscom=rec.syscom[1])
            outrecs += 1
            event = 0
        elif isinstance(rec, HatariEventRecord):
            if rec.regs['D0'] != 0: # D0 is event #
                raise ValueError('Unhandled event %d' % rec.regs['D0'])
            event = event + 1
        elif isinstance(rec, HatariSysstatRecord):
            # See appendix D, p-systems reference
            # [1] time_lo
            # [2] time_hi
            time = (rec.statrec[2] << 16)|rec.statrec[1]
            # print('time: 0x%08x' % (time))
            # This time applies to the instruction already emitted, which is awkward.
            state = (time,)
    # last record
    if nextrec is not None: # emit buffered record, with accumulated state
        nextrec.state = state
        yield nextrec

REGPACK = struct.Struct('IIHHHHHHHHH')
STATEPACK = struct.Struct('I')
RECSIZE = REGPACK.size + 128 + 128 + STATEPACK.size
if __name__ == '__main__':
    with open(sys.argv[1], 'r') as f:
        with open(sys.argv[2], 'wb') as g:
            g.write(struct.pack('I', RECSIZE))
            ht = parse_hatari_trace(f)
            written = 0
            for rec in hatari_to_psystem_trace(ht):
                #print('%05x %04x %04x %04x %05x %04x %04x %04x %04x %04x' % 
                #        (rec.regs.ipc, rec.regs.sp, rec.regs.mp, rec.regs.base, rec.regs.curseg, rec.regs.readyq, rec.regs.evec, rec.regs.curtask, rec.regs.erec, rec.regs.curproc))
                g.write(REGPACK.pack(*rec.regs))
                g.write(rec.stack)
                g.write(rec.syscom)
                g.write(STATEPACK.pack(*rec.state))
                written += 1
            print("Total records written: {:d}".format(written))


