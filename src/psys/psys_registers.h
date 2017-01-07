/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_REGISTERS
#define H_PSYS_REGISTERS

#include "psys_types.h"

/** p-system special "processor" registers.
 * See TIB in psys_constants.h for the positive values, as those registers are
 * treated as word offsets into the current TIB. */
enum psys_reg {
    PSYS_REG_CURTASK = -3, /* Current TIB */
    PSYS_REG_EVEC    = -2, /* Current environment vector */
    PSYS_REG_READYQ  = -1, /* First task ready to run */
};

/* load "processor" register */
extern psys_word psys_lpr(struct psys_state *state, psys_sword reg);

/* store "processor" register */
extern void psys_spr(struct psys_state *state, psys_sword reg, psys_word value);

#endif
