# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import opcodes

####### Basic blocks ########

class BasicBlock:
    '''Basic block of instructions'''
    def __init__(self, addr):
        self.addr = addr # Starting address
        self.instructions = [] # Instructions in basic block
        self.pred = [] # Predecessors
        self.succ = [] # Successors

class BasicBlocks:
    '''Container for basic blocks'''
    def __init__(self, root, blocks):
        self.root = root     # Entry block
        self.blocks = blocks # Array of blocks, in code order

class BBPassInfo:
    '''
    Metadata for pass 1
    '''
    def __init__(self):
        self.incoming = [] # jump target: first of basic block
        self.bbend = False # last instruction in basic block
        self.bb = None # back-reference to bb

def find_basic_blocks(proc, dseg, proclist, debug=False):
    '''
    Perform control flow analysis on a procedure to find basic blocks.
    '''
    if proc.is_native: # Cannot do native code
        return
    seg = dseg.info

    # create mapping from address to index
    inst_by_addr = {inst.addr:inst for inst in proc.instructions}

    # Add temporary information to instructions
    for inst in proc.instructions:
        inst.data = BBPassInfo()

    # pass 1: mark jump targets and ends of basic blocks
    for inst in proc.instructions:
        op = opcodes.OPCODES[inst.opcode]
        if inst.opcode == opcodes.RPU:
            inst.data.bbend = True
            break # full stop
        elif op[2] & opcodes.CFLOW:
            for tgt in inst.get_flow_targets(dseg):
                inst_by_addr[tgt].data.incoming.append(inst)
            inst.data.bbend = True

    # create list of basic blocks
    blocks = []
    block = None
    prev = None
    for inst in proc.instructions:
        if block is None or inst.data.incoming or (prev is not None and prev.data.bbend):
            if prev is not None and not prev.data.bbend:
                # Last instruction was not a bb-ending instruction, add an incoming edge
                # from previous instruciton.
                inst.data.incoming.append(prev)
            # Start new basic block
            block = BasicBlock(inst.addr)
            blocks.append(block)
        block.instructions.append(inst)
        prev = inst

    # pass 2: cross-reference basic blocks
    for bb in blocks:
        for inst in bb.instructions:
            inst.data.bb = bb
    
    # set successors and predecessors
    for bb in blocks:
        for src in bb.instructions[0].data.incoming:
            bb.pred.append(src.data.bb)
            src.data.bb.succ.append(bb)

    # clean up
    for inst in proc.instructions:
        del inst.data

    return BasicBlocks(blocks[0], blocks)

