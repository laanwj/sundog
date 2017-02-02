/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_DEBUG
#define H_PSYS_DEBUG

#include "psys_types.h"

#include <assert.h>

struct psys_state;
#define psys_assert assert

#define PDBG(s, flag) ((s)->debug & PSYS_DBG_##flag)
enum {
    PSYS_DBG_CALL    = 0x1,  /* Extra debugging for calls and returns */
    PSYS_DBG_WARNING = 0x2,  /* Weirdnesses */
    PSYS_DBG_STRINGS = 0x4,  /* Strings and arrays */
    PSYS_DBG_TASK    = 0x10, /* Tasks and synchonization */
    PSYS_DBG_RSP     = 0x20, /* RSP calls */
    PSYS_DBG_REG     = 0x40, /* Register writes and reads */
    /* These are not used by the interpreter itself,
     * but are free for use by the application.
     */
    PSYS_DBG_TRACE       = 0x8,  /* Print every instruction executed */
    PSYS_DBG_TRACE_CALLS = 0x80, /* Trace all procedure calls */
};

void psys_panic(const char *fmt, ...);
void psys_debug(const char *fmt, ...);

void psys_print_traceback(struct psys_state *s);
void psys_print_info(struct psys_state *s);
void psys_print_call_info(struct psys_state *s);

void psys_debug_hexdump_ofs(const psys_byte *data, psys_fulladdr offset, unsigned size);
void psys_debug_hexdump(struct psys_state *s, psys_fulladdr offset, unsigned size);

/** Guess number of arguments for a procedure from number of locals and number
 * of values removed from stack on return.
 * -1 if unknown.
 */
int psys_debug_proc_num_arguments(struct psys_state *s, psys_fulladdr erec, psys_word procedure);

#endif
