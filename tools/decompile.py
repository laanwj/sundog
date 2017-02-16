# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import opcodes
from decompiler.basicblock import find_basic_blocks
from decompiler.dataflow import analyze_basic_block, BBInput, IOutput, OMakeSet
from decompiler.expression import (Expression, OpExpression, ConstantIntExpression, FunctionCall, TakeAddressOf,
        VariableRef, GlobalVariableRef, ParameterRef, ReturnValueRef, LocalVariableRef, TempVariableRef,
        NilExpression)
from decompiler.statement import Statement, ExprStatement, AssignmentStatement
'''
p-code decompilation:

While tracing the flow of execution, keep track of a stack model.
This is fairly trivial as long as no set operations are encountered:

- Normal instructions can be represented as function calls e.g.
  sldc(0x02)
  ldcb(0x12)
  ldci(0x1234)
  mpi(ldci(0x1234), ldcb(0x12))
  ...
  Most of them return one value at most.
  First the instruction arguments, then the stack arguments.

- Storage instructions can be represented as statement:
  stl(0x12, 0x34)
  sro(0x14, ldcb(0x12))
  str(...)
  ...
  After the statement, the stack will (generally) be empty.

- Procedure calls can be represented in the same way. The number of arguments
  for every procedure is known and fixed (except for CFP).
  Return arguments should be handled pecially: we may need a static
  analysis pass to find these. Or use a heuristic. E.g. if something is pushed
  before the function call then used afterwards, this must be a return argument.
  Procedure calls have only up to one return value.

- Some instructions may have multiple return arguments which could be considered one
  unit, e.g. 
  
  - IXP for packed arrays, which LDP consumes. Then again, it also happens
  regularly that LDP input is simply pushed to the stack with other means,
  although the last two arguments are always constants.

  - Another set of stack values that could be considered one unit is the "string pointer"
  or "byte array pointer" which consists of an erec (which can be null for memory) and offset.

  - In this case being able to regard a block of stack values as unit helps, too.

- Set operations are where the difficulty appears: a set is one atomic "unit"
  for various operations but it gets pushed to the stack in multiple steps.
  What makes this somewhat easier is that all sets are statically-sized.
  - Small sets can be pushed as sequence of LDCI.
  - LDC, followed by LDCI for the size.
  - UNI: max(size(a),size(b))
  - INT: min(size(a),size(b))Ggg
  - DIF: size(a)
  - SRS: subrange set (depends on input)

- Control flow: (almost?) always the stack will be empty at control flow statements.
  This can be represented as goto's, ifs or while() statements where appropriate.

- Per basic block
  - Infer initial stack expectation
    e.g. [Int,Int,Set]
      - Complication: If a block of values is found to be a Set, this must be propagated back to previous blocks?
  - Inter stack expectation on exit
  - Should calls be treated as end of basic block?
    I don't think that is necessary in this analysis.

initial: [Int,Int]
dup1 [Int] [Int, Int]
srs  [Int Int] -> [Set]
ldc 0x2,0x2,0x7  -> [Set] [Block]
sldc7  [Set] [Block] [Int]
insert: virtual operation [Block Int] -> [Set]
uni   [Set Set] -> [Set]
inn   [Int Set] -> [Int]

- Insert virtual operation if a Set is needed and there's no set at the top of the stack
- If stack empty and pop happens -> add at start of initial stack expectation

Overall flow:

- Low level:
    - Determine basic blocks
    - Analyze each basic block
      - Link instruction inputs/outputs. Each instruction stack argument can link to either:
        - An output of a previous instruction
        - An input to the basic block (initial stack)
      - Outputs of basic block consist of the stack state.
- High level:
    - Use the resulting data structure to convert to statements and expressions
    - most instructions will be part of expressions - should take care
      to assign shared expressions to temporary variables
      (happens only in case of DUP1?)
    - in-values of BBs should get a temporary variable
    - assignment statements
      (store global, store local, store intermediate, store to memory ...)
    - variable references
      (load global, load local, load intermediate, load from memory...)
      also: take address of, with &
    - function calls (need to handle those that return values)
    - loops

'''

# TODO:
#  - handle instructions with multiple outputs (such as ixp) sanely - need
#  either multi-assignment to temporaries `(a,b,c) = ixp()` (DONE) or "types"
#  that wrap multiple values e.g. 'PackedIndex'.  Or both.  (set is a special
#  example of this). Of the often-used instructions only these have multiple out:
#    - ixp
#    - swap (handled by simply regarding expressions as swapped)
#    - dup1 (handled by assigning expression to temporary)
#
#  - function calls: figure out what is called - there is already code for this in inst.get_call_target()
#  - xjp: jump tables
#  - lco: strings in-place?
#  - sind: indexing

####### Top level ########

def link_basic_blocks(basic_blocks):
    '''Link together basic blocks inputs and outputs with temporaries'''
    # The overall algorithm is to visit all outputs of all basic blocks,
    # and group them together. Each such group is assigned a temporary.
    # Outputs belong to the same group if they share an input on some bb.
    # Inputs are labeled with the same temporary.

    # TODO: as it is a stack oriented machine, it is possible for BB outputs to
    # 'jump over' basic blocks if something is pushed deeper on the stack of
    # the origin bb than the target bb will consume. To handle this it'd be
    # necessary to consider longer paths instead of just direct connections.
    # For now, assert that ins match outs. This seems to be enough to get us
    # through SYSTEM.STARTUP.
    for bb in basic_blocks.blocks:
        for bbs in bb.succ:
            if len(bbs.ins) != len(bb.outs):
                print('DIFFERENCE: %05x outs %d versus %05x ins %d' % (bb.addr, len(bb.outs), bbs.addr, len(bbs.ins)))
                print(bb.outs)
                print(bbs.ins)
                raise NotImplementedError

    tcount = 0x100 # Non-local temps
    # Iterate, assign temporaries
    for bb in basic_blocks.blocks:
        if not bb.outs: # not interesting if there are no outputs
            continue
        if bb.outs[0].temp is not None: # bb was already visited as part of group
            continue
        # Build group of basic blocks
        # We can take this shortcut of making a group of bb-prev and bb-succs
        # because of the above assumption that number of inputs will match number of outputs
        # for every pair of them.
        pred = set()
        succ = set()
        todo = {bb}
        while todo:
            bbi = todo.pop()
            pred.add(bbi)
            for bbs in bbi.succ:
                succ.add(bbs)
                for bbp in bbs.pred:
                    if not bbp in pred:
                        todo.add(bbp)
        # Assign temporaries to group
        for i,_ in enumerate(bb.outs):
            temp = TempVariableRef(tcount)
            tcount += 1
            for bbp in pred:
                bbp.outs[i].temp = temp
            for bbs in succ:
                bbs.ins[i].temp = temp

def emit_statements(proc, dseg, proclist, basic_blocks, debug=False):
    seginfo = dseg.info
    # make expression trees and statements
    # output would be a list of statements "HLIR"
    # statements: these "sink" (or "realize") expressions
    #   also mark some instructions as "sequence points", where current stack contents are assigned names
    #   anything with side effects
    #   These are "STORE" and "CALL" instructions
    #   and end of basic block

    # if certain expression tree (identified by a INode) is used multiple times: assign name (emit "asssign to temporary")
    # procedure calls may have side effects; these should stay in order
    # "multiple uses"
    # the easy way out would be to emit "assign to temporary" for every node, but that'd hardly be more readable than
    # instructionwise disassembly

    # (instructions "ins" are always what is pushed on stack, "outs" are what is popped)
    # (need to be able to evaluate: is this value going to be used? can be done by following outputs. IF not, make a statement?)
    # (stack is "current set of outputs that are going to be used later" - how to query without actually having stack available?)
    # (everything on the stack is, by definition, consumed at some point - only once, though it could be dupped)
    # "is it going to be consumed before next sequence point" - if not, give it a name
    print('===========')

    # index of next temporary to be dealt out
    tcount = 0

    def new_temporary():
        '''
        Get a new temporary. These are local to a function,
        but can be shared between basic blocks.
        '''
        nonlocal tcount
        tcount += 1
        return TempVariableRef(tcount)

    def get_encompassing_function(lvl):
        '''
        Get the encompassing function metadata,
        lvl levels above this one.
        '''
        if lvl is None:
            return None
        meta = proclist[(seginfo.name,proc.num)]
        for _ in range(lvl):
            if meta is None:
                break
            meta = meta.parent
        return meta

    def get_variable_ref(inst):
        '''
        Get a variable reference, based on the instruction type.
        For instructions referencing globals this will return a global,
        for instructions referencing locals this will return a local,
        and so on.
        '''
        op = opcodes.OPCODES[inst.opcode]
        if op[2] & opcodes.GLOBAL:
            segofs = inst.get_global_number(seginfo.gseg_num)
            if segofs is None:
                return None
            iname = seginfo.references.get(segofs[0], b'???')
            return GlobalVariableRef(iname, segofs[1])
        elif op[2] & (opcodes.LOCAL|opcodes.INTRMD):
            meta = get_encompassing_function(inst.get_lex_level())
            lnum = inst.get_local_number()
            if lnum is None:
                return None
            if lnum > meta.num_locals:
                if lnum < meta.rv_offset:
                    # PASCAL-style argument numbers 0..num_params
                    return ParameterRef(meta.key, meta.rv_offset - lnum - 1)
                else:
                    return ReturnValueRef(meta.key, lnum - meta.rv_offset)
            else:
                return LocalVariableRef(meta.key, lnum)
        else:
            return None

    def default_inst_to_expr(inst):
        '''
        Default instruction to expression (create a OpExpression).
        '''
        op = opcodes.OPCODES[inst.opcode]
        args = [ConstantIntExpression(x) for x in inst.args] + [out_to_expr(out) for out in inst.data.ins]
        return OpExpression(op[0].lower(), args)

    def inst_to_expr(inst):
        '''
        Instruction (and its dependency tree) to expression tree.
        '''
        op = opcodes.OPCODES[inst.opcode]
        # TODO: instruction-specific handling
        #   CALL -> FunctionCall / ReturnStatement
        #   ADDR -> TakeAddressOf
        #   CFLOW -> ControlFlowStatement
        # If instruction is simply a constant load, replace it with a ConstantIntExpression
        constval = inst.get_constant()
        ref = get_variable_ref(inst)
        if constval is not None:
            return ConstantIntExpression(constval)
        elif ref is not None:
            if op[2] & opcodes.LOAD: # LOAD -> VariableRef
                return ref
            elif op[2] & opcodes.STORE: # STORE -> AssignmentStatement
                return AssignmentStatement(ref, out_to_expr(inst.data.ins[0]))
            elif op[2] & opcodes.ADDR: # ADDR -> TakeAddressOf
                return TakeAddressOf(ref)
            else:
                return default_inst_to_expr(inst)
        elif inst.opcode == opcodes.DUP1: # if a DUP, just bypass it
            return out_to_expr(inst.data.ins[0])
        elif inst.opcode == opcodes.LDCN: # Load NIL
            return NilExpression()
        else:
            return default_inst_to_expr(inst)

    def out_to_expr(out):
        '''
        Instruction output (and its dependency tree) to expression tree.
        '''
        if out.temp is not None:
            return out.temp
        if isinstance(out, BBInput):
            return out # TODO
        elif isinstance(out, IOutput): # instruction
            # XXX handle out.idx
            if out.inst.opcode == opcodes.SWAP: # special handling for swap
                return out_to_expr(out.inst.data.ins[1-out.idx])
            else:
                return inst_to_expr(out.inst)
        elif isinstance(out, OMakeSet):
            return OpExpression('Set', [out_to_expr(d) for d in out.data])
        else:
            raise NotImplemented('out_to_expr: Cannout handle node %s' % out)

    # For each basic block, emit statements
    # TODO: intercommunication between basic blocks: BBInput should use the
    # same temporary.
    for bb in basic_blocks.blocks:
        print('bb%04x:' % bb.addr)
        seqpoints = []
        for i,inst in enumerate(bb.instructions):
            op = opcodes.OPCODES[inst.opcode]
            if op[2] & (opcodes.STORE | opcodes.CALL | opcodes.CFLOW):
                seqpoints.append((i,inst.addr))
        # add virtual sequence one past the end of BB to catch BB outputs after the last statement
        seqpoints.append((len(bb.instructions), inst.addr+1)) 

        ptr = 0
        for (nexti,nextaddr) in seqpoints:
            while ptr < nexti:
                inst = bb.instructions[ptr]
                multout = len(inst.data.outs) > 1
                if inst.opcode == opcodes.DUP1: # Dups must assign a temporary to the *input* to avoid entirely awkward (but not incorrect) code
                    out = inst.data.ins[0]
                    t = new_temporary()
                    # TODO: emit and store an actual statement
                    print('  %04x: %s = %s' % (inst.addr, t, out_to_expr(out)))
                    out.temp = t
                elif inst.opcode == opcodes.SWAP: # Swaps simply swap the input expressions - leave them well enough aone
                    # This is handled in out_to_expr
                    pass
                else:
                    if multout: # Multiple outputs
                        for out in inst.data.outs:
                            out.temp = new_temporary()
                        temps = [repr(o.temp) for o in inst.data.outs]
                        # TODO: emit and store an actual statement
                        print('  %04x: mult %s = %s' % (inst.addr, ', '.join(temps), inst_to_expr(inst)))
                    elif inst.data.outs: # Single output
                        out = inst.data.outs[0]
                        #print('%d %04x %04x' % (len(out.uses), out.uses[0].addr, nextaddr))
                        if len(out.uses) == 0:
                            raise ValueError('No uses for output %s' % out)
                        if len(out.uses)>1 or out.uses[0].addr > nextaddr:
                            #print('Statement: %s used: %04x nextaddr: %04x' % (out, out.uses[0].addr, nextaddr))
                            if out.temp is None:
                                out.temp = new_temporary()
                            # TODO: emit and store an actual statement
                            print('  %04x: %s = %s' % (inst.addr, out.temp, inst_to_expr(inst)))
                ptr += 1
            if ptr >= len(bb.instructions):
                break
            # at sequence point: emit statement
            # this code conceptually the same as the above, except that a statement is forced,
            # and should be rolled into it
            inst = bb.instructions[ptr]
            op = opcodes.OPCODES[inst.opcode]
            if inst.data.outs:
                assert(len(inst.data.outs)==1)
                out = inst.data.outs[0]
                if out.temp is None:
                    out.temp = new_temporary()
                # TODO: emit and store an actual statement
                print('  %04x: %s = %s' % (inst.addr, out.temp, inst_to_expr(inst)))
            else:
                # TODO: emit and store an actual statement
                print('  %04x: %s' % (inst.addr, inst_to_expr(inst)))
            ptr += 1

def decompile_procedure(proc, dseg, proclist, debug=False):
    '''
    Perform control flow analysis on a procedure to find basic blocks.
    '''
    if proc.is_native: # Cannot do native code
        return
    if debug:
        print('Procedure %s:0x%02x' % (dseg.name, proc.num))
    seg = dseg.info

    basic_blocks = find_basic_blocks(proc, dseg, proclist, debug=False)

    for bb in basic_blocks.blocks:
        analyze_basic_block(proc, dseg, proclist, bb, debug=False)

    link_basic_blocks(basic_blocks)

    emit_statements(proc,dseg,proclist,basic_blocks,debug=True)

def pcode_decompile(dseg, proclist, selection=None):
    if selection is not None:
        decompile_procedure(dseg.proc_by_num[selection], dseg, proclist, True)
    else:
        for proc in dseg.procedures:
            decompile_procedure(proc, dseg, proclist, True)
            print()
