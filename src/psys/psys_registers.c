/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_registers.h"
#include "psys_constants.h"
#include "psys_debug.h"
#include "psys_helpers.h"
#include "psys_state.h"
#include "psys_task.h"

psys_word psys_lpr(struct psys_state *state, psys_sword reg)
{
    if (PDBG(state, REG)) {
        psys_debug("[lpr] %d\n", reg);
    }
    if (reg < 0) {
        switch (reg) {
        case PSYS_REG_CURTASK:
            return state->curtask;
        case PSYS_REG_EVEC:
            return psys_ldw(state, state->erec + PSYS_EREC_Env_Vect);
        case PSYS_REG_READYQ:
            return state->readyq;
        default:
            psys_panic("Read from unknown processor register %d\n", reg);
        }
    } else {
        psys_store_state_to_tib(state);
        return psys_ldw(state, W(state->curtask, reg));
    }
    return 0;
}

void psys_spr(struct psys_state *state, psys_sword reg, psys_word value)
{
    if (PDBG(state, REG)) {
        psys_debug("[spr] %d=0x%04x\n", reg, value);
    }
    if (reg < 0) {
        switch (reg) {
        case PSYS_REG_CURTASK:
            state->curtask = value;
        case PSYS_REG_EVEC:
            return psys_stw(state, state->erec + PSYS_EREC_Env_Vect, value);
        case PSYS_REG_READYQ:
            state->readyq = value;
        default:
            psys_panic("Read from unknown processor register %d\n", reg);
        }
    } else {
        psys_store_state_to_tib(state);
        psys_stw(state, W(state->curtask, reg), value);
        psys_restore_state_from_tib(state);
        if (reg == PSYS_TIB_SP / 2) { /* sp changed - possibly reinitialize locals from trace */
            state->local_init_base  = 5;
            state->local_init_count = 0xffff;
        }
    }
}
