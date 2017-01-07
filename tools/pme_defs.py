# Copyright (c) 2017 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Other P-machine interpreter internal definitions

# "processor registers"
REGS = {
    -3: 'READYQ',
    -2: 'EVEC',
    -1: 'CURTASK',
    # TIB
    0: 'Wait_Q',
    1: 'Prior|Flags',
    2: 'SP_Low',
    3: 'SP_Upr',
    4: 'SP',
    5: 'MP',
    6: 'Reserved',
    7: 'IPC',
    8: 'ENV',
    9: 'TI_BIOResult|Proc_Num',
    10:'Hang_Ptr',
    11:'M_Depend',
    12:'Main_Task',
    13:'Start_MSCW',
}
