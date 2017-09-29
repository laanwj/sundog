/*
 * Copyright (c) 2017 Wladimir J. van der Laan
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */
#ifndef H_DEBUGGER
#define H_DEBUGGER

#include "psys/psys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psys_debugger;

extern struct psys_debugger *psys_debugger_new(struct psys_state *s);

/** Start interactive debugger.
 * *user* should be true if the user requested entering the debugger,
 * or it was entered for an outside reason, false if this was automatic due to
 * psys_debugger_trace returning true.
 */
extern void psys_debugger_run(struct psys_debugger *dbg, bool user);

/** Trace function - call this for every instruction. Returns true if debugger
 * should trigger, false otherwise.
 */
extern bool psys_debugger_trace(struct psys_debugger *dbg);

extern void psys_debugger_destroy(struct psys_debugger *dbg);

#ifdef __cplusplus
}
#endif

#endif
