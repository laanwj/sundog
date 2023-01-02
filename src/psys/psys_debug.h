/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_PSYS_DEBUG
#define H_PSYS_DEBUG

#include "psys_types.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/** Debug info entry for one procedure */
struct util_debuginfo_entry {
    struct psys_function_id proc;
    const char *name;
};

/** Top-level debug info structure.
 * Procedure entries must be in sorted order by (segment, proc_num) to
 * allow for fast searching.
 */
struct util_debuginfo {
    const struct util_debuginfo_entry *entry;
    unsigned len;
};

/** Exit with a fatal error message. Use this on unrecoverable errors only. */
void psys_panic(const char *fmt, ...);

/** Print formattes string to debug console. */
void psys_debug(const char *fmt, ...);

/** Print a traceback for the current stack to the debug console. */
void psys_print_traceback(struct psys_state *s);

/** Print information about current state of the p-machine and the current
 * instruction. */
void psys_print_info(struct psys_state *s);

/** Print information about current instruction, if it is a call
 * instruction. The destination procedure as well as arguments will be printed.
 * A list of procedures to ignore can be passed, as well as a debug info structure
 * (for naming of procedures).
 */
void psys_print_call_info(struct psys_state *s, const struct psys_function_id *ignore, unsigned ignore_len, const struct util_debuginfo *dbginfo);

void psys_debug_hexdump_ofshl(const psys_byte *data, psys_fulladdr offset, unsigned size, psys_byte *hl);
void psys_debug_hexdump_ofs(const psys_byte *data, psys_fulladdr offset, unsigned size);
void psys_debug_hexdump(struct psys_state *s, psys_fulladdr offset, unsigned size);

/** Get current stack depth (number of caller frames on stack).
 */
int psys_debug_stack_depth(struct psys_state *s);

/** Guess number of arguments for a procedure from number of locals and number
 * of values removed from stack on return.
 * -1 if unknown.
 */
int psys_debug_proc_num_arguments(struct psys_state *s, psys_fulladdr erec, psys_word procedure);

/* Get initial erec pointer from KERNEL. From there the list of erecs can be
 * traversed.
 */
psys_fulladdr psys_debug_first_erec_ptr(struct psys_state *s);

/** Assign a segment name. Names are case-insensitive, like Pascal.
 * Names longer than 8 characters will be truncated.
 */
void psys_debug_assign_segment_id(struct psys_segment_id *id, const char *name);

/** Get erec for segment by name. Names are case-insensitive, like Pascal.
 */
psys_fulladdr psys_debug_get_erec_for_segment(struct psys_state *s, const char *name);

/** Get globals base and size for segment by name */
psys_fulladdr psys_debug_get_globals(struct psys_state *s, char *name, psys_word *data_size);

#ifdef __cplusplus
}
#endif

#endif
