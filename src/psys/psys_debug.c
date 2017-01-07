/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#include "psys_debug.h"

#include "psys_constants.h"
#include "psys_helpers.h"
#include "psys_opcodes.h"
#include "psys_state.h"
#include "util/util_minmax.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void psys_panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\x1b[41;30m PANIC: \x1b[0m");
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

void psys_debug(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

void psys_print_traceback(struct psys_state *s)
{
    psys_word curproc, mp, base, erec, ipc, sib, mp_up;

    psys_debug("Traceback [tib=%04x sp=%04x]\n", s->curtask, s->sp);

    curproc = s->curproc;
    ipc     = s->ipc - s->curseg;
    mp      = s->mp;
    base    = s->base;
    erec    = s->erec;

    while (true) {
        sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);

        psys_debug("  %-8.8s:0x%02x:%04x mp=0x%04x base=0x%04x erec=0x%04x\n",
            psys_bytes(s, sib + PSYS_SIB_Seg_Name), curproc, ipc,
            mp, base, erec);

        /* Advance to caller frame */
        erec    = psys_ldw(s, mp + PSYS_MSCW_MSENV);
        curproc = psys_ldsw(s, mp + PSYS_MSCW_MPROC);
        ipc     = psys_ldw(s, mp + PSYS_MSCW_IPC);
        mp_up   = psys_ldw(s, mp + PSYS_MSCW_MSDYN);
        /* Get new base pointer */
        base = psys_ldw(s, erec + PSYS_EREC_Env_Data);
        /* Don't follow static links here, they are much less relevant to debugging. */
        /* No more stack frames? */
        if (mp_up == 0 || mp == mp_up) {
            break;
        }
        mp = mp_up;
    }
}

static int prev_opcode = -1;

void psys_print_info(struct psys_state *s)
{
    unsigned ptr;
    psys_byte opcode;
    struct psys_opcode_desc *op;

    if (prev_opcode != -1) {
        /* print result of previous instruction */
        op = &psys_opcode_descriptions[prev_opcode];
        if (op->num_out > 0) {
            psys_debug("    -> (");
            for (int x = 0; x < op->num_out; ++x) {
                if (x != 0)
                    psys_debug(" ");
                psys_debug("0x%04x", psys_ldw(s, W(s->sp, x)));
            }
            psys_debug(")");
            psys_debug("\n");
        }
        prev_opcode = -1;
    }

    ptr    = s->ipc;
    opcode = psys_ldb(s, ptr++, 0);
    op     = &psys_opcode_descriptions[opcode];

    psys_debug("%.8s:0x%02x:%04x ipc=0x%05x tib=%04x sp=%04x ",
        psys_bytes(s, s->curseg + PSYS_SEG_NAME), s->curproc, s->ipc - s->curseg, s->ipc,
        s->curtask, s->sp);
    /* print instruction */
    psys_debug("%s", op->name);
    /* print args */
    for (int x = 0; x < op->num_args; ++x) {
        int val;
        if (x != 0)
            psys_debug(",");
        psys_debug(" ");
        switch (op->args[x] & PSYS_OPARG_ARGTMASK) {
        case PSYS_OPARG_VAR:
            val = psys_ldb(s, ptr++, 0);
            if (val & 0x80) {
                val = (val & 0x7f) << 8;
                val |= psys_ldb(s, ptr++, 0);
            }
            break;
        case PSYS_OPARG_BYTE:
            val = psys_ldb(s, ptr++, 0);
            break;
        case PSYS_OPARG_WORD:
            val = psys_ldb(s, ptr++, 0);
            val = val | (psys_ldb(s, ptr++, 0) << 8);
            break;
        }
        printf("0x%x", val);
    }

    /* print stack input */
    psys_debug(" (");
    for (int x = 0; x < op->num_in; ++x) {
        if (x != 0)
            psys_debug(" ");
        psys_debug("0x%04x", psys_ldw(s, W(s->sp, x)));
    }
    psys_debug(")");

    psys_debug("\n");
    prev_opcode = opcode;
}

void psys_debug_hexdump_ofs(const psys_byte *data, psys_fulladdr offset, unsigned size)
{
    static const unsigned PER_LINE = 16;
    unsigned ptr                   = 0;
    while (ptr < size) {
        unsigned x;
        unsigned remainder = umin(size - ptr, PER_LINE);
        printf("%05x: ", ptr + offset);
        for (x = 0; x < remainder; ++x) {
            printf("%02x ", data[ptr + x]);
        }
        for (x = remainder; x < PER_LINE; ++x) {
            printf("   ");
        }
        for (x = 0; x < remainder; ++x) {
            psys_byte ch = data[ptr + x];
            if (ch >= 32 && ch < 127)
                printf("%c", ch);
            else
                printf(".");
        }
        printf("\n");
        ptr += 16;
    }
}

void psys_debug_hexdump(struct psys_state *s, psys_fulladdr offset, unsigned size)
{
    psys_debug_hexdump_ofs(psys_bytes(s, offset), offset, size);
}
