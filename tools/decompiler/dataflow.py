# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import opcodes

####### Instruction-level data flow analysis ########

class ILAInfo:
    def __init__(self):
        self.ins = []  # inputs, from ..., TOS-1, TOS0, in reverse order of pop
        self.outs = [] # outputs, from ..., TOS-1, TOS0 in order of push

class StackType:
    INT = 0
    SET = 1

    by_idx = ['INT','SET']

class INode:
    temp = None # name for this node, or None
    def __init__(self):
        self.uses = [] # List of instructions and other INodes where this value is used

class BBInput(INode):
    def __init__(self, type_, idx):
        INode.__init__(self)
        self.type_ = type_
        self.idx = idx
    def __repr__(self):
        return 'BBInput(%s,%d)' % (StackType.by_idx[self.type_],self.idx)

class IOutput(INode):
    def __init__(self, type_, inst, idx):
        INode.__init__(self)
        self.type_ = type_
        self.inst = inst
        self.idx = idx
    def __repr__(self):
        return 'IOutput(%s,inst@%04x,%d)' % (StackType.by_idx[self.type_],self.inst.addr,self.idx)

class OMakeSet(INode):
    def __init__(self, addr, size, data):
        INode.__init__(self)
        self.addr = addr
        self.type_ = StackType.SET
        self.size = size
        self.data = data
    def __repr__(self):
        return 'OMakeSet(%s)' % (self.data)

class EndOfBasicBlock(INode):
    '''Special marker for uses at end of basic block'''
    def __init__(self, addr):
        self.addr = addr

def analyze_basic_block(proc, dseg, proclist, bb, debug):
    if debug:
        print('BB %04x pred=%s succ=%s' % (bb.addr,
            [('%04x'%x.addr) for x in bb.pred],
            [('%04x'%x.addr) for x in bb.succ]))
    seg = dseg.info
    for inst in bb.instructions:
        inst.data = ILAInfo()

    bb.ins = []
    bb.outs = []

    stack = []
    for inst in bb.instructions:
        # determine stack delta
        delta = 0
        op = opcodes.OPCODES[inst.opcode]
        intypes = None
        outtypes = None
        if op[4] is not None and op[3] is not None:
            intypes = [StackType.INT] * op[3]
            outtypes = [StackType.INT] * op[4]
        else:
            if op[2] & opcodes.CALL: # for a call, query the stack depth delta of the target
                tgt = inst.get_call_target(seg.seg_num)
                if tgt is not None:
                    tgtmeta = proclist[seg.references[tgt[0]],tgt[1]]
                    if tgtmeta.num_params is not None:
                        intypes = [StackType.INT] * tgtmeta.num_params
                        outtypes = [] # XXX outtypes in case of return value
                    else: # unknown delta for procedure (native without proper metadata)
                        assert(False)
                else: # unknown target; RPU
                    assert(inst.opcode == opcodes.RPU)
                    break 
            elif inst.opcode == opcodes.STM: # store # words from stack
                intypes = [StackType.INT] * inst.args[0]
                outtypes = []
            elif inst.opcode == opcodes.LDM: # load # words to stack
                intypes = []
                outtypes = [StackType.INT] * inst.args[0]
            elif inst.opcode == opcodes.LDC: # load # words to stack
                intypes = []
                outtypes = [StackType.INT] * inst.args[2]
            else: # unknown delta for this instruction (set ops etc)
                # these are annoying as we can't statically determine the stack depth,
                # it depends on the particular set size
                if inst.opcode == opcodes.SRS:
                    intypes = [StackType.INT] * 2
                    outtypes = [StackType.SET]
                elif inst.opcode == opcodes.ADJ:
                    intypes = [StackType.SET]
                    outtypes = [StackType.INT] * inst.args[0]
                elif inst.opcode == opcodes.INN:
                    intypes = [StackType.SET, StackType.INT]
                    outtypes = [StackType.INT]
                elif inst.opcode in [opcodes.UNI,opcodes.INT,opcodes.DIF]:
                    intypes = [StackType.SET, StackType.SET]
                    outtypes = [StackType.SET]
                elif inst.opcode in [opcodes.EQPWR,opcodes.LEPWR,opcodes.GEPWR]:
                    intypes = [StackType.SET, StackType.SET]
                    outtypes = [StackType.INT]
                else:
                    raise NotImplementedError("Unhandled opcode 0x%02x" % inst.opcode)

        # pop inputs from stack, and add them to instruction inputs
        for typ in intypes:
            if stack:
                val = stack.pop()
                if val.type_ == StackType.INT and typ == StackType.SET:
                    # We expected a SET, but found an INT instead. This is a case we can
                    # handle, as long as the top INT is a constant and there are
                    # enough words on the stack.
                    # Construct a SET.
                    sizeval = val
                    setsize = sizeval.inst.get_constant()
                    assert(setsize is not None)
                    data = [stack.pop() for _ in range(setsize)]
                    val = OMakeSet(inst.addr, sizeval, data)
                    sizeval.uses.append(val) # used by make-set
                    for d in data: # back-reference: mark uses
                        d.uses.append(val)
                else:
                    assert(val.type_ == typ)
            else: # nothing on stack, this is a bb input
                i = BBInput(typ, len(bb.ins))
                bb.ins.append(i)
                val = i
            val.uses.append(inst) # back-reference
            inst.data.ins.append(val)
        # reverse ins from pop order to pascal order TOS-x..TOS-0
        inst.data.ins = list(reversed(inst.data.ins))

        # push outputs to stack
        for typ in outtypes:
            o = IOutput(typ, inst, len(inst.data.outs))
            stack.append(o)
            inst.data.outs.append(o)

    end = EndOfBasicBlock(inst.addr+1) # pointer beyond end
    bb.outs = list(reversed(stack))
    for out in bb.outs:
        out.uses.append(end)

    if debug:
        print('  Inputs: ', bb.ins)
        print('  Outputs: ', bb.outs)
        for inst in bb.instructions:
            print('    %04x: %02x %s %s' % (inst.addr,inst.opcode,inst.data.ins,inst.data.outs))


