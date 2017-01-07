# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from endian import BE,LE

MAX_UNITS = 16
BLOCK_SIZE = 512

def load_imports(data, seg):
    '''
    Load imports table for a segment.
    '''
    ptr = seg.base + seg.codesize
    importend = ptr + seg.imports_size
    imports = {}
    while (ptr+9) < importend:
        idx = LE.getword(data,ptr+8)
        if idx == 0:
            break
        imports[idx] = data[ptr:ptr+8]
        ptr += 10
    return imports

class SegmentInfo:
    '''
    Description of p-system code segment.
    '''
    def __init__(self, endian, base, codesize, name, val1, val2, val3, val4):
        self.base = base
        self.codesize = codesize
        self.name = name
        self.val1 = val1
        self.val2 = val2
        self.val3 = val3
        self.val4 = val4

        self.seg_type = val1&0xff
        self.seg_num = val3&0xff

        if self.seg_type == 0x03: # subsegment
            self.parent_segment = val4
            self.top_info = None
            self.imports_size = 0
        else:
            # +0 globals size
            # +2 imports size
            # +4 highest import offset (2 if no imports)
            # +6 always 0?
            self.parent_segment = None
            self.top_info = (endian.getword(val4, 0), endian.getword(val4, 2), endian.getword(val4, 4), endian.getword(val4, 6))
            self.imports_size = self.top_info[1]*2

    @property
    def name_str(self):
        return self.name.decode().rstrip()

    @property
    def gseg_num(self):
        '''Segment accessed by global-loading instructions'''
        # Determine which segment is referenced for globals - if this is a subsidiary segment,
        # use that.
        if self.parent is not None: # Does this segment have a parent?
            return self.parent.seg_num # segment of parent
        else:
            return self.seg_num # our own


class SegmentDirectory:
    '''
    Directory of segments in a p-system code file.
    '''
    def __init__(self):
        self.segments = []

    @classmethod
    def load(cls, data):
        rv = cls()
        dirblock = 0
        endian = [BE,LE][data[BLOCK_SIZE-2]]
        while True:
            for x in range(MAX_UNITS):
                block = endian.getword(data, dirblock+x*4)
                codesize = endian.getword(data, dirblock+x*4+2) * 2
                name = data[dirblock + 0x40 + x*8:dirblock + 0x40 + x*8+8]

                val1 = endian.getword(data, dirblock + 0xc0 + x*2)
                # flags[relocatable,has_link_info] (upper 8 bit), lex level 1(program)/2(segment)/3(subsegment)
                val2 = endian.getword(data, dirblock + 0xe0 + x*2)
                val3 = endian.getword(data, dirblock + 0x100 + x*2)
                # version/mtype (upper 8 bit), segnum in parent segment imports (lower 8 bit)
                val4 = data[dirblock + 0x120 + x*8:dirblock + 0x120 + x*8 + 8]

                base = block * BLOCK_SIZE
                if base == 0:
                    continue
                rv.segments.append(SegmentInfo(endian, base, codesize, name, val1, val2, val3, val4))

            # 0x1a0 -> next block
            dirblock = BE.getword(data, dirblock+0x1a0) * BLOCK_SIZE
            if dirblock == 0:
                break

        rv.segments_by_name = {}
        for x in rv.segments:
            rv.segments_by_name[x.name] = x

        return rv

    def load_imports(self, data):
        '''
        Load imports, determine segment reference hierarchy.
        '''
        for seg in self.segments:
            if seg.parent_segment is not None:
                seg.parent = self.segments_by_name[seg.parent_segment]
            else:
                seg.parent = None

        for seg in self.segments:
            seg.imports = load_imports(data, seg)
            seg.references = seg.imports.copy()
            seg.references[1] = b'KERNEL  ' # 1 is always kernel
            seg.references[seg.seg_num] = seg.name # self-reference

        for seg in self.segments:
            if seg.parent is not None:
                seg.parent.references[seg.seg_num] = seg.name

        for seg in self.segments:
            if seg.parent is not None:
                seg.references = seg.parent.references


