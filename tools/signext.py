# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
def signext(val, bits):
    half = 1<<(bits-1)
    if val < half:
        return val
    else:
        return val - 2*half


