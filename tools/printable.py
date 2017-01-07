# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#### Printable characters utilities
def is_printable(b):
    return b >= 32 and b < 127

def prchr_(b):
    if is_printable(b):
        return chr(b)
    else:
        return '.'

PRCHR = [prchr_(x) for x in range(256)]

def printable(s):
    return ''.join(PRCHR[x] for x in s)
