# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from signext import signext
import opcodes
from endian import BE,LE
from code import Instruction, Procedure, Segment

def extract_args(op, data, ptr):
    '''
    Load instruction arguments.
    '''
    args = []
    for arg in op[1]:
        signed = arg & opcodes.SIGNED
        arg = arg & opcodes.ARGTMASK
        if arg == opcodes.VAR: # "big"
            v = data[ptr]
            ptr += 1
            if v >= 0x80:
                v = (v & 0x7f) << 8 | data[ptr]
                ptr += 1
        elif arg == 1: # byte
            if signed: 
                v = signext(data[ptr], 8)
            else:
                v = data[ptr]
            ptr += 1
        elif arg == 2: # LE word
            if signed:
                v = signext(LE.getword(data, ptr), 16)
            else:
                v = LE.getword(data, ptr)
            ptr += 2
        else:
            raise NotImplemented
        args.append(v)

    # compute relative offsets
    for i,arg in enumerate(op[1]):
        if arg & opcodes.RELFLAG:
            args[i] = ptr + args[i]
    return (args, ptr)


def disassemble_unit(data, proclist, seg):
    '''
    Disassemble segment unit code
    '''
    rv = Segment()
    rv.info = seg
    rv.data = data
    endian = rv.endian = [BE,LE][data[12]]
    codesize = endian.getword(data, 0)*2 + 2
    rv.name = data[4:12]
    datastart = rv.datastart = endian.getword(data, 14) * 2
    numproc = endian.getword(data, codesize - 2) & 0xff
    begin = rv.begin = 0x16
    end = rv.end = codesize - 2 - 2*numproc

    # take note of function starts, before scanning over the code
    events = []
    for proc in range(0,numproc):
        pbase = codesize - 4 - 2*proc
        value = endian.getword(data, pbase)
        if value != 0: # mark beginning of procedure
            events.append((value*2 - 2, proc + 1)) # start from 1
    events.append((datastart, None))
    events.append((end, None))
    events.sort()

    def close_procedure():
        nonlocal curproc,dis
        dis = False
        if curproc is not None: # set size of last p-code procedure
            curproc.size = ptr - curproc.addr
            rv.procedures.append(curproc)
            curproc = None

    # note: native procedures are in the constant pool, so we don't stop
    # at the beginning of that.
    ptr = begin
    dis = False
    curproc = None
    evidx = 0
    while ptr < end:
        if ptr == events[evidx][0]:
            close_procedure()
            if events[evidx][1] is not None:
                curproc = Procedure()
                curproc.num = events[evidx][1]
                curproc.end_addr = endian.getword(data, ptr)
                curproc.num_locals = endian.getword(data, ptr + 2)
                ptr += 4
                curproc.addr = ptr # start of actual code
                dis = not curproc.is_native
            evidx += 1
        elif dis: # Instruction
            inst = Instruction()
            inst.addr = ptr
            inst.opcode = data[ptr]
            ptr += 1
            op = opcodes.OPCODES[inst.opcode]
            (inst.args, ptr) = extract_args(op, data, ptr)
            inst.size = ptr - inst.addr

            curproc.instructions.append(inst)
            if inst.opcode == 0x96: # RET
                close_procedure()
        else: # skip until next procedure or other event
            ptr = events[evidx][0]

    close_procedure()

    # Pre-analysis

    # fill in procedure by number mapping
    for proc in rv.procedures:
        rv.proc_by_num[proc.num] = proc

    # fill in instruction by addr mapping
    for proc in rv.procedures:
        for inst in proc.instructions:
            proc.inst_by_addr[inst.addr] = inst

    # fill in jump targets
    for proc in rv.procedures:
        for inst in proc.instructions:
            targets = inst.get_flow_targets(rv)
            for tgt in targets:
                if tgt != inst.addr+inst.size: # discontinuous jump
                    try:
                        proc.inst_by_addr[tgt].is_jumptgt = True
                    except KeyError:
                        pass # well, too bad

    return rv


