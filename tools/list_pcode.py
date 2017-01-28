#!/usr/bin/python3
# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import binascii
import sys
import argparse
import bisect
import collections
import operator

from textutil import pad_right
import opcodes, pme_defs
from endian import BE,LE
from printable import is_printable, PRCHR, printable
from metadata import ProcedureMetaData, ProcedureList
from code import Instruction, Procedure, Segment
from disass import disassemble_unit
from segdir import SegmentInfo,SegmentDirectory

# TODO
# - calltree metadata: cross-reference where procedures are called from
# - cross-reference globals and locals:
#   thoughout entire program where is each local/global referenced from?
# - cross-reference SYSCOM:
#   how is this accessed? seems through KERNEL+0x1
# - see further-analysis.txt

##### Colors
class ATTRbase:
    reset = ''
    addr = ''
    jumpaddr = ''
    unitname = ''
    colon = ''
    instr = ''
    bytes = ''
    cstart = ''
    comment = ''
    args = ''
    argsep = ''

    @classmethod
    def for_depth(cls, x):
        return ''

class ATTRcolor(ATTRbase):
    reset = '\x1b[0m'
    addr = '\x1b[97m'
    jumpaddr = '\x1b[93m'
    unitname = '\x1b[30;104m'
    colon = '\x1b[90m'
    instr = '\x1b[94m'
    bytes = '\x1b[37m'
    cstart = '\x1b[90m'
    comment = '\x1b[36m'
    args = '\x1b[96m'
    argsep = '\x1b[36m'

    @classmethod
    def for_depth(cls, x):
        if x is None:
            return '\x1b[30;48;5;236m'
        else:
            return '\x1b[30;48;5;%dm' % (min(max(x,0),15)+240)

ATTR = None

def set_color_mode(color):
    global ATTR
    if color:
        ATTR = ATTRcolor
    else:
        ATTR = ATTRbase

def list_unit(data, base, endian, imports):
    '''
    list of exported functions
    '''
    codesize = endian.getword(data, base)*2 + 2
    segnum_numproc = endian.getword(data, base + codesize - 2)

    segnum = segnum_numproc >> 8 # is this used at all?
    numproc = segnum_numproc & 0xff

    print('  {a.comment}Procedures{a.reset}'.format(a=ATTR))
    # analyze procedures
    addrs = []
    for proc in range(0,numproc):
        pbase = base + codesize - 4 - 2*proc
        value = endian.getword(data, pbase)
        value *= 2
        where = value + base

        end = endian.getword(data, where-2) + base
        
        if value != 0:
            info = (
                where,
                endian.getword(data, where), 
                end,
                data[end],
                end-where+1)
            print('  0x{:02x} {:08x}-{:08x} 0x{:04x} h={:04x} end={:04x} data[end]={:02x} size={:04x}'.format(
                proc+1,where,end,*info))

    if imports:
        print('  {a.comment}Segment reference list{a.reset}'.format(a=ATTR))
        for idx in sorted(imports.keys()):
            print('  0x{:04x} {a.unitname}{}{a.reset}'.format(idx,imports[idx].decode(),a=ATTR))

MAX_BYTES_PER_LINE = 16
def hex_dump_line(data, ptr, by_addr):
    '''
    Hex-dump of one line of data, or until next event.
    '''
    argsep = '{a.argsep},'.format(a=ATTR)
    sptr = ptr + 1 # scan until next procedure or end
    send = ptr+MAX_BYTES_PER_LINE
    while sptr < send:
        if sptr in by_addr:
            break
        sptr += 1
    slc = data[ptr:sptr]
    b = argsep.join('{a.args}0x{:02x}'.format(x,a=ATTR) for x in slc)
    # padding
    padding = ' ' * ((MAX_BYTES_PER_LINE-len(slc))*5)
    # ascii column
    comment = ' {a.cstart}; {a.comment}{}{a.reset}'.format(printable(slc),a=ATTR)

    print('{a.addr}{:04x}{a.colon}:{a.reset}  .db {}{a.reset} {}{}'.format(ptr,b,padding,comment,a=ATTR))
    return sptr

def argcolor(flags):
    if flags & opcodes.RELFLAG:
        return ATTR.jumpaddr
    else:
        return ATTR.args

def argformat(flags, x):
    if flags & opcodes.RELFLAG: # render jump labels always as 04x
        s = '{:04x}'.format(x)
    else:
        s = '0x{:x}'.format(x)
    return argcolor(flags) + s

def procedure_name(proclist, refseg, proc, suffix=''):
    procmeta = proclist[(refseg,proc)]
    if procmeta.name is not None:
        name = '{}:0x{:02x}{} {}'.format(refseg.decode().rstrip(),proc,suffix,procmeta.name)
    else:
        name = '{}:0x{:02x}{}'.format(refseg.decode().rstrip(),proc,suffix)
    return name

def format_instruction(inst, proc, dseg, proclist):
    '''
    Format instruction to (colored) text.
    '''
    # TODO: make a function for comment formatting, DRY
    # some of the per-instruction analysis could be factored out in a format
    # usable for other analysis steps
    data = dseg.data
    endian = dseg.endian
    seginfo = dseg.info
    # instruction to text
    opc = inst.opcode
    args = inst.args
    op = opcodes.OPCODES[opc]
    argsep = '{a.argsep},'.format(a=ATTR)
    strargs = argsep.join(argformat(flags, x) for x,flags in zip(inst.args,op[1]))
    b=' '.join(('%02x' % x) for x in data[inst.addr:inst.addr+inst.size])
    comment = ''
    # show target procedure for calls
    if (op[2] & opcodes.CALL) and opc != opcodes.RPU and opc != opcodes.CFP: # not RTU/CFP as we don't know the target
        (seg, proc) = inst.get_call_target(seginfo.seg_num)
        refseg = seginfo.references.get(seg, b'???')
        comment = ' {a.cstart};{a.comment} {}{a.reset}'.format(
                procedure_name(proclist,refseg,proc),a=ATTR)
    # access global
    elif (op[2] & opcodes.GLOBAL) and (op[2] & (opcodes.LOAD|opcodes.ADDR|opcodes.STORE)):
        (seg,ofs) = inst.get_global_number(seginfo.gseg_num)
        iname = seginfo.references.get(seg, b'???').decode().rstrip()
        name = '{}_G{:x}'.format(iname,ofs)
        comment = ' {a.cstart};{a.comment} {}{a.reset}'.format(name,a=ATTR)
    # access intermediate or local
    elif (op[2] & (opcodes.INTRMD|opcodes.LOCAL)) and (op[2] & (opcodes.LOAD|opcodes.ADDR|opcodes.STORE)):
        # find lexical parent identified by instruction
        meta = None
        lvl = inst.get_lex_level()
        if lvl is not None:
            meta = proclist[(seginfo.name,proc.num)]
            for _ in range(lvl):
                if meta is None:
                    break
                meta = meta.parent
        # format comment if known
        if meta is not None:
            lnum = inst.get_local_number()
            name = '{}_{:02x}_L{:x}'.format(meta.key[0].decode().rstrip(),meta.key[1],lnum)
            if lnum > meta.num_locals:
                if lnum < meta.num_locals - meta.delta + 1:
                    name += ' (param {:d})'.format(lnum - meta.num_locals - 1)
                else:
                    name += ' (retval {:d})'.format(lnum - meta.num_locals + meta.delta - 1)
            comment = ' {a.cstart};{a.comment} {}{a.reset}'.format(name,a=ATTR)
    # lco: print string
    elif opc == opcodes.LCO:
        ofs = dseg.datastart + args[0] * 2
        try:
            s = data[ofs+1:ofs+1+data[ofs]] # load byte-prefixed string
        except IndexError:
            comment = ' {a.cstart}; {a.comment}error: out of range{a.reset}'.format(a=ATTR)
        else:
            if all(is_printable(b) for b in s):
                comment = ' {a.cstart};{a.comment} "{}" @ 0x{:04x}{a.reset}'.format(s.decode(),ofs,a=ATTR)
            else:
                comment = ' {a.cstart};{a.comment} 0x{:04x}{a.reset}'.format(ofs,a=ATTR)
    # xjp: print jump table
    elif opc == opcodes.XJP:
        ofs = dseg.datastart + args[0] * 2
        low = endian.getword(data, ofs)
        high = endian.getword(data, ofs+2)
        comment = ['']
        comment.append('{a.cstart};{a.comment} jump table 0x{:04x}:0x{:04x}{a.reset}'.format(low,high,a=ATTR))
        ofs += 4
        for x in range(low, high+1):
            w = (inst.addr + inst.size + endian.getword(data, ofs)) & 0xffff
            comment.append('{a.cstart};{a.comment}   0x{:04x} to {a.jumpaddr}{:04x}'.format(x,w,a=ATTR))
            ofs += 2
        comment = '\n'.join(comment)

    # print instruction
    return '{}{:04x}{a.colon}:{a.reset}  {a.bytes}{:11s}{a.reset} {a.instr}{}{a.reset} {}{a.reset}{}'.format(
        ATTR.jumpaddr if inst.is_jumptgt else ATTR.addr,
        inst.addr,b,op[0].lower(),strargs,comment,a=ATTR)

class DisassemblyPrintOptions:
    jumpskip = True     # skip a line before jump targets
    print_depth = False # print depth before instruction
    print_depth_after = True # print depth after instruction

def print_disassembly(dseg, data, endian, proclist, opts = DisassemblyPrintOptions(), selection=None):
    '''
    print disassembled unit code
    '''
    ptr = dseg.begin
    seg = dseg.info

    crossreference_constants(dseg)

    def print_procedure(proc):
        print()
        if opts.print_depth or opts.print_depth_after: # do flow analysis if we want depth
            control_flow_analysis(proc, dseg, proclist)
        print('       {a.cstart};{a.comment} start of procedure {}{a.reset}'.format(
            procedure_name(proclist,dseg.name,proc.num),a=ATTR))
        # print some meta-information about procedure, if known
        meta = proclist[(seg.name,proc.num)]
        if meta is not None:
            if meta.parent is not None:
                print('       {a.cstart};{a.comment}   lexical parent: {:s}{a.reset}'.format(procedure_name(proclist, *meta.parent.key),a=ATTR))
            if meta.delta is not None:
                print('       {a.cstart};{a.comment}   num params: {:d}{a.reset}'.format(-meta.delta,a=ATTR))

        print('{a.addr}{:04x}{a.colon}:{a.reset}  .dw {a.args}0x{:04x}{a.reset}'.format(proc.addr-4,proc.end_addr,a=ATTR))
        if proc.is_native:
            comment = ' {a.cstart};{a.comment} native code{a.reset}'.format(a=ATTR)
        else:
            comment = ''
        print('{a.addr}{:04x}{a.colon}:{a.reset}  .dw {a.args}0x{:04x}{a.reset}{}'.format(proc.addr-2,proc.num_locals,comment,a=ATTR))

        first = True
        for inst in proc.instructions:
            if opts.jumpskip and inst.is_jumptgt and not first:
                print()
            if opts.print_depth:
                print('{}{}{a.reset}'.format(
                    ATTR.for_depth(inst.depth),
                    '{:-2d}'.format(inst.depth) if inst.depth is not None else '  ',
                    a=ATTR), end='')
            if opts.print_depth_after:
                print('{}{}{a.reset}'.format(
                    ATTR.for_depth(inst.depth_after),
                    '{:-2d}'.format(inst.depth_after) if inst.depth_after is not None else '  ',
                    a=ATTR), end='')
            if opts.print_depth or opts.print_depth_after:
                print(' ', end='')
            print(format_instruction(inst, proc, dseg, proclist))
            first = False

    # If specific procedure selected, show just that one
    if selection is not None:
        print_procedure(dseg.proc_by_num[selection])
        return

    # treat procedures as events
    # insert dummy procedure for start of constant pool
    events = [(proc.addr-4,proc) for proc in dseg.procedures]
    #bisect.insort_left(events,(dseg.datastart, None))
    events.append((dseg.datastart, None))
    for (addr,refs) in dseg.cpool.items():
        events.append((addr, refs))
    # sort events by address
    events.sort(key=operator.itemgetter(0))

    for addr,proc in events:
        # hexdump bytes until next event
        while ptr < addr:
            ptr = hex_dump_line(data, ptr, {addr})
        if isinstance(proc, Procedure):
            print_procedure(proc)
            if not proc.is_native: # print native functions as hexdump
                ptr = proc.addr + proc.size
        elif isinstance(proc, list): # list of constant pool references
            refstr = ', '.join('{:04x}'.format(x) for x in proc)
            print('       {a.cstart};{a.comment} ref: {}{a.reset}'.format(refstr,a=ATTR))
        else:
            print('       {a.cstart};{a.comment} start of constant pool for {}{a.reset}'.format(dseg.name_str,a=ATTR))
    while ptr < dseg.end: # hexdump remainder
        ptr = hex_dump_line(data, ptr, {dseg.end})

def print_statistics(segments, proclist):
    counts = [0]*256
    calls = collections.defaultdict(int)
    for dseg in segments:
        for proc in dseg.procedures:
            for inst in proc.instructions:
                counts[inst.opcode] += 1
                tgt = inst.get_call_target(dseg.info.seg_num)
                if tgt is not None:
                    calls[dseg.info.references[tgt[0]],tgt[1]] += 1
                
    for opc in range(256):
        print('{:5d} 0x{:02x} {:8s}'.format(counts[opc], opc, opcodes.OPCODES[opc][0]))
    print()
    for (seg,procidx),count in sorted(calls.items(),key=lambda x:x[0]):
        print('{:5d} {:16s}'.format(
            count,
            procedure_name(proclist,seg, procidx)))

def procedure_hierarchy(segments):
    '''
    Analysis: procedure hierarchy (nested procedures).
    '''
    # (a,b): l
    # l = 0   b is a child of a
    # l = 1   b is a child of a's parent
    # l = 2   b is a child of a's grandparent
    # ...
    hmap = {}
    for dseg in segments:
        for proc in dseg.procedures:
            for inst in proc.instructions:
                op = opcodes.OPCODES[inst.opcode]
                if op[2] & opcodes.CALL:
                    tgt = inst.get_call_target(dseg.info.seg_num)
                    ll = inst.get_lex_level()
                    if ll is None:
                        continue
                    key = (dseg.name, proc.num,dseg.info.references[tgt[0]], tgt[1])
                    if key in hmap:
                        if hmap[key] != ll:
                            raise ValueError('Procedure hierarchy conflict')
                    else:
                        hmap[key] = ll
                else:
                    continue

    per_level = collections.defaultdict(list)
    for (s1,n1,s2,n2),ll in sorted(hmap.items(), key=lambda x:x[0]):
        per_level[ll].append((s1,n1,s2,n2))

    class Node:
        def __init__(self, seg, name):
            self.seg = seg
            self.name = name
            self.parent = None
            self.children = []
        @property
        def key(self):
            return (self.seg, self.name)
    nodes = {}
    for (s1,n1,s2,n2),ll in hmap.items():
        if (s1,n1) not in nodes:
            nodes[s1,n1] = Node(s1,n1)
        if (s2,n2) not in nodes:
            nodes[s2,n2] = Node(s2,n2)

    # attempt to fit it all into the hierarchy
    ls1 = list(hmap.items())
    while ls1:
        ls2 = []
        for (s1,n1,s2,n2),ll in ls1:
            m = nodes[s1,n1]
            n = nodes[s2,n2]
            for _ in range(ll):
                m = m.parent
                if m is None:
                    break
            if m is None:
                ls2.append(((s1,n1,s2,n2),ll))
                continue
            if n.parent is not None: # N already is linked, nothing to do but check
                if n.parent is not m:
                    raise ValueError('Procedure hierarchy conflict')
            else: # Make link
                m.children.append(n)
                n.parent = m
        if len(ls1) == len(ls2):
            raise ValueError('Invalid hierarchy - no progress in tree build with remaining nodes')
        ls1 = ls2

    return nodes

def print_procedure_hierarchy(nodes, segments, proclist):
    '''
    Analysis: procedure hierarchy (nested procedures).
    '''
    # Pretty printing
    def print_children(node, level):
        for child in sorted(node.children, key=lambda n:n.key):
            print('  '*level, procedure_name(proclist, child.seg, child.name))
            print_children(child, level+1)

    # Print trees per segment
    for dseg in segments:
        print('{a.unitname}{}{a.reset} {}'.format(dseg.name.decode(),dseg.endian.__name__,a=ATTR))
        for proc in dseg.procedures:
            node = nodes.get((dseg.name, proc.num), None)
            if node is None or node.parent is None:
                print('  ', procedure_name(proclist, dseg.name, proc.num))
                if node is not None:
                    print_children(node,2)

def export_procedure_hierarchy(nodes, out=sys.stdout):
    # Generate parent map for export
    out.write('# Originally generated by list_pcode: procedure_hierarchy\n')
    import pprint
    parent_map = {}
    out.write('hierarchy = ')
    for proc,node in sorted(nodes.items(), key=lambda x:x[0]):
        if node.parent is not None:
            parent_map[proc] = (node.parent.seg,node.parent.name)
    pprint.pprint(parent_map, out)

def procedure_stack_delta(proc):
    '''
    Compute the stack delta for a procedure.
    This is the number of words added (or removed) from the stack
    after return.
    This can be used for further analysis.
    '''
    if proc.is_native:
        return None
    ret_arg = None
    for inst in proc.instructions: # Find the RPU instruction
        if inst.opcode == opcodes.RPU:
            ret_arg = inst.args[0]
    return proc.num_locals - ret_arg

def procedure_deltas(segments, out=sys.stdout):
    '''
    Analysis: procedure stack deltas.
    '''
    import pprint
    delta_map = {}
    out.write('# Originally generated by list_pcode: procedure_deltas\n')
    out.write('deltas = ')
    for dseg in segments:
        for proc in dseg.procedures:
            delta = procedure_stack_delta(proc)
            #print('{:3d} {}'.format(delta if delta is not None else -999,procedure_name(proclist, dseg.name, proc.num)))
            delta_map[(dseg.name, proc.num)] = (delta, None, None, proc.num_locals)
    pprint.pprint(delta_map, stream=out)

def make_calltree(segments):
    links = set()
    for dseg in segments:
        for proc in dseg.procedures:
            src = (dseg.name, proc.num)
            for inst in proc.instructions:
                op = opcodes.OPCODES[inst.opcode]
                if op[2] & opcodes.CALL: # for every call, add a reference
                    tgt = inst.get_call_target(dseg.info.seg_num)
                    if tgt is not None:
                        links.add((src,(dseg.info.references[tgt[0]], tgt[1])))
                else:
                    continue
    return links

def print_calltree(tree, segments, proclist):
    import pprint
    pprint.pprint(tree)
    print()
    # condensed tree
    links2 = set()
    for k,v in tree:
        links2.add((k[0],v[0]))
    pprint.pprint(links2)

def control_flow_analysis(proc, dseg, proclist, debug=False):
    '''
    Perform control flow analysis on a procedure to find stack depth at each instruction.
    '''
    if proc.is_native: # Cannot do native code
        return
    seg = dseg.info

    # create mapping from address to index
    idx_by_addr = {inst.addr:idx for idx, inst in enumerate(proc.instructions)}

    # set initial depth to unknown
    for inst in proc.instructions:
        inst.depth = None
        inst.depth_after = None

    heads = [(0,0)]
    while heads:
        (ptr,depth) = heads.pop()
        if debug:
            print('Head', (ptr,depth))
        while True: # advance follow until end of basic block
            inst = proc.instructions[ptr]
            if debug:
                print('%i %04x %i 0x%x' % (ptr,inst.addr,depth,inst.opcode))
            if inst.depth is not None:
                if inst.depth != depth:
                    print('control_flow_analysis: Mismatch at 0x%08x! %d versus %d' % (inst.addr, inst.depth, depth))
                    return
                else:
                    break # stop linear progress here, we have a match
            inst.depth = depth

            # determine stack delta
            delta = 0
            op = opcodes.OPCODES[inst.opcode]
            if op[4] is not None and op[3] is not None:
                delta += op[4] - op[3]
            else:
                if op[2] & opcodes.CALL: # for a call, query the stack depth delta of the target
                    tgt = inst.get_call_target(seg.seg_num)
                    if tgt is not None:
                        tgtmeta = proclist[seg.references[tgt[0]],tgt[1]]
                        if tgtmeta.delta is not None:
                            delta = tgtmeta.delta
                        else: # unknown delta for procedure (native without proper metadata)
                            delta = None
                    else: # unknown target
                        delta = None
                elif inst.opcode == opcodes.STM: # store # words from stack
                    delta = -inst.args[0]
                elif inst.opcode == opcodes.LDM: # load # words to stack
                    delta = inst.args[0]
                elif inst.opcode == opcodes.LDC: # load # words to stack
                    delta = inst.args[2]
                else: # unknown delta for this instruction (set ops etc)
                    # these are annoying as we can't statically determine the stack depth,
                    # it depends on the particular set size
                    delta = None
            # advance depth
            if depth is not None:
                if delta is not None:
                    depth += delta
                else:
                    depth = None

            inst.depth_after = depth
    
            # we've lost track, give up this path
            if depth is None:
                break

            # watch for ends of basic blocks
            if inst.opcode == opcodes.RPU:
                if inst.depth != 0:
                    print('control_flow_analysis: WARNING: esp is not 0 at return point %08x' % (inst.addr))
                break # full stop
            elif op[2] & opcodes.CFLOW:
                for tgt in inst.get_flow_targets(dseg):
                    if debug:
                        print('  %04x Adding head (%i,%i)' % (tgt, idx_by_addr[tgt], depth))
                    heads.append((idx_by_addr[tgt], depth))
                break # stop linear progress here
            else:
                ptr += 1

def crossreference_constants(dseg):
    '''
    Cross-reference lcos to determine constant pool references.
    '''
    dseg.cpool = {}
    for proc in dseg.procedures:
        for inst in proc.instructions:
            if inst.opcode == opcodes.LCO:
                ofs = dseg.datastart + inst.args[0] * 2
                dseg.cpool.setdefault(ofs, []).append(inst.addr)

def crossreference_syscom(segments):
    '''
    Analyze all accesses to SYSCOM area, create a list by offset.
    '''
    rv = []
    for dseg in segments:
        for proc in dseg.procedures:
            flag = 0
            pc = None
            for inst in proc.instructions:
                if not flag:
                    ginfo = inst.get_global_number(dseg.info.gseg_num)
                    if ginfo is not None:
                        if ginfo == (1,1): # syscom pointer
                            pc = inst.addr
                            flag = 1
                else: # instruction after syscom pointer load
                    if inst.opcode == opcodes.IND: # fetch
                        rv.append((dseg, pc, 0, inst.args[0]))
                    elif opcodes.SIND0 <= inst.opcode <= opcodes.SIND7: # fetch
                        rv.append((dseg, pc, 0, inst.opcode - opcodes.SIND0))
                    elif inst.opcode == opcodes.INC: # get address
                        rv.append((dseg, pc, 1, inst.args[0]))
                    else: # remainder is setting IOERRORs and such
                        #print(format_instruction(inst, proc, dseg, proclist))
                        pass
                    flag = 0
    return rv

def print_syscom(info, segments, proclist):
    print('Accesses:')
    for (dseg, pc, ttpe, offset) in info:
        print('  {:16s} {} 0x{:04x} (byte 0x{:04x})'.format('{}:{:04x}'.format(dseg.name_str, pc), ['v','a'][ttpe], offset, offset*2))
    print()
    print('Unique:')
    for offset in set(x[3] for x in info):
        print('  0x{:04x} (byte 0x{:04x})'.format(offset, offset*2))

def parse_arguments():
    parser = argparse.ArgumentParser(description='sundog pcode disassembler')
    parser.add_argument('-m', dest='mode',
            default=2, type=int,
            help='Output mode (0=list, 1=procedures and exports, 2=disassembly)')
    parser.add_argument('-c', '--color', dest='color',
            default=1, type=int,
            help='Enable/disable color (default 1)')
    parser.add_argument('-s', '--segment', dest='segment',
            default=None, type=str,
            help='Disassemble a specific segment or segment:0xXX procedure')
    parser.add_argument('input', metavar='INFILE', type=str, nargs='+',
            help='P-code files to disassemble')
    return parser.parse_args()

def load_metadata():
    '''Build known procedure list for project'''
    import libcalls_list, appcalls_list 
    import libcalls_deltas, appcalls_deltas
    import libcalls_hierarchy, appcalls_hierarchy
    proclist = ProcedureList()
    proclist.load_deltas(libcalls_deltas.deltas)
    proclist.load_deltas(appcalls_deltas.deltas)
    proclist.load_hierarchy(libcalls_hierarchy.hierarchy)
    proclist.load_hierarchy(appcalls_hierarchy.hierarchy)
    proclist.load_proclist(libcalls_list.LIBCALLS)
    proclist.load_proclist(appcalls_list.APPCALLS)
    return proclist

def main():
    # initial argument parsing
    args = parse_arguments()
    set_color_mode(args.color)
    if args.color:
        print('\x1b[90m[\x1b[95msundog pcode disassembler\x1b[90m]\x1b[0m  \x1b[94mW.J. van der Laan 2017\x1b[0m')
    # select specific segment[:procedure]
    if args.segment is not None:
        s = args.segment.split(':')
        if len(s) >= 2:
            selection = (s[0].upper(),int(s[1],0))
        else:
            selection = (s[0].upper(),None)
    else:
        selection = (None,None)

    proclist = None
    if args.mode not in [5,6]:
        proclist = load_metadata()

    segments = []

    for filename in args.input:
        with open(filename, 'rb') as f:
            data = f.read()

        dir_ = SegmentDirectory.load(data)
        dir_.load_imports(data)
        for seg in dir_.segments:
            if selection[0] is not None and selection[0] != seg.name_str:
                continue
            base = seg.base
            codesize = seg.codesize
            assert(data[base+4:base+12] == seg.name)
            assert(data[base+12:base+14] == b'\x01\x00' or data[base+12:base+14] == b'\x00\x01')

            endian = [BE,LE][data[base+12]]

            size2 = endian.getword(data, base) * 2
            size3 = endian.getword(data, base+14) * 2
            size4 = endian.getword(data, base+2) * 2 # has to do with native instructions / relocations
            # size of "real pool": we don't care
            #assert(endian.getword(data, base+16) == 0x0004)
            hdr = binascii.b2a_hex(data[base+18:base+22]) # Softech "part id"

            if args.mode < 3:
                if seg.parent_segment is not None: # subsegment
                    val4_str = seg.parent_segment.decode()
                else:
                    # +0 ?
                    # +2 imports size
                    # +4 highest import offset (2 if no imports)
                    # +6 always 0?
                    val4_str = '0x{:04x} 0x{:04x}'.format(seg.top_info[0], seg.top_info[2])
                print('{a.addr}0x{:04x}-0x{:04x}{a.reset} [d]0x{:04x} [2]0x{:04x} [4]0x{:04x} {a.unitname}{}{a.reset} {} {} {:04x} {:04x} {:04x} {}'.format(
                    base, base+codesize, base+size3, base+size2, base+size4, seg.name.decode(), 
                    endian.__name__,
                    hdr.decode(),seg.val1,seg.val2,seg.val3,
                    val4_str, a=ATTR))

            if args.mode == 1:
                list_unit(data, base, endian, seg.imports)
            if args.mode == 2:
                dseg = disassemble_unit(data[base:base+seg.codesize], proclist, seg)
                disopt = DisassemblyPrintOptions()
                disopt.procedure = selection[1]
                print_disassembly(dseg, data[base:base+seg.codesize], endian, proclist, disopt, selection[1])
                print('{a.cstart};{a.comment} ----------------------------------{a.reset}'.format(a=ATTR))
            if args.mode >= 3: # disassemble all units for statistics
                segments.append(disassemble_unit(data[base:base+seg.codesize], proclist, seg))

    if args.mode == 3:
        print_statistics(segments, proclist)
    if args.mode == 4:
         nodes = procedure_hierarchy(segments)
         print_procedure_hierarchy(nodes, segments, proclist)
    if args.mode == 5:
         nodes = procedure_hierarchy(segments)
         export_procedure_hierarchy(nodes)
    if args.mode == 6:
         procedure_deltas(segments)
    if args.mode == 7:
         tree = make_calltree(segments)
         print_calltree(tree, segments, proclist)
    if args.mode == 8:
         info = crossreference_syscom(segments)
         print_syscom(info, segments, proclist)

if __name__ == '__main__':
    main()

