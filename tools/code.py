# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import opcodes

### Data structures for disassembly
class Instruction:
    def __init__(self):
        self.addr = None # address in segment
        self.size = None # size of instruction in bytes
        self.opcode = None # numeric opcode 0-255
        self.args = ()
        self.is_jumptgt = False # is a target of a jump?

    def get_call_target(self, seg_num):
        '''
        Get call target segment and procedure number.
        '''
        if self.opcode in [opcodes.CLP, opcodes.CGP, opcodes.SCIP1, opcodes.SCIP2]:
            seg = seg_num
            proc = self.args[0]
        elif self.opcode in [opcodes.CIP]: # CIP
            seg = seg_num
            proc = self.args[1]
        elif self.opcode in [opcodes.CXL, opcodes.CXG]: # CXL CXG
            seg = self.args[0]
            proc = self.args[1]
        elif self.opcode in [opcodes.CXI]: # CXI
            seg = self.args[0]
            proc = self.args[2]
        elif opcodes.SCXG1 <= self.opcode <= opcodes.SCXG8: # SCXG
            seg = self.opcode - 0x6f
            proc = self.args[0]
        else:
            return None
        return (seg,proc)

    def get_lex_level(self):
        '''
        Get (delta) lex level for LOCAL and INTRMD instructions.
        '''
        if opcodes.OPCODES[self.opcode][2] & opcodes.LOCAL: # local is at lex level 0
            return 0
        if self.opcode in [opcodes.LDA,opcodes.LOD,opcodes.CIP,opcodes.LSL,opcodes.STR]: # (1,VAR,)
            return self.args[0]
        elif self.opcode == opcodes.CXI: # (1,1,1)
            return self.args[1]
        elif self.opcode in [opcodes.SLOD1, opcodes.SCIP1]: # (VAR,)
            return 1
        elif self.opcode in [opcodes.SLOD2, opcodes.SCIP2]: # (VAR,)
            return 2
        else:
            return None

    def get_flow_targets(self, dseg):
        '''
        Get list of known jump targets for instructions.
        Non-control-flow instructions will simply continue to the next instruction unimpeded.
        '''
        if self.opcode in [opcodes.UJP, opcodes.UJPL, opcodes.NAT_INFO]:
            #unconditional jump
            return [self.args[0]]
        elif self.opcode in [opcodes.EFJ, opcodes.NFJ, opcodes.FJP, opcodes.FJPL, opcodes.TJP]:
            # conditional jump
            return [self.addr+self.size,self.args[0]]
        elif self.opcode == opcodes.XJP:
            # case jump
            endian = dseg.endian
            ofs = dseg.datastart + self.args[0] * 2
            low = endian.getword(dseg.data, ofs)
            high = endian.getword(dseg.data, ofs+2)
            ofs += 4
            rv = [self.addr+self.size]
            for x in range(low, high+1):
                w = (self.addr + self.size + endian.getword(dseg.data, ofs)) & 0xffff
                rv.append(w)
                ofs += 2
            return rv
        elif self.opcode == opcodes.RPU or self.opcode == opcodes.NATIVE:
            return [] # execution ends, or unknown
        else: # just continue
            return [self.addr+self.size]

    def get_local_number(self):
        '''
        Get # of referenced local or intermediate.
        '''
        if opcodes.SLDL1 <= self.opcode <= opcodes.SLDL16:
            return self.opcode - opcodes.SLDL1 + 1
        elif opcodes.SLLA1 <= self.opcode <= opcodes.SLLA8:
            return self.opcode - opcodes.SLLA1 + 1
        elif opcodes.SSTL1 <= self.opcode <= opcodes.SSTL8:
            return self.opcode - opcodes.SSTL1 + 1
        elif self.opcode in [opcodes.LLA, opcodes.LDL, opcodes.STL, opcodes.SLOD1, opcodes.SLOD2]:
            return self.args[0]
        elif self.opcode in [opcodes.LDA, opcodes.LOD, opcodes.STR]:
            return self.args[1]
        else:
            return None

    def get_global_number(self, gsegnum):
        '''
        Return (seg,ofs) of referenced global.
        '''
        op = opcodes.OPCODES[self.opcode]
        if (op[2] & opcodes.GLOBAL) and (op[2] & (opcodes.LOAD|opcodes.ADDR|opcodes.STORE)):
            if op[2] & opcodes.ISEG:
                seg = self.args[0]
                ofs = self.args[1]
            elif opcodes.SLDO1 <= self.opcode <= opcodes.SLDO16:
                seg = gsegnum
                ofs = self.opcode - opcodes.SLDO1 + 1
            else:
                seg = gsegnum
                ofs = self.args[0]
            return (seg,ofs)
        else:
            return None

    def get_constant(self):
        '''
        Return value for load constants, or None if
        not a constant.
        '''
        if opcodes.SLDC0 <= self.opcode <= opcodes.SLDC31:
            return self.opcode - opcodes.SLDC0
        elif self.opcode in {opcodes.LDCI,opcodes.LDCB}:
            return self.args[0]
        else:
            return None

class Procedure:
    def __init__(self):
        self.num = None
        self.addr = None
        self.size = None
        self.instructions = []
        self.inst_by_addr = {}
        self.num_locals = None # Number of locals
        self.end_addr = None
    @property
    def is_native(self):
        return self.num_locals == 0xffff

class Segment:
    def __init__(self):
        self.name = None
        self.info = None  # Reference to SegmentInfo
        self.procedures = []
        self.proc_by_num = {}
        self.data = None  # Segment data bytes
        self.datastart = None
        self.end = None
        self.endian = None
    @property
    def name_str(self):
        return self.name.decode().rstrip()

