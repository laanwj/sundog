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
#include <string.h>

/** HACK: state used for print_info */
static int prev_opcode = -1;

/** U+2192 RIGHT ARROW */
#define U_RIGHT_ARROW "\xe2\x86\x92"

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

int psys_debug_proc_num_arguments(struct psys_state *s, psys_fulladdr erec, psys_word procedure)
{
    psys_word num_locals;
    psys_fulladdr segment;
    psys_word end_pointer;
    psys_fulladdr addr = lookup_procedure(s, erec, procedure, false, &num_locals, &end_pointer, &segment);
    psys_byte *data;
    if (addr == PSYS_ADDR_ERROR || num_locals == 0xffff) { /* Not resident or native */
        return -1;
    }
    data = psys_bytes(s, segment);
    if (data[end_pointer] != PSOP_RPU) { /* End pointer does not point to return */
        return -1;
    }
    /* Subtract return argument */
    return data[end_pointer + 1] - num_locals;
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

int psys_debug_stack_depth(struct psys_state *s)
{
    psys_word mp = s->mp, mp_up;
    int depth    = 0;

    while (true) {
        mp_up = psys_ldw(s, mp + PSYS_MSCW_MSDYN);
        /* No more stack frames? */
        if (mp_up == 0 || mp == mp_up) {
            break;
        }
        mp = mp_up;
        depth += 1;
    }
    return depth;
}

/** Return true if *opcode* refers to a call instruction, false otherwise */
static bool is_call(psys_byte opcode)
{
    if ((opcode >= PSOP_CLP && opcode <= PSOP_CXI)
        || (opcode >= PSOP_SCXG1 && opcode <= PSOP_SCXG8)
        || opcode == PSOP_CFP
        || opcode == PSOP_SCIP1
        || opcode == PSOP_SCIP2) {
        return true;
    }
    return false;
}

static bool get_call_destination(struct psys_state *s, psys_fulladdr *erec_out, psys_byte *procedure_out)
{
    psys_byte *instr   = psys_bytes(s, s->ipc);
    psys_byte opcode   = instr[0];
    psys_fulladdr erec = s->erec;
    psys_byte procedure;

    if (opcode == PSOP_CLP || opcode == PSOP_CGP || opcode == PSOP_SCIP1 || opcode == PSOP_SCIP2) {
        procedure = instr[1];
    } else if (opcode == PSOP_CIP) {
        procedure = instr[2];
    } else if (opcode == PSOP_CXL || opcode == PSOP_CXG) {
        erec      = psys_lookup_ref_segment(s, instr[1], false);
        procedure = instr[2];
    } else if (opcode == PSOP_CXI) {
        erec      = psys_lookup_ref_segment(s, instr[1], false);
        procedure = instr[3];
    } else if (opcode >= PSOP_SCXG1 && opcode <= PSOP_SCXG8) {
        erec      = psys_lookup_ref_segment(s, opcode - PSOP_SCXG1 + 1, false);
        procedure = instr[1];
    } else if (opcode == PSOP_CFP) {
        return false; /* TODO */
    } else {
        return false;
    }
    if (erec == PSYS_ADDR_ERROR) {
        return false;
    }

    if (erec_out) {
        *erec_out = erec;
    }
    if (procedure_out) {
        *procedure_out = procedure;
    }
    return true;
}

void psys_print_info(struct psys_state *s)
{
    unsigned ptr;
    psys_byte opcode;
    struct psys_opcode_desc *op;
    int num_in;

    if (prev_opcode != -1) {
        op = &psys_opcode_descriptions[prev_opcode];
        /* print result of previous instruction */
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

    /* in case of call instruction, try to determine # arguments */
    num_in = op->num_in;
    if (is_call(opcode)) {
        psys_fulladdr erec;
        psys_byte procedure;
        if (get_call_destination(s, &erec, &procedure)) {
            num_in = psys_debug_proc_num_arguments(s, erec, procedure);
        }
    }

    /* print stack input */
    psys_debug(" (");
    for (int x = 0; x < num_in; ++x) {
        if (x != 0)
            psys_debug(" ");
        psys_debug("0x%04x", psys_ldw(s, W(s->sp, x)));
    }
    psys_debug(")");

    psys_debug("\n");
    prev_opcode = opcode;
}

static int psys_function_id_compare(const void *key_, const void *entry_)
{
    const struct psys_function_id *key   = (const struct psys_function_id *)key_;
    const struct psys_function_id *entry = (const struct psys_function_id *)entry_;
    int n                                = memcmp(&key->seg, &entry->seg, 8);
    if (n) {
        return n;
    }
    return key->proc_num - entry->proc_num;
}

void psys_print_call_info(struct psys_state *s, const struct psys_function_id *ignore,
    unsigned ignore_len, const struct util_debuginfo *dbginfo)
{
    psys_byte opcode;
    int num_in = -1;
    psys_fulladdr erec;
    const psys_byte *segname = NULL;
    int depth, i;
    struct psys_function_id key;

    opcode = psys_ldb(s, s->ipc, 0);

    if (!is_call(opcode)) {
        return;
    }

    /* In case of call instruction, try to determine which procedure,
     * and # arguments
     */
    if (get_call_destination(s, &erec, &key.proc_num)) {
        struct psys_function_id key_wild; /* wildcard key */
        psys_word sib = psys_ldw(s, erec + PSYS_EREC_Env_SIB);
        segname       = psys_bytes(s, sib + PSYS_SIB_Seg_Name);
        memcpy(&key.seg, segname, 8);

        /* It is possible to ignore certain segment-procedure pairs,
         * which cause a lot of noise.
         */
        if (bsearch(&key, ignore, ignore_len,
                sizeof(struct psys_function_id), psys_function_id_compare)) {
            return;
        }
        /* It's also possible to ignore all functions in a segment with a special
         * (segname, 0xff) pair.
         */
        key_wild.seg      = key.seg;
        key_wild.proc_num = 0xff;
        if (bsearch(&key_wild, ignore, ignore_len,
                sizeof(struct psys_function_id), psys_function_id_compare)) {
            return;
        }

        num_in = psys_debug_proc_num_arguments(s, erec, key.proc_num);
    }

    psys_debug("[%04x] ", s->curtask);
    /* Indent for stack depth */
    depth = psys_debug_stack_depth(s);
    for (i = 0; i < depth; ++i) {
        psys_debug(" ");
    }

    psys_debug("%.8s:0x%02x:%04x " U_RIGHT_ARROW " ",
        psys_bytes(s, s->curseg + PSYS_SEG_NAME), s->curproc, s->ipc - s->curseg);

    if (segname) {
        const struct util_debuginfo_entry *entry;

        psys_debug("%-8.8s:0x%x ", key.seg.name, key.proc_num);

        /* If there is an associated debug info entry for this procedure, print
         * the name. */
        entry = (const struct util_debuginfo_entry *)bsearch(&key, dbginfo->entry, dbginfo->len,
            sizeof(struct util_debuginfo_entry), psys_function_id_compare);
        if (entry) {
            psys_debug("%s", entry->name);
        }
    }

    if (num_in >= 0) {
        /* print stack input. Arguments are in reversed (pascal) order, thus count down */
        psys_debug("(");
        for (int x = num_in - 1; x >= 0; --x) {
            psys_debug("0x%04x", psys_ldw(s, W(s->sp, x)));
            if (x != 0)
                psys_debug(", ");
        }
        psys_debug(")");
    }

    psys_debug("\n");
}

static void highlight(psys_byte attr)
{
    if (attr) {
        int fg, bg;
        if ((attr & 0x0f) >= 8) {
            fg = (attr & 0x0f) + 90 - 8;
        } else {
            fg = (attr & 0x0f) + 30;
        }
        if ((attr >> 4) >= 8) {
            bg = (attr >> 4) + 100 - 8;
        } else {
            bg = (attr >> 4) + 40;
        }
        printf("\x1b[%d;%dm", fg, bg);
    }
}
static void unhighlight(psys_byte attr)
{
    if (attr) {
        printf("\x1b[0m");
    }
}

void psys_debug_hexdump_ofshl(const psys_byte *data, psys_fulladdr offset, unsigned size, psys_byte *hl)
{
    static const unsigned PER_LINE = 16;
    unsigned ptr                   = 0;
    while (ptr < size) {
        unsigned x;
        unsigned remainder = umin(size - ptr, PER_LINE);
        printf("%05x: ", ptr + offset);
        for (x = 0; x < remainder; ++x) {
            if (hl) {
                highlight(hl[ptr + x]);
            }
            printf("%02x ", data[ptr + x]);
            if (hl) {
                unhighlight(hl[ptr + x]);
            }
        }
        for (x = remainder; x < PER_LINE; ++x) {
            printf("   ");
        }
        for (x = 0; x < remainder; ++x) {
            psys_byte ch = data[ptr + x];
            if (hl) {
                highlight(hl[ptr + x]);
            }
            if (ch >= 32 && ch < 127)
                printf("%c", ch);
            else
                printf(".");
            if (hl) {
                unhighlight(hl[ptr + x]);
            }
        }
        printf("\n");
        ptr += 16;
    }
}

void psys_debug_hexdump_ofs(const psys_byte *data, psys_fulladdr offset, unsigned size)
{
    psys_debug_hexdump_ofshl(data, offset, size, NULL);
}

void psys_debug_hexdump(struct psys_state *s, psys_fulladdr offset, unsigned size)
{
    psys_debug_hexdump_ofs(psys_bytes(s, offset), offset, size);
}
