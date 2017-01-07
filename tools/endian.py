# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

##### Endian/word handling
import struct

class BE:
    @staticmethod
    def getword(data, x):
        return struct.unpack('>H', data[x:x+2])[0]
    @staticmethod
    def getlong(data, x):
        return struct.unpack('>I', data[x:x+2])[0]

class LE:
    @staticmethod
    def getword(data, x):
        return struct.unpack('<H', data[x:x+2])[0]
    @staticmethod
    def getlong(data, x):
        return struct.unpack('<I', data[x:x+2])[0]

