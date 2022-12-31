/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "test_common.h"

#include "psys/psys_constants.h"
#include "psys/psys_debug.h"
#include "psys/psys_helpers.h"
#include "psys/psys_interpreter.h"
#include "psys/psys_opcodes.h"
#include "psys/psys_state.h"

#include "util/memutil.h"

#include <stdio.h>
#include <string.h>

/* Called before every instruction executed.
 * Put debug hooks and tracing here.
 */
static void psys_trace(struct psys_state *s, void *dummy)
{
    /* psys_debug("ipc=0x%05x sp=%04x\n", s->ipc, s->sp); */
    /* Stop execution on any fault */
    if (psys_ldw(s, s->syscom + PSYS_SYSCOM_FAULT_TYPE)) {
        s->running = false;
    }
}

static void reset_state(struct psys_state *state)
{
    state->running = false;
    state->ipc     = 0x10080;
    state->sp      = 0xff00;
    state->base    = 0x8000;
    state->mp      = 0xff00;
    state->curseg  = 0x10000;
    state->readyq  = 0; /* TODO */
    state->curtask = 0; /* TODO */
    state->erec    = 0x0080;
    state->curproc = 1;
    memset(state->memory, 0, state->mem_size);
}

/* set up VM state for testing */
static struct psys_state *new_state()
{
    struct psys_state *state = CALLOC_STRUCT(psys_state);

    state->running  = false;
    state->mem_size = 128 * 1024;
    state->memory   = malloc(state->mem_size);
    memset(state->memory, 0, state->mem_size);
    reset_state(state);

    state->trace = &psys_trace;
    /* state->debug = PSYS_DBG_WARNING; */
    return state;
}

int main()
{
    struct psys_state *state = new_state();
    psys_byte *code          = psys_bytes(state, state->ipc);

    { /* Basic arithmetic */
        static const psys_byte testcode[] = {
            PSOP_SLDC1,
            PSOP_SLDC2,
            PSOP_ADI,
            PSOP_SSTL1,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_interpreter(state);
        CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 3);
    }

    { /* Multiply and divide */
        static const psys_byte testcode[] = {
            PSOP_SLDC2,
            PSOP_LDCB,
            0x80,
            PSOP_MPI,
            PSOP_LDCB,
            0x12,
            PSOP_SBI,
            PSOP_LDCB,
            0x3,
            PSOP_DVI,
            PSOP_SSTL1,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_interpreter(state);
        CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 79);
    }

    {
        static const psys_byte testcode[] = {
            PSOP_LDCI,
            0x12,
            0xff,
            PSOP_LDCI,
            0x13,
            0xff,
            PSOP_MPI,
            PSOP_SSTL1,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_interpreter(state);
        CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 0xdc56);
    }

    {
        static const psys_byte testcode[] = {
            PSOP_LDCI,
            0x12,
            0xff,
            PSOP_LDCI,
            0x02,
            0x00,
            PSOP_DVI,
            PSOP_SSTL1,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_interpreter(state);
        CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 0xff89);
    }

    { /* sets */
        static const psys_byte testcode[] = {
            PSOP_LDCI,
            0x12,
            0x00,
            PSOP_DUP1,
            PSOP_DUP1,
            PSOP_SRS,
            PSOP_INN,
            PSOP_SSTL1,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_interpreter(state);
        CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 0x1);
    }

    { /* sets */
        static const psys_byte testcode[] = {
            PSOP_LDCI,
            0xff,
            0xff,
            PSOP_LDCI,
            0x12,
            0x00,
            PSOP_DUP1,
            PSOP_SRS,
            PSOP_INN,
            PSOP_SSTL1,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_stw(state, W(state->mp + PSYS_MSCW_VAROFS, 1), 0x1234);
        psys_interpreter(state);
        CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 0x0);
    }

    { /* sets */
        static const psys_byte testcode[] = {
            PSOP_SLLA1,
            PSOP_LDCB,
            0x00,
            PSOP_LDCB,
            0x0f,
            PSOP_SRS,
            PSOP_LDCB,
            0x20,
            PSOP_LDCB,
            0x2f,
            PSOP_SRS,
            PSOP_UNI,
            PSOP_LDCB,
            0x40,
            PSOP_LDCB,
            0x4f,
            PSOP_SRS,
            PSOP_UNI,
            PSOP_LDCB,
            0x60,
            PSOP_LDCB,
            0x6f,
            PSOP_SRS,
            PSOP_UNI,
            PSOP_ADJ,
            0x8,
            PSOP_STM,
            0x8,
            PSOP_RPU,
        };
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_stw(state, W(state->mp + PSYS_MSCW_VAROFS, 1), 0x1234);
        psys_interpreter(state);
        psys_word *w = psys_words(state, W(state->mp + PSYS_MSCW_VAROFS, 1));
        for (int x = 0; x < 4; ++x) {
            CHECK_EQUAL(w[x * 2], 0xffff);
            CHECK_EQUAL(w[x * 2 + 1], 0x0000);
        }
    }
    { /* sets and loop */
        // clang-format off
    static const psys_byte testcode[] = {
        PSOP_SLDC0, /* reset loop counter to 0 */
        PSOP_SSTL1,
        PSOP_SLLA2, /* address for storing set */
        PSOP_LDCB, 0x01, /* empty set */
        PSOP_LDCB, 0x00,
        PSOP_SRS,
        /* label */
/* 0*/  PSOP_SLDL1,
/* 1*/  PSOP_LDCB, 0x04,
/* 3*/  PSOP_MPI,
/* 4*/  PSOP_SLDL1,
/* 5*/  PSOP_LDCB, 0x04,
/* 7*/  PSOP_MPI,
/* 8*/  PSOP_LDCB, 0x02,
/*10*/  PSOP_ADI,
/*11*/  PSOP_SRS,
/*12*/  PSOP_UNI, /* add in bits to current set */
/*13*/  PSOP_SLDL1, /* increase loop counter */
/*14*/  PSOP_INCI,
/*15*/  PSOP_SSTL1,
/*16*/  PSOP_SLDL1, /* check loop condition */
/*17*/  PSOP_LDCB, 0x20,
/*19*/  PSOP_EFJ, -21,
/*21*/  /* store set */
        PSOP_ADJ, 0x8,
        PSOP_STM, 0x8,
        PSOP_RPU,
    };
        // clang-format on
        reset_state(state);
        memcpy(code, testcode, sizeof(testcode));
        psys_stw(state, W(state->mp + PSYS_MSCW_VAROFS, 1), 0x1234);
        psys_interpreter(state);
        psys_word *w = psys_words(state, W(state->mp + PSYS_MSCW_VAROFS, 2));
        for (int x = 0; x < 8; ++x) {
            CHECK_EQUAL(w[x], 0x7777);
        }
        // CHECK_EQUAL(psys_ldw(state, W(state->mp + PSYS_MSCW_VAROFS, 1)), 0x0);
    }
    return 0;
}
